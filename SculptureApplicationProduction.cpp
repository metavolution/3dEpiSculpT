// Application-owned production admission and producer/transport outcome reconciliation.
// These translation units share one SculptureApplication owner; none is a second scheduler.

#include "SculptureApplication.h"
#include "SculptureVulkanDeviceIdentity.h"
#include "EpiFeedbackFieldExternalTransport.h"
#include "SculptureModeProjection.h"
#include "SculptureCanonicalReleaseAdmission.h"
#include "SculptureCapturePresetProvenance.h"

#include <epi/backend/gpu_types.hpp>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace epi_sculpt {

namespace {
[[nodiscard]] constexpr std::uint64_t nextNonZeroSequence(
    std::uint64_t current) noexcept {
    ++current;
    return current == 0U ? 1U : current;
}

// Compact, world and implicit producer payloads carry a 32-bit generation;
// zero is reserved for an absent identity. Never accept work whose journaled
// identity would be truncated or become the sentinel in the producer ABI.
[[nodiscard]] constexpr std::uint64_t nextProducerGeneration32(
    std::uint64_t current) noexcept {
    return current >= std::numeric_limits<std::uint32_t>::max() ? 0U : current + 1U;
}

[[nodiscard]] epi::CanonicalFeedbackProductionResult produceCanonicalFeedback(
    EpiFeedbackFieldProducer& producer,
    const SculptureCanonicalPublicationInput& input,
    std::size_t ringIndex) {
    return producer.produce_result(input.mode, input.productionDelta,
        input.generation, input.resetEpoch, ringIndex, input.seedImageSelectionRevision, input.seedImageInput);
}

[[nodiscard]] SculptureCanonicalPublicationInputs::Candidate prepareCanonicalFeedbackInput(
    const SculptureFramePlan& framePlan,
    const std::string& sourceStateId, CapturePresetProvenance provenance,
    std::uint64_t generation, std::uint64_t resetEpoch,
    std::uint64_t receiptSequence, float controlTime, float deltaSeconds) {
    const SculptureProgramParams params = framePlan.paramsForProduction(
        SculptureProductionRoute::CanonicalFeedback);
    epi::VisualMode resolved;
    if (const epi::VisualMode* canonicalModeBase =
            framePlan.canonicalFeedbackModeBase();
        canonicalModeBase && framePlan.canonicalFeedbackProjectionBaseline()) {
        resolved = mode_projection::resolveCanonicalModeWithSculptureOverrides(
            *canonicalModeBase, *framePlan.canonicalFeedbackProjectionBaseline(), params);
    } else {
        // The same explicit legacy projection used by the producer adapter.
        mode_projection::sculptureParamsToVisualMode(params, resolved, sourceStateId);
    }
    // Allocate/own all fallible semantic data before OpenCL can accept work.
    // No serialization, pixel copies or native handles enter this record.
    // Retain the frozen source's encoded input; never reacquire its pathname.
    const auto* capturedSource = framePlan.capturedCanonicalSource();
    return std::make_shared<SculptureCanonicalPublicationInput>(std::move(resolved),
        sourceStateId, std::string(provenance.id), std::string(provenance.revision),
        generation, resetEpoch, receiptSequence, controlTime, deltaSeconds,
        framePlan.seedImageSelectionRevision(),
        capturedSource ? capturedSource->seedImageInput : epi::SeedImageInput::Owner{});
}

[[nodiscard]] bool restoreCanonicalFeedbackCheckpoint(
    EpiFeedbackFieldProducer& producer,
    const epi::FeedbackFieldCheckpoint& checkpoint,
    const SculptureFramePlan& framePlan,
    std::uint64_t resumedResetEpoch) {
    const SculptureProgramParams params = framePlan.paramsForProduction(
        SculptureProductionRoute::CanonicalFeedback);
    if (const epi::VisualMode* canonicalModeBase =
            framePlan.canonicalFeedbackModeBase()) {
        if (const SculptureProgramParams* projectedBaseline =
                framePlan.canonicalFeedbackProjectionBaseline()) {
            const epi::VisualMode resolved =
                mode_projection::resolveCanonicalModeWithSculptureOverrides(
                    *canonicalModeBase, *projectedBaseline, params);
            const auto* capturedSource = framePlan.capturedCanonicalSource();
            return producer.restore_checkpoint(
                checkpoint, resolved, resumedResetEpoch, framePlan.seedImageSelectionRevision(),
                capturedSource ? capturedSource->seedImageInput : epi::SeedImageInput::Owner{});
        }
    }
    return producer.restore_checkpoint(
        checkpoint, params, resumedResetEpoch);
}
} // namespace

void SculptureApplication::beginCompactHostStagedTransition() noexcept {
    // At exhaustion no next compact generation exists. Keep the floor bounded;
    // the fresh semantic lease still excludes deliveries from the old preset.
    const auto nextGeneration = nextProducerGeneration32(sculptureGeneration_);
    compactHostStagedGenerationFloor_ = nextGeneration == 0U
        ? std::numeric_limits<std::uint32_t>::max()
        : static_cast<std::uint32_t>(nextGeneration);
    // Zero is reserved for an un-stamped/unknown delivery, so wrap retains a
    // non-zero semantic identity without touching native slot ownership.
    ++compactPresentationLeaseEpoch_;
    if (compactPresentationLeaseEpoch_ == 0U) {
        compactPresentationLeaseEpoch_ = 1U;
    }
}

void SculptureApplication::recordCompactSubmission(
    std::uint32_t generation,
    std::uint64_t leaseEpoch) noexcept {
    auto& stamp = compactSubmissionStamps_[
        generation % kCompactSubmissionStampCapacity];
    stamp = {.generation = generation, .leaseEpoch = leaseEpoch, .valid = true};
}

bool SculptureApplication::deliveryMatchesCompactLease(
    const SculptureSnapshotDelivery& delivery,
    std::uint64_t leaseEpoch) const noexcept {
    if (!delivery.matchesSnapshot() || leaseEpoch == 0U) return false;
    const auto& stamp = compactSubmissionStamps_[
        delivery.generation % kCompactSubmissionStampCapacity];
    return stamp.valid && stamp.generation == delivery.generation
        && stamp.leaseEpoch == leaseEpoch;
}

// complex async handoff of external semaphores between Vulkan and OpenCL.
bool SculptureApplication::clearCanonicalExternalPresentationTargets() noexcept {
    if (!canonicalFeedbackProducer_) return true;
    bool hasImports = false;
    for (std::size_t i = 0; i < EpiFeedbackFieldExternalTransport::kSlotCount; ++i)
        hasImports |= canonicalFeedbackProducer_->has_external_presentation_target(i);
    if (!hasImports) return true;
    // This is a cold replacement transaction. Slot counters alone do not
    // establish native retirement, and recording must never hide a GPU wait.
    if (!presentationHost_ || !presentationHost_->hasDeviceIsland()
        || presentationHost_->frameLoop().frameActive()) return false;
    if (canonicalFeedbackProducer_->wait_idle() == epi::QueueDrainResult::Failed
        || presentationHost_->engine().waitIdleChecked() != VK_SUCCESS) return false;
    for (std::size_t i = 0; i < EpiFeedbackFieldExternalTransport::kSlotCount; ++i) {
        if (!canonicalFeedbackProducer_->clear_external_presentation_target_after_native_retirement(i))
            return false;
    }
    return true;
}

