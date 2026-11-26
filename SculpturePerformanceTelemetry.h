#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace epi_sculpt {

/**
 * Target frame rate tiers for the real-time performance monitor and adaptive frame-pacing.
 */
enum class TargetFrameRateTier : std::uint8_t {
    Fps60 = 0,     // 16.6667 ms budget (standard baseline)
    Fps120 = 1,    //  8.3333 ms budget (high-refresh / VR target)
    Fps144 = 2,    //  6.9444 ms budget (esports / ultra-high refresh)
    Fps240 = 3,    //  4.1667 ms budget (extreme high-frequency compute)
    Unlimited = 4  // Free-running uncapped simulation
};

[[nodiscard]] constexpr float tierTargetFps(TargetFrameRateTier tier) noexcept {
    switch (tier) {
    case TargetFrameRateTier::Fps60:     return 60.0f;
    case TargetFrameRateTier::Fps120:    return 120.0f;
    case TargetFrameRateTier::Fps144:    return 144.0f;
    case TargetFrameRateTier::Fps240:    return 240.0f;
    case TargetFrameRateTier::Unlimited: return 0.0f;
    }
    return 60.0f;
}

[[nodiscard]] constexpr float tierBudgetMs(TargetFrameRateTier tier) noexcept {
    switch (tier) {
    case TargetFrameRateTier::Fps60:     return 16.6667f;
    case TargetFrameRateTier::Fps120:    return 8.3333f;
    case TargetFrameRateTier::Fps144:    return 6.9444f;
    case TargetFrameRateTier::Fps240:    return 4.1667f;
    case TargetFrameRateTier::Unlimited: return 0.0f;
    }
    return 16.6667f;
}

[[nodiscard]] constexpr std::string_view tierName(TargetFrameRateTier tier) noexcept {
    switch (tier) {
    case TargetFrameRateTier::Fps60:     return "60 FPS (16.7ms)";
    case TargetFrameRateTier::Fps120:    return "120 FPS (8.3ms)";
    case TargetFrameRateTier::Fps144:    return "144 FPS (6.9ms)";
    case TargetFrameRateTier::Fps240:    return "240 FPS (4.2ms)";
    case TargetFrameRateTier::Unlimited: return "Unlimited (Uncapped)";
    }
    return "60 FPS";
}

/**
 * Snapshot of real-time performance telemetry metrics.
 */
struct PerformanceTelemetry {
    float currentFps = 0.0f;
    float smoothedFps = 0.0f;
    float currentFrameTimeMs = 16.6667f;
    float smoothedFrameTimeMs = 16.6667f;
    float minFrameTimeMs = 0.0f;
    float maxFrameTimeMs = 0.0f;
    float jitterStdDevMs = 0.0f;        // Frame-to-frame standard deviation
    float low1PercentFps = 0.0f;        // 99th percentile worst-case frame rate
    float lastCpuTimeMs = 0.0f;
    float smoothedCpuTimeMs = 0.0f;
    float lastGpuTimeMs = 0.0f;
    float smoothedGpuTimeMs = 0.0f;
    std::uint64_t totalFrames = 0U;
    std::uint32_t hitchCount = 0U;      // Frames exceeding 1.5x target frame budget
    bool budgetExceeded = false;
    float headroomRatio = 1.0f;         // Budget / CurrentTime (>1.0: headroom, <1.0: deficit)
    float headroomMs = 0.0f;            // Target frame budget minus frame interval (>0ms: headroom, <0ms: deficit)
    TargetFrameRateTier targetTier = TargetFrameRateTier::Fps60;
    float targetFps = 60.0f;
    float targetBudgetMs = 16.6667f;
    std::string budgetStatus = "Nominal"; // "Optimal", "Nominal", "Throttled", "Hitching"
};

/**
 * Configuration parameters for the performance monitor.
 */
struct PerformanceMonitorConfig {
    TargetFrameRateTier targetTier = TargetFrameRateTier::Fps60;
    float smoothingAlpha = 0.12f;       // Exponential moving average filter coefficient
    float hitchThresholdFactor = 1.50f; // Multiplier of frame budget to flag a hitch
    std::size_t historyCapacity = 120U; // Number of historical frames in the sliding ring buffer
    bool enableAutoGovernorPacing = true;
    std::uint32_t consecutiveHitchesForThrottle = 3U;
    std::uint32_t stableHeadroomFramesForRecovery = 60U;
};

} // namespace epi_sculpt
