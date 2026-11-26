#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <memory>
#include <iostream>

#include "SculptureWorldRecipe.h"
#include "SculpturePerformanceTelemetry.h"

namespace epi_sculpt {

// Forward declarations
enum class NavigatorAction : std::uint8_t;
struct TimelineActionCue;
struct SculptureStateDiff;

/**
 * ============================================================================
 * EPI C / 3dEpiSculpTMorph - REACTIVE WORLD NAVIGATOR OBSERVER
 * ============================================================================
 * Educational Annotation: DECOUPLED EVENT NOTIFICATION MANIFOLD
 * ----------------------------------------------------------------------------
 * In accordance with the Project Vision (docu/project-vision.txt:177-181):
 *   "2. Event System Foundation:
 *       - EventBus implementation
 *       - Performance monitoring
 *       - Decoupled communication"
 *
 * In high-performance visual computing and real-time mathematical dynamic
 * systems, polling state variables every frame introduces tight architectural
 * coupling and computational waste.
 *
 * INavigatorObserver defines a synchronous, zero-allocation event observer
 * contract. Subsystems (telemetry collectors, OSC/MIDI bridges, headless
 * test harnesses, live HUD overlays, and sequence recorders) register as
 * observers with SculptureWorldNavigator to receive instantaneous push notifications
 * when state changes occur.
 * ============================================================================
 */
class INavigatorObserver {
public:
    virtual ~INavigatorObserver() = default;

    // --- World Lifecycle & Smooth Transition ---
    virtual void onWorldSelected(std::size_t index, const WorldCatalogEntry& entry, float transitionDuration) {}
    virtual void onWorldTransitionStarted(std::size_t fromIndex, std::size_t toIndex, const WorldCatalogEntry& toEntry, float duration) {}
    virtual void onWorldTransitionProgress(float progress, float rawProgress) {}
    virtual void onWorldTransitionCompleted(std::size_t index, const WorldCatalogEntry& entry) {}

    // --- Command Center Action Dispatch ---
    virtual void onActionDispatched(NavigatorAction action, float value, std::string_view actionName) {}

    // --- Script-Driven Timeline Director ---
    virtual void onTimelineLoaded(std::string_view title, float duration, std::size_t cueCount) {}
    virtual void onTimelinePlaybackChanged(bool isPlaying, bool isPaused, float currentTime, float duration) {}
    virtual void onTimelineCueFired(const TimelineActionCue& cue, float timestamp) {}
    virtual void onTimelineCompleted() {}

    // --- Cinematic Sequence Recording Lifecycle ---
    virtual void onSequenceRecordingStarted(std::string_view sessionPath, std::size_t targetFps) {}
    virtual void onSequenceRecordingFrame(std::uint32_t frameIndex, float elapsedSeconds, std::size_t queueDepth) {}
    virtual void onSequenceRecordingStopped(std::uint32_t totalFrames, std::string_view manifestPath, std::string_view scriptPath) {}

    // --- State Persistence (Snapshots) ---
    virtual void onSnapshotRequested(std::string_view directory) {}
    virtual void onSnapshotLoaded(std::string_view filepath, bool success) {}

    // --- 4D Foliation & Latent Space Mutation ---
    virtual void onLatentSpaceMutated(float latentW, float rotorAlpha, float rotorBeta) {}

    // --- Performance & Frame Pacing ---
    virtual void onPerformanceMetricsUpdated(const PerformanceTelemetry& telemetry) {}

    // --- State Parameter Patches & Baseline Drift ---
    virtual void onParameterPatchApplied(const SculptureStateDiff& diff, std::string_view tag) {}
    virtual void onBaselineReverted() {}

    // --- Live Parameter History (Undo/Redo) ---
    virtual void onHistoryStateChanged(std::size_t undoDepth, std::size_t redoDepth, std::string_view action) {}

    // --- Multidimensional Morph Space (Vector Pad) ---
    virtual void onMorphSpaceUpdated(float cursorX, float cursorY, std::string_view motionMode) {}
    virtual void onMorphSpaceBankChanged(std::size_t bankIndex, std::string_view bankName) {}
};

/**
 * Standard diagnostic logging observer for terminal telemetry and test verification.
 */
class NavigatorLoggingObserver : public INavigatorObserver {
public:
    explicit NavigatorLoggingObserver(std::string tag = "[NavigatorObserver]", bool verbose = true)
        : tag_(std::move(tag)), verbose_(verbose) {}

