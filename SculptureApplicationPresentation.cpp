// View construction and presentation of completed, generation-qualified deliveries.
// These translation units share one SculptureApplication owner; none is a second scheduler.

#include "SculptureApplication.h"
#include "CanonicalFeedbackFrameSelection.h"
#include "SculptureApplicationHud.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>

namespace epi_sculpt {

// Launch observation changes no scientific program parameters or playback state.
// Rendering and semantic saves share this application policy; ordinary captures
// still record the actual camera after navigation/stereo composition.
mgfx::Camera SculptureApplication::applyStartupCamera(mgfx::Camera camera) const noexcept {
    if (startupCameraPose_) {
        const auto& pose = *startupCameraPose_;
        camera.eye = mgfx::math::vec3(pose.eye[0], pose.eye[1], pose.eye[2]);
        camera.target = mgfx::math::vec3(pose.target[0], pose.target[1], pose.target[2]);
        camera.up = mgfx::math::vec3(0.0f, 1.0f, 0.0f);
    }
    return camera; // Preserve the authored lens, clip range and projection jitter.
}

void SculptureApplication::formatHud(
    char* buffer, std::size_t size, const SculptureFramePlan& framePlan) {
    formatSculptureApplicationHud(buffer, size, framePlan, *this);
}

// One scene path is shared by desktop, stereo and immersive XR observations.
bool SculptureApplication::renderScenePass(
    mgfx::Drawable& draw,
    const mgfx::Camera& camera,
    const SculptureProgramParams& params,
    const SculptureWorldState& worldState,
    const SculptureSnapshotDelivery& delivery,
    const WorldSnapshotDelivery& worldDelivery,
    const EpiFeedbackFieldDelivery* canonicalField,
    bool dedicatedWorld,
    bool canonicalReliefRequested,
    bool canonicalFeedbackUnavailable,
    float deltaSeconds,
    const char* hud) {

    if (dedicatedWorld) {

        presentationBindings_->compactBridge().rebuild(nullptr);

        // Only present the completed world delivery. An allocated external
        // grid buffer is not a delivery: it needs producer completion and a
        // Vulkan acquire/retirement contract before it can become drawable.
        return presentationBindings_->director().drawWorldScene(
            draw, presentationBindings_->worldBridge(),
            worldDelivery, worldState.time, hud, nullptr);

    } else {
        presentationBindings_->director().drawScene(
            draw, camera, presentationBindings_->compactBridge(), params, delivery,
            worldState.time, worldState.morph, deltaSeconds, hud,
            canonicalReliefRequested, canonicalField, canonicalFeedbackUnavailable,
            nullptr, nullptr);
    }
    return false;
}

void SculptureApplication::renderFrame(FrameContext& ctx) {
    const SculptureFramePlan& framePlan = ctx.plan();
    // Draws and capture intent consume the same frozen presentation record.
    // A palette/optics edit never acquires a new scientific publication.
    presentationBindings_->setVolumePresentation(framePlan.presentation().volumePresentation);
    const SculptureProgramParams& params = framePlan.params();
    const auto extent = presentationHost_->engine().swapchain().extent();
    const float w = static_cast<float>(extent.width);
    const float h = static_cast<float>(extent.height);
    const auto& worldState = framePlan.worldState();
    const auto& delivery = ctx.delivery;
    const float deltaSeconds = ctx.deltaSeconds;
    const char* hud = ctx.hud;
    const SculptureProductionDecision production = framePlan.production();
    const SculptureProductionRoute routeDecision = production.route;
    const bool dedicatedWorld = production.ownsDedicatedWorldProduction();
    // Canonical intent owns presentation even when its producer is unavailable:
    // passing false would silently route the same frame through compact draws.
    const bool canonicalReliefRequested = production.requestsCanonicalFeedback();
    SculptureSnapshotDelivery presentationDelivery{};
    SculptureSnapshotDelivery retirementDelivery{};
    const bool compactDeliveryCurrent = production.ownsCompactProduction()
        && delivery.matchesSnapshot()
        // The application stops admitting compact work at UINT32_MAX; accepted
        // generations never wrap. Ordinary ordering also handles floor zero and
        // gaps larger than half the 32-bit range without hiding valid output.
        && delivery.generation >= compactHostStagedGenerationFloor_
        && deliveryMatchesCompactLease(
            delivery, framePlan.compactPresentationLeaseEpoch());
    if (compactDeliveryCurrent) {
        presentationDelivery = delivery;
    } else if (delivery.usesExternalMemory()) {
        // A native ticket remains a synchronization obligation even when its
        // semantic lease is stale or its route is no longer compact. Defer its
        // terminal handoff to the active recording callback below.
        retirementDelivery = delivery;
    }
    std::optional<CanonicalFeedbackFrameSelection> canonicalFrameSelection;
    const auto selectCanonicalFieldForFrame = [&]() noexcept {
        if (!canonicalFrameSelection) {
            canonicalFrameSelection.emplace(CanonicalFeedbackFrameSelection::capture(
                production.usesCanonicalFeedback(), canonicalFeedbackFields_.latest()));
            ctx.canonicalPublicationInput.reset();
            if (const auto* field = canonicalFrameSelection->fieldForView(0U)) {
                ctx.canonicalPublicationInput = canonicalPublicationInputs_.find(*field,
                    publishedProductionReceiptSequence(SculptureProductionRoute::CanonicalFeedback,
                        field->descriptor.generation, field->descriptor.resetEpoch));
            }
        }
        return canonicalFrameSelection->fieldForView(0U);
    };
    // -------------------------------------------------------------------------
    // Educational Annotation: CAMERA DISPATCH & MULTI-VIEW PRESENTATION
    // - VR/Stereo SBS: Renders left & right eye views with calibrated interpupillary distance (IPD)
    // - Desktop Mono: Computes Halton(2,3) sub-pixel jittered camera when TAA is enabled
    // -------------------------------------------------------------------------
    const WorldSnapshotDelivery worldDelivery = dedicatedWorld
        ? sculptureProducer_->worldDelivery() : WorldSnapshotDelivery{};
    SculpturePresentationReceipt& receipt = ctx.presentationReceipt;
    receipt.route = routeDecision;
    receipt.frameSequence = framePlan.frameSequence();
    receipt.deviceGeneration = presentationHost_->deviceGeneration();
    receipt.surfaceGeneration = presentationHost_->context().surfaceGeneration();
    bool worldDrawSelected = false;
    // Resolve observation intent from the completed world, then pass that
    // value into either frame entry point. Empty intent inherits the baseline.
    const auto postIntent =
        presentationBindings_->director().worldPresentationIntent(worldDelivery);
    const bool profileObservingPresentation = dedicatedWorld
        && worldProfileDriver_.active();
    std::optional<std::chrono::steady_clock::time_point> presentationStart;
    if (profileObservingPresentation) {
        presentationStart = std::chrono::steady_clock::now();
    }
    
    const SculptureProductionSchedule* const canonicalSchedule =
        ctx.productionSchedules.find(SculptureProductionRoute::CanonicalFeedback);
    bool productionPrepared = false;
    const auto prepareProduction = [&] {
        if (productionPrepared) return;
        productionPrepared = true;
        // External dependencies belong to the frame being recorded. Enqueuing
        // them before runFrame used to lose them when FrameLoop cleared its lists.
        // Prepare once even when multiple views observe this world generation.
        if (routeDecision == SculptureProductionRoute::CanonicalFeedback
            && canonicalSchedule != nullptr
            && canonicalSchedule->deferredToFrameRecording()
            && ctx.canonicalCadenceProposal) {
            dispatchCanonicalFeedback(
                framePlan, *canonicalSchedule, *ctx.canonicalCadenceProposal);
        }
        // Dedicated-world dependencies are owned by the world render bridge's
        // transport transaction. Registering another release here duplicates a
        // binary signal and can settle ownership before that transaction does.
    };

    // One recording body owns contribution selection, deferred production,
    // rendering, native retirement, and capture admission. Mono/stereo differ
    // only in camera construction and Engine entrypoint; keeping the body shared
    // prevents provenance or transport behavior drifting between them.
    const auto recordPresentation = [&](mgfx::Drawable& draw,
                                        const mgfx::Camera& presentationCamera,
                                        const mgfx::Camera& captureCamera) {
        // Procedural payload is a desired Engine value. Every application
        // collection starts empty; only an admitted route installs this
        // family's input, including after switching away from a volume.
        draw.engine().setProceduralPayload({});
        prepareProduction();
        const EpiFeedbackFieldDelivery* const canonicalField =
            selectCanonicalFieldForFrame();
        if (canonicalField != nullptr) {
            receipt.selectProduction(
                SculptureProductionRoute::CanonicalFeedback,
                canonicalField->descriptor.generation,
                publishedProductionReceiptSequence(
                    SculptureProductionRoute::CanonicalFeedback,
                    canonicalField->descriptor.generation,
                    canonicalField->descriptor.resetEpoch),
                canonicalField->descriptor.resetEpoch);
        }

        prepareImplicitProduction(ctx, presentationCamera);
        worldDrawSelected = renderScenePass(draw, presentationCamera, params, worldState,
                        presentationDelivery, worldDelivery,
                        canonicalField, dedicatedWorld, canonicalReliefRequested,
                        production.canonicalFeedbackUnavailable(),
                        deltaSeconds, hud);
        const auto compactGeneration = presentationBindings_->compactBridge()
            .collectedGenerationFor(draw.engine().activeFrameAttempt());
        ctx.fieldCandidate = presentationBindings_->compactBridge().fieldCandidateFor(
            draw.engine().activeFrameAttempt());
        if (production.ownsCompactProduction() && compactGeneration != 0U) {
            receipt.selectProduction(
                SculptureProductionRoute::CompactInstances, compactGeneration,
                publishedProductionReceiptSequence(
                    SculptureProductionRoute::CompactInstances, compactGeneration));
        }
        if (worldDrawSelected) {
            receipt.selectProduction(
                SculptureProductionRoute::DedicatedWorld,
                worldDelivery.generation,
                publishedProductionReceiptSequence(
                    SculptureProductionRoute::DedicatedWorld,
                    worldDelivery.generation));
        }

        if (includesImplicitField(framePlan.presentationOwnership())) {
            const ImplicitPresentationSelection implicitSelection =
                presentationBindings_->director().implicitPresentationSelection();
            if (implicitSelection.valid()) {
                receipt.selectProduction(
                    SculptureProductionRoute::ImplicitField,
                    implicitSelection.generation,
                    publishedProductionReceiptSequence(
                        SculptureProductionRoute::ImplicitField,
                        implicitSelection.generation));
            }
        }

        static_cast<void>(presentationBindings_->compactBridge().retireExternalDelivery(
            draw, retirementDelivery));
        handleFrameCapture(ctx, captureCamera);
    };

    mgfx::vk::FrameOutcome frameOutcome = mgfx::vk::FrameOutcome::SkippedZeroExtent;
    if (framePlan.presentation().vrEnabled) {
        const mgfx::Camera authoredCamera = applyStartupCamera(presentationBindings_->director().composeCamera(
            framePlan.presentation().cinematic,
            framePlan.presentation().cinematicTime,
            worldState));
        const mgfx::Camera centerCamera = liveCameraControls_->applyCamera(
            authoredCamera, deltaSeconds);
        const auto stereo = SculpturePresentationDirector::composeStereoRig(
            centerCamera, framePlan.presentation().stereoRig);
        frameOutcome = presentationHost_->engine().runStereoFrame(stereo.leftEye, stereo.rightEye, [&](mgfx::Drawable& draw) {
            // The engine invokes this callback once before it records the
            // two-eye family. Keeping capture admission here binds it to this
            // receipt and avoids a minimized frame leaving stale provenance.
            recordPresentation(draw, stereo.leftEye, authoredCamera);
        }, postIntent);
    } else {
        const mgfx::Camera authoredCamera = applyStartupCamera(framePlan.presentation().temporalActive ?
            presentationBindings_->director().composeJitteredCamera(
                framePlan.presentation().cinematic,
                framePlan.presentation().cinematicTime,
                worldState,
                sculptureGeneration_,
                w, h
            ) :
            presentationBindings_->director().composeCamera(
                framePlan.presentation().cinematic,
                framePlan.presentation().cinematicTime,
                worldState
            ));
        const mgfx::Camera camera = liveCameraControls_->applyCamera(
            authoredCamera, deltaSeconds);

        frameOutcome = presentationHost_->engine().runFrame(camera, [&](mgfx::Drawable& draw) {
            // Engine invokes this only for a recording frame; FrameLoop
            // performs readback preflight afterward. Skipped/minimized frames
            // keep intent pending instead of freezing an obsolete camera.
            recordPresentation(draw, camera, camera);
        }, postIntent);
    }

    receipt.settle(frameOutcome);
    if (ctx.captureObservation) {
        static_cast<void>(ctx.captureObservation->identity.observeSubmission(
            {receipt.frameSequence, presentationHost_->deviceGeneration(),
             presentationHost_->context().surfaceGeneration()},
            receipt.submitted() ? presentationHost_->frameLoop().lastSubmittedSerial() : 0U,
            receipt.submitted()));
        if (ctx.captureObservation->fieldCandidate && receipt.submitted()) {
            ctx.captureObservation->fieldAccepted = presentationBindings_->compactBridge().fieldCandidateAccepted(
                *ctx.captureObservation->fieldCandidate, presentationHost_->frameLoop().lastSubmittedSerial());
        }
    }
    // Publish only the native outcome here. The outer frame-exit boundary
    // closes pending opportunities and projects observations after ALL stages,
    // including optional post-render work, have returned or unwound.

    if (presentationStart && receipt.submitted() && worldDrawSelected && worldDelivery
        && presentationBindings_->worldBridge().metrics().matches(worldDelivery.generation)) {
        worldPresentationMetrics_ = {
            .generation = worldDelivery.generation,
            .submitCpuMilliseconds = std::chrono::duration<float, std::milli>(
                std::chrono::steady_clock::now() - *presentationStart).count(),
        };
        if (worldDelivery.metrics) {
            telemetry_.recordWorldGeneration(
                *worldDelivery.metrics, 
                presentationBindings_->worldBridge().metrics(), 
                worldPresentationMetrics_);
        }
    }
}

void SculptureApplication::observePresentationPerformance() noexcept {
    if (!presentationScalePolicy_.autoEnabled()) return;

    const mgfx::RenderFrameGpuTelemetry& sample =
        presentationHost_->engine().lastRenderGpuTelemetry();
    const std::optional<float> requested = presentationScalePolicy_.observe(
        sample,
        presentationHost_->engine().requestedSceneRenderScale(),
        presentationHost_->engine().effectiveSceneRenderScale(),
        presentationHost_->engine().sceneTargetGeneration(),
        presentationHost_->engine().sceneTargetRequestBlocked(),
        presentationHost_->deviceGeneration());
    if (!requested) return;

    if (presentationHost_->engine().requestSceneRenderScale(*requested)) {
        const auto& state = presentationScalePolicy_.state();
        std::fprintf(stderr,
            "[3dEpiSculpTMorph] adaptive scene scale request -> %.3f "
            "(GPU %.2fms smoothed, target %.2fms, submission %llu)\n",
            static_cast<double>(*requested),
            state.smoothedGpuMilliseconds,
            presentationScalePolicy_.config().targetGpuMilliseconds,
            static_cast<unsigned long long>(state.lastSubmissionSerial));
    }
}

void SculptureApplication::observeWorldProfile(float deltaSeconds) {
    const WorldSnapshotDelivery delivery = sculptureProducer_->worldDelivery();
    const WorldBridgeMetrics& bridge = presentationBindings_->worldBridge().metrics();
    const WorldPresentationMetrics* presentation =
        worldPresentationMetrics_.matches(delivery.generation)
            ? &worldPresentationMetrics_ : nullptr;
    const auto completion = worldProfileDriver_.observe(
        deltaSeconds, sculptureProducer_->worldKernelReady(), delivery,
        bridge, presentation);
    if (!completion) return;
    std::fprintf(stderr, "[3dEpiSculpTMorph] profile %s: %s (%u accepted generations)\n",
                 completion->sidecarWritten ? "written" : "write failed",
                 completion->path.c_str(), completion->summary.acceptedGenerationCount);
    // A profile invocation from the CLI is a bounded measurement command.
    // Leave through the existing run-loop lifecycle if it was requested at startup.
    if (!profileSessionIsInteractive_) {
        worldProfileExitRequested_ = true;
    }
}

void SculptureApplication::executeStagePresentation(FrameContext& ctx) {
    if (sculptureProducer_->backendName() != producerBackend_) {
        producerBackend_ = sculptureProducer_->backendName();
        std::fprintf(stderr, "[3dEpiSculpTMorph] producer changed: %s (%s)\n",
                     producerBackend_.c_str(),
                     std::string(sculptureProducer_->diagnostic()).c_str());
    }

    const SculptureFramePlan& framePlan = ctx.plan();
    const SculptureWorldState& worldState = framePlan.worldState();
    // Once per evaluation, before either mono or stereo recording; the device
    // adapter consumes an owned observation, not live semantic/audio state.
    presentationBindings_->acousticMist().apply(presentationHost_->engine(), ctx.acousticMist);
    formatHud(ctx.hud, sizeof(ctx.hud), framePlan);
    renderFrame(ctx);
    observePresentationPerformance();
    observeWorldProfile(ctx.deltaSeconds);

    if (remoteServer_ && (framePlan.frameSequence() % 4U == 0U)) {
        const float crossfadePos = control_.sources().deckActive() ? control_.sources().deck().crossfadePosition() : 0.0f;
        const float bpm = control_.sources().deckActive() ? control_.sources().deck().bpm() : 120.0f;
        const float fps = ctx.deltaSeconds > 0.0001f ? (1.0f / ctx.deltaSeconds) : 60.0f;
        remoteServer_->broadcastTelemetry(
            fps,
            worldState.morph,
            worldState.time,
            crossfadePos,
            bpm,
            false,
            framePlan.capturedWorldRecipe() && framePlan.production().ownsDedicatedWorldProduction()
                ? framePlan.capturedWorldRecipe()->presetId : presetAt(framePlan.presetIndex()).title,
            control_.sources().morphSpace().isActive(),
            control_.sources().morphSpace().cursorX(),
            control_.sources().morphSpace().cursorY(),
            control_.sources().morphSpace().activeBankTitle()
        );
    }
}

} // namespace epi_sculpt
