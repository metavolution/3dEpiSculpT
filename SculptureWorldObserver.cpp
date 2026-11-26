#include "SculptureWorldObserver.h"
#include "SculptureWorldNavigator.h"
#include "SculptureTimelineDirector.h"
#include "SculptureSerializer.h"
#include <cstdio>
#include <iostream>

namespace epi_sculpt {

void NavigatorLoggingObserver::onWorldSelected(std::size_t index, const WorldCatalogEntry& entry, float transitionDuration) {
    ++eventCount_;
    if (verbose_) {
        std::printf("%s [WorldSelected] Index: %zu | Preset: '%s' | Title: '%s' | Transition: %.1fs\n",
                    tag_.c_str(), index, std::string(entry.canonicalPresetId).c_str(),
                    std::string(entry.title).c_str(), transitionDuration);
    }
}

void NavigatorLoggingObserver::onWorldTransitionCompleted(std::size_t index, const WorldCatalogEntry& entry) {
    ++eventCount_;
    ++transitionCompletedCount_;
    if (verbose_) {
        std::printf("%s [TransitionCompleted] Arrived at world [%zu]: '%s' (%s)\n",
                    tag_.c_str(), index, std::string(entry.title).c_str(),
                    worldGeneratorName(entry.generator).data());
    }
}

void NavigatorLoggingObserver::onActionDispatched(NavigatorAction action, float value, std::string_view actionName) {
    ++eventCount_;
    ++actionDispatchedCount_;
    lastActionName_ = std::string(actionName);
    if (verbose_) {
        std::printf("%s [ActionDispatched] %s (value=%.3f)\n",
                    tag_.c_str(), lastActionName_.c_str(), value);
    }
}

void NavigatorLoggingObserver::onTimelineCueFired(const TimelineActionCue& cue, float timestamp) {
    ++eventCount_;
    ++cueFiredCount_;
    if (verbose_) {
        std::printf("%s [TimelineCue] @%.2fs -> Action: %s (value=%.2f) | Desc: '%s'\n",
                    tag_.c_str(), timestamp, actionName(cue.action).data(), cue.value, cue.description.c_str());
    }
}

void NavigatorLoggingObserver::onTimelinePlaybackChanged(bool isPlaying, bool isPaused, float currentTime, float duration) {
    ++eventCount_;
    if (verbose_) {
        std::printf("%s [TimelinePlayback] Playing: %s | Paused: %s | Time: %.2f / %.2fs\n",
                    tag_.c_str(), isPlaying ? "YES" : "NO", isPaused ? "YES" : "NO", currentTime, duration);
    }
}

void NavigatorLoggingObserver::onSequenceRecordingStarted(std::string_view sessionPath, std::size_t targetFps) {
    ++eventCount_;
    if (verbose_) {
        std::printf("%s [RecordingStarted] Path: '%s' @ %zu FPS\n",
                    tag_.c_str(), std::string(sessionPath).c_str(), targetFps);
    }
}

void NavigatorLoggingObserver::onSequenceRecordingStopped(std::uint32_t totalFrames, std::string_view manifestPath, std::string_view scriptPath) {
    ++eventCount_;
    if (verbose_) {
        std::printf("%s [RecordingStopped] Captured %u frames | Manifest: '%s'\n",
                    tag_.c_str(), totalFrames, std::string(manifestPath).c_str());
    }
}

void NavigatorLoggingObserver::onSnapshotRequested(std::string_view directory) {
    ++eventCount_;
    ++snapshotRequestedCount_;
    if (verbose_) {
        std::printf("%s [SnapshotRequested] Target directory: '%.*s'\n",
                    tag_.c_str(), static_cast<int>(directory.size()), directory.data());
    }
}

void NavigatorLoggingObserver::onSnapshotLoaded(std::string_view filepath, bool success) {
    ++eventCount_;
    if (verbose_) {
        std::printf("%s [SnapshotLoaded] Path: '%s' -> %s\n",
                    tag_.c_str(), std::string(filepath).c_str(), success ? "SUCCESS" : "FAILED");
    }
}

void NavigatorLoggingObserver::onLatentSpaceMutated(float latentW, float rotorAlpha, float rotorBeta) {
    ++eventCount_;
    if (verbose_) {
        std::printf("%s [LatentMutated] W: %.3f | Rotor Alpha: %.3f rad | Rotor Beta: %.3f rad\n",
                    tag_.c_str(), latentW, rotorAlpha, rotorBeta);
    }
}

void NavigatorLoggingObserver::onPerformanceMetricsUpdated(const PerformanceTelemetry& telemetry) {
    ++eventCount_;
    ++metricsUpdatedCount_;
    lastTelemetry_ = telemetry;
    if (verbose_ && telemetry.budgetExceeded) {
        std::printf("%s [PerformanceAlert] Budget exceeded! FPS: %.1f (%.1fms), Hitch count: %u\n",
                    tag_.c_str(), telemetry.smoothedFps, telemetry.smoothedFrameTimeMs, telemetry.hitchCount);
    }
}

void NavigatorLoggingObserver::onParameterPatchApplied(const SculptureStateDiff& diff, std::string_view tag) {
    ++eventCount_;
    ++patchAppliedCount_;
    if (verbose_) {
        std::printf("%s [ParameterPatchApplied] Tag: '%.*s' | Deltas: %zu\n",
                    tag_.c_str(), static_cast<int>(tag.size()), tag.data(), diff.size());
    }
}

void NavigatorLoggingObserver::onBaselineReverted() {
    ++eventCount_;
    ++baselineRevertedCount_;
    if (verbose_) {
        std::printf("%s [BaselineReverted] Active parameters restored to authored recipe baseline.\n",
                    tag_.c_str());
    }
}

void NavigatorLoggingObserver::onHistoryStateChanged(std::size_t undoDepth, std::size_t redoDepth, std::string_view action) {
    ++eventCount_;
    ++historyChangedCount_;
    if (verbose_) {
        std::printf("%s [HistoryStateChanged] Action: '%.*s' | Undo depth: %zu, Redo depth: %zu\n",
                    tag_.c_str(), static_cast<int>(action.size()), action.data(), undoDepth, redoDepth);
    }
}

void NavigatorLoggingObserver::onMorphSpaceUpdated(float cursorX, float cursorY, std::string_view motionMode) {
    ++eventCount_;
    ++morphSpaceUpdatedCount_;
    if (verbose_) {
        std::printf("%s [MorphSpaceUpdated] Cursor: (%.2f, %.2f) | Mode: %.*s\n",
                    tag_.c_str(), cursorX, cursorY, static_cast<int>(motionMode.size()), motionMode.data());
    }
}

void NavigatorLoggingObserver::onMorphSpaceBankChanged(std::size_t bankIndex, std::string_view bankName) {
    ++eventCount_;
    ++morphSpaceBankChangedCount_;
    if (verbose_) {
        std::printf("%s [MorphSpaceBankChanged] Bank [%zu]: '%.*s'\n",
                    tag_.c_str(), bankIndex, static_cast<int>(bankName.size()), bankName.data());
    }
}

} // namespace epi_sculpt

