#include "SculptureStagedComputeGovernor.h"
#include <cstdio>
#include <sstream>
#include <iomanip>

namespace epi_sculpt {

namespace {

[[nodiscard]] constexpr StageResolutionScale stepDown(StageResolutionScale current, StageResolutionScale limit) noexcept {
    switch (current) {
    case StageResolutionScale::Full:
        return StageResolutionScale::Half;
    case StageResolutionScale::Half:
        return (limit == StageResolutionScale::Half) ? StageResolutionScale::Half : StageResolutionScale::Quarter;
    case StageResolutionScale::Quarter:
        return (limit == StageResolutionScale::Quarter) ? StageResolutionScale::Quarter : StageResolutionScale::Eighth;
    case StageResolutionScale::Eighth:
        return StageResolutionScale::Eighth;
    }
    return StageResolutionScale::Full;
}

[[nodiscard]] constexpr StageResolutionScale stepUp(StageResolutionScale current) noexcept {
    switch (current) {
    case StageResolutionScale::Eighth:
        return StageResolutionScale::Quarter;
    case StageResolutionScale::Quarter:
        return StageResolutionScale::Half;
    case StageResolutionScale::Half:
        return StageResolutionScale::Full;
    case StageResolutionScale::Full:
        return StageResolutionScale::Full;
    }
    return StageResolutionScale::Full;
}

} // namespace

bool SculptureStagedComputeGovernor::observeGpuLatency(float kernelMilliseconds) noexcept {
    if (!std::isfinite(kernelMilliseconds) || kernelMilliseconds <= 0.0f) {
        return false;
    }

    telemetry_.instantaneousLatencyMs = kernelMilliseconds;
    if (telemetry_.smoothedLatencyMs <= 0.0f) {
        telemetry_.smoothedLatencyMs = kernelMilliseconds;
    } else {
        const float alpha = config_.latencySmoothingAlpha;
        telemetry_.smoothedLatencyMs = (1.0f - alpha) * telemetry_.smoothedLatencyMs + alpha * kernelMilliseconds;
    }

    if (cooldownTimer_ > 0U) {
        --cooldownTimer_;
        return false;
    }

    const float target = config_.targetFrameTimeMs;
    const float overloadThreshold = target * config_.throttleThresholdRatio;
    const float recoveryThreshold = target * config_.recoveryThresholdRatio;

    // 1. Sustained Overload Detection
    if (telemetry_.smoothedLatencyMs > overloadThreshold) {
        ++overloadCounter_;
        recoveryCounter_ = 0U;

        if (overloadCounter_ >= 3U) {
            overloadCounter_ = 0U;
            throttleDown();
            cooldownTimer_ = config_.cooldownFrames;
            return true;
        }
        return false;
    }

    // 2. Sustained Headroom Detection
    if (telemetry_.smoothedLatencyMs < recoveryThreshold) {
        ++recoveryCounter_;
        overloadCounter_ = 0U;

        if (recoveryCounter_ >= 8U) {
            recoveryCounter_ = 0U;
            promoteQuality();
            cooldownTimer_ = config_.cooldownFrames;
            return true;
        }
        return false;
    }

    // Performance within nominal equilibrium window
    overloadCounter_ = 0U;
    recoveryCounter_ = 0U;
    telemetry_.governanceState = "Equilibrium";
    telemetry_.isThrottled = (activeResolutionScale_ != StageResolutionScale::Full || activeTemporalCadence_ > 1U);
    return false;
}

void SculptureStagedComputeGovernor::throttleDown() noexcept {
    bool changed = false;

    // Prefer temporal cadence decimation first if enabled (cheapest, preserves spatial resolution)
    if (config_.enableTemporalDecimation && activeTemporalCadence_ < config_.maxTemporalCadence) {
        activeTemporalCadence_ *= 2U;
        if (activeTemporalCadence_ > config_.maxTemporalCadence) {
            activeTemporalCadence_ = config_.maxTemporalCadence;
        }
        changed = true;
        telemetry_.governanceState = "Temporal Throttling";
    }
    // Then engage spatial downscaling
    else if (config_.enableAutoResolutionTiering) {
        const auto nextScale = stepDown(activeResolutionScale_, config_.maxDownscale);
        if (nextScale != activeResolutionScale_) {
            activeResolutionScale_ = nextScale;
            changed = true;
            telemetry_.governanceState = "Spatial Downscaling";
        }
    }

    if (changed) {
        ++telemetry_.tierTransitionCount;
        telemetry_.isThrottled = true;
        updateEffectiveSavings();
    }
}

void SculptureStagedComputeGovernor::promoteQuality() noexcept {
    bool changed = false;

    // Restore spatial resolution first
    if (activeResolutionScale_ != StageResolutionScale::Full) {
        activeResolutionScale_ = stepUp(activeResolutionScale_);
        changed = true;
        telemetry_.governanceState = "Spatial Recovery";
    }
    // Then restore temporal cadence
    else if (activeTemporalCadence_ > 1U) {
        activeTemporalCadence_ /= 2U;
        if (activeTemporalCadence_ < 1U) activeTemporalCadence_ = 1U;
        changed = true;
        telemetry_.governanceState = "Temporal Recovery";
    }

    if (changed) {
        ++telemetry_.tierTransitionCount;
        telemetry_.isThrottled = (activeResolutionScale_ != StageResolutionScale::Full || activeTemporalCadence_ > 1U);
        if (!telemetry_.isThrottled) {
            telemetry_.governanceState = "Full Fidelity";
        }
        updateEffectiveSavings();
    }
}

void SculptureStagedComputeGovernor::updateEffectiveSavings() noexcept {
    telemetry_.activeResolutionScale = activeResolutionScale_;
    telemetry_.activeTemporalCadence = activeTemporalCadence_;

    // Calculate analytical reduction factor based on cubic grid volume and temporal step:
    // Full (50^3 = 125,000) vs Half (25^3 = 15,625) = 8.0x reduction
    // Quarter (12^3 = 1,728) = ~72.3x reduction
    // Eighth (6^3 = 216) = ~578.7x reduction
    float spatialRatio = 1.0f;
    switch (activeResolutionScale_) {
    case StageResolutionScale::Full: spatialRatio = 1.0f; break;
    case StageResolutionScale::Half: spatialRatio = 8.0f; break;
    case StageResolutionScale::Quarter: spatialRatio = 72.34f; break;
    case StageResolutionScale::Eighth: spatialRatio = 578.70f; break;
    }

    const float temporalRatio = static_cast<float>(activeTemporalCadence_);
    telemetry_.effectiveComputeSavingsRatio = spatialRatio * temporalRatio;
}

std::string SculptureStagedComputeGovernor::formatHudStatus() const {
    char buffer[128];
    std::snprintf(
        buffer, sizeof(buffer),
        "STAGED [Res:%s Cadence:1/%u Savings:%.1fx Latency:%.1fms (%s)]",
        name(activeResolutionScale_).data(),
        activeTemporalCadence_,
        telemetry_.effectiveComputeSavingsRatio,
        telemetry_.smoothedLatencyMs,
        telemetry_.governanceState
    );
    return std::string(buffer);
}

} // namespace epi_sculpt