void SculptureApplication::refreshExternalInteropEligibility() noexcept {
    if (!presentationHost_ || !presentationHost_->hasDeviceIsland()) return;

    auto& context = presentationHost_->context();
    const auto vulkanUuid = queryVulkanDeviceUuid(context.device().physical());
    if (sculptureProducer_) {
        static_cast<void>(sculptureProducer_->requalifyExternalMemoryInterop(
            vulkanUuid,
            externalInteropRequested_ && context.device().supportsExternalMemoryFd(),
            externalInteropRequested_ && context.device().supportsExternalSemaphoreFd()));
    }

    canonicalExternalTransportSupported_ = false;
    canonicalExternalDeliveryEnabled_ = false;
    if (!externalInteropRequested_) {
        if (canonicalFeedbackProducer_)
            canonicalFeedbackProducer_->set_external_presentation_enabled(false);
        std::fprintf(stderr, "[3dEpiSculpTMorph] transport: explicit host-staged delivery\n");
        return;
    }
    if (!canonicalFeedbackProducer_ || !canonicalFeedbackProducer_->ready()) return;

    const epi::FeedbackInteropDecision decision = epi::choose_feedback_interop({
        .producer = canonicalFeedbackProducer_->interop_capabilities(),
        .consumer = EpiFeedbackFieldExternalTransport::query_consumer_capabilities(context),
    });
    canonicalExternalTransportSupported_ = decision.uses_external_memory();
    canonicalFeedbackProducer_->set_external_presentation_enabled(false);

    std::fprintf(stderr,
        "[3dEpiSculpTMorph] canonical interop for Vulkan device generation %llu: %s\n",
        static_cast<unsigned long long>(presentationHost_->deviceGeneration()),
        std::string(decision.reason).c_str());
}

bool SculptureApplication::establishCanonicalExternalTransport() {
    if (!canonicalExternalTransportSupported_ || !canonicalFeedbackProducer_
        || !canonicalFeedbackProducer_->ready() || presentationBindings_->canonicalTransport()
        || presentationHost_->frameLoop().frameActive()) {
        return false;
    }

    // Keep the candidate alive across failure handling: OpenCL may already
    // retain some of its allocations when a later import fails.
    std::unique_ptr<EpiFeedbackFieldExternalTransport> transport;
    const auto retireFailedImport = [&] {
        if (!clearCanonicalExternalPresentationTargets()) {
            // Preserve the exporter with any imports whose retirement is uncertain.
            if (transport) presentationBindings_->adoptCanonicalTransport(std::move(transport));
            disableCanonicalExternalDelivery("import setup could not retire native users");
        }
    };
    try {
        transport = std::make_unique<EpiFeedbackFieldExternalTransport>(
            presentationHost_->context(), canonicalFeedbackTransportConfig_);
        if (!bindCanonicalProducerToTransport(*transport))
            throw std::runtime_error("canonical producer import setup failed");

        presentationBindings_->adoptCanonicalTransport(std::move(transport));
        canonicalFeedbackProducer_->set_external_presentation_enabled(true);
        canonicalExternalDeliveryEnabled_ = true;
        return true;
    } catch (const std::exception& error) {
        retireFailedImport();
        std::fprintf(stderr,
            "[3dEpiSculpTMorph] canonical external feedback unavailable (%s)\n",
            error.what());
        return false;
    } catch (...) {
        retireFailedImport();
        std::fprintf(stderr, "[3dEpiSculpTMorph] canonical external feedback unavailable (unknown error)\n");
        return false;
    }
}

bool SculptureApplication::bindCanonicalProducerToTransport(
    EpiFeedbackFieldExternalTransport& transport) {
    if (!canonicalFeedbackProducer_ || !canonicalFeedbackProducer_->ready()) return false;
    if (!clearCanonicalExternalPresentationTargets()) return false;
    for (std::size_t i = 0; i < EpiFeedbackFieldExternalTransport::kSlotCount; ++i) {
        auto producerImport = transport.export_producer_import(
            static_cast<std::uint32_t>(i));
        if (!producerImport) {
            std::fprintf(stderr,
                "[3dEpiSculpTMorph] Warning: could not export canonical external slot %zu\n", i);
            return false;
        }
        if (!canonicalFeedbackProducer_->bind_external_presentation_target(
                producerImport->texture, i)) {
            std::fprintf(stderr,
                "[3dEpiSculpTMorph] Warning: could not bind canonical external slot %zu\n", i);
            return false;
        }
    }
    return true;
}

bool SculptureApplication::rebindCanonicalExternalTransport() {
    if (!canonicalExternalTransportSupported_ || !presentationBindings_
        || !presentationBindings_->canonicalTransport()
        || !canonicalFeedbackProducer_ || !canonicalFeedbackProducer_->ready()) {
        return false;
    }
    if (!bindCanonicalProducerToTransport(*presentationBindings_->canonicalTransport())) {
        canonicalFeedbackProducer_->set_external_presentation_enabled(false);
        canonicalExternalDeliveryEnabled_ = false;
        return false;
    }
    canonicalFeedbackProducer_->set_external_presentation_enabled(true);
    canonicalExternalDeliveryEnabled_ = true;
    return true;
}

bool SculptureApplication::establishWorldExternalTransport() {
    if (!externalInteropRequested_ || !presentationHost_ || !presentationHost_->hasDeviceIsland()
        || !presentationBindings_ || !sculptureProducer_
        || !sculptureProducer_->externalMemoryInteropReady()) {
        return false;
    }
    if (presentationBindings_->worldTransport()) return true;

    SculptureWorldExternalTransport::Config config{};
    // Keep exported allocations alive until all partial OpenCL imports have
    // been cleared, including when import or bookkeeping throws.
    std::unique_ptr<SculptureWorldExternalTransport> transport;
    try {
        transport = std::make_unique<SculptureWorldExternalTransport>(
            presentationHost_->context(), config);
        bool ready = true;
        for (std::uint32_t slot = 0U;
             slot < SculptureWorldExternalTransport::kSlotCount; ++slot) {
            const auto import = transport->exportOpenClImport(slot);
            if (!import
                || !sculptureProducer_->importWorldExternalOutput(slot, *import)
                || !transport->markOpenClImported(slot)) {
                ready = false;
                break;
            }
        }
        if (ready) {
            presentationBindings_->adoptWorldTransport(std::move(transport));
            return true;
        }
    } catch (const std::exception& error) {
        std::fprintf(stderr,
            "[3dEpiSculpTMorph] world external transport unavailable (%s); using host-staged world delivery\n",
            error.what());
    } catch (...) {
        std::fprintf(stderr,
            "[3dEpiSculpTMorph] world external transport unavailable; using host-staged world delivery\n");
    }

    if (!sculptureProducer_->clearWorldExternalOutputs()) {
        std::fprintf(stderr, "[3dEpiSculpTMorph] world import retirement failed; stopping before exporter destruction\n");
        std::terminate();
    }
    presentationBindings_->clearWorldTransport();
    return false;
}

