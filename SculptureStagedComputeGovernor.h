#pragma once

#include "SculptureProgramSchema.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <algorithm>
#include <cmath>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: MULTI-STAGE STAGED COMPUTE & ADAPTIVE GOVERNOR
// -----------------------------------------------------------------------------
// To achieve 8K @ 120 FPS on high-performance GPUs (as outlined in docu/project-vision.txt),
// computation must be partitioned by spatial and temporal frequencies:
//
// 1. High-Frequency Geometry & Raymarch Loop (Stages 1-4: Affine, Inner, Reduce, Color):
//    - Evaluated every frame at 100% native viewport resolution (120+ Hz).
//
// 2. Low-Frequency Spatial Volumetrics & Seed Injection (Stages 5-6: WCA Seed, CA Diffusion):
//    - Smooth spatial potentials and cellular fluid diffusion have high spatial coherence.
//    - Evaluated on downsampled volumetric grids (Half: 25^3, Quarter: 12^3).
//    - Evaluated at sub-Nyquist temporal cadences (every 2 or 4 ticks).
//    - Reconstructed via hardware-equivalent trilinear tensor interpolation.
//
// The Staged Compute Governor monitors GPU execution times in a closed feedback loop.
// If GPU kernel latencies exceed the frame budget (e.g. >16.6ms for 60fps or >8.3ms for 120fps),
// it autonomously downscales auxiliary stages and engages temporal decimation with hysteresis,
// guaranteeing zero frame drops and preserving rock-solid 60/120 FPS interactive performance.
// -----------------------------------------------------------------------------

struct StagedComputeConfig {
    float targetFrameTimeMs = 16.6667f;      // 60 FPS target baseline (8.333ms for 120 FPS)
    float throttleThresholdRatio = 1.10f;    // Throttle down when kernelMs > 1.10 * target
    float recoveryThresholdRatio = 0.70f;    // Promote quality when kernelMs < 0.70 * target
    std::uint32_t cooldownFrames = 30U;      // Hysteresis cooldown to prevent visual/resolution thrashing
    bool enableAutoResolutionTiering = true; // Dynamically scale spatial auxiliary grid
    bool enableTemporalDecimation = true;    // Dynamically skip auxiliary evaluation ticks
    StageResolutionScale maxDownscale = StageResolutionScale::Quarter; // Lowest permitted spatial resolution
    std::uint32_t maxTemporalCadence = 4U;   // Maximum frame decimation interval (1, 2, or 4)
    float latencySmoothingAlpha = 0.15f;     // Exponential moving average filter coefficient
};

struct StagedComputeTelemetry {
    float smoothedLatencyMs = 0.0f;
    float instantaneousLatencyMs = 0.0f;
    StageResolutionScale activeResolutionScale = StageResolutionScale::Full;
    std::uint32_t activeTemporalCadence = 1U;
    float effectiveComputeSavingsRatio = 1.0f;
    bool isThrottled = false;
    std::uint32_t tierTransitionCount = 0U;
    std::uint64_t totalDecimatedFrames = 0U;
    const char* governanceState = "Nominal";
};

class SculptureStagedComputeGovernor {
public:
    SculptureStagedComputeGovernor() noexcept
        : SculptureStagedComputeGovernor(StagedComputeConfig{}) {}

    explicit SculptureStagedComputeGovernor(const StagedComputeConfig& config) noexcept
        : config_(config), activeResolutionScale_(StageResolutionScale::Full) {
        updateEffectiveSavings();
    }

    void setConfig(const StagedComputeConfig& config) noexcept {
        config_ = config;
        updateEffectiveSavings();
    }

    [[nodiscard]] const StagedComputeConfig& config() const noexcept { return config_; }
    [[nodiscard]] const StagedComputeTelemetry& telemetry() const noexcept { return telemetry_; }

    // Sets target frame time (e.g. 16.6667f for 60Hz, 8.3333f for 120Hz)
    void setTargetFrameRate(float targetFps) noexcept {
        if (targetFps > 1.0f) {
            config_.targetFrameTimeMs = 1000.0f / targetFps;
        }
    }

    // Manual override of resolution tier
    void setResolutionScale(StageResolutionScale scale) noexcept {
        activeResolutionScale_ = scale;
        cooldownTimer_ = config_.cooldownFrames;
        updateEffectiveSavings();
    }

    [[nodiscard]] StageResolutionScale resolutionScale() const noexcept {
        return activeResolutionScale_;
    }

    // Closed-loop GPU telemetry ingestion.
    // Adjusts spatial resolution scale and temporal decimation cadence based on actual GPU timing.
    bool observeGpuLatency(float kernelMilliseconds) noexcept;

    // Queries whether low-frequency auxiliary stages (WCA seed, fluid CA) should evaluate on this tick
    [[nodiscard]] bool shouldEvaluateAuxiliaryStage(std::uint64_t frameIndex) const noexcept {
        if (!config_.enableTemporalDecimation || activeTemporalCadence_ <= 1U) {
            return true;
        }
        return (frameIndex % activeTemporalCadence_) == 0U;
    }

    // Stamps the determined resolution scale and multi-stage execution hints into program params
    void applyToParams(SculptureProgramParams& params) const noexcept {
        params.auxiliaryResolutionScale = activeResolutionScale_;
    }

    // Formats a human-readable telemetry status string for HUD display
    [[nodiscard]] std::string formatHudStatus() const;

    void reset() noexcept {
        activeResolutionScale_ = StageResolutionScale::Full;
        activeTemporalCadence_ = 1U;
        cooldownTimer_ = 0U;
        overloadCounter_ = 0U;
        recoveryCounter_ = 0U;
        telemetry_ = {};
        telemetry_.activeResolutionScale = activeResolutionScale_;
        telemetry_.activeTemporalCadence = activeTemporalCadence_;
        updateEffectiveSavings();
    }

private:
    void updateEffectiveSavings() noexcept;
    void throttleDown() noexcept;
    void promoteQuality() noexcept;

    StagedComputeConfig config_{};
    StagedComputeTelemetry telemetry_{};
    StageResolutionScale activeResolutionScale_ = StageResolutionScale::Full;
    std::uint32_t activeTemporalCadence_ = 1U;
    std::uint32_t cooldownTimer_ = 0U;
    std::uint32_t overloadCounter_ = 0U;
    std::uint32_t recoveryCounter_ = 0U;
};

} // namespace epi_sculpt
