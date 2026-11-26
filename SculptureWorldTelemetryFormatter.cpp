#include "SculptureWorldTelemetryFormatter.h"
#include "SculptureWorldNavigator.h"
#include "SculptureAudioFeedDirector.h"
#include "SculptureFluidAtmosphereManager.h"
#include "SculptureTimelineDirector.h"
#include "SculptureWorldExternalTransport.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace epi_sculpt {

void SculptureWorldTelemetryFormatter::formatHud(
    const SculptureWorldNavigator& nav,
    char* buffer,
    std::size_t size,
    std::uint32_t generation,
    std::size_t instanceCount) noexcept {
    if (!buffer || size == 0) return;
    const auto& entry = nav.activeEntry();

    const float speed = nav.speedMultiplier();
    const char* const pauseTag = nav.isPaused() ? " [PAUSED]" : "";
    char mutTag[128] = "";
    const auto drift = nav.diffFromBaseline();
    if (!drift.empty()) {
        if (!nav.undoStack_.empty()) {
            std::snprintf(
                mutTag,
                sizeof(mutTag),
                " [MUTATED] [DRIFT: %zu] [UNDO: %zu/%zu]",
                drift.size(),
                nav.undoStack_.size(),
                nav.redoStack_.size());
        } else {
            std::snprintf(mutTag, sizeof(mutTag), " [MUTATED] [DRIFT: %zu]", drift.size());
        }
    } else if (nav.areParametersMutated()) {
        if (!nav.undoStack_.empty()) {
            std::snprintf(
                mutTag,
                sizeof(mutTag),
                " [MUTATED] [UNDO: %zu/%zu]",
                nav.undoStack_.size(),
                nav.redoStack_.size());
        } else {
            std::snprintf(mutTag, sizeof(mutTag), " [MUTATED]");
        }
    } else if (!nav.undoStack_.empty() || !nav.redoStack_.empty()) {
        std::snprintf(
            mutTag,
            sizeof(mutTag),
            " [UNDO: %zu/%zu]",
            nav.undoStack_.size(),
            nav.redoStack_.size());
    }

    char modeTag[256] = "";
    if (nav.presentationMode_ != WorldPresentationMode::PointCloud) {
        std::snprintf(
            modeTag,
            sizeof(modeTag),
            " | MANIFOLD: %s [%s, %zu tris: %s]",
            nav.presentationMode_ == WorldPresentationMode::ReliefManifold ? "Surface" : "Hybrid",
            name(nav.reliefConfig_.projection),
            nav.currentReliefData_.triangleCount(),
            metricDescription(nav.reliefConfig_.projection));
    }
    if (nav.stereoEnabled_) {
        char sbsBuf[64];
        std::snprintf(sbsBuf, sizeof(sbsBuf), " | [SBS STEREO: %.1fmm]", nav.ipdMeters_ * 1000.0f);
        std::strncat(modeTag, sbsBuf, sizeof(modeTag) - std::strlen(modeTag) - 1);
    }

    char govTag[96] = "";
    const auto activeResScale = nav.resolutionScale();
    const char* resStr = "1/1 Full";
    if (activeResScale == StageResolutionScale::Half) {
        resStr = "1/2 Bilinear (Saved 75%)";
    } else if (activeResScale == StageResolutionScale::Quarter) {
        resStr = "1/4 Bilinear (Saved 93%)";
    } else if (activeResScale == StageResolutionScale::Eighth) {
        resStr = "1/8 Bilinear (Saved 98%)";
    }

    if (nav.coordinator_.isStagedComputeGovernorEnabled()) {
        std::snprintf(
            govTag,
            sizeof(govTag),
            " | [RES: %s%s]",
            resStr,
            nav.isAutoResolutionScalingEnabled() ? " AUTO" : "");
    } else {
        std::snprintf(govTag, sizeof(govTag), " | [RES: %s]", resStr);
    }

    char foliationTag[256] = "";
    const auto& cp = nav.coordinator_.currentParams();
    if (nav.currentRecipe_.uses4DProjection || cp.enable4D > 0U || nav.currentParams_.enable4D > 0U) {
        if (nav.polychoronMorphProgress() > 0.001f) {
            std::snprintf(
                foliationTag,
                sizeof(foliationTag),
                " | 4D: %s [%s -> %s %.0f%%, W:%.2f a:%.2f b:%.2f]",
                nav.is4DFoliationWaveEnabled() ? "WAVE" : "STATIC",
                polychoronName(nav.polychoronSource()).data(),
                polychoronName(nav.polychoronTarget()).data(),
                nav.polychoronMorphProgress() * 100.0f,
                cp.latentW,
                cp.rotorAlpha,
                cp.rotorBeta);
        } else {
            std::snprintf(
                foliationTag,
                sizeof(foliationTag),
                " | 4D: %s [%s, W:%.2f a:%.2f b:%.2f t:%.2f]",
                nav.is4DFoliationWaveEnabled() ? "WAVE" : "STATIC",
                name(nav.active4DManifoldType()).data(),
                cp.latentW,
                cp.rotorAlpha,
                cp.rotorBeta,
                cp.rotorTheta4D);
        }
    }

    char paletteTag[96] = "";
    if (nav.isPaletteAgentEnabled()) {
        std::snprintf(
            paletteTag,
            sizeof(paletteTag),
            " | OKLab-Agent [%s]",
            nav.coordinator_.paletteAgent().formatHudStatus().c_str());
    }

    char learnTag[256] = "";
    if (nav.isLearnModeActive()) {
        const auto mask = nav.coordinator_.stageMask();
        const auto diag = nav.coordinator_.diagnosticStage();
        const auto activeMask = mask & learn_mode::kAllStages;
        std::string_view soloFormula = "";
        for (std::size_t s = 0; s < learn_mode::kStageCount; ++s) {
            auto stg = static_cast<learn_mode::Stage>(s);
            if (activeMask == learn_mode::bit(stg)) {
                soloFormula = learn_mode::stageMathematicalFormula(stg);
                break;
            }
        }
        if (!soloFormula.empty()) {
            std::snprintf(
                learnTag,
                sizeof(learnTag),
                " | [LEARN SOLO: 1:%c 2:%c 3:%c 4:%c 5:%c 6:%c | %.*s]",
                learn_mode::enabled(mask, learn_mode::Stage::Transform) ? '+' : '-',
                learn_mode::enabled(mask, learn_mode::Stage::Reduce) ? '+' : '-',
                learn_mode::enabled(mask, learn_mode::Stage::FeedbackSample) ? '+' : '-',
                learn_mode::enabled(mask, learn_mode::Stage::Color) ? '+' : '-',
                learn_mode::enabled(mask, learn_mode::Stage::Seed) ? '+' : '-',
                learn_mode::enabled(mask, learn_mode::Stage::Blend) ? '+' : '-',
                static_cast<int>(soloFormula.size() > 40 ? 40 : soloFormula.size()),
                soloFormula.data());
        } else {
            std::snprintf(
                learnTag,
                sizeof(learnTag),
                " | [LEARN: 1:%c 2:%c 3:%c 4:%c 5:%c 6:%c | Diag: %s]",
                learn_mode::enabled(mask, learn_mode::Stage::Transform) ? '+' : '-',
                learn_mode::enabled(mask, learn_mode::Stage::Reduce) ? '+' : '-',
                learn_mode::enabled(mask, learn_mode::Stage::FeedbackSample) ? '+' : '-',
                learn_mode::enabled(mask, learn_mode::Stage::Color) ? '+' : '-',
                learn_mode::enabled(mask, learn_mode::Stage::Seed) ? '+' : '-',
                learn_mode::enabled(mask, learn_mode::Stage::Blend) ? '+' : '-',
                learn_mode::diagnosticDescription(diag));
        }
    }

    char recTag[128] = "";
    if (nav.isSequenceRecording()) {
        try {
            const auto st = nav.sequenceRecordingStats();
            std::snprintf(
                recTag,
                sizeof(recTag),
                " | REC: [%u frames, %.1fs, q:%u]",
                st.recordedFrameCount,
                st.elapsedSeconds,
                st.queuedFrameCount);
        } catch (...) {
            // The HUD observes recording; it cannot change its lifecycle.
            std::snprintf(recTag, sizeof(recTag), " | REC: [stats unavailable]");
        }
    }

    char timelineTag[128] = "";
    if (nav.isTimelinePlaying()) {
        const auto st = nav.timelineDirector_->status();
        std::snprintf(
            timelineTag,
            sizeof(timelineTag),
            " | TIMELINE: \"%s\" (%.0f%%)",
            st.activeScriptTitle.c_str(),
            st.progress * 100.0f);
    }

    char perfTag[1024] = "";
    if (nav.performanceMonitor_ != nullptr && nav.performanceMonitor_->isEnabled()) {
        const auto& pt = nav.performanceMonitor_->telemetry();
        std::snprintf(
            perfTag,
            sizeof(perfTag),
            " | PERF: %.1f FPS (%.1fms, Headroom: %+0.1fms, 1%% low: %.1f) [%s]",
            pt.smoothedFps,
            pt.smoothedFrameTimeMs,
            pt.headroomMs,
            pt.low1PercentFps,
            pt.budgetStatus.c_str());
    }

    if (nav.currentRecipe_.generator == WorldGeneratorKind::NeonBubblesphere) {
        char topologyTag[96] = "";
        std::snprintf(
            topologyTag,
            sizeof(topologyTag),
            " | [MIRROR CELLS: %s]",
            mirrorCellTopologyName(nav.currentRecipe_.mirrorTopology).data());
        const std::size_t curLen = std::strlen(perfTag);
        if (curLen < sizeof(perfTag) - 1) {
            std::strncat(perfTag, topologyTag, sizeof(perfTag) - curLen - 1);
        }

        if (nav.externalTransport_ != nullptr) {
            std::uint32_t imported = 0U;
            std::uint32_t primed = 0U;
            std::uint32_t released = 0U;
            std::uint32_t quarantined = 0U;
            for (std::uint32_t slot = 0U;
                 slot < SculptureWorldExternalTransport::kSlotCount;
                 ++slot) {
                switch (nav.externalTransport_->slotStatus(slot).state) {
                case VulkanOpenClRingSlotState::Imported: ++imported; break;
                case VulkanOpenClRingSlotState::Primed: ++primed; break;
                case VulkanOpenClRingSlotState::Released: ++released; break;
                case VulkanOpenClRingSlotState::Quarantined: ++quarantined; break;
                case VulkanOpenClRingSlotState::Allocated: break;
                }
            }
            char ringTag[112] = "";
            std::snprintf(
                ringTag,
                sizeof(ringTag),
                " | [WORLD RING I:%u P:%u R:%u Q:%u]",
                imported,
                primed,
                released,
                quarantined);
            const std::size_t curLen2 = std::strlen(perfTag);
            if (curLen2 < sizeof(perfTag) - 1) {
                std::strncat(perfTag, ringTag, sizeof(perfTag) - curLen2 - 1);
            }
        }
    }

    if (nav.atmosphereManager_ != nullptr && nav.atmosphereManager_->isEnabled()) {
        char atmoTag[96] = "";
        std::snprintf(
            atmoTag,
            sizeof(atmoTag),
            " | ATMO: %s (%ux%u)",
            nav.atmosphereManager_->isHardwareAccelerated() ? "HW-Interop" : "Headless",
            nav.atmosphereManager_->mistAxis(),
            nav.atmosphereManager_->mistAxis());
        const std::size_t curLen = std::strlen(perfTag);
        if (curLen < sizeof(perfTag) - 1) {
            std::strncat(perfTag, atmoTag, sizeof(perfTag) - curLen - 1);
        }
    }

    if (nav.audioDirector_ != nullptr && nav.audioDirector_->isEnabled()) {
        char audioTag[160] = "";
        const auto status = nav.audioDirector_->formatHudStatus();
        std::snprintf(audioTag, sizeof(audioTag), " | %s", status.c_str());
        const std::size_t curLen = std::strlen(perfTag);
        if (curLen < sizeof(perfTag) - 1) {
            std::strncat(perfTag, audioTag, sizeof(perfTag) - curLen - 1);
        }
    }

    if (const auto* volMem = nav.coordinator_.volumetricMemory()) {
        char volTag[160] = "";
        const auto& vt = volMem->telemetry();
        const auto& ct = nav.volumetricCoupler_.telemetry();
        if (nav.volumetricCoupler_.config().enableCoupling && ct.active) {
            std::snprintf(
                volTag,
                sizeof(volTag),
                " | [VOL: %u^3 %s rho:%.2f occ:%.0f%% | COUPLED: adv:%c buoy:%c vor:%.2f culled:%zu]",
                vt.activeDimension,
                vt.gpuAccelerated ? "GPU" : "CPU",
                vt.meanDensity,
                vt.macroOccupancyRatio * 100.0f,
                nav.volumetricCoupler_.config().enableAdvection ? '+' : '-',
                nav.volumetricCoupler_.config().enableDensityBuoyancy ? '+' : '-',
                ct.meanVorticityMagnitude,
                ct.culledVacantInstances);
        } else {
            std::snprintf(
                volTag,
                sizeof(volTag),
                " | [VOL: %u^3 %s rho:%.2f occ:%.0f%%]",
                vt.activeDimension,
                vt.gpuAccelerated ? "GPU" : "CPU",
                vt.meanDensity,
                vt.macroOccupancyRatio * 100.0f);
        }
        const std::size_t curLen = std::strlen(perfTag);
        if (curLen < sizeof(perfTag) - 1) {
            std::strncat(perfTag, volTag, sizeof(perfTag) - curLen - 1);
        }
    }

    if (nav.isPerformanceDeckEnabled() && nav.performanceDeck() != nullptr) {
        char deckTag[160] = "";
        const auto* deck = nav.performanceDeck();
        const float pos = deck->crossfadePosition();
        char gauge[12] = "----------";
        const int marker = std::clamp(static_cast<int>(std::round(pos * 9.0f)), 0, 9);
        gauge[marker] = '|';
        std::snprintf(
            deckTag,
            sizeof(deckTag),
            " | [DECK: A:\"%s\" [%s] B:\"%s\" (%.2f)%s]",
            deck->deck(DeckId::DeckA).presetTitle.c_str(),
            gauge,
            deck->deck(DeckId::DeckB).presetTitle.c_str(),
            pos,
            deck->isAutoCrossfading() ? " AUTO" : "");
        const std::size_t curLen = std::strlen(perfTag);
        if (curLen < sizeof(perfTag) - 1) {
            std::strncat(perfTag, deckTag, sizeof(perfTag) - curLen - 1);
        }
    }

    if (nav.isMorphSpaceActive() && nav.morphSpace_ != nullptr) {
        char morphTag[160] = "";
        std::snprintf(
            morphTag,
            sizeof(morphTag),
            " | [MORPH: %s]",
            nav.morphSpace_->formatHudStatus().c_str());
        const std::size_t curLen = std::strlen(perfTag);
        if (curLen < sizeof(perfTag) - 1) {
            std::strncat(perfTag, morphTag, sizeof(perfTag) - curLen - 1);
        }
    }

    char pipelineTag[256] = "";
    std::snprintf(
        pipelineTag,
        sizeof(pipelineTag),
        " | [Pipeline] Transform: %s | Reduce: %s | Color: %s | Seed: %s",
        name(cp.inner).data(),
        name(cp.reduction).data(),
        name(cp.colorEffect).data(),
        name(cp.seedType).data());
    const std::size_t curLen2 = std::strlen(perfTag);
    if (curLen2 < sizeof(perfTag) - 1) {
        std::strncat(perfTag, pipelineTag, sizeof(perfTag) - curLen2 - 1);
    }

    if (nav.tourChoreographer_ != nullptr && nav.tourChoreographer_->isEnabled()) {
        const auto tourStatus = nav.tourChoreographer_->status();
        if (tourStatus.inTransition) {
            std::snprintf(
                buffer,
                size,
                "EPI C WORLD CATALOG [%zu/%zu] | %s | %s | GEN: %u | SPEED: %.2fx%s%s%s%s%s%s%s%s%s%s | AUTOPILOT MORPHING (%u%%) -> %s | [O] TOUR [Y] PLAYLIST [V] MODE [B] PROJ [4] 4D [X] MANI [Z] AGENT [L] LEARN [KP +/-] SPEED [KP *] PAUSE [F11] FPS",
                nav.activeIndex_ + 1U,
                nav.availableWorlds_.size(),
                entry.title.data(),
                worldGeneratorName(nav.currentRecipe_.generator).data(),
                generation,
                speed,
                pauseTag,
                mutTag,
                modeTag,
                govTag,
                foliationTag,
                paletteTag,
                learnTag,
                recTag,
                timelineTag,
                perfTag,
                static_cast<unsigned>(tourStatus.transitionProgress * 100.0f),
                tourStatus.nextPresetId.c_str());
        } else if (tourStatus.userIntervention) {
            std::snprintf(
                buffer,
                size,
                "EPI C WORLD CATALOG [%zu/%zu] | %s | %s | GEN: %u | SPEED: %.2fx%s%s%s%s%s%s%s%s%s%s | AUTOPILOT (USER INTERVENTION PAUSE) | [O] TOUR [Y] PLAYLIST [V] MODE [B] PROJ [4] 4D [X] MANI [Z] AGENT [L] LEARN [KP +/-] SPEED [KP *] PAUSE [F11] FPS",
                nav.activeIndex_ + 1U,
                nav.availableWorlds_.size(),
                entry.title.data(),
                worldGeneratorName(nav.currentRecipe_.generator).data(),
                generation,
                speed,
                pauseTag,
                mutTag,
                modeTag,
                govTag,
                foliationTag,
                paletteTag,
                learnTag,
                recTag,
                timelineTag,
                perfTag);
        } else {
            std::snprintf(
                buffer,
                size,
                "EPI C WORLD CATALOG [%zu/%zu] | %s | %s | GEN: %u | SPEED: %.2fx%s%s%s%s%s%s%s%s%s%s | AUTOPILOT [%s (%zu/%zu) Dwell: %.1fs] | [O] TOUR [Y] PLAYLIST [V] MODE [B] PROJ [4] 4D [X] MANI [Z] AGENT [L] LEARN [KP +/-] SPEED [KP *] PAUSE [F11] FPS",
                nav.activeIndex_ + 1U,
                nav.availableWorlds_.size(),
                entry.title.data(),
                worldGeneratorName(nav.currentRecipe_.generator).data(),
                generation,
                speed,
                pauseTag,
                mutTag,
                modeTag,
                govTag,
                foliationTag,
                paletteTag,
                learnTag,
                recTag,
                timelineTag,
                perfTag,
                tourStatus.tourName.c_str(),
                tourStatus.waypointIndex + 1U,
                tourStatus.totalWaypoints,
                tourStatus.timeRemainingInDwell);
        }
    } else {
        if (nav.transitionState_.active) {
            std::snprintf(
                buffer,
                size,
                "EPI C WORLD CATALOG [%zu/%zu] | %s | %s (ID %u) | GEN: %u | %zu INSTANCES | SPEED: %.2fx%s%s%s%s%s%s%s%s%s%s | TRANSITION (%u%%) -> %s | [N/Shift+N] WORLD [P] SBS [V] MODE [B] PROJ [G] GOV [4] 4D [X] MANI [Z] AGENT [L] LEARN [KP +/-] SPEED [KP *] PAUSE [I/K] MAP/FOLD [SHIFT+K] CELLS [M] MUTATE [C] PALETTE [BKSP] RESET [SPACE] SNAP [F8] LOAD [F9] REC [ [ / ] ] W-SLICE [F11] FPS",
                nav.activeIndex_ + 1U,
                nav.availableWorlds_.size(),
                entry.title.data(),
                worldGeneratorName(nav.currentRecipe_.generator).data(),
                static_cast<unsigned>(nav.currentRecipe_.generator),
                generation,
                instanceCount,
                speed,
                pauseTag,
                mutTag,
                modeTag,
                govTag,
                foliationTag,
                paletteTag,
                learnTag,
                recTag,
                timelineTag,
                perfTag,
                static_cast<unsigned>(nav.transitionState_.progress * 100.0f),
                nav.transitionState_.toPresetName.c_str());
        } else {
            std::snprintf(
                buffer,
                size,
                "EPI C WORLD CATALOG [%zu/%zu] | %s | %s (ID %u) | GEN: %u | %zu INSTANCES | SPEED: %.2fx%s%s%s%s%s%s%s%s%s%s | [N/Shift+N] WORLD [P] SBS [V] MODE [B] PROJ [G] GOV [4] 4D [X] MANI [Z] AGENT [L] LEARN [KP +/-] SPEED [KP *] PAUSE [I/K] MAP/FOLD [SHIFT+K] CELLS [M] MUTATE [C] PALETTE [BKSP] RESET [SPACE] SNAP [F8] LOAD [F9] REC [ [ / ] ] W-SLICE [F11] FPS",
                nav.activeIndex_ + 1U,
                nav.availableWorlds_.size(),
                entry.title.data(),
                worldGeneratorName(nav.currentRecipe_.generator).data(),
                static_cast<unsigned>(nav.currentRecipe_.generator),
                generation,
                instanceCount,
                speed,
                pauseTag,
                mutTag,
                modeTag,
                govTag,
                foliationTag,
                paletteTag,
                learnTag,
                recTag,
                timelineTag,
                perfTag);
        }
    }
}

} // namespace epi_sculpt
