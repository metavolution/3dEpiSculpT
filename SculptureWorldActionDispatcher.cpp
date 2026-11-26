#include "SculptureWorldActionDispatcher.h"
#include "SculpturePerformanceMonitor.h"
#include "SculptureFluidAtmosphereManager.h"
#include "SculptureAudioFeedDirector.h"
#include "SculptureEventBus.h"
#include "SculptureVolumetricControl.h"

#include <algorithm>
#include <cmath>

namespace epi_sculpt {

bool SculptureWorldActionDispatcher::dispatchAction(
    SculptureWorldNavigator& navigator,
    NavigatorAction action,
    float value
) noexcept {
    navigator.notifyObservers([&](INavigatorObserver& obs) {
        obs.onActionDispatched(action, value, actionName(action));
    });

    switch (action) {
    case NavigatorAction::ReloadPresetsAndKernel:
        if (navigator.boundEventBus_) {
            navigator.boundEventBus_->publish(EventReloadShaders{});
        }
        return true;

    case NavigatorAction::NextWorld:
        if (navigator.isTourActive()) {
            navigator.stepTourNext();
        } else {
            navigator.nextWorld(value > 0.0f ? value : 1.5f);
        }
        return true;

    case NavigatorAction::PrevWorld:
        if (navigator.isTourActive()) {
            navigator.stepTourPrev();
        } else {
            navigator.prevWorld(value > 0.0f ? value : 1.5f);
        }
        return true;

    case NavigatorAction::ToggleTour:
        navigator.toggleTour();
        return true;

    case NavigatorAction::CycleTour:
        navigator.cycleCuratedTour();
        return true;

    case NavigatorAction::StepTourNext:
        navigator.stepTourNext();
        return true;

    case NavigatorAction::StepTourPrev:
        navigator.stepTourPrev();
        return true;

    case NavigatorAction::Mutate:
        navigator.mutate(value > 0.0f ? value : 0.22f, 2.0f);
        return true;

    case NavigatorAction::RandomizePalette:
        navigator.randomizePalette(value > 0.0f ? value : 1.5f);
        return true;

    case NavigatorAction::ResetParameters:
        navigator.resetParameters(value > 0.0f ? value : 1.2f);
        return true;

    case NavigatorAction::CycleInnerMap:
        navigator.cycleInnerMap(value > 0.0f ? value : 1.0f);
        return true;

    case NavigatorAction::CycleReduction:
        navigator.cycleReduction(value > 0.0f ? value : 1.0f);
        return true;

    case NavigatorAction::CycleMirrorTopology:
        return navigator.cycleMirrorTopology();

    case NavigatorAction::SetMirrorTopology: {
        const int topology = static_cast<int>(std::round(value));
        if (topology < static_cast<int>(MirrorCellTopology::Triangular) ||
            topology > static_cast<int>(MirrorCellTopology::Hexagonal)) {
            return false;
        }
        return navigator.setMirrorTopology(static_cast<MirrorCellTopology>(topology));
    }

    case NavigatorAction::ResetCamera:
        navigator.resetCameraFraming();
        return true;

    case NavigatorAction::CyclePresentationMode:
        navigator.cyclePresentationMode();
        return true;

    case NavigatorAction::CycleReliefProjection:
        navigator.cycleReliefProjection();
        return true;

    case NavigatorAction::CycleResolutionScale:
        navigator.cycleResolutionScale();
        return true;

    case NavigatorAction::ToggleStagedGovernor:
        navigator.toggleStagedGovernor();
        return true;

    case NavigatorAction::Toggle4DFoliationWave:
        navigator.toggle4DFoliationWave();
        return true;

    case NavigatorAction::Cycle4DManifold:
        navigator.cycle4DManifoldType();
        return true;

    case NavigatorAction::CyclePolychoronSource:
        navigator.cyclePolychoronSource();
        return true;

    case NavigatorAction::CyclePolychoronTarget:
        navigator.cyclePolychoronTarget();
        return true;

    case NavigatorAction::StepPolychoronMorph: {
        float nextP = navigator.polychoronMorphProgress() + (value != 0.0f ? value : 0.25f);
        if (nextP > 1.01f) nextP = 0.0f;
        navigator.setPolychoronMorphProgress(nextP);
        return true;
    }

    case NavigatorAction::CycleRotorSpeed:
        navigator.cycleRotorSpeed();
        return true;

    case NavigatorAction::TogglePaletteAgent:
        navigator.togglePaletteAgent();
        return true;

    case NavigatorAction::CyclePaletteAgentMode:
        navigator.cyclePaletteAgentMode();
        return true;

    case NavigatorAction::CycleLearnStage:
        if (value > 0.0f) {
            const auto rawVal = static_cast<std::uint32_t>(value);
            if (rawVal == learn_mode::kResetStagesCommand) {
                navigator.resetLearnMode();
            } else if (rawVal == learn_mode::kCycleDiagnosticCommand) {
                navigator.cycleDiagnosticStage();
            } else if ((rawVal & learn_mode::kSoloStageCommandMask) != 0U) {
                const auto stageBits = rawVal & ~learn_mode::kSoloStageCommandMask;
                for (std::uint8_t s = 0; s < 6; ++s) {
                    if ((stageBits & (1U << s)) != 0U) {
                        navigator.setLearnStageSolo(static_cast<learn_mode::Stage>(s));
                        break;
                    }
                }
            } else if ((rawVal & learn_mode::kAccumulativeStageCommandMask) != 0U) {
                const auto maxStageIdx = rawVal & ~learn_mode::kAccumulativeStageCommandMask;
                if (maxStageIdx < 6U) {
                    navigator.setLearnStageAccumulative(static_cast<learn_mode::Stage>(maxStageIdx));
                }
            } else if ((rawVal & learn_mode::kAllStages) != 0U) {
                navigator.coordinator_.setStageMask(rawVal);
            } else {
                navigator.cycleLearnStage();
            }
        } else {
            navigator.cycleLearnStage();
        }
        return true;

    case NavigatorAction::ResetLearnMode:
    case NavigatorAction::ResetLearnStages:
        navigator.resetLearnMode();
        return true;

    case NavigatorAction::SoloStage1:
        navigator.setLearnStageSolo(learn_mode::Stage::Transform);
        return true;

    case NavigatorAction::SoloStage2:
        navigator.setLearnStageSolo(learn_mode::Stage::Reduce);
        return true;

    case NavigatorAction::SoloStage3:
        navigator.setLearnStageSolo(learn_mode::Stage::FeedbackSample);
        return true;

    case NavigatorAction::SoloStage4:
        navigator.setLearnStageSolo(learn_mode::Stage::Color);
        return true;

    case NavigatorAction::SoloStage5:
        navigator.setLearnStageSolo(learn_mode::Stage::Seed);
        return true;

    case NavigatorAction::SoloStage6:
        navigator.setLearnStageSolo(learn_mode::Stage::Blend);
        return true;

    case NavigatorAction::SoloStage: {
        const auto idx = static_cast<int>(value);
        if (idx >= 0 && idx < 6) {
            navigator.setLearnStageSolo(static_cast<learn_mode::Stage>(idx));
            return true;
        }
        return false;
    }

    case NavigatorAction::SetLearnAccumulative: {
        const auto idx = static_cast<int>(value);
        if (idx >= 0 && idx < 6) {
            navigator.setLearnStageAccumulative(static_cast<learn_mode::Stage>(idx));
            return true;
        }
        return false;
    }

    case NavigatorAction::CycleDiagnostic:
        navigator.cycleDiagnosticStage();
        return true;

    case NavigatorAction::IncreaseSpeed:
        navigator.increaseSpeed(value > 0.0f ? value : 1.2f);
        return true;

    case NavigatorAction::DecreaseSpeed:
        navigator.decreaseSpeed(value > 0.0f ? value : 1.2f);
        return true;

    case NavigatorAction::ResetSpeed:
        navigator.resetSpeed();
        return true;

    case NavigatorAction::TogglePause:
        navigator.togglePause();
        return true;

    case NavigatorAction::AdjustLatentW:
        navigator.adjustLatentW(value);
        return true;

    case NavigatorAction::AdjustRotorAlpha:
        navigator.adjustRotorAlpha(value);
        return true;

    case NavigatorAction::AdjustRotorBeta:
        navigator.adjustRotorBeta(value);
        return true;

    case NavigatorAction::Reset4DRotors:
        navigator.reset4DRotors();
        return true;

    case NavigatorAction::ToggleSequenceRecording:
        navigator.toggleSequenceRecording();
        return true;

    case NavigatorAction::RequestSnapshot:
        navigator.requestSnapshot();
        return true;

    case NavigatorAction::ReloadLatestSnapshot:
        if (!navigator.latestSnapshotPath_.empty()) {
            return navigator.loadSnapshotBundle(navigator.latestSnapshotPath_);
        }
        return false;

    case NavigatorAction::ToggleTimeline:
        navigator.toggleTimeline();
        return true;

    case NavigatorAction::TogglePerfMonitor:
        navigator.togglePerformanceMonitor();
        return true;

    case NavigatorAction::CycleTargetFps:
        navigator.cycleTargetFps();
        return true;

    case NavigatorAction::ResetPerfStats:
        if (navigator.performanceMonitor_ != nullptr) {
            navigator.performanceMonitor_->reset();
        }
        return true;

    case NavigatorAction::RevertToBaseline:
        navigator.resetParameters(value > 0.0f ? value : 0.0f);
        return true;

    case NavigatorAction::ExportDiffPatch:
        return navigator.exportBaselineDiff("snapshots/" + navigator.currentPresetName_ + "_patch.epidiff");

    case NavigatorAction::ToggleAtmosphere:
        if (navigator.atmosphereManager_ != nullptr) {
            navigator.atmosphereManager_->setEnabled(!navigator.atmosphereManager_->isEnabled());
            return true;
        }
        return false;

    case NavigatorAction::CycleAudioFeedMode:
        if (navigator.audioDirector_ != nullptr) {
            navigator.audioDirector_->cycleMode();
            return true;
        }
        return false;

    case NavigatorAction::AdjustAudioBpm:
        if (navigator.audioDirector_ != nullptr) {
            navigator.audioDirector_->adjustBpm(value != 0.0f ? value : 5.0f);
            return true;
        }
        return false;

    case NavigatorAction::ToggleAudioFeed:
        if (navigator.audioDirector_ != nullptr) {
            navigator.audioDirector_->toggleEnabled();
            return true;
        }
        return false;

    case NavigatorAction::BuildBeatSyncedTimeline:
        return navigator.generateBeatSyncedTimeline(value > 0.0f ? value : 0.0f);

    case NavigatorAction::ExportTimelineScript:
        return navigator.exportActiveTimeline();

    case NavigatorAction::TogglePerformanceDeck:
        navigator.togglePerformanceDeck();
        return true;

    case NavigatorAction::CrossfadeDeck:
        navigator.setCrossfadePosition(value);
        return true;

    case NavigatorAction::TriggerAutoCrossfade:
        navigator.triggerAutoCrossfade(value > 0.0f ? value : 3.0f);
        return true;

    case NavigatorAction::SwapPerformanceDecks:
        navigator.swapPerformanceDecks();
        return true;

    case NavigatorAction::LoadDeckA:
        navigator.loadActiveToDeck(DeckId::DeckA);
        return true;

    case NavigatorAction::LoadDeckB:
        navigator.loadActiveToDeck(DeckId::DeckB);
        return true;

    case NavigatorAction::SetVariableResolutionScale: {
        const int scaleInt = static_cast<int>(std::round(value));
        StageResolutionScale scale = StageResolutionScale::Full;
        if (scaleInt == 1) scale = StageResolutionScale::Half;
        else if (scaleInt == 2) scale = StageResolutionScale::Quarter;
        else if (scaleInt == 3) scale = StageResolutionScale::Eighth;
        navigator.setVariableResolutionScale(scale);
        return true;
    }

    case NavigatorAction::ToggleAutoResolutionScaling:
        navigator.setAutoResolutionScalingEnabled(!navigator.isAutoResolutionScalingEnabled());
        return true;

    case NavigatorAction::PlayCategoryTour:
        if (navigator.activeTourCategory_.empty()) {
            navigator.cycleCategoryTour();
        } else {
            navigator.selectCategoryTour(navigator.activeTourCategory_, value > 0.0f ? value : 3.5f);
        }
        return true;

    case NavigatorAction::CycleCategoryTour:
        navigator.cycleCategoryTour();
        return true;

    case NavigatorAction::CycleVolumetricResolution:
        navigator.cycleVolumetricResolution();
        return true;

    case NavigatorAction::ToggleVolumetricFeedback:
        navigator.toggleVolumetricFeedback();
        return true;

    case NavigatorAction::ResetVolumetricMemory:
        navigator.resetVolumetricMemory();
        return true;

    case NavigatorAction::ToggleVolumetricWrap:
        navigator.toggleVolumetricWrap();
        return true;

    case NavigatorAction::ToggleVolumetricBackend:
        navigator.toggleVolumetricBackend();
        return true;

    case NavigatorAction::ToggleVolumetricCoupling:
        navigator.toggleVolumetricCoupling();
        return true;

    case NavigatorAction::ToggleVolumetricAdvection:
        navigator.toggleVolumetricAdvection();
        return true;

    case NavigatorAction::ToggleAcousticSynesthesia:
        navigator.toggleAcousticSynesthesia();
        return true;

    case NavigatorAction::Undo:
        return navigator.undo();

    case NavigatorAction::Redo:
        return navigator.redo();

    case NavigatorAction::ToggleMorphSpace:
        navigator.toggleMorphSpace();
        return true;

    case NavigatorAction::CycleMorphSpaceMotion:
        navigator.cycleMorphSpaceMotion();
        return true;

    case NavigatorAction::SetMorphSpaceX:
        if (navigator.morphSpace_ != nullptr) {
            navigator.setMorphSpaceCursor(value, navigator.morphSpace_->cursorY());
            return true;
        }
        return false;

    case NavigatorAction::SetMorphSpaceY:
        if (navigator.morphSpace_ != nullptr) {
            navigator.setMorphSpaceCursor(navigator.morphSpace_->cursorX(), value);
            return true;
        }
        return false;

    case NavigatorAction::NudgeMorphSpaceX:
        if (navigator.morphSpace_ != nullptr) {
            navigator.nudgeMorphSpaceCursor(value, 0.0f);
            return true;
        }
        return false;

    case NavigatorAction::NudgeMorphSpaceY:
        if (navigator.morphSpace_ != nullptr) {
            navigator.nudgeMorphSpaceCursor(0.0f, value);
            return true;
        }
        return false;

    case NavigatorAction::CycleMorphSpaceBank:
        navigator.cycleMorphSpaceBank();
        return true;

    case NavigatorAction::ToggleMorphSpacePath:
        navigator.toggleMorphSpacePath();
        return true;

    case NavigatorAction::SyncMorphSpaceCategory:
        navigator.syncMorphSpaceToActiveCategory();
        return true;

    case NavigatorAction::BindMorphSpaceCategory: {
        const auto cats = navigator.availableTourCategories();
        if (value >= 0.0f && !cats.empty()) {
            const auto idx = static_cast<std::size_t>(value) % cats.size();
            navigator.bindMorphSpaceToCategory(cats[idx], true);
        } else {
            navigator.syncMorphSpaceToActiveCategory();
        }
        return true;
    }

    case NavigatorAction::ToggleStereoSBS:
        navigator.toggleStereo();
        return true;

    default:
        return false;
    }
}

void SculptureWorldActionDispatcher::bindEventBus(
    SculptureWorldNavigator& navigator,
    SculptureEventBus& eventBus
) {
    navigator.boundEventBus_ = &eventBus;
    if (navigator.performanceMonitor_ != nullptr) {
        navigator.performanceMonitor_->bindEventBus(&eventBus);
    }

    eventBus.subscribeTo<EventWorldCatalogSelect>([&navigator](const EventWorldCatalogSelect& ev) {
        navigator.selectWorld(ev.catalogIndex, ev.durationSeconds);
    });

    eventBus.subscribeTo<EventToggleState>([&navigator, &eventBus](const EventToggleState& ev) {
        const bool coordBound = navigator.coordinator_.isBoundToEventBus(&eventBus);
        switch (ev.target) {
        case EventToggleState::Target::Pause:
            if (!coordBound) {
                navigator.dispatchAction(NavigatorAction::TogglePause, 0.0f);
            }
            break;
        case EventToggleState::Target::Topology4D:
            if (!coordBound) {
                navigator.dispatchAction(NavigatorAction::Toggle4DFoliationWave, 0.0f);
            }
            break;
        case EventToggleState::Target::RecordSequence:
            navigator.dispatchAction(NavigatorAction::ToggleSequenceRecording, 0.0f);
            break;
        default:
            break;
        }
    });

    eventBus.subscribeTo<EventPlaybackRate>([&navigator, &eventBus](const EventPlaybackRate& ev) {
        if (navigator.coordinator_.isBoundToEventBus(&eventBus)) {
            return;
        }
        switch (ev.action) {
        case EventPlaybackRate::Action::Increase:
            navigator.dispatchAction(NavigatorAction::IncreaseSpeed, 1.2f);
            break;
        case EventPlaybackRate::Action::Decrease:
            navigator.dispatchAction(NavigatorAction::DecreaseSpeed, 1.2f);
            break;
        case EventPlaybackRate::Action::Reset:
            navigator.dispatchAction(NavigatorAction::ResetSpeed, 0.0f);
            break;
        }
    });

    eventBus.subscribeTo<EventTimelineControl>([&navigator](const EventTimelineControl& ev) {
        switch (ev.action) {
        case EventTimelineControl::Action::Play:
            navigator.playTimeline();
            break;
        case EventTimelineControl::Action::Pause:
            navigator.pauseTimeline();
            break;
        case EventTimelineControl::Action::Stop:
            navigator.stopTimeline();
            break;
        case EventTimelineControl::Action::TogglePlayPause:
            navigator.toggleTimeline();
            break;
        case EventTimelineControl::Action::LoadFile:
            if (!ev.argument.empty()) {
                navigator.loadTimelineScript(ev.argument);
            }
            break;
        case EventTimelineControl::Action::BuildBeatSynced:
            navigator.generateBeatSyncedTimeline(ev.bpm);
            break;
        case EventTimelineControl::Action::Export:
            navigator.exportActiveTimeline(ev.argument);
            break;
        default:
            break;
        }
    });

    eventBus.subscribeTo<EventSequenceRecorderControl>([&navigator](const EventSequenceRecorderControl& ev) {
        switch (ev.action) {
        case EventSequenceRecorderControl::Action::Start:
            navigator.startSequenceRecording(ev.sessionName);
            break;
        case EventSequenceRecorderControl::Action::Stop:
            navigator.stopSequenceRecording();
            break;
        case EventSequenceRecorderControl::Action::Toggle:
            navigator.toggleSequenceRecording(ev.sessionName);
            break;
        }
    });

    eventBus.subscribeTo<EventSnapshotControl>([&navigator](const EventSnapshotControl& ev) {
        switch (ev.action) {
        case EventSnapshotControl::Action::Request:
            navigator.dispatchAction(NavigatorAction::RequestSnapshot, 0.0f);
            break;
        case EventSnapshotControl::Action::ReloadLatest:
            navigator.dispatchAction(NavigatorAction::ReloadLatestSnapshot, 0.0f);
            break;
        }
    });

    eventBus.subscribeTo<EventParameterPatch>([&navigator](const EventParameterPatch& ev) {
        if (!ev.diff.empty()) {
            navigator.applyPatch(ev.diff, ev.tag);
        }
    });

    eventBus.subscribeTo<EventMutation>([&navigator](const EventMutation& ev) {
        switch (ev.action) {
        case EventMutation::Action::StepMutation:
            navigator.dispatchAction(NavigatorAction::Mutate, ev.strength);
            break;
        case EventMutation::Action::ResetBase:
            navigator.dispatchAction(NavigatorAction::ResetParameters, 0.0f);
            break;
        case EventMutation::Action::AdjustLatentW:
            navigator.dispatchAction(NavigatorAction::AdjustLatentW, ev.strength);
            break;
        case EventMutation::Action::AdjustRotorAlpha:
            navigator.dispatchAction(NavigatorAction::AdjustRotorAlpha, ev.strength);
            break;
        case EventMutation::Action::AdjustRotorBeta:
            navigator.dispatchAction(NavigatorAction::AdjustRotorBeta, ev.strength);
            break;
        default:
            break;
        }
    });

    eventBus.subscribeTo<EventRotorNudge>([&navigator](const EventRotorNudge& ev) {
        if (ev.deltaAlpha != 0.0f) {
            navigator.dispatchAction(NavigatorAction::AdjustRotorAlpha, ev.deltaAlpha);
        }
        if (ev.deltaBeta != 0.0f) {
            navigator.dispatchAction(NavigatorAction::AdjustRotorBeta, ev.deltaBeta);
        }
        if (ev.deltaTheta4D != 0.0f) {
            navigator.adjustRotorTheta4D(ev.deltaTheta4D);
        }
    });

    eventBus.subscribeTo<EventFoliationManifold>([&navigator](const EventFoliationManifold& ev) {
        navigator.set4DManifoldType(static_cast<Manifold4DType>(ev.manifoldType));
    });

    eventBus.subscribeTo<EventSteering>([&navigator](const EventSteering& ev) {
        if (ev.deltaLatentW != 0.0f) {
            navigator.dispatchAction(NavigatorAction::AdjustLatentW, ev.deltaLatentW);
        }
    });

    eventBus.subscribeTo<EventLearnStage>([&navigator](const EventLearnStage& ev) {
        if (ev.maskBit == learn_mode::kResetStagesCommand) {
            navigator.resetLearnMode();
        } else if (ev.maskBit == learn_mode::kCycleDiagnosticCommand) {
            navigator.cycleDiagnosticStage();
        } else if ((ev.maskBit & learn_mode::kSoloStageCommandMask) != 0U) {
            const auto stageBits = ev.maskBit & ~learn_mode::kSoloStageCommandMask;
            for (std::uint8_t s = 0; s < 6; ++s) {
                if ((stageBits & (1U << s)) != 0U) {
                    navigator.setLearnStageSolo(static_cast<learn_mode::Stage>(s));
                    break;
                }
            }
        } else if ((ev.maskBit & learn_mode::kAccumulativeStageCommandMask) != 0U) {
            const auto maxStageIdx = ev.maskBit & ~learn_mode::kAccumulativeStageCommandMask;
            if (maxStageIdx < 6U) {
                navigator.setLearnStageAccumulative(static_cast<learn_mode::Stage>(maxStageIdx));
            }
        } else if (ev.maskBit == 0U) {
            navigator.cycleLearnStage();
        } else if ((ev.maskBit & learn_mode::kAllStages) != 0U) {
            navigator.coordinator_.setStageMask(ev.maskBit);
        } else {
            navigator.cycleLearnStage();
        }
    });

    eventBus.subscribeTo<EventPerformancePacingControl>([&navigator](const EventPerformancePacingControl& ev) {
        switch (ev.action) {
        case EventPerformancePacingControl::Action::ToggleMonitor:
            navigator.togglePerformanceMonitor();
            break;
        case EventPerformancePacingControl::Action::CycleTargetTier:
            navigator.cycleTargetFps();
            break;
        case EventPerformancePacingControl::Action::SetTargetTier:
            navigator.setTargetFpsTier(static_cast<TargetFrameRateTier>(ev.targetTier));
            break;
        case EventPerformancePacingControl::Action::ResetStats:
            if (navigator.performanceMonitor_ != nullptr) {
                navigator.performanceMonitor_->reset();
            }
            break;
        case EventPerformancePacingControl::Action::SetResolutionScale:
            navigator.setVariableResolutionScale(static_cast<StageResolutionScale>(ev.resolutionScale));
            break;
        case EventPerformancePacingControl::Action::ToggleAutoResolution:
            navigator.setAutoResolutionScalingEnabled(!navigator.isAutoResolutionScalingEnabled());
            break;
        }
    });

    eventBus.subscribeTo<EventAtmosphereControl>([&navigator](const EventAtmosphereControl& ev) {
        if (navigator.atmosphereManager_ == nullptr) return;
        switch (ev.action) {
        case EventAtmosphereControl::Action::Toggle:
            navigator.dispatchAction(NavigatorAction::ToggleAtmosphere);
            break;
        case EventAtmosphereControl::Action::Enable:
            navigator.atmosphereManager_->setEnabled(true);
            break;
        case EventAtmosphereControl::Action::Disable:
            navigator.atmosphereManager_->setEnabled(false);
            break;
        }
    });

    eventBus.subscribeTo<EventAudioFeedControl>([&navigator](const EventAudioFeedControl& ev) {
        if (navigator.audioDirector_ == nullptr) return;
        switch (ev.action) {
        case EventAudioFeedControl::Action::CycleMode:
            navigator.audioDirector_->cycleMode();
            break;
        case EventAudioFeedControl::Action::SetMode:
            navigator.audioDirector_->setMode(static_cast<AudioFeedMode>(ev.mode));
            break;
        case EventAudioFeedControl::Action::AdjustBpm:
            navigator.audioDirector_->adjustBpm(ev.value);
            break;
        case EventAudioFeedControl::Action::SetBpm:
            navigator.audioDirector_->setBpm(ev.value);
            break;
        case EventAudioFeedControl::Action::ToggleEnabled:
            navigator.audioDirector_->toggleEnabled();
            break;
        case EventAudioFeedControl::Action::SetGain:
            navigator.audioDirector_->setMasterGain(ev.value);
            break;
        }
    });

    eventBus.subscribeTo<EventPerformanceDeckControl>([&navigator](const EventPerformanceDeckControl& ev) {
        if (navigator.audioDirector_ != nullptr) {
            if (ev.action == EventPerformanceDeckControl::Action::SetBpm) {
                navigator.audioDirector_->setBpm(ev.value);
            } else if (ev.action == EventPerformanceDeckControl::Action::NudgeBpm) {
                navigator.audioDirector_->adjustBpm(ev.value);
            }
        }
        switch (ev.action) {
        case EventPerformanceDeckControl::Action::SetCrossfadePosition:
            navigator.setCrossfadePosition(ev.value);
            break;
        case EventPerformanceDeckControl::Action::NudgeCrossfade:
            navigator.nudgeCrossfade(ev.value);
            break;
        case EventPerformanceDeckControl::Action::TriggerAutoCrossfade:
            navigator.triggerAutoCrossfade(ev.value > 0.0f ? ev.value : 3.0f,
                                           ev.targetDeck == 0 ? std::make_optional(DeckId::DeckA)
                                                              : (ev.targetDeck == 1 ? std::make_optional(DeckId::DeckB) : std::nullopt));
            break;
        case EventPerformanceDeckControl::Action::SwapDecks:
            navigator.swapPerformanceDecks();
            break;
        default:
            break;
        }
    });

    eventBus.subscribeTo<EventThematicTourControl>([&navigator](const EventThematicTourControl& ev) {
        switch (ev.action) {
        case EventThematicTourControl::Action::SelectTour:
            if (!ev.identifier.empty()) {
                navigator.selectCuratedTour(ev.identifier);
            }
            break;
        case EventThematicTourControl::Action::CycleTour:
            navigator.cycleCuratedTour();
            break;
        case EventThematicTourControl::Action::SelectCategory:
            if (!ev.identifier.empty()) {
                navigator.selectCategoryTour(ev.identifier, ev.transitionSeconds, ev.dwellSeconds);
            }
            break;
        case EventThematicTourControl::Action::CycleCategory:
            navigator.cycleCategoryTour();
            break;
        case EventThematicTourControl::Action::ToggleAutopilot:
            navigator.toggleTour();
            break;
        }
    });

    eventBus.subscribeTo<EventVolumetricControl>([&navigator, &eventBus](const EventVolumetricControl& ev) {
        if (navigator.coordinator_.isBoundToEventBus(&eventBus)) {
            // Coordinator handles volumetric memory mutation and telemetry broadcast.
            // Synchronize audio director if acoustic coupling was toggled.
            if (ev.action == EventVolumetricControl::Action::ToggleAcousticCoupling && navigator.audioDirector_ != nullptr) {
                const auto* volMem = navigator.coordinator_.volumetricMemory();
                if (volMem != nullptr) {
                    navigator.audioDirector_->setVolumetricCouplingEnabled(volMem->config().enableAcousticCoupling);
                }
            }
            return;
        }

        switch (ev.action) {
        case EventVolumetricControl::Action::ToggleFeedback:
            navigator.dispatchAction(NavigatorAction::ToggleVolumetricFeedback, 0.0f);
            break;
        case EventVolumetricControl::Action::CycleResolution:
            navigator.dispatchAction(NavigatorAction::CycleVolumetricResolution, 0.0f);
            break;
        case EventVolumetricControl::Action::ResetMemory:
            navigator.dispatchAction(NavigatorAction::ResetVolumetricMemory, 0.0f);
            break;
        case EventVolumetricControl::Action::ToggleTorusWrap:
            navigator.dispatchAction(NavigatorAction::ToggleVolumetricWrap, 0.0f);
            break;
        case EventVolumetricControl::Action::SetResolution:
        case EventVolumetricControl::Action::ToggleWcaSeed:
        case EventVolumetricControl::Action::SetDecayFactor: {
            auto* volMem = navigator.coordinator_.volumetricMemory();
            if (volMem != nullptr) {
                applyVolumetricControl(*volMem, ev);
            }
            break;
        }
        case EventVolumetricControl::Action::ToggleAcousticCoupling:
            navigator.dispatchAction(NavigatorAction::ToggleAcousticSynesthesia, 0.0f);
            break;
        case EventVolumetricControl::Action::ToggleBackend:
            navigator.dispatchAction(NavigatorAction::ToggleVolumetricBackend, 0.0f);
            break;
        }

        const auto* volMem = navigator.coordinator_.volumetricMemory();
        if (volMem != nullptr) {
            eventBus.post(makeVolumetricTelemetryEvent(*volMem));
        }
    });

    eventBus.subscribeTo<EventHistoryControl>([&navigator](const EventHistoryControl& ev) {
        switch (ev.action) {
        case EventHistoryControl::Action::Undo:
            navigator.dispatchAction(NavigatorAction::Undo, 0.0f);
            break;
        case EventHistoryControl::Action::Redo:
            navigator.dispatchAction(NavigatorAction::Redo, 0.0f);
            break;
        case EventHistoryControl::Action::Clear:
            navigator.clearHistory();
            break;
        }
    });

    eventBus.subscribeTo<EventMorphSpaceControl>([&navigator](const EventMorphSpaceControl& ev) {
        if (!navigator.morphSpace_) return;
        switch (ev.action) {
        case EventMorphSpaceControl::Action::ToggleActive:
            navigator.toggleMorphSpace();
            break;
        case EventMorphSpaceControl::Action::SetCursor:
            if (ev.cursorX >= 0.0f && ev.cursorY >= 0.0f) {
                navigator.setMorphSpaceCursor(ev.cursorX, ev.cursorY);
            } else if (ev.cursorX >= 0.0f && navigator.morphSpace_ != nullptr) {
                navigator.setMorphSpaceCursor(ev.cursorX, navigator.morphSpace_->cursorY());
            } else if (ev.cursorY >= 0.0f && navigator.morphSpace_ != nullptr) {
                navigator.setMorphSpaceCursor(navigator.morphSpace_->cursorX(), ev.cursorY);
            }
            break;
        case EventMorphSpaceControl::Action::NudgeCursor:
            navigator.nudgeMorphSpaceCursor(ev.cursorX, ev.cursorY);
            break;
        case EventMorphSpaceControl::Action::SetMotionMode:
            navigator.morphSpace_->setMotionMode(static_cast<MorphSpaceMotion>(ev.mode));
            break;
        case EventMorphSpaceControl::Action::CycleMotionMode:
            navigator.cycleMorphSpaceMotion();
            break;
        case EventMorphSpaceControl::Action::SetMotionSpeed:
            navigator.morphSpace_->setMotionSpeed(ev.speed);
            break;
        case EventMorphSpaceControl::Action::SetShepardPower:
            navigator.morphSpace_->setShepardPower(ev.shepardPower);
            break;
        case EventMorphSpaceControl::Action::LoadBank:
            if (!ev.bankName.empty()) {
                navigator.loadMorphSpaceBank(ev.bankName);
            } else {
                navigator.loadMorphSpaceBank(ev.bankIndex);
            }
            break;
        case EventMorphSpaceControl::Action::CycleBank:
            navigator.cycleMorphSpaceBank();
            break;
        case EventMorphSpaceControl::Action::TogglePath:
            navigator.toggleMorphSpacePath();
            break;
        case EventMorphSpaceControl::Action::ClearPath:
            navigator.morphSpace_->clearPath();
            break;
        case EventMorphSpaceControl::Action::AddWaypoint:
            navigator.morphSpace_->addWaypoint(ev.cursorX, ev.cursorY, ev.dwellSec, ev.travelSec);
            break;
        case EventMorphSpaceControl::Action::SyncCategory:
            navigator.syncMorphSpaceToActiveCategory();
            break;
        case EventMorphSpaceControl::Action::BindCategory:
            if (!ev.category.empty()) {
                navigator.bindMorphSpaceToCategory(ev.category, /*autoLoad=*/true);
            }
            break;
        }
    });
}

} // namespace epi_sculpt