void SculptureApplication::tryRestoreCanonicalExternalDelivery() {
    if (!canonicalExternalTransportSupported_ || canonicalExternalDeliveryEnabled_
        || canonicalFeedbackGeneration_ < canonicalExternalRecoveryNextAttemptGeneration_) {
        return;
    }

    // A previous external field may still be selected for the current frame.
    // Wait until host-staged output has replaced it and every native handoff
    // has retired before changing the bridge's transport pointer. A failed
    // recreation leaves no transport; later bounded retries are then safe.
    const EpiFeedbackFieldDelivery* latest = canonicalFeedbackFields_.latest();
    if (latest && latest->transport == EpiFeedbackFieldTransport::ExternalMemory) {
        return;
    }

    if (presentationBindings_->canonicalTransport()) {
        if (!presentationBindings_->canonicalTransport()->readyForRebuild()) return;
        if (!clearCanonicalExternalPresentationTargets()) return;
        presentationBindings_->clearCanonicalTransport();
    }
    canonicalFeedbackTransportConfig_.allocationGeneration = nextNonZeroSequence(
        canonicalFeedbackTransportConfig_.allocationGeneration);

    if (establishCanonicalExternalTransport()) {
        std::fprintf(stderr,
            "[3dEpiSculpTMorph] canonical external feedback transport restored\n");
        return;
    }

    canonicalExternalRecoveryNextAttemptGeneration_ = canonicalFeedbackGeneration_
        + kCanonicalExternalRecoveryRetryGenerations_;
    canonicalFeedbackProducer_->set_external_presentation_enabled(false);
    std::fprintf(stderr,
        "[3dEpiSculpTMorph] canonical external feedback restore deferred; using host-staged OpenCL delivery\n");
}

void SculptureApplication::disableCanonicalExternalDelivery(const char* reason) noexcept {
    if (!canonicalFeedbackProducer_) return;
    canonicalExternalDeliveryEnabled_ = false;
    canonicalFeedbackProducer_->set_external_presentation_enabled(false);
    canonicalExternalRecoveryNextAttemptGeneration_ = canonicalFeedbackGeneration_
        + kCanonicalExternalRecoveryRetryGenerations_;
    std::fprintf(stderr,
        "[3dEpiSculpTMorph] canonical external feedback disabled: %s; retaining in-flight leases and using host-staged OpenCL delivery\n",
        reason);
}

void SculptureApplication::maybeRetainCanonicalFeedbackCheckpoint(
    const EpiFeedbackFieldDelivery& delivery) noexcept {
    if (!canonicalFeedbackProducer_
        || delivery.transport != EpiFeedbackFieldTransport::HostStaged
        || !canonicalFeedbackCheckpoints_.shouldRetainHostDelivery(delivery.descriptor)) {
        return;
    }
    try {
        auto checkpoint = canonicalFeedbackProducer_->checkpoint_from_host_delivery(delivery);
        if (checkpoint && canonicalFeedbackCheckpoints_.retain(std::move(*checkpoint))) {
            std::fprintf(stderr,
                "[3dEpiSculpTMorph] retained canonical feedback checkpoint generation %llu epoch %llu (%zu/%zu) without extra GPU readback\n",
                static_cast<unsigned long long>(delivery.descriptor.generation),
                static_cast<unsigned long long>(delivery.descriptor.resetEpoch),
                canonicalFeedbackCheckpoints_.size(),
                CanonicalFeedbackCheckpointStore::kCapacity);
        }
    } catch (...) {
        // Checkpoint retention is resilience policy, never frame authority. A
        // failed host copy must not invalidate an already accepted delivery.
    }
}

void SculptureApplication::noteCanonicalFeedbackProducerRejection() noexcept {
    if (canonicalFeedbackProducerRejectStreak_ <
        std::numeric_limits<std::uint32_t>::max()) {
        ++canonicalFeedbackProducerRejectStreak_;
    }
    if (canonicalFeedbackProducerRejectStreak_
        >= kCanonicalFeedbackProducerRecoveryRejectThreshold_) {
        canonicalFeedbackProducerRecoveryPending_ = true;
    }
}

bool SculptureApplication::recoverCanonicalFeedbackProducer(
    const SculptureFramePlan& framePlan,
    bool allowCheckpointRestore) noexcept {
    if (!canonicalFeedbackProducerRecoveryPending_ || !presentationHost_
        || !presentationHost_->hasDeviceIsland() || !presentationBindings_) {
        return false;
    }
    if (canonicalFeedbackProducerRecoveryCooldownFrames_ > 0U) {
        --canonicalFeedbackProducerRecoveryCooldownFrames_;
        return false;
    }

    const std::uint64_t sourceResetEpoch = canonicalFeedbackResetEpoch_;
    if (sourceResetEpoch == std::numeric_limits<std::uint64_t>::max()
        || canonicalFeedbackFields_.resetEpoch() > sourceResetEpoch) {
        // Do not drain or replace the producer if a fresh, monotonic history
        // epoch cannot be represented or the publication owner is ahead.
        canonicalFeedbackProducerRecoveryCooldownFrames_ =
            kCanonicalFeedbackProducerRecoveryCooldownFrames_;
        std::fprintf(stderr,
            "[3dEpiSculpTMorph] canonical producer recovery declined: reset epoch exhausted or inconsistent\n");
        return false;
    }
    const std::uint64_t resumedResetEpoch = sourceResetEpoch + 1U;
    try {
        // Recovery itself is a cold-path event. Before discarding a producer
        // that repeatedly rejected work, make one last attempt to materialize
        // its most recent accepted recursive field. This is not periodic frame
        // work and therefore does not turn checkpointing into a hidden cadence.
        if (canonicalFeedbackProducer_ && canonicalFeedbackProducer_->ready()) {
            if (auto checkpoint = canonicalFeedbackProducer_->capture_checkpoint()) {
                static_cast<void>(canonicalFeedbackCheckpoints_.retain(
                    std::move(*checkpoint)));
            }
            canonicalFeedbackProducer_->set_external_presentation_enabled(false);
            if (canonicalFeedbackProducer_->wait_idle() == epi::QueueDrainResult::Failed) {
                canonicalExternalDeliveryEnabled_ = false;
                canonicalFeedbackProducerRecoveryCooldownFrames_ =
                    kCanonicalFeedbackProducerRecoveryCooldownFrames_;
                std::fprintf(stderr, "[3dEpiSculpTMorph] canonical producer recovery deferred: OpenCL drain failed\n");
                return false;
            }
            if (!clearCanonicalExternalPresentationTargets()) return false;
        }
        canonicalExternalDeliveryEnabled_ = false;

        auto replacement = std::make_unique<EpiFeedbackFieldProducer>(
            canonicalFeedbackProducerConfig_);
        if (!replacement->initialize()) {
            canonicalFeedbackProducer_ = std::move(replacement);
            canonicalFeedbackProducerRecoveryCooldownFrames_ =
                kCanonicalFeedbackProducerRecoveryCooldownFrames_;
            std::fprintf(stderr,
                "[3dEpiSculpTMorph] canonical producer reconstruction deferred: %s\n",
                canonicalFeedbackProducer_->diagnostic().c_str());
            return false;
        }

        bool restoredCheckpoint = false;
        std::uint64_t restoredGeneration = 0U;
        if (allowCheckpointRestore) {
            if (const epi::FeedbackFieldCheckpoint* checkpoint =
                    canonicalFeedbackCheckpoints_.newestCompatible(
                        canonicalFeedbackProducerConfig_.sourceStateId,
                        canonicalFeedbackProducerConfig_.kernelDimension,
                        canonicalFeedbackProducerConfig_.kernelDimension,
                        epi::FeedbackFieldFormat::Rgba16Linear,
                        sourceResetEpoch)) {
                restoredGeneration = checkpoint->descriptor.generation;
                restoredCheckpoint = restoreCanonicalFeedbackCheckpoint(
                    *replacement, *checkpoint, framePlan, resumedResetEpoch);
            }
        }

        if (!canonicalPublicationInputs_.invalidate(canonicalFeedbackFields_, resumedResetEpoch)) {
            throw std::logic_error("canonical recovery reset epoch is stale");
        }
        canonicalFeedbackProducer_ = std::move(replacement);
        canonicalFeedbackResetEpoch_ = resumedResetEpoch;
        canonicalFeedbackCadence_.reset();
        canonicalFeedbackProducerRejectStreak_ = 0U;
        canonicalFeedbackProducerRecoveryPending_ = false;
        canonicalFeedbackProducerRecoveryCooldownFrames_ = 0U;

        // Requalification is producer-specific even though the Vulkan device
        // did not change. If the existing native ring remains valid, bind the
        // replacement OpenCL context to it; otherwise continue host-staged and
        // let the normal bounded transport-rebuild policy retire/recreate it.
        refreshExternalInteropEligibility();
        bool externalRebound = false;
        if (canonicalExternalTransportSupported_
            && presentationBindings_->canonicalTransport()) {
            externalRebound = rebindCanonicalExternalTransport();
        } else if (canonicalExternalTransportSupported_
                   && !presentationBindings_->canonicalTransport()) {
            externalRebound = establishCanonicalExternalTransport();
        }
        if (!externalRebound) {
            canonicalFeedbackProducer_->set_external_presentation_enabled(false);
            canonicalExternalDeliveryEnabled_ = false;
            canonicalExternalRecoveryNextAttemptGeneration_ =
                canonicalFeedbackGeneration_ + kCanonicalExternalRecoveryRetryGenerations_;
        }

        if (restoredCheckpoint) {
            std::fprintf(stderr,
                "[3dEpiSculpTMorph] canonical producer reconstructed from checkpoint generation %llu into epoch %llu; global generation remains %llu\n",
                static_cast<unsigned long long>(restoredGeneration),
                static_cast<unsigned long long>(canonicalFeedbackResetEpoch_),
                static_cast<unsigned long long>(canonicalFeedbackGeneration_));
        } else {
            std::fprintf(stderr,
                "[3dEpiSculpTMorph] canonical producer reconstructed from semantic state into fresh epoch %llu; global generation remains %llu\n",
                static_cast<unsigned long long>(canonicalFeedbackResetEpoch_),
                static_cast<unsigned long long>(canonicalFeedbackGeneration_));
        }
        return true;
    } catch (const std::exception& error) {
        canonicalFeedbackProducerRecoveryCooldownFrames_ =
            kCanonicalFeedbackProducerRecoveryCooldownFrames_;
        std::fprintf(stderr,
            "[3dEpiSculpTMorph] canonical producer reconstruction failed: %s\n",
            error.what());
    } catch (...) {
        canonicalFeedbackProducerRecoveryCooldownFrames_ =
            kCanonicalFeedbackProducerRecoveryCooldownFrames_;
        std::fprintf(stderr,
            "[3dEpiSculpTMorph] canonical producer reconstruction failed with unknown error\n");
    }
    return false;
}

