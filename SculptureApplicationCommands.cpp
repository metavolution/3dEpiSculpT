// Owner-thread semantic command dispatch and modulation-graph edits.
// These translation units share one SculptureApplication owner; none is a second scheduler.

#include "SculptureApplication.h"
#include "SculptureVulkanDeviceIdentity.h"
#include "SculptureLodEngine.h"
#include "SculptureModulationReconciliation.h"
#include "SculptureModulationInspection.h"
#include "SculptureNumericEdit.h"
#include "SculptureSerializer.h"
#include "SculptureSequencePlayback.h"
#include "SculptureTimelineControl.h"
#include "SculptureSpatialGestureEngine.h"
#include "SculptureVolumetricAtmosphere.h"
#include "SculptureWorldProfileSidecar.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace epi_sculpt {

namespace {
template <typename>
inline constexpr bool kUnhandledSculptureEvent = false;
} // namespace

void SculptureApplication::handleEvent(const SculptureEvent& event) {
    std::visit([this](const auto& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, EventPresetSelect>) {
            selectPreset(arg.index, arg.durationSeconds, SculpturePresetTakeover::ReplaceSource);
        } else if constexpr (std::is_same_v<T, EventPresetCycle>) {
            if (const auto index = control_.cyclePresetIndex(presetLibrary_, arg.offset)) {
                selectPreset(*index, 1.6f, SculpturePresetTakeover::ReplaceSource);
            }
        } else if constexpr (std::is_same_v<T, EventWorldCatalogSelect>) {
            const auto catalog = worldCatalog();
            if (arg.catalogIndex >= catalog.size()) return;
            if (!catalog[arg.catalogIndex].hasCompleteRoute()) {
                std::fprintf(stderr, "[3dEpiSculpTMorph] route unavailable: %s\n",
                             std::string(catalog[arg.catalogIndex].title).c_str());
                return;
            }
            // A catalog entry already names its canonical ID, not a search term.
            if (const auto index = presetLibrary_.findExactIndex(
                    catalog[arg.catalogIndex].canonicalPresetId)) {
                if (!selectPreset(*index, arg.durationSeconds,
                                  SculpturePresetTakeover::ReplaceSource)) return;
                std::fprintf(stderr,
                    "[3dEpiSculpTMorph] world catalog %zu: %s\n",
                    arg.catalogIndex + 1U,
                    std::string(catalog[arg.catalogIndex].title).c_str());
            } else {
                std::fprintf(stderr,
                    "[3dEpiSculpTMorph] World preset ID is missing or ambiguous; "
                    "selection retained: %s\n",
                    std::string(catalog[arg.catalogIndex].canonicalPresetId).c_str());
            }
        } else if constexpr (std::is_same_v<T, EventToggleState>) {
            switch (arg.target) {
            case EventToggleState::Target::Pause:
                control_.runtime().togglePause();
                std::fprintf(stderr, "[SculptureApplication] Simulation %s\n",
                    control_.runtime().paused() ? "PAUSED" : "RESUMED");
                break;
            case EventToggleState::Target::Cinematic:
                if (control_.sources().cinematic()) {
                    control_.sources().leaveCinematic(appState_.stateJustLoaded);
                } else {
                    clearCapturedSources();
                    control_.sources().enterCinematic();
                }
                break;
            case EventToggleState::Target::Temporal:
                appState_.temporalActive = !appState_.temporalActive;
                std::fprintf(stderr, "[SculptureApplication] Temporal Feedback %s\n",
                    appState_.temporalActive ? "ENABLED" : "DISABLED");
                break;
            case EventToggleState::Target::Topology4D: {
                SculptureProgramParams params = control_.sources().presetMorph().currentParams();
                params.enable4D = params.enable4D ? 0U : 1U;
                control_.sources().presetMorph().setSource(params);
                break;
            }
            case EventToggleState::Target::VoxelMode: {
                SculptureProgramParams params = control_.sources().presetMorph().currentParams();
                params.outputMode = (params.outputMode + 1U) % 4U;
                params.outputModeB = params.outputMode;
                control_.sources().presetMorph().setSource(params);
                // Mode changes require a publication under the new semantic
                // lease, even if controls cycle twice before a rendered frame.
                beginCompactHostStagedTransition();
                break;
            }
            case EventToggleState::Target::RecordSequence:
                appState_.capture.onEvent(arg);
                std::fprintf(stderr, "[SculptureApplication] sequence recording %s\n",
                    appState_.capture.recording() ? "STARTED" : "STOPPED");
                break;
            case EventToggleState::Target::StereoVR:
                appState_.enableVR = !appState_.enableVR;
                std::fprintf(stderr, "[SculptureApplication] Stereo SBS VR %s\n",
                    appState_.enableVR ? "ENABLED" : "DISABLED");
                break;
            }
        } else if constexpr (std::is_same_v<T, EventReliefContourControl>) {
            appState_.reliefContours.apply(arg.action);
            std::fprintf(stderr, "[SculptureApplication] Relief contours %.2f levels/luminance (%s)\n",
                static_cast<double>(appState_.reliefContours.value()),
                appState_.reliefContours.hasOverride() ? "session override" : "authored");
        } else if constexpr (std::is_same_v<T, EventTemporalReset>) {
            appState_.requestTemporalReset = true;
        } else if constexpr (std::is_same_v<T, EventLearnStage>) {
            SculptureProgramParams params = control_.sources().presetMorph().currentParams();
            if (arg.maskBit == learn_mode::kCycleDiagnosticCommand) {
                params.stageMask = learn_mode::cycleDiagnostic(params.stageMask);
            } else if (arg.maskBit == learn_mode::kResetStagesCommand) {
                params.stageMask = learn_mode::withAllStages(params.stageMask);
            } else {
                params.stageMask ^= arg.maskBit;
            }
            control_.sources().presetMorph().setSource(params);
            std::fprintf(stderr,
                "[SculptureApplication] Learn stage mask=0x%02X diagnostic=%u\n",
                params.stageMask & 0xFFU,
                static_cast<unsigned>(learn_mode::diagnostic(params.stageMask)));
        } else if constexpr (std::is_same_v<T, EventPlaybackRate>) {
            switch (arg.action) {
            case EventPlaybackRate::Action::Increase:
                control_.runtime().playbackRate().increase();
                break;
            case EventPlaybackRate::Action::Decrease:
                control_.runtime().playbackRate().decrease();
                break;
            case EventPlaybackRate::Action::Reset:
                control_.runtime().playbackRate().reset();
                break;
            }
            std::fprintf(stderr, "[SculptureApplication] Time Speed: %.3fx\n",
                control_.runtime().playbackRate().value());
        } else if constexpr (std::is_same_v<T, EventFileAction>) {
            handleFileAction(arg);
        } else if constexpr (std::is_same_v<T, EventCapture>) {
            appState_.capture.onEvent(arg);
        } else if constexpr (std::is_same_v<T, EventSteering>) {
            control_.steerManual(arg.deltaLatentW, arg.deltaSimTime);
        } else if constexpr (std::is_same_v<T, EventMutation>) {
            switch (arg.action) {
            case EventMutation::Action::ToggleTour:
                if (control_.sources().presetMorph().isPlaylistActive()) control_.sources().presetMorph().stopPlaylist();
                else {
                    control_.sources().presetMorph().startPlaylist();
                    if (control_.sources().presetMorph().isPlaylistActive()) clearCapturedSources();
                }
                break;
            case EventMutation::Action::StepMutation:
                // Admission precedes source takeover: refusal preserves both
                // stochastic history and the current source/playback moment.
                if (const auto candidate = mutator_.tryMutate(
                        control_.sources().presetMorph().currentParams(), arg.strength)) {
                    control_.sources().presetMorph().stopPlaylist();
                    control_.sources().presetMorph().setSource(*candidate);
                }
                break;
            case EventMutation::Action::ResetBase:
                // The control owner admits first, then stops/reset/retargets.
                // Refusal must not consume playback or manual rotor history.
                selectPreset(control_.presetIndex(), 1.6f,
                             SculpturePresetTakeover::ResetBase);
                break;
            case EventMutation::Action::AdjustLatentW:
                control_.adjustManualOffset(SculptureManualParameter::LatentW, arg.strength); break;
            case EventMutation::Action::AdjustRotorAlpha:
                control_.adjustManualOffset(SculptureManualParameter::RotorAlpha, arg.strength); break;
            case EventMutation::Action::AdjustRotorBeta:
                control_.adjustManualOffset(SculptureManualParameter::RotorBeta, arg.strength); break;
            case EventMutation::Action::AdjustRotorTheta:
                control_.adjustManualOffset(SculptureManualParameter::RotorTheta, arg.strength); break;
            case EventMutation::Action::AdjustTopologyBlend:
                control_.adjustManualOffset(SculptureManualParameter::TopologyBlend, arg.strength); break;
            case EventMutation::Action::AdjustHue:
                control_.adjustManualOffset(SculptureManualParameter::Hue, arg.strength); break;
            case EventMutation::Action::AdjustSeedAlpha:
                control_.adjustManualOffset(SculptureManualParameter::SeedAlpha, arg.strength); break;
            case EventMutation::Action::SetLatentWOffset:
                control_.setManualOffset(SculptureManualParameter::LatentW, arg.strength); break;
            case EventMutation::Action::SetRotorAlphaOffset:
                control_.setManualOffset(SculptureManualParameter::RotorAlpha, arg.strength); break;
            case EventMutation::Action::SetRotorBetaOffset:
                control_.setManualOffset(SculptureManualParameter::RotorBeta, arg.strength); break;
            case EventMutation::Action::SetRotorThetaOffset:
                control_.setManualOffset(SculptureManualParameter::RotorTheta, arg.strength); break;
            case EventMutation::Action::SetTopologyBlendOffset:
                control_.setManualOffset(SculptureManualParameter::TopologyBlend, arg.strength); break;
            case EventMutation::Action::SetHueOffset:
                control_.setManualOffset(SculptureManualParameter::Hue, arg.strength); break;
            case EventMutation::Action::SetSeedAlphaOffset:
                control_.setManualOffset(SculptureManualParameter::SeedAlpha, arg.strength); break;
            case EventMutation::Action::Toggle4D:
                control_.toggleManual4D(activeParams_.enable4D != 0U);
                break;
            case EventMutation::Action::StepTourNext:
                if (control_.sources().presetMorph().isPlaylistActive()) {
                    clearCapturedSources();
                    control_.sources().presetMorph().stepPlaylistNext();
                } else if (control_.sources().deckActive()) {
                    control_.sources().deck().triggerBeatSyncedCrossfade(2U);
                } else {
                    handleEvent(EventPresetCycle{1});
                }
                break;
            case EventMutation::Action::StepTourPrev:
                if (control_.sources().presetMorph().isPlaylistActive()) {
                    clearCapturedSources();
                    control_.sources().presetMorph().stepPlaylistPrevious();
                } else {
                    handleEvent(EventPresetCycle{-1});
                }
                break;
            case EventMutation::Action::ToggleTourPause:
                if (control_.sources().presetMorph().isPlaylistActive()) {
                    control_.sources().presetMorph().togglePlaylistPause();
                    std::fprintf(stderr, "[SculptureApplication] Tour Playlist %s\n",
                        control_.sources().presetMorph().isPlaylistPaused() ? "PAUSED" : "RESUMED");
                }
                break;
            case EventMutation::Action::ToggleTourPlaybackMode: {
                const auto requestedMode =
                    control_.sources().presetMorph().tourPlaybackMode() == TourPlaybackMode::StepAndDwell
                        ? TourPlaybackMode::ContinuousSpline
                        : TourPlaybackMode::StepAndDwell;
                if (!control_.sources().presetMorph().trySetTourPlaybackMode(requestedMode)) {
                    std::fprintf(stderr,
                        "[SculptureApplication] Tour playback mode request rejected; current playback retained.\n");
                    break;
                }
                std::fprintf(stderr, "[SculptureApplication] Tour Playback Mode: %s\n",
                    control_.sources().presetMorph().tourPlaybackMode() == TourPlaybackMode::ContinuousSpline
                        ? "ContinuousSpline" : "StepAndDwell");
                break;
            }
            }
        } else if constexpr (std::is_same_v<T, EventReloadShaders>) {
            // Kernel reload is independent and still precedes preset effects.
            // A rejected catalog replacement must not undo its result.
            const bool kernelsReloaded = sculptureProducer_->reloadKernels();
            const bool presetsReloaded = reloadPresetLibrary();
            
            std::fprintf(stderr, "[SculptureApplication] Hot reload triggered. Presets: %s, Kernels: %s\n",
                presetsReloaded ? "SUCCESS" : "FAIL", kernelsReloaded ? "SUCCESS" : "UNAVAILABLE");
        } else if constexpr (std::is_same_v<T, EventMorphEasingCycle>) {
            MorphEnvelopeConfig config = control_.sources().presetMorph().envelopeConfig();
            config.easing = static_cast<MorphEasing>(
                (static_cast<std::uint8_t>(config.easing) + 1U)
                % static_cast<std::uint8_t>(kMorphEasingCount));
            control_.sources().presetMorph().setEnvelopeConfig(config);
            std::fprintf(stderr, "[SculptureApplication] Morph Easing: %s\n",
                name(config.easing).data());
        } else if constexpr (std::is_same_v<T, EventTourCycle>) {
            if (arg.direction > 0) {
                control_.sources().presetMorph().stepPlaylistNext();
            } else {
                control_.sources().presetMorph().stepPlaylistPrevious();
            }
        } else if constexpr (std::is_same_v<T, EventCameraModeCycle>) {
            const int count = static_cast<int>(kCameraModeCount);
            const int current = static_cast<int>(presentationBindings_->director().cameraMode());
            const int next = ((current + arg.offset) % count + count) % count;
            presentationBindings_->director().setCameraMode(static_cast<CameraMode>(next));
            std::fprintf(stderr, "[SculptureApplication] Camera Tour: %s\n",
                name(presentationBindings_->director().cameraMode()).data());
        } else if constexpr (std::is_same_v<T, EventSonificationModeCycle>) {
            if (!appState_.audioEnabled) {
                std::fprintf(stderr,
                    "[SculptureApplication] Audio remains OFF (default policy)\n");
                return;
            }
            const auto current = sculptureAudio_.getSonificationMode();
            const auto next = static_cast<SonificationMode>(
                (static_cast<int>(current) + 1) % 5);
            sculptureAudio_.setSonificationMode(next);
        } else if constexpr (std::is_same_v<T, EventSonificationVolume>) {
            if (!appState_.audioEnabled) return;
            sculptureAudio_.setSonificationVolume(std::clamp(arg.targetVolume, 0.0f, 1.0f));
        } else if constexpr (std::is_same_v<T, EventRenderStrategyCycle>) {
            pipeline_.cycleRenderStrategy();
            beginCompactHostStagedTransition();
            std::fprintf(stderr, "[SculptureApplication] Render Strategy: %s (GPU-native)\n",
                name(pipeline_.renderStrategy()).data());
        } else if constexpr (std::is_same_v<T, EventResolutionScaleCycle>) {
            SculptureProgramParams params = control_.sources().presetMorph().currentParams();
            switch (params.auxiliaryResolutionScale) {
            case StageResolutionScale::Full:
                params.auxiliaryResolutionScale = StageResolutionScale::Half;
                break;
            case StageResolutionScale::Half:
                params.auxiliaryResolutionScale = StageResolutionScale::Quarter;
                break;
            case StageResolutionScale::Quarter:
                params.auxiliaryResolutionScale = StageResolutionScale::Eighth;
                break;
            case StageResolutionScale::Eighth:
                params.auxiliaryResolutionScale = StageResolutionScale::Full;
                break;
            }
            control_.sources().presetMorph().setSource(params);
        } else if constexpr (std::is_same_v<T, EventGestureFilterReset>) {
            // No-op
        } else if constexpr (std::is_same_v<T, EventUpsampleFilterCycle>) {
            // No-op
        } else if constexpr (std::is_same_v<T, EventTriggerProfileSession>) {
            if (!worldProfileDriver_.active()) {
                WorldProfileIdentity identity;
                identity.canonicalPresetId = capturedWorldRecipe_
                    ? capturedWorldRecipe_->presetId : selectedPreset().id;
                const auto& revision = capturedWorldRecipe_
                    ? capturedWorldRecipe_->presetRevision : selectedPreset().revision;
                identity.sourceRevision = revision.empty() ? "unversioned" : revision;
                identity.requestedRecipe = activeWorldRecipe();
                identity.effectiveRecipe = worldQualityPolicy_.apply(activeWorldRecipe());
                identity.openClDeviceId = producerBackend_;
                identity.vulkanDeviceId = describeVulkanPhysicalDevice(presentationHost_->context().device().physical());
                identity.qualityTier = "high";
                identity.viewConfiguration = presentationHost_->context().hasXR() ? "openxr" : "mono";

                WorldProfileSessionConfig profileConfig{};
                const auto unixMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                const std::string profilePath = SculptureWorldProfileSidecar::makeSessionPath(
                    "profiles", selectedPreset().id,
                    static_cast<std::uint64_t>(std::max<std::int64_t>(0, unixMilliseconds)), 1U);
                worldProfileDriver_.start(std::move(identity), profileConfig, profilePath);
                profileSessionIsInteractive_ = true;
                std::fprintf(stderr, "[3dEpiSculpTMorph] triggered profile session for '%s'\n", selectedPreset().id.c_str());
            } else {
                std::fprintf(stderr, "[3dEpiSculpTMorph] profile session already active\n");
            }
        } else if constexpr (std::is_same_v<T, EventSpatialGesture>) {
            Pose6DOF pose{};
            pose.position = {arg.posX, arg.posY, arg.posZ};
            pose.orientation = {arg.quatW, arg.quatX, arg.quatY, arg.quatZ};
            pose.trigger = arg.trigger;
            pose.grip = arg.grip;
            pose.valid = arg.valid;
            if (arg.isLeftHand) pipeline_.gestureEngine().setLeftHandPose(pose);
            else pipeline_.gestureEngine().setRightHandPose(pose);
        } else if constexpr (std::is_same_v<T, EventStereoRigConfig>) {
            // Admit the complete request before changing either eye's intent.
            // clamp/max do not reject NaN; refusal also preserves toe-in state.
            if (!std::isfinite(arg.ipd) || !std::isfinite(arg.convergenceDist)) return;
            stereoRig_.interpupillaryDistance = std::clamp(arg.ipd, 0.0f, 0.2f);
            stereoRig_.focalConvergenceDistance = std::max(arg.convergenceDist, 0.01f);
            stereoRig_.toeInConvergence = arg.toeIn;
        } else if constexpr (std::is_same_v<T, EventFoliationManifold>) {
            activeParams_.enable4D = static_cast<std::uint32_t>(arg.manifoldType) + 1U;
            control_.setManual4D(true);
            control_.sources().presetMorph().setSource(activeParams_);
        } else if constexpr (std::is_same_v<T, EventLodGovernorToggle>) {
            auto config = pipeline_.lodGovernorConfig();
            config.enableTelemetryDrivenLod = !config.enableTelemetryDrivenLod;
            pipeline_.setLodGovernorConfig(config);
        } else if constexpr (std::is_same_v<T, EventAtmosphereProfileCycle>) {
            constexpr int kProfileCount = 5;
            const int current = static_cast<int>(
                pipeline_.atmosphereEngine().config().profile);
            const int next = ((current + arg.direction) % kProfileCount
                + kProfileCount) % kProfileCount;
            pipeline_.setAtmosphereProfile(static_cast<MediumProfile>(next));
        } else if constexpr (std::is_same_v<T, EventGestureFilterReset>) {
            pipeline_.gestureEngine().gestureFilter().reset();
            pipeline_.lodGovernor().reset();
        } else if constexpr (std::is_same_v<T, EventKnotBraid>) {
            const auto candidate = numeric_edit::prepareFiniteSums(
                std::array{activeParams_.rotorAlpha, activeParams_.rotorBeta, activeParams_.latentW},
                std::array{arg.deltaP * 0.1f, arg.deltaQ * 0.1f, arg.deltaPhase * 0.05f});
            if (!candidate) return;
            activeParams_.rotorAlpha = (*candidate)[0];
            activeParams_.rotorBeta = (*candidate)[1];
            activeParams_.latentW = (*candidate)[2];
        } else if constexpr (std::is_same_v<T, EventRotorNudge>) {
            control_.nudgeManualRotors(activeParams_, arg.deltaAlpha, arg.deltaBeta, arg.deltaTheta4D);
        } else if constexpr (std::is_same_v<T, EventMultiTrackControl>) {
            // Reserved command: this facade does not yet apply multi-track edits.
            // Do not delegate implicitly to the non-owning legacy coordinator.
        } else if constexpr (std::is_same_v<T, EventPerformanceDeckControl>) {
            std::uint32_t beatSyncedBars = 2U;
            // Admit scalar values and cue availability before source takeover/defaulting.
            // Owner methods independently protect their direct callers.
            switch (arg.action) {
            case EventPerformanceDeckControl::Action::SetCrossfadePosition:
            case EventPerformanceDeckControl::Action::NudgeCrossfade:
            case EventPerformanceDeckControl::Action::TriggerAutoCrossfade:
            case EventPerformanceDeckControl::Action::SetMasterFilter:
            case EventPerformanceDeckControl::Action::TriggerTapeStop:
            case EventPerformanceDeckControl::Action::TriggerBeatRoll:
            case EventPerformanceDeckControl::Action::TriggerChaosBurst:
            case EventPerformanceDeckControl::Action::TapTempo:
            case EventPerformanceDeckControl::Action::SetBpm:
            case EventPerformanceDeckControl::Action::NudgeBpm:
            case EventPerformanceDeckControl::Action::SetMacroMasterAmount:
                if (!std::isfinite(arg.value)) return;
                break;
            case EventPerformanceDeckControl::Action::TriggerBeatSyncedCrossfade:
                // UINT32_MAX rounds up to 2^32 in float. Check the exclusive
                // bound in double before conversion or either source effect.
                if (!std::isfinite(arg.value)
                    || static_cast<double>(arg.value) >= 4294967296.0) return;
                beatSyncedBars = static_cast<std::uint32_t>(arg.value > 0.0f ? arg.value : 2.0f);
                break;
            case EventPerformanceDeckControl::Action::RecallCue:
            case EventPerformanceDeckControl::Action::RecallCueToOpposite:
                // A missing cue must not retire the currently selected source.
                // Lookup is read-only; actual loading/prewarm stays after takeover.
                if (control_.sources().deck().cue(arg.bankIndex) == nullptr) return;
                break;
            default:
                break;
            }
            clearCapturedSources();
            control_.sources().setDeckActive(true);
            switch (arg.action) {
            case EventPerformanceDeckControl::Action::SetCrossfadePosition:
                control_.sources().deck().setCrossfadePosition(arg.value);
                break;
            case EventPerformanceDeckControl::Action::NudgeCrossfade:
                control_.sources().deck().nudgeCrossfade(arg.value);
                break;
            case EventPerformanceDeckControl::Action::TapTempo:
                control_.sources().deck().tapTempo(arg.value > 0.0f ? arg.value : -1.0f);
                break;
            case EventPerformanceDeckControl::Action::SetBpm:
                control_.sources().deck().setBpm(arg.value);
                break;
            case EventPerformanceDeckControl::Action::NudgeBpm:
                control_.sources().deck().nudgeBpm(arg.value);
                break;
            case EventPerformanceDeckControl::Action::TriggerAutoCrossfade:
                control_.sources().deck().triggerAutoCrossfade(
                    arg.value > 0.0f ? arg.value : 4.0f,
                    arg.targetDeck == 0 ? std::optional<DeckId>(DeckId::DeckA) :
                    (arg.targetDeck == 1 ? std::optional<DeckId>(DeckId::DeckB) : std::nullopt));
                break;
            case EventPerformanceDeckControl::Action::TriggerBeatSyncedCrossfade:
                control_.sources().deck().triggerBeatSyncedCrossfade(
                    beatSyncedBars,
                    arg.targetDeck == 0 ? std::optional<DeckId>(DeckId::DeckA) :
                    (arg.targetDeck == 1 ? std::optional<DeckId>(DeckId::DeckB) : std::nullopt));
                break;
            case EventPerformanceDeckControl::Action::ToggleTourBeatSync:
                control_.sources().deck().toggleTourBeatSync();
                break;
            case EventPerformanceDeckControl::Action::SwapDecks:
                control_.sources().deck().swapDecks();
                break;
            case EventPerformanceDeckControl::Action::StoreCue:
                control_.sources().deck().storeCue(arg.bankIndex, activeParams_);
                break;
            case EventPerformanceDeckControl::Action::RecallCue:
                if (control_.sources().deck().recallCue(arg.bankIndex, static_cast<DeckId>(arg.targetDeck))) {
                    appState_.requestTemporalReset = true;
                }
                break;
            case EventPerformanceDeckControl::Action::RecallCueToOpposite:
                if (control_.sources().deck().recallCueToOppositeDeck(arg.bankIndex)) {
                    appState_.requestTemporalReset = true;
                }
                break;
            case EventPerformanceDeckControl::Action::TriggerTapeStop:
                control_.sources().deck().triggerTapeStop(arg.value > 0.0f ? arg.value : 1.2f);
                break;
            case EventPerformanceDeckControl::Action::TriggerBeatRoll:
                control_.sources().deck().triggerBeatRoll(arg.value > 0.0f ? arg.value : 0.25f, 1.0f);
                break;
            case EventPerformanceDeckControl::Action::TriggerChaosBurst:
                control_.sources().deck().triggerChaosBurst(arg.value > 0.0f ? arg.value : 1.0f);
                break;
            case EventPerformanceDeckControl::Action::SetMasterFilter:
                control_.sources().deck().setMasterFilter(arg.value);
                break;
            case EventPerformanceDeckControl::Action::ToggleMacroEngine: {
                auto& macro = control_.sources().deck().macroEngine();
                macro.setMasterBypass(!macro.isMasterBypassed());
                break;
            }
            case EventPerformanceDeckControl::Action::SetMacroMasterAmount:
                control_.sources().deck().macroEngine().setMasterAmount(arg.value);
                break;
            }
        } else if constexpr (std::is_same_v<T, EventMidiProfile>) {
            if (midiServer_) {
                const std::string& prof = arg.profileName;
                if (prof == "launchpad" || prof == "launchcontrol") {
                    midiServer_->mapper().setProfile(ControlSurfaceProfile::LaunchControlXL);
                } else if (prof == "korg" || prof == "nanokontrol") {
                    midiServer_->mapper().setProfile(ControlSurfaceProfile::KorgNano);
                } else if (prof == "apc" || prof == "akai") {
                    midiServer_->mapper().setProfile(ControlSurfaceProfile::AkaiAPC);
                } else if (prof == "keyboard" || prof == "synth") {
                    midiServer_->mapper().setProfile(ControlSurfaceProfile::GenericKeyboard);
                } else {
                    midiServer_->mapper().setProfile(ControlSurfaceProfile::StandardDJ);
                }
                std::fprintf(stderr, "[SculptureApplication] Switched MIDI profile to: %s\n",
                             std::string(midiServer_->mapper().profileName()).c_str());
            }
        } else if constexpr (std::is_same_v<T, EventTimelineControl>) {
            const bool wasCinematic = control_.sources().cinematic();
            const auto outcome = control_.sources().timelineControl(arg);
            if (!wasCinematic && control_.sources().cinematic()) clearCapturedSources();
            if (outcome.selectedSequenceCue) {
                // Includes existing application defaults for an empty cue map.
                applyModulationGraph(control_.sources().sequence().currentRoutingMap());
            }
            if (outcome.handled) return;
            switch (arg.action) {
            case EventTimelineControl::Action::LoadFile:
                std::fprintf(stderr, "[SculptureApplication] Timeline file load: %s\n", arg.argument.c_str());
                break;
            case EventTimelineControl::Action::BuildTour:
                std::fprintf(stderr, "[SculptureApplication] Tour timeline built: %s\n", arg.argument.c_str());
                break;
            case EventTimelineControl::Action::BuildBeatSynced:
                std::fprintf(stderr, "[SculptureApplication] Beat-synced timeline built: bpm=%.1f\n", arg.bpm);
                break;
            case EventTimelineControl::Action::Export:
                std::fprintf(stderr, "[SculptureApplication] Timeline exported: %s\n", arg.argument.c_str());
                break;
            default: break;
            }
        } else if constexpr (std::is_same_v<T, EventSequenceRecorderControl>) {
            appState_.capture.onEvent(arg);
        } else if constexpr (std::is_same_v<T, EventPaletteAgentControl>) {
            switch (arg.action) {
            case EventPaletteAgentControl::Action::Toggle:
                activeParams_.chromaBreath = (activeParams_.chromaBreath > 0.05f) ? 0.0f : 0.35f;
                control_.sources().presetMorph().setSource(activeParams_);
                break;
            case EventPaletteAgentControl::Action::SetMode:
                activeParams_.seedPulseFrequency = 0.1f * (1.0f + arg.mode);
                control_.sources().presetMorph().setSource(activeParams_);
                break;
            case EventPaletteAgentControl::Action::Stimulate:
                activeParams_.seedPulseAmplitude = std::clamp(activeParams_.seedPulseAmplitude + arg.intensity * 0.4f, 0.0f, 2.0f);
                control_.sources().presetMorph().setSource(activeParams_);
                break;
            case EventPaletteAgentControl::Action::Reseed:
                mutator_.reseed(arg.seed);
                break;
            }
        } else if constexpr (std::is_same_v<T, EventThematicTourControl>) {
            switch (arg.action) {
            case EventThematicTourControl::Action::SelectTour:
                if (!arg.identifier.empty()) {
                    if (presetLibrary_.buildCuratedTour(control_.sources().presetMorph(), arg.identifier,
                            arg.transitionSeconds, arg.dwellSeconds)) {
                        control_.sources().presetMorph().startPlaylist();
                    } else {
                        std::fprintf(stderr, "[SculptureApplication] Tour request rejected; playback retained: %s\n",
                                     arg.identifier.c_str());
                    }
                }
                break;
            case EventThematicTourControl::Action::CycleTour:
                control_.sources().presetMorph().stepPlaylistNext();
                break;
            case EventThematicTourControl::Action::SelectCategory:
                if (!arg.identifier.empty()) {
                    if (presetLibrary_.buildCategoryTour(control_.sources().presetMorph(), arg.identifier,
                            arg.transitionSeconds, arg.dwellSeconds)) {
                        control_.sources().presetMorph().startPlaylist();
                    } else {
                        std::fprintf(stderr, "[SculptureApplication] Category request rejected; playback retained: %s\n",
                                     arg.identifier.c_str());
                    }
                }
                break;
            case EventThematicTourControl::Action::CycleCategory: {
                const auto cats = presetLibrary_.categories();
                if (!cats.empty()) {
                    categoryTourCycleIndex_ = (categoryTourCycleIndex_ + 1) % cats.size();
                    if (presetLibrary_.buildCategoryTour(control_.sources().presetMorph(), cats[categoryTourCycleIndex_],
                            arg.transitionSeconds, arg.dwellSeconds)) {
                        control_.sources().presetMorph().startPlaylist();
                    } else {
                        std::fprintf(stderr, "[SculptureApplication] Category request rejected; playback retained: %s\n",
                                     cats[categoryTourCycleIndex_].c_str());
                    }
                }
                break;
            }
            case EventThematicTourControl::Action::ToggleAutopilot:
                control_.sources().presetMorph().togglePlaylistPause();
                break;
            }
        } else if constexpr (std::is_same_v<T, EventAudioFeedControl>) {
            switch (arg.action) {
            case EventAudioFeedControl::Action::CycleMode:
                break;
            case EventAudioFeedControl::Action::SetMode:
                break;
            case EventAudioFeedControl::Action::AdjustBpm:
                break;
            case EventAudioFeedControl::Action::SetBpm:
                break;
            case EventAudioFeedControl::Action::ToggleEnabled:
                appState_.audioEnabled = !appState_.audioEnabled;
                break;
            case EventAudioFeedControl::Action::SetGain:
                break;
            }
        } else if constexpr (std::is_same_v<T, EventPerformancePacingControl>) {
            switch (arg.action) {
            case EventPerformancePacingControl::Action::ToggleMonitor:
                break;
            case EventPerformancePacingControl::Action::CycleTargetTier:
                break;
            case EventPerformancePacingControl::Action::SetTargetTier:
                break;
            case EventPerformancePacingControl::Action::ResetStats:
                stagedGovernor_.reset();
                break;
            case EventPerformancePacingControl::Action::SetResolutionScale:
                stagedGovernor_.setResolutionScale(static_cast<StageResolutionScale>(arg.resolutionScale));
                break;
            case EventPerformancePacingControl::Action::ToggleAutoResolution: {
                auto cfg = stagedGovernor_.config();
                cfg.enableAutoResolutionTiering = !cfg.enableAutoResolutionTiering;
                stagedGovernor_.setConfig(cfg);
                break;
            }
            }
        } else if constexpr (std::is_same_v<T, EventSceneRenderScaleControl>) {
            switch (arg.action) {
            case EventSceneRenderScaleControl::Action::Cycle: {
                const float next = SculpturePresentationScalePolicy::nextManualScale(
                    presentationHost_->engine().requestedSceneRenderScale());
                if (presentationHost_->engine().requestSceneRenderScale(next)) {
                    presentationScalePolicy_.acceptManualOverride();
                    std::fprintf(stderr,
                        "[SculptureApplication] scene render scale requested: %.3f (manual)\n",
                        static_cast<double>(next));
                }
                break;
            }
            case EventSceneRenderScaleControl::Action::Set:
                if (presentationHost_->engine().requestSceneRenderScale(arg.scale)) {
                    presentationScalePolicy_.acceptManualOverride();
                    std::fprintf(stderr,
                        "[SculptureApplication] scene render scale requested: %.3f (manual)\n",
                        static_cast<double>(arg.scale));
                }
                break;
            case EventSceneRenderScaleControl::Action::ToggleAuto: {
                const bool enable = !presentationScalePolicy_.autoEnabled();
                if (enable && !presentationHost_->engine().gpuPassTimingEnabled()) {
                    std::fprintf(stderr,
                        "[SculptureApplication] automatic scene scaling requires GPU pass "
                        "timing (--gpu-pass-timing or --auto-scene-scale)\n");
                    break;
                }
                presentationScalePolicy_.setAutoEnabled(enable);
                std::fprintf(stderr,
                    "[SculptureApplication] automatic scene scaling: %s\n",
                    enable ? "ON" : "OFF");
                break;
            }
            }
        } else if constexpr (std::is_same_v<T, EventPerformanceMetrics>) {
            // Passive telemetry metrics observation: no state mutation required
        } else if constexpr (std::is_same_v<T, EventParameterPatch>) {
            if (!arg.diff.empty()) {
                SculptureSerializer::applyDiff(activeParams_, arg.diff);
                control_.sources().presetMorph().setSource(activeParams_);
            }
        } else if constexpr (std::is_same_v<T, EventAtmosphereControl>) {
            switch (arg.action) {
            case EventAtmosphereControl::Action::Toggle:
                break;
            case EventAtmosphereControl::Action::Enable:
                break;
            case EventAtmosphereControl::Action::Disable:
                break;
            }
        } else if constexpr (std::is_same_v<T, EventSnapshotControl>) {
            appState_.capture.onEvent(arg);
        } else if constexpr (std::is_same_v<T, EventVolumetricControl>) {
            // Reserved command: no volumetric-parameter mutation is wired here yet.
        } else if constexpr (std::is_same_v<T, EventVolumetricTelemetry>) {
            // Passive volumetric buffer telemetry
        } else if constexpr (std::is_same_v<T, EventHistoryControl>) {
            switch (arg.action) {
            case EventHistoryControl::Action::Undo:
            case EventHistoryControl::Action::Redo:
            case EventHistoryControl::Action::Clear:
                break;
            }
        } else if constexpr (std::is_same_v<T, EventMorphSpaceControl>) {
            switch (arg.action) {
            case EventMorphSpaceControl::Action::SetCursor:
                if (arg.cursorX >= 0.0f && arg.cursorY >= 0.0f) {
                    control_.sources().morphSpace().setCursor(arg.cursorX, arg.cursorY);
                } else if (arg.cursorX >= 0.0f) {
                    control_.sources().morphSpace().setCursor(arg.cursorX, control_.sources().morphSpace().cursorY());
                } else if (arg.cursorY >= 0.0f) {
                    control_.sources().morphSpace().setCursor(control_.sources().morphSpace().cursorX(), arg.cursorY);
                }
                break;
            case EventMorphSpaceControl::Action::NudgeCursor:
                control_.sources().morphSpace().nudgeCursor(arg.cursorX, arg.cursorY);
                break;
            case EventMorphSpaceControl::Action::SetMotionMode:
                control_.sources().morphSpace().setMotionMode(static_cast<MorphSpaceMotion>(arg.mode));
                break;
            case EventMorphSpaceControl::Action::CycleMotionMode:
                control_.sources().morphSpace().cycleMotionMode();
                break;
            case EventMorphSpaceControl::Action::SetMotionSpeed:
                control_.sources().morphSpace().setMotionSpeed(arg.speed);
                break;
            case EventMorphSpaceControl::Action::SetShepardPower:
                control_.sources().morphSpace().setShepardPower(arg.shepardPower);
                break;
            case EventMorphSpaceControl::Action::ToggleActive:
                if (!control_.sources().morphSpace().isActive()) clearCapturedSources();
                control_.sources().morphSpace().setActive(!control_.sources().morphSpace().isActive());
                std::fprintf(stderr, "[SculptureApplication] 2D Morph Space %s\n",
                    control_.sources().morphSpace().isActive() ? "ACTIVATED" : "DEACTIVATED");
                break;
            case EventMorphSpaceControl::Action::LoadBank:
                if (!arg.bankName.empty()) {
                    control_.sources().morphSpace().loadBank(arg.bankName);
                } else {
                    control_.sources().morphSpace().loadBank(arg.bankIndex);
                }
                break;
            case EventMorphSpaceControl::Action::CycleBank:
                control_.sources().morphSpace().cycleBank();
                std::fprintf(stderr, "[SculptureApplication] Morph Space Bank: %.*s\n",
                    static_cast<int>(control_.sources().morphSpace().activeBankTitle().size()),
                    control_.sources().morphSpace().activeBankTitle().data());
                break;
            case EventMorphSpaceControl::Action::TogglePath:
                control_.sources().morphSpace().setMotionMode(
                    control_.sources().morphSpace().motionMode() == MorphSpaceMotion::Path
                        ? MorphSpaceMotion::Static
                        : MorphSpaceMotion::Path);
                break;
            case EventMorphSpaceControl::Action::ClearPath:
                control_.sources().morphSpace().clearPath();
                break;
            case EventMorphSpaceControl::Action::AddWaypoint:
                control_.sources().morphSpace().addWaypoint(arg.cursorX, arg.cursorY, arg.dwellSec, arg.travelSec);
                break;
            case EventMorphSpaceControl::Action::SyncCategory:
                syncMorphSpaceToActiveCategory();
                std::fprintf(stderr, "[SculptureApplication] Morph Space synced to active category: %.*s\n",
                    static_cast<int>(control_.sources().morphSpace().activeBankTitle().size()),
                    control_.sources().morphSpace().activeBankTitle().data());
                break;
            case EventMorphSpaceControl::Action::BindCategory:
                if (!arg.category.empty()) {
                    bindMorphSpaceToCategory(arg.category, /*autoLoad=*/true);
                }
                break;
            }
        } else if constexpr (std::is_same_v<T, EventReliefProjectionCycle>) {
            cycleReliefProjection(arg.direction);
        } else if constexpr (std::is_same_v<T, EventReliefProjectionSelect>) {
            if (arg.projectionMode < 0 || arg.projectionMode >= static_cast<int>(kReliefManifoldProjectionCount)) {
                activeReliefProjectionOverride_ = std::nullopt;
                std::fprintf(stderr, "[SculptureApplication] 2.5D Epimorphic Relief Projection: AUTO-RESOLVED (%s)\n",
                             name(resolveActiveReliefProjection()));
            } else {
                activeReliefProjectionOverride_ = static_cast<ReliefManifoldProjection>(arg.projectionMode);
                std::fprintf(stderr, "[SculptureApplication] 2.5D Epimorphic Relief Projection: SET -> %s\n",
                             name(*activeReliefProjectionOverride_));
            }
        } else {
            static_assert(kUnhandledSculptureEvent<T>,
                "Every SculptureEvent must have an explicit application handler");
        }
    }, event);
}

void SculptureApplication::applyModulationGraph(const std::vector<std::string>& routingMap) {
    ModulationRouteDiagnostic diagnostic;
    const auto outcome = control_.reconcileModulationGraph(
        routingMap, appState_.audioEnabled, diagnostic);
    if (outcome == modulation_routing::ReconcileOutcome::Rejected) {
        std::fprintf(stderr,
            "[SculptureApplication] Rejected modulation graph (%zu routes, route-index=%zu, reason=%s); retaining previous graph\n",
            control_.lastRequestedRoutingMap().size(), diagnostic.routeIndex,
            modulation_inspection::name(diagnostic.error).data());
    }
}

} // namespace epi_sculpt
