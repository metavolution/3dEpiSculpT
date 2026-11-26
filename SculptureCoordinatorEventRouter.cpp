/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - SCULPTURE COORDINATOR EVENT ROUTER IMPLEMENTATION
 * ============================================================================
 */

#include "SculptureCoordinatorEventRouter.h"
#include "SculpturePipelineCoordinator.h"
#include "SculptureSerializer.h"
#include "SculptureVolumetricControl.h"

#include <algorithm>
#include <string>
#include <vector>

namespace epi_sculpt {

void SculptureCoordinatorEventRouter::bindAll(
    SculpturePipelineCoordinator& coordinator,
    SculptureEventBus& eventBus
) {
    bindPlaybackAndTourEvents(coordinator, eventBus);
    bindPerformanceDeckEvents(coordinator, eventBus);
    bindFileAndSequenceEvents(coordinator, eventBus);
    bindModifierEvents(coordinator, eventBus);
}

void SculptureCoordinatorEventRouter::bindPlaybackAndTourEvents(
    SculpturePipelineCoordinator& coordinator,
    SculptureEventBus& eventBus
) {
    eventBus.subscribeTo<EventPlaybackRate>([&coordinator](const EventPlaybackRate& ev) {
        switch (ev.action) {
        case EventPlaybackRate::Action::Increase: coordinator.increaseSpeed(1.2f); break;
        case EventPlaybackRate::Action::Decrease: coordinator.decreaseSpeed(1.2f); break;
        case EventPlaybackRate::Action::Reset: coordinator.resetSpeed(); break;
        }
    });

    eventBus.subscribeTo<EventTemporalReset>([&coordinator](const EventTemporalReset&) {
        if (coordinator.volumetricMemory_) {
            coordinator.volumetricMemory_->reset();
        }
    });

    eventBus.subscribeTo<EventLearnStage>([&coordinator](const EventLearnStage& ev) {
        if (ev.maskBit == learn_mode::kCycleDiagnosticCommand) {
            coordinator.cycleDiagnosticStage();
        } else if (ev.maskBit == learn_mode::kResetStagesCommand) {
            coordinator.resetLearnModeStages();
        } else if ((ev.maskBit & learn_mode::kSoloStageCommandMask) != 0U) {
            const auto bitVal = ev.maskBit & ~learn_mode::kSoloStageCommandMask;
            for (std::uint32_t s = 0; s < 6; ++s) {
                if (bitVal == (1U << s)) {
                    coordinator.setLearnStageSolo(static_cast<learn_mode::Stage>(s));
                    break;
                }
            }
        } else if ((ev.maskBit & learn_mode::kAccumulativeStageCommandMask) != 0U) {
            const auto maxIdx = ev.maskBit & ~learn_mode::kAccumulativeStageCommandMask;
            coordinator.setLearnStageAccumulative(static_cast<learn_mode::Stage>(std::min(maxIdx, 5U)));
        } else if ((ev.maskBit & learn_mode::kAllStages) != 0 &&
                   (ev.maskBit & ~learn_mode::kAllStages) == 0 &&
                   (ev.maskBit & (ev.maskBit - 1)) == 0) {
            for (std::uint32_t s = 0; s < 6; ++s) {
                if (ev.maskBit == (1U << s)) {
                    coordinator.toggleStage(static_cast<learn_mode::Stage>(s));
                    break;
                }
            }
        } else {
            coordinator.setStageMask(ev.maskBit);
        }
    });

    eventBus.subscribeTo<EventSteering>([&coordinator](const EventSteering& ev) {
        auto params = coordinator.choreographer_.currentParams();
        params.latentW = std::clamp(params.latentW + ev.deltaLatentW, 0.0f, 1.0f);
        coordinator.setTargetParams(params, 0.2f, MorphEasing::Linear);
    });

    eventBus.subscribeTo<EventFoliationManifold>([&coordinator](const EventFoliationManifold& ev) {
        coordinator.set4DManifoldType(static_cast<Manifold4DType>(ev.manifoldType));
    });

    eventBus.subscribeTo<EventResolutionScaleCycle>([&coordinator](const EventResolutionScaleCycle&) {
        coordinator.cycleResolutionScale();
    });

    eventBus.subscribeTo<EventUpsampleFilterCycle>([&coordinator](const EventUpsampleFilterCycle&) {
        coordinator.cycleUpsampleFilter();
    });

    eventBus.subscribeTo<EventMorphEasingCycle>([&coordinator](const EventMorphEasingCycle&) {
        coordinator.cycleMorphEasing();
    });

    eventBus.subscribeTo<EventTourCycle>([&coordinator](const EventTourCycle& ev) {
        coordinator.cycleCuratedTour(ev.direction);
    });

    eventBus.subscribeTo<EventRotorNudge>([&coordinator](const EventRotorNudge& ev) {
        auto params = coordinator.choreographer_.currentParams();
        params.rotorAlpha += ev.deltaAlpha;
        params.rotorBeta += ev.deltaBeta;
        params.rotorTheta4D += ev.deltaTheta4D;
        coordinator.setTargetParams(params, 0.1f, MorphEasing::Linear);
    });

    eventBus.subscribeTo<EventKnotBraid>([&coordinator](const EventKnotBraid& ev) {
        auto params = coordinator.choreographer_.currentParams();
        params.rotorAlpha = std::clamp(params.rotorAlpha + ev.deltaP * 0.1f, -10.0f, 10.0f);
        params.rotorBeta = std::clamp(params.rotorBeta + ev.deltaQ * 0.1f, -10.0f, 10.0f);
        coordinator.setTargetParams(params, 0.3f, MorphEasing::Smoothstep);
    });
}

void SculptureCoordinatorEventRouter::bindPerformanceDeckEvents(
    SculpturePipelineCoordinator& coordinator,
    SculptureEventBus& eventBus
) {
    eventBus.subscribeTo<EventMultiTrackControl>([&coordinator](const EventMultiTrackControl& ev) {
        const auto layer = static_cast<TrackLayer>(ev.layerIndex);
        switch (ev.action) {
        case EventMultiTrackControl::Action::TogglePlay:
            if (coordinator.multiTrack_.isPlaying()) coordinator.multiTrack_.pause();
            else coordinator.multiTrack_.play();
            break;
        case EventMultiTrackControl::Action::ToggleMute:
            coordinator.multiTrack_.setTrackMuted(layer, !coordinator.multiTrack_.isTrackMuted(layer));
            break;
        case EventMultiTrackControl::Action::ToggleSolo:
            coordinator.multiTrack_.setTrackSolo(layer, !coordinator.multiTrack_.isTrackSolo(layer));
            break;
        case EventMultiTrackControl::Action::ResetAll:
            coordinator.multiTrack_.clearSolos();
            for (std::size_t i = 0; i < 4; ++i) {
                coordinator.multiTrack_.setTrackMuted(static_cast<TrackLayer>(i), false);
            }
            break;
        case EventMultiTrackControl::Action::SetLayerSpeed:
            coordinator.multiTrack_.setTrackSpeed(layer, ev.speedMultiplier);
            break;
        }
    });
}

void SculptureCoordinatorEventRouter::bindFileAndSequenceEvents(
    SculpturePipelineCoordinator& coordinator,
    SculptureEventBus& eventBus
) {
    eventBus.subscribeTo<EventWorldCatalogSelect>([&coordinator](const EventWorldCatalogSelect& ev) {
        if (!coordinator.library_ || ev.catalogIndex >= coordinator.library_->entries().size()) return;
        const auto& entry = coordinator.library_->entries()[ev.catalogIndex];
        coordinator.activeLoadedPresetName_ = entry.id;
        coordinator.setTargetParams(entry.params, ev.durationSeconds, MorphEasing::Smoothstep);
        coordinator.loadPresetModulationRoutes(entry.routingMap);
    });

    eventBus.subscribeTo<EventFileAction>([&coordinator](const EventFileAction& ev) {
        if (ev.action == EventFileAction::Action::ExportStandaloneHtml) {
            ShaderSynthesisConfig cfg{};
            cfg.title = std::string(coordinator.activeTourPresetName());
            coordinator.exportStandaloneHtmlToFile(ev.path.empty() ? "sculpture_export.html" : ev.path, cfg);
        } else if (ev.action == EventFileAction::Action::ExportGeometry) {
            EpimorphicReliefConfig reliefCfg{};
            reliefCfg.closeSolidMesh = true;
            reliefCfg.colorizeCurvature = true;
            reliefCfg.projection = EpimorphicReliefGenerator::resolveReliefProjection(coordinator.currentParams());
            const std::string basePath = ev.path.empty() ? "sculpture_export" : ev.path;
            coordinator.exportReliefMeshObj(basePath + ".obj", reliefCfg, coordinator.activeTourPresetName());
            coordinator.exportReliefMeshStl(basePath + ".stl", reliefCfg, true, coordinator.activeTourPresetName());
        } else if (ev.action == EventFileAction::Action::ExportReliefMesh) {
            EpimorphicReliefConfig reliefCfg{};
            reliefCfg.closeSolidMesh = true;
            reliefCfg.colorizeCurvature = true;
            reliefCfg.solidBaseDepth = 0.25f;
            reliefCfg.computeAnalyticalNormals = true;
            reliefCfg.projection = EpimorphicReliefGenerator::resolveReliefProjection(coordinator.currentParams());
            const std::string basePath = ev.path.empty() ? "epimorphic_relief_3dprint" : ev.path;
            coordinator.exportReliefMeshObj(basePath + ".obj", reliefCfg, coordinator.activeTourPresetName());
            coordinator.exportReliefMeshStl(basePath + ".stl", reliefCfg, true, coordinator.activeTourPresetName());
        } else if (ev.action == EventFileAction::Action::SaveState) {
            SculptureWorldMode wm{};
            wm.presetName = std::string(coordinator.activeTourPresetName());
            wm.params = coordinator.choreographer_.currentParams();
            if (auto* m = coordinator.macroEngine()) {
                wm.macroAutomationJson = m->serializeJson();
            }
            SculptureSerializer::saveEpimodeToFile(ev.path.empty() ? "user_state.epimode" : ev.path, wm);
        } else if (ev.action == EventFileAction::Action::LoadState) {
            SculptureWorldState state{};
            std::string presetName;
            SculptureProgramParams params{};
            std::vector<std::string> routes{};
            std::string macroJson;
            if (SculptureSerializer::loadEpimodeFromFile(ev.path, state, presetName, params, routes, nullptr, &macroJson)) {
                coordinator.activeLoadedPresetName_ = presetName;
                coordinator.setTargetParams(params, 0.5f, MorphEasing::Smoothstep);
                if (!macroJson.empty()) {
                    if (auto* m = coordinator.macroEngine()) {
                        m->deserializeJson(macroJson);
                    }
                }
            }
        }
    });

    eventBus.subscribeTo<EventTimelineControl>([&coordinator](const EventTimelineControl& ev) {
        switch (ev.action) {
        case EventTimelineControl::Action::Play:
            coordinator.playTimeline();
            break;
        case EventTimelineControl::Action::Pause:
            coordinator.pauseTimeline();
            break;
        case EventTimelineControl::Action::Stop:
            coordinator.stopTimeline();
            break;
        case EventTimelineControl::Action::TogglePlayPause:
            if (coordinator.timelineSequencer_ && coordinator.timelineSequencer_->isPlaying()) {
                coordinator.pauseTimeline();
            } else {
                coordinator.playTimeline();
            }
            break;
        case EventTimelineControl::Action::NextCue:
            if (coordinator.timelineSequencer_ && coordinator.performanceDeck_) {
                coordinator.timelineSequencer_->nextCue(*coordinator.performanceDeck_, coordinator.library_);
            }
            break;
        case EventTimelineControl::Action::PreviousCue:
            if (coordinator.timelineSequencer_ && coordinator.performanceDeck_) {
                coordinator.timelineSequencer_->previousCue(*coordinator.performanceDeck_, coordinator.library_);
            }
            break;
        case EventTimelineControl::Action::SkipToCue:
            if (coordinator.timelineSequencer_ && coordinator.performanceDeck_) {
                coordinator.timelineSequencer_->skipToCue(ev.cueIndex, *coordinator.performanceDeck_, coordinator.library_);
            }
            break;
        case EventTimelineControl::Action::LoadFile:
            coordinator.loadTimelineFromFile(ev.argument);
            break;
        case EventTimelineControl::Action::BuildTour:
            coordinator.buildTimelineFromTour(ev.argument, ev.dwellSec, ev.transSec);
            break;
        case EventTimelineControl::Action::BuildBeatSynced:
        case EventTimelineControl::Action::Export:
            break;
        }
    });

    eventBus.subscribeTo<EventSequenceRecorderControl>([&coordinator](const EventSequenceRecorderControl& ev) {
        switch (ev.action) {
        case EventSequenceRecorderControl::Action::Start:
            coordinator.startSequenceRecording(ev.sessionName);
            break;
        case EventSequenceRecorderControl::Action::Stop:
            coordinator.stopSequenceRecording();
            break;
        case EventSequenceRecorderControl::Action::Toggle:
            if (coordinator.isSequenceRecording()) {
                coordinator.stopSequenceRecording();
            } else {
                coordinator.startSequenceRecording(ev.sessionName);
            }
            break;
        }
    });
}

void SculptureCoordinatorEventRouter::bindModifierEvents(
    SculpturePipelineCoordinator& coordinator,
    SculptureEventBus& eventBus
) {
    eventBus.subscribeTo<EventToggleState>([&coordinator](const EventToggleState& ev) {
        switch (ev.target) {
        case EventToggleState::Target::Pause:
            coordinator.togglePause();
            break;
        case EventToggleState::Target::Cinematic:
            if (coordinator.autoDirector_) {
                coordinator.enableAutoDirector(false);
            } else {
                coordinator.enableAutoDirector(true);
            }
            break;
        case EventToggleState::Target::Topology4D:
            coordinator.toggle4DFoliationWave();
            break;
        case EventToggleState::Target::Temporal:
            if (coordinator.volumetricMemory_) {
                auto cfg = coordinator.volumetricMemory_->config();
                cfg.enableHyper4DWave = !cfg.enableHyper4DWave;
                coordinator.volumetricMemory_->configure(cfg);
            }
            break;
        case EventToggleState::Target::VoxelMode:
            if (coordinator.volumetricMemory_) {
                auto cfg = coordinator.volumetricMemory_->config();
                cfg.enableFeedback = !cfg.enableFeedback;
                coordinator.volumetricMemory_->configure(cfg);
            }
            break;
        default:
            break;
        }
    });

    eventBus.subscribeTo<EventMutation>([&coordinator](const EventMutation& ev) {
        switch (ev.action) {
        case EventMutation::Action::StepMutation:
            coordinator.mutateParameters(ev.strength > 0.0f ? ev.strength : 0.2f);
            break;
        case EventMutation::Action::ToggleTour:
            if (coordinator.isTourActive()) coordinator.stopTour();
            else coordinator.startCuratedTour(
                coordinator.config_.defaultTourName,
                coordinator.config_.defaultTransitionSeconds,
                coordinator.config_.defaultDwellSeconds);
            break;
        case EventMutation::Action::StepTourNext:
            coordinator.stepTourNext(true);
            break;
        case EventMutation::Action::StepTourPrev:
            coordinator.stepTourPrevious(true);
            break;
        case EventMutation::Action::ToggleTourPause:
            coordinator.toggleTourPause();
            break;
        case EventMutation::Action::ToggleTourPlaybackMode:
            coordinator.setTourPlaybackMode(coordinator.tourPlaybackMode() == TourPlaybackMode::StepAndDwell
                ? TourPlaybackMode::ContinuousSpline
                : TourPlaybackMode::StepAndDwell);
            break;
        case EventMutation::Action::Toggle4D:
            coordinator.toggle4DFoliationWave();
            break;
        default:
            break;
        }
    });

    eventBus.subscribeTo<EventLodGovernorToggle>([&coordinator](const EventLodGovernorToggle&) {
        coordinator.enableStagedGovernor_ = !coordinator.enableStagedGovernor_;
    });

    eventBus.subscribeTo<EventPaletteAgentControl>([&coordinator](const EventPaletteAgentControl& ev) {
        switch (ev.action) {
        case EventPaletteAgentControl::Action::Toggle:
            coordinator.togglePaletteAgent();
            break;
        case EventPaletteAgentControl::Action::SetMode:
            coordinator.paletteAgent_.setMode(static_cast<PaletteAgentMode>(ev.mode));
            break;
        case EventPaletteAgentControl::Action::Stimulate:
            coordinator.paletteAgent_.stimulate(ev.intensity);
            break;
        case EventPaletteAgentControl::Action::Reseed:
            coordinator.paletteAgent_.reseed(ev.seed);
            break;
        }
    });

    eventBus.subscribeTo<EventVolumetricControl>([&coordinator, &eventBus](const EventVolumetricControl& ev) {
        if (!coordinator.volumetricMemory_) return;
        auto& volume = *coordinator.volumetricMemory_;
        applyVolumetricControl(volume, ev);
        eventBus.post(makeVolumetricTelemetryEvent(volume));
    });
}

} // namespace epi_sculpt