void SculptureApplication::observeCompactProductionPublication(
    const SculptureSnapshotDelivery& delivery) noexcept {
    if (!delivery.matchesSnapshot() || delivery.generation == 0U) return;

    SculptureProductionPublicationIdentity identity{
        .generation = delivery.generation,
        .slotIndex = delivery.slotIndex,
        .transportLeaseEpoch = delivery.ownershipEpoch,
        .transport = delivery.usesExternalMemory()
            ? SculptureProductionTransport::ExternalMemory
            : SculptureProductionTransport::HostStaged,
    };
    static_cast<void>(productionControl_.publish(
        SculptureProductionRoute::CompactInstances, identity));
}

void SculptureApplication::observeWorldProductionPublication(
    const WorldSnapshotDelivery& delivery) noexcept {
    // The bridge consumes the complete delivery contract: generation, metrics,
    // count and slot must agree before the ledger can report a publication.
    if (!delivery) return;

    SculptureProductionPublicationIdentity identity{
        .generation = delivery.generation,
        .slotIndex = delivery.slotIndex,
        .transport = delivery.usesExternalMemory()
            ? SculptureProductionTransport::ExternalMemory
            : SculptureProductionTransport::HostStaged,
    };
    static_cast<void>(productionControl_.publish(
        SculptureProductionRoute::DedicatedWorld, identity));
}

void SculptureApplication::observeCanonicalProductionPublication(
    const EpiFeedbackFieldDelivery& delivery) noexcept {
    if (!delivery.isConsumable()) return;

    SculptureProductionPublicationIdentity identity{
        .generation = delivery.descriptor.generation,
        .resetEpoch = delivery.descriptor.resetEpoch,
        .transport = delivery.transport == EpiFeedbackFieldTransport::ExternalMemory
            ? SculptureProductionTransport::ExternalMemory
            : SculptureProductionTransport::HostStaged,
    };
    if (delivery.externalLease) {
        identity.slotIndex = delivery.externalLease->slotIndex;
        identity.allocationGeneration = delivery.externalLease->allocationGeneration;
        identity.readySerial = delivery.externalLease->readySerial;
    }
    static_cast<void>(productionControl_.publish(
        SculptureProductionRoute::CanonicalFeedback, identity));
}

void SculptureApplication::observeImplicitProductionPublication(
    const ImplicitFrameIdentity& publishedFrame) noexcept {
    if (!publishedFrame.valid()) return;

    SculptureProductionPublicationIdentity identity{
        .generation = publishedFrame.generation,
        .slotIndex = publishedFrame.slotIndex,
        .allocationGeneration = publishedFrame.allocationGeneration,
        .transport = publishedFrame.externalMemory
            ? SculptureProductionTransport::ExternalMemory
            : SculptureProductionTransport::HostStaged,
    };
    static_cast<void>(productionControl_.publish(
        SculptureProductionRoute::ImplicitField, identity));
}

void SculptureApplication::observeImplicitProductionAttempt(
    void* context,
    std::uint64_t scheduleSequence,
    const ImplicitProductionAttemptReport& report) noexcept {
    if (context == nullptr) return;
    static_cast<SculptureApplication*>(context)->settleImplicitProductionAttempt(
        scheduleSequence, report);
}

void SculptureApplication::settleImplicitProductionAttempt(
    std::uint64_t scheduleSequence,
    const ImplicitProductionAttemptReport& report) noexcept {
    PendingImplicitProduction* pending = nullptr;
    for (auto& candidate : pendingImplicitProductions_) {
        if (candidate.matches(scheduleSequence)) {
            pending = &candidate;
            break;
        }
    }

    // The opaque observer token must resolve to the exact request generation.
    // A mismatched native callback cannot be promoted into mathematical truth.
    if (pending == nullptr
        || report.generation != pending->request.frame.generation) {
        static_cast<void>(productionControl_.defer(
            scheduleSequence,
            SculptureProductionDeferralReason::TransportRejected));
        if (pending != nullptr) {
            implicitObservationCadence_.completeAttempt(pending->request, false);
            pending->clear();
        }
        return;
    }

    implicitObservationCadence_.completeAttempt(
        pending->request, report.accepted());

    if (report.accepted()) {
        implicitGeneration_ = report.generation;
        appState_.temporalReset.acknowledge(
            pending->temporalReset, SculptureProductionRoute::ImplicitField);
        static_cast<void>(productionControl_.accept(
            scheduleSequence, report.generation));
    } else {
        SculptureProductionDeferralReason reason =
            SculptureProductionDeferralReason::ProducerRejected;
        switch (report.outcome) {
        case ImplicitProductionAttemptOutcome::Accepted:
            break;
        case ImplicitProductionAttemptOutcome::ProducerBackpressure:
            reason = SculptureProductionDeferralReason::ProducerBackpressure;
            break;
        case ImplicitProductionAttemptOutcome::ProducerRejected:
            reason = SculptureProductionDeferralReason::ProducerRejected;
            break;
        case ImplicitProductionAttemptOutcome::FrameSubmissionRejected:
            reason = SculptureProductionDeferralReason::FrameSubmissionRejected;
            break;
        case ImplicitProductionAttemptOutcome::TransportBackpressure:
            reason = SculptureProductionDeferralReason::TransportBackpressure;
            break;
        case ImplicitProductionAttemptOutcome::TransportRejected:
            reason = SculptureProductionDeferralReason::TransportRejected;
            break;
        }
        static_cast<void>(productionControl_.defer(scheduleSequence, reason));
    }

    pending->clear();
}