    void onWorldSelected(std::size_t index, const WorldCatalogEntry& entry, float transitionDuration) override;
    void onWorldTransitionCompleted(std::size_t index, const WorldCatalogEntry& entry) override;
    void onActionDispatched(NavigatorAction action, float value, std::string_view actionName) override;
    void onTimelineCueFired(const TimelineActionCue& cue, float timestamp) override;
    void onTimelinePlaybackChanged(bool isPlaying, bool isPaused, float currentTime, float duration) override;
    void onSequenceRecordingStarted(std::string_view sessionPath, std::size_t targetFps) override;
    void onSequenceRecordingStopped(std::uint32_t totalFrames, std::string_view manifestPath, std::string_view scriptPath) override;
    void onSnapshotRequested(std::string_view directory) override;
    void onSnapshotLoaded(std::string_view filepath, bool success) override;
    void onLatentSpaceMutated(float latentW, float rotorAlpha, float rotorBeta) override;
    void onPerformanceMetricsUpdated(const PerformanceTelemetry& telemetry) override;
    void onParameterPatchApplied(const SculptureStateDiff& diff, std::string_view tag) override;
    void onBaselineReverted() override;
    void onHistoryStateChanged(std::size_t undoDepth, std::size_t redoDepth, std::string_view action) override;
    void onMorphSpaceUpdated(float cursorX, float cursorY, std::string_view motionMode) override;
    void onMorphSpaceBankChanged(std::size_t bankIndex, std::string_view bankName) override;

    [[nodiscard]] std::size_t eventCount() const noexcept { return eventCount_; }
    [[nodiscard]] std::size_t actionDispatchedCount() const noexcept { return actionDispatchedCount_; }
    [[nodiscard]] std::size_t cueFiredCount() const noexcept { return cueFiredCount_; }
    [[nodiscard]] std::size_t transitionCompletedCount() const noexcept { return transitionCompletedCount_; }
    [[nodiscard]] std::size_t metricsUpdatedCount() const noexcept { return metricsUpdatedCount_; }
    [[nodiscard]] std::size_t snapshotRequestedCount() const noexcept { return snapshotRequestedCount_; }
    [[nodiscard]] std::size_t patchAppliedCount() const noexcept { return patchAppliedCount_; }
    [[nodiscard]] std::size_t baselineRevertedCount() const noexcept { return baselineRevertedCount_; }
    [[nodiscard]] std::size_t historyChangedCount() const noexcept { return historyChangedCount_; }
    [[nodiscard]] std::size_t morphSpaceUpdatedCount() const noexcept { return morphSpaceUpdatedCount_; }
    [[nodiscard]] std::size_t morphSpaceBankChangedCount() const noexcept { return morphSpaceBankChangedCount_; }
    [[nodiscard]] std::string_view lastActionName() const noexcept { return lastActionName_; }
    [[nodiscard]] const PerformanceTelemetry& lastTelemetry() const noexcept { return lastTelemetry_; }

    void resetCounts() noexcept {
        eventCount_ = 0U;
        actionDispatchedCount_ = 0U;
        cueFiredCount_ = 0U;
        transitionCompletedCount_ = 0U;
        metricsUpdatedCount_ = 0U;
        snapshotRequestedCount_ = 0U;
        patchAppliedCount_ = 0U;
        baselineRevertedCount_ = 0U;
        historyChangedCount_ = 0U;
        morphSpaceUpdatedCount_ = 0U;
        morphSpaceBankChangedCount_ = 0U;
        lastActionName_ = "";
        lastTelemetry_ = PerformanceTelemetry{};
    }

private:
    std::string tag_;
    bool verbose_ = true;
    std::size_t eventCount_ = 0U;
    std::size_t actionDispatchedCount_ = 0U;
    std::size_t cueFiredCount_ = 0U;
    std::size_t transitionCompletedCount_ = 0U;
    std::size_t metricsUpdatedCount_ = 0U;
    std::size_t snapshotRequestedCount_ = 0U;
    std::size_t patchAppliedCount_ = 0U;
    std::size_t baselineRevertedCount_ = 0U;
    std::size_t historyChangedCount_ = 0U;
    std::size_t morphSpaceUpdatedCount_ = 0U;
    std::size_t morphSpaceBankChangedCount_ = 0U;
    std::string lastActionName_{};
    PerformanceTelemetry lastTelemetry_{};
};

} // namespace epi_sculpt
