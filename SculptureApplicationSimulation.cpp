// Input/time, semantic composition and immutable FramePlan construction.
// These translation units share one SculptureApplication owner; none is a second scheduler.

#include "SculptureApplication.h"
#include "SculptureReliefContours.h"
#include "SculptureParameterComposer.h"
#include "SculptureSequencePlayback.h"
#include "SculptureSpatialGestureEngine.h"
#include "mgfx/vk/XRSwapchain.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <optional>
#include <utility>
#include <vector>

namespace epi_sculpt {

void SculptureApplication::configureWorldCadence(const WorldRecipe& recipe, std::optional<float> effectiveHz) noexcept {
    worldQualityPolicy_.configure(recipe);
    worldProducerCadence_.setConfig({
        .updateHz = effectiveHz.value_or(static_cast<float>(recipe.generatorHz)),
        .dispatchOnParameterChange = true,
        .enableAdaptiveThrottling = true,
        .targetLatencyMilliseconds = 20.0f,
    });
}

const WorldRecipe& SculptureApplication::activeWorldRecipe() const noexcept {
    return capturedWorldRecipe_ ? capturedWorldRecipe_->recipe : selectedPreset().worldRecipe;
}

void SculptureApplication::clearCapturedSources() noexcept {
    if (capturedCanonicalSource_) {
        capturedCanonicalSource_.reset();
        // A root-only source change is invisible to sculpture parameter dirtiness.
        // Use the existing reset intent rather than advancing producer identity here.
        appState_.requestTemporalReset = true;
    }
    if (!capturedWorldRecipe_) return;
    capturedWorldRecipe_.reset();
    configureWorldCadence(selectedPreset().worldRecipe);
}

std::optional<SculptureAcousticMistObservation> SculptureApplication::composeParameters(
    const SculptureProgramParams& base, float time, float deltaSeconds,
    const SculptureAudioObservation& audio, SculptureCompositionObservation& composition) {
    activeParams_ = base;
    // Reset is an explicit command, never durable preset state. Freeze binds
    // pending commands to contributions; producer acceptance acknowledges them.
    activeParams_.temporalReset = false;

    // -------------------------------------------------------------------------
    // Educational Annotation: CLOSED-LOOP ADAPTIVE COMPUTE GOVERNOR
    // Evaluates GPU kernel latencies against the 60Hz/120Hz frame budget.
    // Dynamically stamps auxiliary spatial resolution scaling (Full -> Half -> Quarter)
    // to reduce load near the target frame budget; this is not a no-drop guarantee.
    // -------------------------------------------------------------------------
    stagedGovernor_.applyToParams(activeParams_);
    
    // The host owns measured governor policy and audio acquisition. Control owns
    // the complete ordered semantic modifier evaluation and its numerical history.
    return control_.composePostBase(activeParams_, time, deltaSeconds, audio,
        appState_.temporalActive, composition);
}

SculptureProductionCapabilities
SculptureApplication::productionCapabilities() const noexcept {
    return {
        .canonicalFeedbackProducerAvailable = canonicalFeedbackProducer_
            && canonicalFeedbackProducer_->ready(),
        .dedicatedWorldTransportAvailable = presentationBindings_->worldTransport()
            && presentationBindings_->worldTransport()->operational(),
    };
}

SculptureProductionDecision
SculptureApplication::resolveActiveProductionDecision() const noexcept {
    return resolveProductionDecision(
        presentationOwnership_, pipeline_.renderStrategy(), productionCapabilities());
}

SculptureWorldState SculptureApplication::resolveSimulationState(
    const SculptureProductionDecision& production) const noexcept {
    const float defaultTime = control_.runtime().simulationTime();
    const float defaultMorph = 0.5f - 0.5f * std::cos(0.34f * defaultTime);
    const float defaultCameraAngle = 0.16f * defaultTime;

    const SculptureWorldState liveWorldState{
        .time = control_.sources().cinematic() ? control_.sources().sequence().currentState().time : defaultTime,
        .morph = control_.sources().cinematic() ? control_.sources().sequence().currentState().morph : defaultMorph,
        .cameraAngle = control_.sources().cinematic() ? control_.sources().sequence().currentState().cameraAngle : defaultCameraAngle,
    };
    SculptureWorldState resolved = control_.runtime().resolve(liveWorldState);
    const bool canApplyWorldFrame = production.ownsDedicatedWorldProduction()
        && !control_.sources().cinematic()
        && !control_.runtime().paused()
        && presentationBindings_->director().cameraMode() == CameraMode::Orbit;
    if (canApplyWorldFrame) {
        const SculptureWorldFrameHint hint = makeWorldFrameHint(
            activeWorldRecipe());
        resolved.cameraEye[0] = hint.eyeX;
        resolved.cameraEye[1] = hint.eyeY;
        resolved.cameraEye[2] = hint.eyeZ;
        resolved.cameraTarget[0] = hint.targetX;
        resolved.cameraTarget[1] = hint.targetY;
        resolved.cameraTarget[2] = hint.targetZ;
        resolved.cameraUp[0] = 0.0f;
        resolved.cameraUp[1] = 1.0f;
        resolved.cameraUp[2] = 0.0f;
        resolved.hasCameraOverride = false;
    }
    return resolved;
}

void SculptureApplication::updateSonification(
    const gpu_abi::SculptureAnalyticsGpu* analytics,
    float morph,
    const SculptureProgramParams& params) {
    // -------------------------------------------------------------------------
    // Educational Annotation: GEOMETRY-TO-AUDIO ENTROPY COUPLING
    // Extracts average kinetic energy, orbital radius, and 4D latent coordinate
    // directly from the GPU analytics buffer to modulate continuous oscillators.
    // -------------------------------------------------------------------------
    if (!appState_.audioEnabled) return;
    if (analytics && analytics->activeInstanceCount > 0) {
        float avgOrbit = analytics->totalOrbit / std::min(analytics->activeInstanceCount, 100U);
        float avgMag = analytics->totalKineticEnergy / std::min(analytics->activeInstanceCount, 100U);
        sculptureAudio_.updateSonificationState(
            avgOrbit,
            avgMag,
            morph,
            params.latentW,
            static_cast<std::uint32_t>(params.inner)
        );
    }
}

void SculptureApplication::executeStageTimeAndInput(FrameContext& ctx) {
    presentationHost_->context().pollEvents();
    const auto frameTime = std::chrono::steady_clock::now();
    ctx.deltaSeconds = std::clamp(
        std::chrono::duration<float>(frameTime - previousFrameTime_).count(),
        0.0f, 0.1f
    );
    previousFrameTime_ = frameTime;

    // Remote/MIDI adapters may post from worker threads. They are queued by
    // SculptureEventBus and become authoritative only here, on the application
    // thread, before simulation advances or the frame plan is frozen.
    static_cast<void>(eventBus_->drainPending());

    if (presentationHost_->context().hasXR() && presentationHost_->context().xrSwapchain()) {
        auto left = presentationHost_->context().xrSwapchain()->getLeftHand();
        if (left.isActive) {
            Pose6DOF pose{};
            pose.position = {left.pose.position[0], left.pose.position[1], left.pose.position[2]};
            pose.orientation = {left.pose.orientation[3], left.pose.orientation[0], left.pose.orientation[1], left.pose.orientation[2]};
            pose.trigger = left.trigger;
            pose.valid = true;
            pipeline_.gestureEngine().setLeftHandPose(pose);
        } else {
            pipeline_.gestureEngine().setLeftHandPose({});
        }

        auto right = presentationHost_->context().xrSwapchain()->getRightHand();
        if (right.isActive) {
            Pose6DOF pose{};
            pose.position = {right.pose.position[0], right.pose.position[1], right.pose.position[2]};
            pose.orientation = {right.pose.orientation[3], right.pose.orientation[0], right.pose.orientation[1], right.pose.orientation[2]};
            pose.trigger = right.trigger;
            pose.valid = true;
            pipeline_.gestureEngine().setRightHandPose(pose);
        } else {
            pipeline_.gestureEngine().setRightHandPose({});
        }
        
        std::vector<mgfx::vk::XRSwapchain::ViewPose> views;
        if (presentationHost_->context().xrSwapchain()->locateViews(views) && !views.empty()) {
            Pose6DOF headPose{};
            headPose.position = epi_sculpt::SculptureVec3{views[0].position[0], views[0].position[1], views[0].position[2]};
            headPose.orientation = epi_sculpt::Quaternion4D{views[0].orientation[3], views[0].orientation[0], views[0].orientation[1], views[0].orientation[2]};
            headPose.valid = true;
            pipeline_.gestureEngine().setHeadPose(headPose);
        } else {
            pipeline_.gestureEngine().setHeadPose({});
        }
    }
    
    // Process keyboard/mouse command queues
    commandCenter_->update(ctx.deltaSeconds);
    liveCameraControls_->beginFrame(ctx.deltaSeconds);
}

void SculptureApplication::executeStageAdvanceSimulation(FrameContext& ctx) {
    std::size_t previousPresetIndex = control_.presetIndex();
    const bool stateLoadedThisFrame = appState_.stateJustLoaded;

    const auto advanced = control_.advanceSources(ctx.deltaSeconds);
    ctx.simulationDelta = advanced.simulationDelta;
    ctx.composition = advanced.composition;
    const bool playlistWasActive = advanced.playlistWasActive;
    
    bool playlistIdentityChanged = false;
    // Completion may select the final spline cue and stop playback in one update.
    // Reconcile that cue before freezing its parameters, just like a playing cue.
    if (control_.sources().presetMorph().isPlaylistActive() || playlistWasActive) {
        const std::size_t playlistIndex = control_.sources().presetMorph().playlistIndex();
        const MorphTarget* target = control_.sources().presetMorph().playlistTarget(playlistIndex);
        if (target != nullptr && target->presetId.empty()) {
            // Parameter-only cues change position without selecting an asset.
            appliedPlaylistTargetIndex_ = playlistIndex;
        } else if (target != nullptr
                   && (appliedPlaylistTargetIndex_ != playlistIndex
                       || target->presetId != selectedPreset().id)) {
            // A replacement tour can reuse an index while selecting a new asset.
            // Cue IDs are durable identity: titles/aliases or duplicate IDs must
            // never select another recipe, cadence or routing map here.
            if (const std::optional<std::size_t> assetIndex = presetLibrary_.findExactIndex(target->presetId)) {
                applyPresetIdentity(*assetIndex);
                appliedPlaylistTargetIndex_ = playlistIndex;
                playlistIdentityChanged = true;
            }
        }
    } else {
        appliedPlaylistTargetIndex_.reset();
    }

    if (stateLoadedThisFrame) {
        // Playlist identity above may select asset routes. The admitted loaded
        // moment wins here, without a late parser or allocation failure.
        if (appState_.loadedModulationGraph) {
            static_cast<void>(control_.publishModulationGraph(
                std::move(*appState_.loadedModulationGraph)));
            appState_.loadedModulationGraph.reset();
        }
        appState_.requestTemporalReset = true;
        appState_.stateJustLoaded = false;
    } else if (!control_.sources().cinematic() && control_.presetIndex() != previousPresetIndex
               && !playlistIdentityChanged) {
        applyModulationGraph(selectedPreset().routingMap);
    }

    control_.sources().advanceCinematic(ctx.simulationDelta, stateLoadedThisFrame);
    if (control_.sources().cinematic()) {
        // Empty authored maps select host defaults too. The application boundary
        // compares resolved intent before changing any compiled route state.
        applyModulationGraph(control_.sources().sequence().currentRoutingMap());
    }
}

void SculptureApplication::executeStageParameterComposition(FrameContext& ctx) {
    const float simTime = control_.sources().cinematic() ? control_.sources().sequence().currentState().time : control_.runtime().simulationTime();
    float compositionDelta = ctx.simulationDelta;
    const auto base = control_.sources().composeBase(simTime, compositionDelta, &ctx.composition);
    // Deck FX scale the local modifier/envelope time, never source-clock time.
    const auto audio = sculptureAudio_.observeModulation(appState_.audioEnabled, compositionDelta);
    ctx.acousticMist = composeParameters(base, simTime, compositionDelta, audio, ctx.composition);
    const auto contours = appState_.reliefContours.compose(
        {activeParams_.reliefContourDensity, activeParams_.reliefContourStyle});
    activeParams_.reliefContourDensity = contours.density;
    activeParams_.reliefContourStyle = contours.style;
}

void SculptureApplication::executeStageFreezeFramePlan(FrameContext& ctx) {
    // This is the semantic commit point for one frame. Input/simulation and
    // parameter composition may mutate live application state before here;
    // every later stage receives one frozen contract and must not re-read
    // activeParams_, route policy, or mutable presentation flags for this frame.
    const SculptureProductionDecision production = resolveActiveProductionDecision();
    const SculptureWorldState worldState = resolveSimulationState(production);
    const SculpturePresetAsset& preset = selectedPreset();
    const WorldRecipe requestedWorldRecipe = activeWorldRecipe();
    const WorldRecipe effectiveWorldRecipe = worldQualityPolicy_.apply(
        requestedWorldRecipe);
    const epi::VisualMode* const canonicalModeBase = capturedCanonicalSource_
        ? &capturedCanonicalSource_->mode
        : (preset.canonicalFeedbackMode ? &*preset.canonicalFeedbackMode : nullptr);
    const SculptureProgramParams* const canonicalProjectionBaseline =
        capturedCanonicalSource_ ? &capturedCanonicalSource_->projectionBaseline
            : (canonicalModeBase ? &preset.params : nullptr);

    const SculptureResetContributions resetContributions =
        temporalResetContributions(production, presentationOwnership_);
    appState_.temporalReset.retain(resetContributions);
    if (appState_.requestTemporalReset) {
        appState_.temporalReset.request(resetContributions);
        appState_.requestTemporalReset = false;
    }
    const SculptureTemporalResetIntent temporalReset = appState_.temporalReset.intent();
    activeParams_.temporalReset = temporalReset.pending();

    ctx.framePlan.emplace(
        activeParams_,
        canonicalModeBase,
        canonicalProjectionBaseline,
        worldState,
        production,
        requestedWorldRecipe,
        effectiveWorldRecipe,
        worldProducerCadence_.effectiveUpdateHz(),
        control_.presetIndex(),
        presentationOwnership_,
        compactPresentationLeaseEpoch_,
        ctx.frameCounter,
        SculptureFramePlan::PresentationState{
            .cinematic = control_.sources().cinematic(),
            .vrEnabled = appState_.enableVR,
            .temporalActive = appState_.temporalActive,
            .cinematicTime = control_.runtime().cinematicTime(),
            .stereoRig = stereoRig_,
            .volumePresentation = presentationBindings_->volumePresentation(),
        },
        temporalReset,
        capturedWorldRecipe_ ? &*capturedWorldRecipe_ : nullptr,
        capturedCanonicalSource_,
        capturedCanonicalSource_ ? capturedCanonicalSource_->seedImageSelectionRevision : 0U,
        ctx.composition);

    // Explicit saves observe this composed semantic intent before production.
    // The save workflow contains its own failures and never spends GPU cadence.
    flushPendingStateSaves(ctx.plan());
}

void SculptureApplication::executeStageAuxiliaryFeedback(FrameContext& ctx) {
    // EDUCATIONAL NOTE:
    // Recovery preserves the last coherent OpenCL delivery; it does not run
    // a second CPU mathematical backend. This stage is reserved for bounded
    // presentation-side work (sonification/haptics) after compute decisions.
    // Fault-tolerant containment guarantees that audio or peripheral haptic glitches
    // do not abort or compromise the core graphical rendering loop.
    try {
        const SculptureFramePlan& framePlan = ctx.plan();
        updateSonification(
            ctx.delivery.analytics, framePlan.worldState().morph, framePlan.params());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[SculptureApplication] Non-fatal sonification warning: %s\n", e.what());
    }
    
    // Process XR Haptics
    try {
        if (presentationHost_->context().hasXR() && presentationHost_->context().xrSwapchain()) {
            const auto& audioFeatures = pipeline_.audioFeatures();
            auto leftPulse = pipeline_.gestureEngine().computeHapticFeedback(audioFeatures, true);
            if (leftPulse.amplitude > 0.0f) {
                presentationHost_->context().xrSwapchain()->applyHaptic(true, leftPulse.amplitude, leftPulse.durationSeconds, leftPulse.frequencyHz);
            }
            auto rightPulse = pipeline_.gestureEngine().computeHapticFeedback(audioFeatures, false);
            if (rightPulse.amplitude > 0.0f) {
                presentationHost_->context().xrSwapchain()->applyHaptic(false, rightPulse.amplitude, rightPulse.durationSeconds, rightPulse.frequencyHz);
            }
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[SculptureApplication] Non-fatal XR haptics warning: %s\n", e.what());
    }
}

} // namespace epi_sculpt