void SculptureApplication::cancelPendingImplicitProductions(
    SculptureProductionDeferralReason reason) noexcept {
    for (auto& pending : pendingImplicitProductions_) {
        if (pending.scheduleSequence == 0U) continue;
        implicitObservationCadence_.completeAttempt(pending.request, false);
        static_cast<void>(productionControl_.defer(
            pending.scheduleSequence, reason));
        pending.clear();
    }
}

void SculptureApplication::prepareImplicitProduction(
    FrameContext& ctx,
    const mgfx::Camera& camera) {
    const SculptureFramePlan& framePlan = ctx.plan();
    if (!includesImplicitField(framePlan.presentationOwnership())) return;

    const auto nextGeneration = nextProducerGeneration32(implicitGeneration_);
    // Preparation also polls earlier accepted work. At exhaustion, use the
    // last valid identity only for that poll and discard the prepared request;
    // it must never reach cadence admission or producer submission again.
    const auto generation = nextGeneration == 0U
        ? implicitGeneration_ : static_cast<std::uint32_t>(nextGeneration);

    ImplicitFrameRequest request{};
    const bool requestReady = presentationBindings_->director().prepareImplicitObservation(
            camera,
            framePlan.paramsForProduction(SculptureProductionRoute::ImplicitField),
            framePlan.worldState().time,
            framePlan.worldState().morph,
            ctx.deltaSeconds,
            generation,
            request);

    // Polling can publish an earlier accepted generation before the new
    // request fails validation. Observe that independent outcome even when no
    // new request may enter cadence or production admission.
    observeImplicitProductionPublication(
        presentationBindings_->director().implicitPublishedFrame());
    if (!requestReady || nextGeneration == 0U
        || productionControl_.scheduleSequenceExhausted()) return;

    PendingImplicitProduction* pending = nullptr;
    for (auto& candidate : pendingImplicitProductions_) {
        // The next generation is based on accepted work. Until the existing
        // callback settles, another offer would reuse that same generation.
        // Polling above remains available without consuming cadence here.
        if (candidate.scheduleSequence != 0U) return;
        if (pending == nullptr) pending = &candidate;
    }
    if (pending == nullptr) {
        // Keep the bounded-capacity fallback if the storage is ever empty.
        return;
    }

    const double observationSeconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    // Token identity survives acknowledgment. Keep excluding its payload from
    // view dirtiness after pending() clears; submit the original request below.
    if (!implicitObservationCadence_.beginAttempt(
            observationSeconds,
            request,
            presentationBindings_->director().implicitUpdateHz(),
            framePlan.temporalReset().pending(SculptureProductionRoute::ImplicitField),
            framePlan.temporalReset().sequence != 0U
                ? ImplicitObservationCadence::ResetComparison::CommandToken
                : ImplicitObservationCadence::ResetComparison::Payload)) {
        return;
    }

    SculptureProductionSchedule* const schedule = productionControl_.record(
        ctx.productionSchedules,
        {
            .route = SculptureProductionRoute::ImplicitField,
            .frameSequence = framePlan.frameSequence(),
            .timing = {
                .submit = true,
                .simulationDeltaSeconds = request.frame.deltaSeconds,
                .simulationTime = request.frame.time,
                .cadenceDue = true,
                .parameterTriggered = false,
            },
            .generationIntent = generation,
        });
    if (schedule == nullptr) {
        implicitObservationCadence_.completeAttempt(request, false);
        return;
    }

    pending->scheduleSequence = schedule->scheduleSequence;
    pending->request = request;
    pending->temporalReset = framePlan.temporalReset();

    try {
        presentationBindings_->director().submitImplicitObservation(
            request,
            {
                .handler = &SculptureApplication::observeImplicitProductionAttempt,
                .context = this,
                .token = schedule->scheduleSequence,
            });
        // Host-staged submission polls again before attempting new work. That
        // poll can publish an older accepted generation after preparation and
        // before the draw selects it, even if this new submission is rejected.
        observeImplicitProductionPublication(
            presentationBindings_->director().implicitPublishedFrame());
    } catch (...) {
        // Submission adapters may throw only for broken native invariants. The
        // frame will still fail, but settle the control-plane transaction first
        // so recovery never inherits a permanently pending cadence/receipt.
        implicitObservationCadence_.completeAttempt(request, false);
        static_cast<void>(productionControl_.defer(
            schedule->scheduleSequence,
            SculptureProductionDeferralReason::TransportRejected));
        pending->clear();
        throw;
    }
}

std::uint64_t SculptureApplication::publishedProductionReceiptSequence(
    SculptureProductionRoute route,
    std::uint64_t generation,
    std::uint64_t resetEpoch) const noexcept {
    const SculptureProductionReceipt* receipt =
        productionControl_.findPublished(route, generation, resetEpoch);
    return receipt ? receipt->scheduleSequence : 0U;
}

void SculptureApplication::deferCanonicalProduction(
    const SculptureProducerCadenceProposal<SculptureProgramParams>& proposal,
    std::uint64_t scheduleSequence, SculptureProductionDeferralReason reason) noexcept {
    canonicalFeedbackCadence_.defer(proposal);
    static_cast<void>(productionControl_.defer(scheduleSequence, reason));
}

