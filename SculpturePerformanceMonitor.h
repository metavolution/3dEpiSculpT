#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <chrono>
#include <memory>

#include "SculptureWorldObserver.h"
#include "SculpturePerformanceTelemetry.h"

namespace epi_sculpt {

// Forward declarations
class SculptureStagedComputeGovernor;
class SculptureEventBus;

class SculpturePerformanceMonitor : public INavigatorObserver {

public:
    explicit SculpturePerformanceMonitor(PerformanceMonitorConfig config = {}) noexcept;
    ~SculpturePerformanceMonitor() override = default;

    [[nodiscard]] const PerformanceMonitorConfig& config() const noexcept { return config_; }
    void setConfig(const PerformanceMonitorConfig& config) noexcept;

    // Enable / Disable monitor
    void setEnabled(bool enabled) noexcept { enabled_ = enabled; }
    [[nodiscard]] bool isEnabled() const noexcept { return enabled_; }

    // Target frame rate tier controls
    void setTargetTier(TargetFrameRateTier tier) noexcept;
    void cycleTargetTier() noexcept;
    [[nodiscard]] TargetFrameRateTier targetTier() const noexcept { return config_.targetTier; }
    [[nodiscard]] float targetFps() const noexcept { return tierTargetFps(config_.targetTier); }
    [[nodiscard]] float targetBudgetMs() const noexcept { return tierBudgetMs(config_.targetTier); }

    // Telemetry accessors
    [[nodiscard]] const PerformanceTelemetry& telemetry() const noexcept { return telemetry_; }
    [[nodiscard]] std::string formatHud() const;
    [[nodiscard]] std::string formatSparkline(std::size_t width = 24) const;

    // -------------------------------------------------------------------------
    // Frame Profiling Instrumentation Hooks
    // -------------------------------------------------------------------------
    // Call beginFrame() at the start of tick() or updateFrame()
    void beginFrame() noexcept;
    // Call markCpuWorkEnd() when CPU dispatch and presentation transform work completes
    void markCpuWorkEnd() noexcept;
    // Call recordGpuLatency() when OpenCL kernel metrics are received from producer
    void recordGpuLatency(float gpuMilliseconds) noexcept;
    // Call endFrame() at the conclusion of frame execution
    void endFrame() noexcept;

    // Direct manual observation for simulation/test loops
    void recordFrameSample(float frameIntervalMs, float cpuMs = 0.0f, float gpuMs = 0.0f) noexcept;

    // Governor & EventBus bindings
    void bindGovernor(SculptureStagedComputeGovernor* governor) noexcept;
    void unbindGovernor() noexcept { governor_ = nullptr; }
    void bindEventBus(SculptureEventBus* eventBus) noexcept { eventBus_ = eventBus; }
    void unbindEventBus() noexcept { eventBus_ = nullptr; }

    // Reset statistical counters
    void reset() noexcept;

    // -------------------------------------------------------------------------
    // INavigatorObserver Interface Implementation
    // -------------------------------------------------------------------------
    void onWorldSelected(std::size_t index, const WorldCatalogEntry& entry, float transitionDuration) override;
    void onWorldTransitionStarted(std::size_t fromIndex, std::size_t toIndex, const WorldCatalogEntry& toEntry, float duration) override;
    void onWorldTransitionCompleted(std::size_t index, const WorldCatalogEntry& entry) override;
    void onActionDispatched(NavigatorAction action, float value, std::string_view actionName) override;

private:
    void updateBudgetStatus() noexcept;
    void recalculateStatistics() noexcept;
    void syncGovernorTarget() noexcept;

    PerformanceMonitorConfig config_{};
    PerformanceTelemetry telemetry_{};
    bool enabled_ = true;

    // High-resolution timing markers
    using Clock = std::chrono::steady_clock;
    Clock::time_point frameStartTime_{};
    Clock::time_point lastFrameEndTime_{};
    bool hasPreviousFrame_ = false;

    // Sliding ring buffer for zero-allocation statistics
    std::vector<float> frameTimeHistory_{};
    std::vector<float> sortedScratch_{};
    std::size_t historyHead_ = 0U;
    std::size_t historyCount_ = 0U;

    // Governor & EventBus weak bindings
    SculptureStagedComputeGovernor* governor_ = nullptr;
    SculptureEventBus* eventBus_ = nullptr;

    // Adaptive pacing state machine
    std::uint32_t consecutiveHitchCount_ = 0U;
    std::uint32_t stableHeadroomCount_ = 0U;
    bool inTransition_ = false;
};

} // namespace epi_sculpt
