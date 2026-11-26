#include "SculptureImplicitObservationSession.h"
#include "ImplicitExternalImageTransport.h"
#include "SculptureVulkanDeviceIdentity.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <utility>

namespace epi_sculpt {

namespace {
    [[nodiscard]] ImplicitProductionAttemptOutcome mapImplicitSubmissionResult(
        ImplicitSubmissionResult result) noexcept {
        switch (result) {
        case ImplicitSubmissionResult::Submitted:
            return ImplicitProductionAttemptOutcome::Accepted;
        case ImplicitSubmissionResult::Backpressure:
            return ImplicitProductionAttemptOutcome::ProducerBackpressure;
        case ImplicitSubmissionResult::Superseded:
        case ImplicitSubmissionResult::Failed:
            return ImplicitProductionAttemptOutcome::ProducerRejected;
        }
        return ImplicitProductionAttemptOutcome::ProducerRejected;
    }

} // namespace

SculptureImplicitObservationSession::SculptureImplicitObservationSession(
    mgfx::Engine& engine, const ImplicitFieldProgram& program)
    : engine_(engine), implicitSurfaceComposition_(program.surfaceComposition) {
    // Resource ownership belongs to the composition root. Presentation never
    // guesses a repository-relative kernel path or mutates process state.
    implicitConfig_.kernelPath = program.kernelPath;
    implicitConfig_.kernelEntryPoint = program.kernelEntryPoint;
    matchOpenClToVulkanDevice(
        implicitConfig_, engine_.context().device().physical());
    // The shared material binding still owns four textures, even for optical
    // integrals. Keep their miss payloads initialized on both transport routes.
    implicitConfig_.includeDepth = true;
    implicitConfig_.includeNormal = true;
    implicitConfig_.includeVelocity = true;

    // Initialize Implicit rendering elements (quad & unlit material)
    implicitQuadMesh_.emplace(mgfx::vk::Mesh::makePlane(engine_.context(), 2.0f, 1));
    // The default instanced path must not pay for an unused full-resolution
    // OpenCL raymarcher. Resources are created on first implicit presentation.
}

SculptureImplicitObservationSession::~SculptureImplicitObservationSession() {
    // Native work/callbacks must already be retired by the owning device island.
    // Release/drain OpenCL imports before member destruction frees exporters.
    implicitProducer_.reset();
}

void SculptureImplicitObservationSession::setImplicitRenderScale(float scale) noexcept {
    const float normalized = std::clamp(scale, 0.25f, 1.0f);
    if (std::abs(normalized - implicitRenderScale_) <= 1.0e-4f) return;
    implicitRenderScale_ = normalized;
    // Force a matched producer/texture recreation on the next presentation
    // frame. The existing image remains valid until the replacement succeeds.
    rejectedImplicitWidth_ = 0U;
    rejectedImplicitHeight_ = 0U;
    implicitResizeRetryCountdown_ = 0U;
}

void SculptureImplicitObservationSession::setImplicitFixedExtent(VkExtent2D extent) {
    if (implicitProducer_) throw std::logic_error("fixed implicit extent must be set before initialization");
    if (!extent.width || !extent.height || extent.width > 8192 || extent.height > 8192
        || static_cast<std::uint64_t>(extent.width) * extent.height > 3840ULL * 2160ULL * 4ULL)
        throw std::invalid_argument("fixed implicit extent exceeds supported pixel budget");
    implicitFixedExtent_ = extent;
}

ImplicitFrameIdentity SculptureImplicitObservationSession::implicitPublishedFrame() const noexcept {
    if (!implicitProducer_) return {};

    if (implicitExternalTransport_) {
        const ImplicitExternalFrameView frame = implicitProducer_->externalFrameView();
        if (!frame.valid()) return {};
        return {
            .generation = frame.generation,
            .slotIndex = frame.slotIndex,
            .allocationGeneration = frame.allocationGeneration,
            .externalMemory = true,
        };
    }

    if (!implicitProducer_->hasValidOutput()) return {};
    return {
        .generation = implicitProducer_->telemetry().deliveredGeneration,
        .externalMemory = false,
    };
}

bool SculptureImplicitObservationSession::prepareImplicitObservation(
    const mgfx::Camera& camera,
    const SculptureProgramParams& params,
    float time,
    float morph,
    float deltaSeconds,
    std::uint32_t generation,
    ImplicitFrameRequest& outRequest) {
    if (implicitRetirementGate_.stopped() || generation == 0U) return false;

    const VkExtent2D extent = engine_.swapchain().extent();
    bool extentReady = ensureImplicitExtent(extent);
    if (implicitRetirementGate_.stopped()) return false;
    if (extentReady && !implicitProducer_->pollCompleted()) {
        implicitResizeDiagnostic_ = implicitProducer_->diagnostic();
        extentReady = false;
    }
    if (!extentReady || !implicitProducer_) return false;

    const ImplicitCamera observationCamera{
        {camera.eye.x, camera.eye.y, camera.eye.z},
        {camera.target.x, camera.target.y, camera.target.z},
        {camera.up.x, camera.up.y, camera.up.z},
        camera.fovYRad,
        0.0f,
    };
    if (!makeImplicitFrameRequest(
        params,
        time,
        morph,
        deltaSeconds,
        generation,
        implicitProducer_->width(),
        implicitProducer_->height(),
        observationCamera,
        outRequest,
        implicitResizeDiagnostic_)) {
        return false;
    }
    outRequest.producerEpoch = implicitProducerEpoch_;
    return true;
}

void SculptureImplicitObservationSession::submitImplicitObservation(
    const ImplicitFrameRequest& request,
    ImplicitProductionAttemptObserver observer) {
    const auto report = [&](ImplicitProductionAttemptOutcome outcome) noexcept {
        observer.notify({.outcome = outcome, .generation = request.frame.generation});
    };

    if (implicitRetirementGate_.stopped() || !implicitProducer_) {
        report(ImplicitProductionAttemptOutcome::ProducerRejected);
        return;
    }

    if (!implicitExternalTransport_) {
        const auto result = implicitProducer_->submitRequest(request);
        if (result == ImplicitSubmissionResult::Submitted) {
            implicitResizeDiagnostic_.clear();
        } else {
            implicitResizeDiagnostic_ = implicitProducer_->diagnostic();
        }
        report(mapImplicitSubmissionResult(result));
        return;
    }

    // Quarantine records reported failures; it is not a timeout detector for
    // hung binary semaphores. Recovery still needs both queues to retire before
    // resources can be freed.
    const auto diagnostics = implicitExternalTransport_->diagnostics();
    if (diagnostics.unavailableSlotCount() >= ImplicitExternalImageTransport::kSlotCount) {
        implicitResizeDiagnostic_ = "All external slots quarantined (circuit breaker tripped)";
        implicitExternalRecoveryPending_ = true;
        report(ImplicitProductionAttemptOutcome::TransportRejected);
        return;
    }

    const auto slotIndex = implicitProducer_->nextExternalSlotIndex();
    if (!slotIndex) {
        implicitResizeDiagnostic_ = "implicit external transport backpressure: no reusable slot";
        report(ImplicitProductionAttemptOutcome::TransportBackpressure);
        return;
    }

    auto& frameLoop = engine_.frameLoop();
    if (!frameLoop.canEnqueueSubmission({
            .externalSignals = 1U,
            .commitCallbacks = 1U,
        })) {
        // Fail before recording the initial queue-family release. A partial
        // handoff is worse than dropping this observation opportunity.
        implicitResizeDiagnostic_ =
            "implicit external transport backpressure: frame submission budget exhausted";
        report(ImplicitProductionAttemptOutcome::TransportBackpressure);
        return;
    }
    // Keep the complete immutable request alive without exceeding FrameLoop's
    // bounded inline closure storage. Allocate before recording any release.
    auto submittedRequest = std::make_unique<const ImplicitFrameRequest>(request);
    if (!implicitExternalTransport_->enqueueInitialRelease(engine_, *slotIndex)) {
        implicitResizeDiagnostic_ = "implicit external transport rejected initial release";
        report(ImplicitProductionAttemptOutcome::TransportRejected);
        return;
    }

    auto* transport = implicitExternalTransport_.get();
    auto* producer = implicitProducer_.get();
    const std::uint32_t generation = request.frame.generation;
    const VkSemaphore release = transport->vulkanReleaseSemaphore(*slotIndex);
    const bool commitQueued = frameLoop.enqueueSubmitCommit(
        [this, transport, producer, generation, request = std::move(submittedRequest),
         observer, slot = *slotIndex](bool success) {
            if (!success) {
                implicitResizeDiagnostic_ = "implicit external Vulkan-release submission failed";
                implicitExternalRecoveryPending_ = true;
                observer.notify({
                    .outcome = ImplicitProductionAttemptOutcome::FrameSubmissionRejected,
                    .generation = generation,
                });
                return;
            }
            if (!transport->markVulkanReleaseSubmitted(slot, generation)) {
                implicitResizeDiagnostic_ = "implicit external Vulkan-release ownership transition failed";
                implicitExternalRecoveryPending_ = true;
                observer.notify({
                    .outcome = ImplicitProductionAttemptOutcome::TransportRejected,
                    .generation = generation,
                });
                return;
            }

            const auto result = producer->submitExternalRequest(*request, slot);
            if (result != ImplicitSubmissionResult::Submitted) {
                implicitResizeDiagnostic_ = producer->diagnostic();
                static_cast<void>(transport->quarantineTicket(slot, generation));
                implicitExternalRecoveryPending_ = true;
            } else {
                implicitResizeDiagnostic_.clear();
            }
            observer.notify({
                .outcome = mapImplicitSubmissionResult(result),
                .generation = generation,
            });
        });
    if (!commitQueued) {
        // The inline adapter has already settled the observer with success=false.
        // Abort this frame so the graph-side release is not submitted without
        // a matching ownership commit.
        throw std::runtime_error(
            "implicit external submit callback rejected after successful preflight");
    }
    if (!frameLoop.enqueueExternalSignal(
            release, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT)) {
        // Settlement is already registered. Aborting recording makes FrameLoop
        // invoke it with success=false rather than leaving a signal-less handoff.
        throw std::runtime_error(
            "implicit external release signal rejected after successful preflight");
    }
}

void SculptureImplicitObservationSession::setImplicitQuality(
    ImplicitQualityConfig quality) noexcept {
    // AdvancedDemoRunner applies this before the first extent allocation.
    // A live quality-mode change must go through the explicit producer rebuild
    // path rather than mutating an in-flight OpenCL policy.
    implicitConfig_.quality = quality;
}

void SculptureImplicitObservationSession::setImplicitExternalTransportEnabled(
    bool enabled) noexcept {
    // This is configured before the first implicit extent is allocated. Do
    // not tear down a live producer here: a future runtime toggle should use
    // the existing explicit recovery/rebuild transaction instead.
    implicitExternalTransportEnabled_ = enabled;
}

bool SculptureImplicitObservationSession::ensureImplicitExtent(
    VkExtent2D swapchainExtent) {
    if (implicitRetirementGate_.stopped()) return false;
    if (swapchainExtent.width == 0U || swapchainExtent.height == 0U) {
        return false;
    }

    // Mono is the live presentation contract and consumes the complete image.
    // Stereo requests validate their own even-width side-by-side requirement.
    const std::uint32_t targetWidth = implicitFixedExtent_ ? implicitFixedExtent_->width : std::max(
        1U, static_cast<std::uint32_t>(std::lround(
            static_cast<float>(swapchainExtent.width) * implicitRenderScale_)));
    const std::uint32_t targetHeight = implicitFixedExtent_ ? implicitFixedExtent_->height : std::max(
        1U, static_cast<std::uint32_t>(std::lround(
            static_cast<float>(swapchainExtent.height) * implicitRenderScale_)));
    if (implicitProducer_
        && implicitProducer_->width() == targetWidth
        && implicitProducer_->height() == targetHeight
        && !implicitExternalRecoveryPending_) {
        return true;
    }
    if (rejectedImplicitWidth_ == targetWidth
        && rejectedImplicitHeight_ == targetHeight
        && implicitResizeRetryCountdown_ > 0U) {
        --implicitResizeRetryCountdown_;
        return false;
    }

    // Gate the entire replacement transaction before allocating, resetting, or
    // rebinding anything. Steady-state frames return above without waiting.
    const bool replaced = implicitRetirementGate_.replaceAfterRetirement(
        [this]() noexcept {
            return (implicitProducer_ || implicitMaterial_ != mgfx::kInvalidMaterial)
                ? engine_.waitIdleChecked() : VK_SUCCESS;
        },
        [&]() -> bool {
            if (implicitExternalRecoveryPending_ && implicitProducer_) {
                std::fprintf(stderr, "[3dEpiSculpTMorph] Initiating explicit teardown and graceful recovery of implicit transport due to: %s\n", implicitResizeDiagnostic_.c_str());

                // Permanently downgrade to staged fallback for this session
                implicitExternalTransportEnabled_ = false;
                implicitProducer_.reset();
                implicitExternalTransport_.reset();
            }

            auto nextConfig = implicitConfig_;
            nextConfig.width = targetWidth;
            nextConfig.height = targetHeight;

            try {
                // Build both sides before publishing either. A failed OpenCL or Vulkan
                // allocation leaves the last valid pair alive and drawable.
                // Importer must unwind before exporter if later texture/material
                // construction fails. Reverse local destruction order is intentional.
                std::unique_ptr<ImplicitExternalImageTransport> nextExternalTransport;
                auto nextProducer = std::make_unique<ImplicitFrameProducer>(nextConfig);
                const std::uint64_t nextAllocationGeneration =
                    implicitAllocationGeneration_ + 1U;
                std::string nextTransportDiagnostic = implicitExternalTransportEnabled_
                    ? "Vulkan external image/semaphore export unavailable or disabled"
                    : "external image transport disabled by policy";

                // The external-image route is opportunistic and all-or-nothing. A
                // failed capability/import probe leaves the staged producer
                // intact, so presentation can continue without a second math path.
                if (implicitExternalTransportEnabled_
                    && !implicitExternalDisabled_
                    && engine_.context().device().supportsExternalMemoryFd()
                    && engine_.context().device().supportsExternalSemaphoreFd()) {
                    try {
                        ImplicitExternalImageTransport::Config transportConfig{};
                        transportConfig.width = targetWidth;
                        transportConfig.height = targetHeight;
                        transportConfig.allocationGeneration = nextAllocationGeneration;
                        auto candidate = std::make_unique<ImplicitExternalImageTransport>(
                            engine_.context(), transportConfig);
                        auto imports = candidate->exportOpenClImports();
                        if (imports && nextProducer->importZeroCopyBuffers(std::move(*imports))) {
                            nextExternalTransport = std::move(candidate);
                        }
                        nextTransportDiagnostic = imports ? nextProducer->diagnostic()
                            : "Vulkan failed to export the complete implicit image/semaphore set";
                    } catch (const std::exception& error) {
                        // Keep selection diagnostics separate from transient frame errors;
                        // successful staged delivery must not erase why import was rejected.
                        nextTransportDiagnostic = error.what();
                    }
                }

                mgfx::vk::Image::CreateInfo textureInfo{};
                textureInfo.width = targetWidth;
                textureInfo.height = targetHeight;
                textureInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                textureInfo.addressU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                textureInfo.addressV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                auto nextTexture = std::make_unique<mgfx::vk::Image>(
                    engine_.context(), textureInfo);

                mgfx::vk::Image::CreateInfo depthInfo = textureInfo;
                depthInfo.format = VK_FORMAT_R32_SFLOAT;
                auto nextDepthTexture = std::make_unique<mgfx::vk::Image>(
                    engine_.context(), depthInfo);

                auto nextNormalTexture = std::make_unique<mgfx::vk::Image>(
                    engine_.context(), textureInfo); // OpenCL uses float4 for normals too

                mgfx::vk::Image::CreateInfo velocityInfo = textureInfo;
                velocityInfo.format = VK_FORMAT_R32G32_SFLOAT;
                auto nextVelocityTexture = std::make_unique<mgfx::vk::Image>(
                    engine_.context(), velocityInfo);

                if (implicitMaterial_ == mgfx::kInvalidMaterial) {
                    mgfx::pbr::Material unlitMaterial{};
                    unlitMaterial.unlit = true;
                    unlitMaterial.roughness = 0.4f; // Enable SSR reflections on the implicit surface
                    // This is a camera-space compositor, not a physical surface.
                    // Its visibility must never depend on the source plane's winding
                    // after view-space placement or Vulkan's Y-flipped projection.
                    unlitMaterial.doubleSided = true;
                    implicitMaterial_ = engine_.materials()->createMaterial(
                        unlitMaterial,
                        nextTexture.get(),
                        nextNormalTexture.get(),
                        nextVelocityTexture.get(), // bound to mrao
                        nextDepthTexture.get()     // bound to occlusion
                    );
                } else {
                    // The checked retirement gate precedes allocation and rebinding.
                    mgfx::MaterialTextureInputs textures{};
                    textures.albedo = nextTexture.get();
                    textures.normal = nextNormalTexture.get();
                    textures.mrao = nextVelocityTexture.get();
                    textures.occlusion = nextDepthTexture.get();
                    engine_.materials()->updateMaterialTextures(
                        implicitMaterial_, textures);
                }

                implicitConfig_ = std::move(nextConfig);
                implicitProducer_ = std::move(nextProducer);
                ++implicitProducerEpoch_;
                if (implicitProducerEpoch_ == 0U) ++implicitProducerEpoch_;
                implicitExternalTransport_ = std::move(nextExternalTransport);
                if (implicitExternalTransport_) {
                    implicitAllocationGeneration_ = nextAllocationGeneration;
                }
                implicitTransportDiagnostic_ = std::move(nextTransportDiagnostic);
                implicitTexture_ = std::move(nextTexture);
                implicitDepthTexture_ = std::move(nextDepthTexture);
                implicitNormalTexture_ = std::move(nextNormalTexture);
                implicitVelocityTexture_ = std::move(nextVelocityTexture);

                implicitTextureReady_ = false;
                implicitExternalRecoveryPending_ = false;
                lastImplicitUploadGeneration_ = 0U;
                stagedImplicitObservation_.reset();
                implicitPresentationSelection_ = {};
                rejectedImplicitWidth_ = 0U;
                rejectedImplicitHeight_ = 0U;
                implicitResizeRetryCountdown_ = 0U;
                implicitResizeDiagnostic_.clear();
                return true;
            } catch (const std::exception& error) {
                // Back off before retrying the same extent. This avoids turning a
                // transient allocation failure into a per-frame hitch while retaining
                // eventual recovery from temporary memory or driver pressure.
                rejectedImplicitWidth_ = targetWidth;
                rejectedImplicitHeight_ = targetHeight;
                implicitResizeRetryCountdown_ = 120U;
                implicitResizeDiagnostic_ = error.what();
                return false;
            }
        });
    if (implicitRetirementGate_.stopped()) {
        implicitResizeDiagnostic_ = "implicit session stopped: Vulkan retirement failed (VkResult "
            + std::to_string(static_cast<int>(implicitRetirementGate_.result())) + ")";
        std::fprintf(stderr, "[3dEpiSculpTMorph] %s\n", implicitResizeDiagnostic_.c_str());
    }
    return replaced;
}

void SculptureImplicitObservationSession::drawObservation(
    mgfx::Drawable& draw, const mgfx::math::mat4& quadModel) {
    if (!usesEngine(draw.engine())) {
        throw std::invalid_argument("implicit observation draw belongs to a different Engine");
    }
    if (implicitRetirementGate_.stopped()) return;
    bool implicitCopyScheduled = false;
    if (!implicitTextureReady_) implicitPresentationSelection_ = {};
    // Both transports update presenter-owned images. External delivery uses
    // GPU-local copies; staging uses the existing host upload path.

    if (implicitProducer_ && implicitTexture_) {
        if (implicitExternalTransport_) {
            const ImplicitExternalFrameView frame =
                implicitProducer_->externalFrameView();
            const bool extentMatches = frame.width == implicitTexture_->width()
                && frame.height == implicitTexture_->height();
            const bool newerGeneration = !implicitTextureReady_
                || frame.generation != lastImplicitUploadGeneration_;
            auto& frameLoop = engine_.frameLoop();
            const bool submissionCapacity = frameLoop.canEnqueueSubmission({
                .externalWaits = 1U,
                .commitCallbacks = 1U,
            });
            if (frame.valid() && extentMatches && newerGeneration
                && !submissionCapacity) {
                // Keep the completed external frame pinned in producer
                // ownership. No Vulkan transition has happened yet, so the
                // exact same generation can be retried next display frame.
                implicitResizeDiagnostic_ =
                    "implicit presentation backpressure: frame submission budget exhausted";
            } else if (frame.valid() && extentMatches && newerGeneration
                && implicitExternalTransport_->markOpenClWriteComplete(frame)
                && implicitExternalTransport_->acquireForVulkanRead(frame)) {
                // Materials always reference these persistent images. The
                // external ring is only a copy source, never a display cache.
                const bool allEnqueued = implicitExternalTransport_->enqueuePresentationCopy(
                    engine_, frame, {implicitTexture_.get(), implicitDepthTexture_.get(),
                        implicitNormalTexture_.get(), implicitVelocityTexture_.get()},
                    implicitTextureReady_);
                if (allEnqueued) {
                    auto* transport = implicitExternalTransport_.get();
                    auto* producer = implicitProducer_.get();
                    // Register terminal ownership settlement before appending
                    // the native wait. Any later recording failure therefore
                    // closes the exact lease with success=false.
                    const bool commitQueued = frameLoop.enqueueSubmitCommit(
                        [this, transport, producer, frame](bool success) {
                            if (success
                                && transport->releaseVulkanReadSubmitted(frame)
                                && producer->releaseExternalFrameAfterVulkanSubmit(frame)) {
                                lastImplicitUploadGeneration_ = frame.generation;
                                implicitTextureReady_ = true;
                                return;
                            }
                            (void)transport->quarantine(frame);
                            implicitResizeDiagnostic_ =
                                "implicit external presentation submit failed; slot quarantined";
                            implicitExternalRecoveryPending_ = true;
                            implicitTextureReady_ = false;
                        });
                    if (!commitQueued) {
                        throw std::runtime_error(
                            "implicit presentation commit rejected after successful preflight");
                    }
                    const bool waitQueued = frameLoop.enqueueExternalWait(
                        transport->openClReadySemaphore(frame.slotIndex),
                        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
                    if (!waitQueued) {
                        // The callback is already registered; aborting this
                        // frame settles it false instead of submitting a copy
                        // without its OpenCL-ready dependency.
                        throw std::runtime_error(
                            "implicit presentation wait rejected after successful preflight");
                    }
                    // Do not signal Vulkan-release here: the next production
                    // request signals it once, after this copy/ownership
                    // release in graphics-queue order. No double binary signal.
                    // The queued copy precedes this frame's draw. Persistent
                    // readiness/generation are committed only after submission.
                    implicitCopyScheduled = true;
                    implicitPresentationSelection_ = {
                        .generation = frame.generation,
                        .slotIndex = frame.slotIndex,
                        .allocationGeneration = frame.allocationGeneration,
                        .externalMemory = true,
                    };
                } else {
                    (void)implicitExternalTransport_->quarantine(frame);
                    implicitResizeDiagnostic_ =
                        "implicit external image set could not be copied";
                    implicitExternalRecoveryPending_ = true;
                }
            }
        } else if (implicitProducer_->hasValidOutput()) {
            const auto& output = implicitProducer_->getOutput();
            const bool extentMatches = implicitProducer_->width() == implicitTexture_->width()
                && implicitProducer_->height() == implicitTexture_->height();
            const std::uint32_t currentGeneration =
                implicitProducer_->telemetry().deliveredGeneration;
            if (extentMatches && !output.color.empty()
                && (!implicitTextureReady_ || currentGeneration != lastImplicitUploadGeneration_)) {
                implicitTexture_->uploadStaged(
                    output.color.data(), output.color.size() * sizeof(float));
                if (implicitDepthTexture_ && !output.depth.empty()) {
                    implicitDepthTexture_->uploadStaged(
                        output.depth.data(), output.depth.size() * sizeof(float));
                }
                if (implicitNormalTexture_ && !output.normal.empty()) {
                    implicitNormalTexture_->uploadStaged(
                        output.normal.data(), output.normal.size() * sizeof(float));
                }
                if (implicitVelocityTexture_ && !output.velocity.empty()) {
                    implicitVelocityTexture_->uploadStaged(
                        output.velocity.data(), output.velocity.size() * sizeof(float));
                }
                if (!implicitTextureReady_) {
                    mgfx::MaterialTextureInputs textures{};
                    textures.albedo = implicitTexture_.get();
                    textures.normal = implicitNormalTexture_ ? implicitNormalTexture_.get() : nullptr;
                    textures.occlusion = implicitDepthTexture_ ? implicitDepthTexture_.get() : nullptr;
                    textures.mrao = implicitVelocityTexture_ ? implicitVelocityTexture_.get() : nullptr;
                    engine_.materials()->updateMaterialTextures(implicitMaterial_, textures);
                }
                lastImplicitUploadGeneration_ = currentGeneration;
                implicitTextureReady_ = true;
                stagedImplicitObservation_ = implicitProducer_->deliveredRequest();
                implicitPresentationSelection_ = {
                    .generation = currentGeneration,
                    .externalMemory = false,
                };
            }
        }
    }
    
    // A producer may need several display ticks before its first async
    // completion. Never sample an uninitialized Vulkan image during that
    // startup interval or after a rejected payload.
    if ((implicitTextureReady_ || implicitCopyScheduled) && implicitTexture_ && implicitQuadMesh_) {
        draw.draw(*implicitQuadMesh_, implicitMaterial_, quadModel,
            mgfx::math::vec4(1.0f), implicitSurfaceComposition_
                ? mgfx::pbr::RenderMode::ImplicitGBufferInjection
                : mgfx::pbr::RenderMode::Unlit);
    }

}

} // namespace epi_sculpt