void SculptureApplication::executeCanonicalFeedbackProduction(
    const SculptureFramePlan& framePlan,
    const SculptureProductionSchedule& schedule,
    const SculptureProducerCadenceProposal<SculptureProgramParams>& proposal,
    SculptureCanonicalPublicationInputs::Candidate input, std::uint32_t ringIndex,
    std::optional<CanonicalExternalSubmission> external) {
    auto production = produceCanonicalFeedback(*canonicalFeedbackProducer_, *input, ringIndex);
    const auto generation = input->generation;
    const auto resetEpoch = input->resetEpoch;
    const auto sequence = input->productionReceiptSequence;
    const auto expectedTransport = external ? EpiFeedbackFieldTransport::ExternalMemory
                                            : EpiFeedbackFieldTransport::HostStaged;

    // Both routes commit the same semantic facts exactly once. The optional
    // context supplies only the external native side effects; it owns no state.
    settleCanonicalProduction(production.mathematicsAccepted,
        [&]() noexcept {
            if (external)
                external->transport.quarantineSubmittedRelease(ringIndex, external->ticket);
            if (!production.requiredImageUnavailable()) noteCanonicalFeedbackProducerRejection();
            deferCanonicalProduction(proposal, sequence,
                SculptureProductionDeferralReason::ProducerRejected);
            if (external) {
                disableCanonicalExternalDelivery("external OpenCL production failed before recursive acceptance");
            }
        },
        [&]() noexcept {
            canonicalFeedbackProducerRejectStreak_ = 0U;
            canonicalFeedbackGeneration_ = std::max(canonicalFeedbackGeneration_, generation);
            canonicalFeedbackCadence_.commit(proposal);
            appState_.temporalReset.acknowledge(
                framePlan.temporalReset(), SculptureProductionRoute::CanonicalFeedback);
            static_cast<void>(productionControl_.accept(
                sequence, generation, resetEpoch, schedule.semanticLeaseEpoch));
        },
        [&] {
            auto field = std::move(production.delivery);
            if (!field || (external && field->transport != expectedTransport))
                return CanonicalPublicationResult::MissingDelivery;
            if (external) {
                field->externalLease = external->transport.submitOpenClWrite(
                    ringIndex, generation, resetEpoch, external->ticket,
                    [this](const auto& request, auto& ownership) noexcept {
                        return canonicalFeedbackProducer_->submit_external_feedback(request, ownership);
                    });
            } else {
                maybeRetainCanonicalFeedbackCheckpoint(*field);
            }
            if (!canonicalPublicationInputs_.publish(
                    canonicalFeedbackFields_, std::move(field), expectedTransport, std::move(input))) {
                return CanonicalPublicationResult::Rejected;
            }
            if (const auto* published = canonicalFeedbackFields_.latest())
                observeCanonicalProductionPublication(*published);
            return CanonicalPublicationResult::Published;
        },
        [&](CanonicalPublicationResult result) noexcept {
            if (external)
                external->transport.quarantineSubmittedRelease(ringIndex, external->ticket);
            static_cast<void>(productionControl_.rejectPublication(sequence));
            if (external) {
                disableCanonicalExternalDelivery(result == CanonicalPublicationResult::MissingDelivery
                    ? "external OpenCL delivery rejected" : "external delivery publication rejected");
            }
        });
}

void SculptureApplication::dispatchCanonicalFeedback(
    const SculptureFramePlan& framePlan,
    const SculptureProductionSchedule& schedule,
    const SculptureProducerCadenceProposal<SculptureProgramParams>& proposal) {
    if (!schedule.submitDue()) return;
    if (schedule.generationIntent == 0U) {
        deferCanonicalProduction(proposal, schedule.scheduleSequence,
            SculptureProductionDeferralReason::GenerationExhausted);
        return;
    }
    if (!canonicalFeedbackCadence_.canCommit(proposal)) {
        deferCanonicalProduction(proposal, schedule.scheduleSequence,
            SculptureProductionDeferralReason::CadenceProposalStale);
        return;
    }
    if (!canonicalFeedbackProducer_ || !canonicalFeedbackProducer_->ready()) {
        noteCanonicalFeedbackProducerRejection();
        deferCanonicalProduction(proposal, schedule.scheduleSequence,
            SculptureProductionDeferralReason::ProducerRejected);
        return;
    }

    // Mathematical identity and transport slot selection are intentionally
    // independent. A rejected frame submission or external handoff may consume
    // a transport *attempt*, but it must not consume a mathematical generation.
    // The application admitted this mathematical identity with the schedule;
    // native transport attempt identities remain independent of that choice.
    const std::uint64_t candidateGeneration = schedule.generationIntent;
    const std::uint64_t transportAttempt =
        nextNonZeroSequence(canonicalFeedbackTransportAttemptSequence_);
    const std::uint32_t preferredRingIndex = static_cast<std::uint32_t>(
        (transportAttempt - 1U) % EpiFeedbackFieldExternalTransport::kSlotCount);
    const auto prepareInput = [&] {
        const auto& preset = presetAt(framePlan.presetIndex());
        const auto provenance = resolveCapturePresetProvenance(framePlan, preset.id, preset.revision);
        return prepareCanonicalFeedbackInput(framePlan, canonicalFeedbackProducerConfig_.sourceStateId,
            provenance, candidateGeneration, canonicalFeedbackResetEpoch_, schedule.scheduleSequence,
            proposal.simulationTime, proposal.simulationDeltaSeconds);
    };

    if (canonicalExternalDeliveryEnabled_
        && presentationBindings_->canonicalTransport()) {
        auto* transport = presentationBindings_->canonicalTransport();
        const auto writableSlot = transport->findWritableSlot(preferredRingIndex);
        if (!writableSlot) {
            const auto health = transport->health();
            if (health.unavailable == EpiFeedbackFieldExternalTransport::kSlotCount) {
                disableCanonicalExternalDelivery("all canonical external slots quarantined");
            } else {
                // Every non-quarantined slot is still in an ownership cycle.
                // Backpressure is cheaper and more correct than discarding the
                // whole transport because the preferred ring position is busy.
                deferCanonicalProduction(proposal, schedule.scheduleSequence,
                    SculptureProductionDeferralReason::TransportBackpressure);
                return;
            }
        } else {
            const std::uint32_t ringIndex = *writableSlot;
            if (!canonicalFeedbackProducer_->has_external_presentation_target(ringIndex)) {
                disableCanonicalExternalDelivery("writable canonical slot is not imported by producer");
            } else {
                auto& frameLoop = presentationHost_->engine().frameLoop();
                // Native handoff records are a bounded per-frame resource just like
                // the external image ring itself. Capacity pressure is ordinary
                // backpressure: do not reserve a generation, advance cadence, or
                // mutate transport ownership until the whole submit-side transaction
                // can be represented.
                if (!frameLoop.canEnqueueSubmission({
                        .externalSignals = 1U,
                        .commitCallbacks = 1U,
                    })) {
                    deferCanonicalProduction(proposal, schedule.scheduleSequence,
                        SculptureProductionDeferralReason::TransportBackpressure);
                    return;
                }

                // FrameContext owns these immutable values until FrameLoop has
                // settled its callbacks. Borrow them instead of copying params.
                const auto* const framePlanForSubmit = &framePlan;
                const auto* const proposalForSubmit = &proposal;
                const auto* const scheduleForSubmit = &schedule;
                auto input = prepareInput();
                const bool commitQueued = frameLoop.enqueueSubmitCommit(
                    [this, ringIndex, framePlanForSubmit, proposalForSubmit,
                     scheduleForSubmit, input = std::move(input)](bool success) {
                        const auto& settledProposal = *proposalForSubmit;
                        const auto sequence = scheduleForSubmit->scheduleSequence;
                        if (!success) {
                            deferCanonicalProduction(settledProposal, sequence,
                                SculptureProductionDeferralReason::FrameSubmissionRejected);
                            return;
                        }
                        auto* currentTransport = presentationBindings_->canonicalTransport();
                        if (!currentTransport) {
                            deferCanonicalProduction(settledProposal, sequence,
                                SculptureProductionDeferralReason::TransportRejected);
                            disableCanonicalExternalDelivery("canonical transport missing after Vulkan release");
                            return;
                        }
                        epi::FeedbackExternalSlotTicket ticket{};
                        const auto admission = admitCanonicalReleasedWork(
                            [&] { return currentTransport->markVulkanReleaseSubmitted(
                                ringIndex, input->generation, &ticket); },
                            [&] { return canonicalFeedbackCadence_.canCommit(settledProposal); },
                            [&] { currentTransport->quarantineSubmittedRelease(ringIndex, ticket); },
                            [&] {
                                deferCanonicalProduction(settledProposal, sequence,
                                    SculptureProductionDeferralReason::CadenceProposalStale);
                            },
                            [&] { disableCanonicalExternalDelivery(
                                "stale canonical cadence proposal after Vulkan release"); });
                        if (admission == CanonicalReleaseAdmission::ReleaseRejected) {
                            deferCanonicalProduction(settledProposal, sequence,
                                SculptureProductionDeferralReason::TransportRejected);
                            disableCanonicalExternalDelivery("cannot record submitted canonical Vulkan release");
                            return;
                        }
                        if (admission == CanonicalReleaseAdmission::Stale) return;

                        executeCanonicalFeedbackProduction(*framePlanForSubmit,
                            *scheduleForSubmit, settledProposal, input, ringIndex,
                            CanonicalExternalSubmission{*currentTransport, ticket});
                });
                if (!commitQueued) {
                    // The compatibility adapter has already settled the callback
                    // with success=false. No native ownership record was queued.
                    return;
                }

                if (!frameLoop.enqueueExternalSignal(
                        presentationBindings_->canonicalTransport()->vulkanReleaseSemaphore(ringIndex),
                        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT)) {
                    // Preflight made this impossible during single-threaded frame
                    // recording. Abort the frame rather than submit a commit record
                    // without its matching native semaphore operation; FrameLoop
                    // will settle the already-queued callback with success=false.
                    throw std::logic_error(
                        "canonical feedback handoff capacity changed after preflight");
                }

                // Rotate transport attempts only after the complete native
                // submission transaction is represented. This sequence is resource
                // policy, not mathematical identity; the latter advances only when
                // OpenCL accepts the canonical step.
                canonicalFeedbackTransportAttemptSequence_ = transportAttempt;
                return;
            }
        }
    }

    // Host staging enters the same semantic operation immediately. External
    // delivery enters it only after acceptance of its matching graphics release.
    executeCanonicalFeedbackProduction(framePlan, schedule, proposal,
        prepareInput(), preferredRingIndex);
}

// we isolate performance analytics and improve educational modularity.
bool SculptureApplication::dispatchWorldProduction(
    const SculptureFramePlan& framePlan,
    const SculptureProductionSchedule& schedule) {
    const SculptureWorldState& worldState = framePlan.worldState();
    static_cast<void>(sculptureProducer_->produceWorld(
        framePlan.paramsForProduction(SculptureProductionRoute::DedicatedWorld),
        framePlan.effectiveWorldRecipe(),
        worldState.time, worldState.morph,
        schedule.timing.simulationDeltaSeconds,
        static_cast<std::uint32_t>(schedule.generationIntent)));

    const auto& worldTelemetry = sculptureProducer_->worldTelemetry();
    if (worldTelemetry.submissionAccepted) {
        worldProducerCadence_.observeProducerLatency(worldTelemetry.openClCompletionMilliseconds);
        worldQualityPolicy_.observe(worldTelemetry.openClCompletionMilliseconds,
                                    worldTelemetry.openClKernelMilliseconds,
                                    worldTelemetry.openClReadbackMilliseconds,
                                    worldTelemetry.pendingSlots);
        stagedGovernor_.observeGpuLatency(worldTelemetry.openClCompletionMilliseconds);
    }
    return worldTelemetry.submissionAccepted;
}

void SculptureApplication::executeStageProduction(FrameContext& ctx) {
    // Production scheduling is now a two-phase control-plane transaction:
    // prepare decides whether work is due, while cadence/generation commit only
    // after the producer (or deferred frame-recording handoff) accepts it.
    const SculptureFramePlan& framePlan = ctx.plan();
    const SculptureWorldState& worldState = framePlan.worldState();
    const SculptureProductionDecision production = framePlan.production();
    const SculptureProgramParams cadenceParams = framePlan.paramsForCadence(production.route);
    const SculptureTemporalResetIntent& temporalReset = framePlan.temporalReset();

    const bool implicitPresentationActive =
        includesImplicitField(framePlan.presentationOwnership());
    // Prepare the route transition on a value copy. A failed epoch admission
    // must not consume the route change before the next frame can observe it.
    auto nextRouteSession = productionRouteSession_;
    const SculptureProductionRouteTransition routeTransition =
        nextRouteSession.observe(production.route, implicitPresentationActive);

    // A command stays pending through rejection, but reserves its canonical
    // epoch only once. Repeating invalidation on every retry would discard the
    // very history identity that the deferred production is meant to accept.
    const bool canonicalCommandReset = production.usesCanonicalFeedback()
        && temporalReset.pending(SculptureProductionRoute::CanonicalFeedback)
        && temporalReset.sequence != canonicalFeedbackResetRequestSequence_;
    const bool canonicalSemanticReset = routeTransition.canonicalActivityChanged
        || canonicalCommandReset
        || (production.usesCanonicalFeedback() && temporalReset.sequence == 0U
            && cadenceParams.temporalReset);
    if (canonicalSemanticReset) {
        if (canonicalFeedbackResetEpoch_ == std::numeric_limits<std::uint64_t>::max()) {
            throw std::overflow_error("canonical feedback reset epoch exhausted");
        }
        if (canonicalFeedbackFields_.resetEpoch() > canonicalFeedbackResetEpoch_) {
            throw std::logic_error("canonical delivery epoch exceeds application epoch");
        }
        const auto nextEpoch = canonicalFeedbackResetEpoch_ + 1U;
        if (!canonicalPublicationInputs_.invalidate(canonicalFeedbackFields_, nextEpoch)) {
            throw std::logic_error("canonical feedback reset epoch is stale");
        }
        canonicalFeedbackResetEpoch_ = nextEpoch;
    }

    productionRouteSession_ = nextRouteSession;
    // Route changes reset only the cadence owners affected by this transition.
    if (routeTransition.resetCompactCadence) compactProducerCadence_.reset();
    if (routeTransition.resetWorldCadence) worldProducerCadence_.reset();
    if (routeTransition.resetCanonicalCadence) canonicalFeedbackCadence_.reset();
    if (routeTransition.resetImplicitCadence) implicitObservationCadence_.reset();
    if (canonicalSemanticReset) {
        canonicalFeedbackCadence_.reset();
        if (canonicalCommandReset) {
            canonicalFeedbackResetRequestSequence_ = temporalReset.sequence;
        }
    }

    // Producer recovery is serviced at the application production boundary,
    // never from a native submit callback. This guarantees that transport
    // callbacks merely report failure while ownership/lifetime composition is
    // changed from one stable frame stage. A semantic reset deliberately
    // forbids checkpoint replay from the preceding recursive history.
    if (production.usesCanonicalFeedback() && canonicalFeedbackGeneration_ > 0U
        && (!canonicalFeedbackProducer_ || !canonicalFeedbackProducer_->ready())) {
        canonicalFeedbackProducerRecoveryPending_ = true;
    }
    if (production.usesCanonicalFeedback()
        && canonicalFeedbackGeneration_ != std::numeric_limits<std::uint64_t>::max()
        && canonicalFeedbackProducerRecoveryPending_) {
        static_cast<void>(recoverCanonicalFeedbackProducer(
            framePlan, !canonicalSemanticReset
                && !temporalReset.pending(SculptureProductionRoute::CanonicalFeedback)));
    }

    ctx.packedSculpture = nullptr;
    ctx.productionSchedules.clear();
    ctx.canonicalCadenceProposal.reset();
    if (productionControl_.scheduleSequenceExhausted()
        && !productionScheduleExhaustionReported_) {
        std::fprintf(stderr,
            "[SculptureApplication] production schedule identity exhausted; retaining available deliveries\n");
        productionScheduleExhaustionReported_ = true;
    }
    const auto recordOpportunity = [this, &ctx](
        const SculptureProductionControlPlane::Opportunity& opportunity)
        -> SculptureProductionSchedule& {
        SculptureProductionSchedule* const schedule =
            productionControl_.record(ctx.productionSchedules, opportunity);
        if (schedule == nullptr) {
            throw std::logic_error(
                "Sculpture production control-plane capacity/invariant rejected opportunity");
        }
        return *schedule;
    };

    switch (production.route) {
    case SculptureProductionRoute::CanonicalFeedback: {
        auto proposal = canonicalFeedbackCadence_.prepare(
            worldState.time, cadenceParams, temporalReset.pending(production.route));
        if (proposal.submit && productionControl_.scheduleSequenceExhausted()) {
            // No receipt identity means no admissible producer transaction.
            // Keep cadence due while the last publication remains observable.
            canonicalFeedbackCadence_.defer(proposal);
            break;
        }
        // Mathematical generation is global across reset epochs and must not
        // wrap. Zero records that no new identity can be represented.
        const auto generationIntent = canonicalFeedbackGeneration_
            == std::numeric_limits<std::uint64_t>::max()
            ? 0U : canonicalFeedbackGeneration_ + 1U;
        SculptureProductionSchedule& schedule = recordOpportunity({
            .route = production.route,
            .frameSequence = framePlan.frameSequence(),
            .timing = makeProductionTiming(proposal),
            .generationIntent = generationIntent,
            .resetEpoch = canonicalFeedbackResetEpoch_,
        });
        if (!schedule.submitDue()) {
            canonicalFeedbackCadence_.defer(proposal);
        } else if (generationIntent == 0U) {
            // Do not submit work, advance cadence or acknowledge a reset when
            // its accepted mathematical identity would collide with history.
            canonicalFeedbackCadence_.defer(proposal);
            static_cast<void>(productionControl_.defer(
                schedule.scheduleSequence,
                SculptureProductionDeferralReason::GenerationExhausted));
        } else {
            // Only due work can reach the deferred frame-recording handoff.
            // Keep its owned proposal alive until that frame is settled.
            ctx.canonicalCadenceProposal.emplace(std::move(proposal));
        }
        break;
    }
    case SculptureProductionRoute::ImplicitField:
        break;
    case SculptureProductionRoute::CanonicalFeedbackUnavailable:
        break;
    case SculptureProductionRoute::DedicatedWorld: {
        auto proposal = worldProducerCadence_.prepare(
            worldState.time, cadenceParams, temporalReset.pending(production.route));
        if (proposal.submit && productionControl_.scheduleSequenceExhausted()) {
            worldProducerCadence_.defer(proposal);
            break;
        }
        const auto generationIntent = nextProducerGeneration32(worldGeneration_);
        SculptureProductionSchedule& schedule = recordOpportunity({
            .route = production.route,
            .frameSequence = framePlan.frameSequence(),
            .timing = makeProductionTiming(proposal),
            .generationIntent = generationIntent,
        });
        if (!schedule.submitDue()) {
            worldProducerCadence_.defer(proposal);
            break;
        }
        if (generationIntent == 0U) {
            // No producer call or cadence/reset commit without a valid identity.
            worldProducerCadence_.defer(proposal);
            static_cast<void>(productionControl_.defer(
                schedule.scheduleSequence,
                SculptureProductionDeferralReason::GenerationExhausted));
            break;
        }
        if (dispatchWorldProduction(framePlan, schedule)) {
            worldGeneration_ = static_cast<std::uint32_t>(schedule.generationIntent);
            worldProducerCadence_.commit(proposal);
            appState_.temporalReset.acknowledge(temporalReset, production.route);
            static_cast<void>(productionControl_.accept(
                schedule.scheduleSequence, schedule.generationIntent));
        } else {
            worldProducerCadence_.defer(proposal);
            static_cast<void>(productionControl_.defer(
                schedule.scheduleSequence,
                SculptureProductionDeferralReason::ProducerBackpressure));
        }
        break;
    }
    case SculptureProductionRoute::CompactInstances: {
        auto proposal = compactProducerCadence_.prepare(
            worldState.time, cadenceParams, temporalReset.pending(production.route));
        if (proposal.submit && productionControl_.scheduleSequenceExhausted()) {
            compactProducerCadence_.defer(proposal);
            ctx.packedSculpture = sculptureProducer_->pollCompleted();
            break;
        }
        const auto generationIntent = nextProducerGeneration32(sculptureGeneration_);
        SculptureProductionSchedule& schedule = recordOpportunity({
            .route = production.route,
            .frameSequence = framePlan.frameSequence(),
            .timing = makeProductionTiming(proposal),
            .generationIntent = generationIntent,
            .semanticLeaseEpoch = framePlan.compactPresentationLeaseEpoch(),
        });
        if (!schedule.submitDue()) {
            compactProducerCadence_.defer(proposal);
            // Completion is independent of future scientific opportunities.
            // A paused restored moment can have one accepted async dispatch
            // whose delivery arrives after its cadence/reset was committed.
            ctx.packedSculpture = sculptureProducer_->pollCompleted();
            break;
        }
        if (generationIntent == 0U) {
            compactProducerCadence_.defer(proposal);
            static_cast<void>(productionControl_.defer(
                schedule.scheduleSequence,
                SculptureProductionDeferralReason::GenerationExhausted));
            ctx.packedSculpture = sculptureProducer_->pollCompleted();
            break;
        }

        const auto generation = static_cast<std::uint32_t>(schedule.generationIntent);
        ctx.packedSculpture = sculptureProducer_->produce(
            framePlan.paramsForProduction(production.route),
            worldState.time, worldState.morph,
            proposal.simulationDeltaSeconds, generation);

        const auto& prodTelem = sculptureProducer_->telemetry();
        if (prodTelem.submissionAccepted) {
            sculptureGeneration_ = schedule.generationIntent;
            recordCompactSubmission(
                generation, framePlan.compactPresentationLeaseEpoch());
            compactProducerCadence_.commit(proposal);
            appState_.temporalReset.acknowledge(temporalReset, production.route);
            static_cast<void>(productionControl_.accept(
                schedule.scheduleSequence, schedule.generationIntent,
                schedule.resetEpoch, schedule.semanticLeaseEpoch));

            const float latency = prodTelem.outputFromOpenCl
                ? prodTelem.openClCompletionMilliseconds
                : prodTelem.cpuCallMilliseconds;
            compactProducerCadence_.observeProducerLatency(latency);
            stagedGovernor_.observeGpuLatency(latency);
        } else {
            compactProducerCadence_.defer(proposal);
            static_cast<void>(productionControl_.defer(
                schedule.scheduleSequence,
                SculptureProductionDeferralReason::ProducerBackpressure));
        }
        break;
    }
    }

    ctx.delivery = sculptureProducer_->delivery();
    observeCompactProductionPublication(ctx.delivery);
    observeWorldProductionPublication(sculptureProducer_->worldDelivery());
    if (const EpiFeedbackFieldDelivery* canonicalDelivery = canonicalFeedbackFields_.latest()) {
        observeCanonicalProductionPublication(*canonicalDelivery);
    }
}

} // namespace epi_sculpt
