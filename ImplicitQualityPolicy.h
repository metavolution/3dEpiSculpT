#pragma once

#include "SculptureProgramSchema.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace epi_sculpt {

struct ImplicitQualityConfig {
    bool enabled = true;
    float targetKernelMilliseconds = 8.0f;
    float overloadRatio = 1.10f;
    float recoveryRatio = 0.65f;
    std::uint32_t minimumRaySteps = 48U;
    std::uint32_t maximumRaySteps = 192U;
    std::uint32_t initialRaySteps = 128U;
    std::uint32_t adjustmentStep = 16U;
    std::uint32_t overloadSamples = 2U;
    std::uint32_t recoverySamples = 8U;
    bool enableAdaptiveResolution = false;
    StageResolutionScale minResolutionScale = StageResolutionScale::Full;
    StageResolutionScale maxResolutionScale = StageResolutionScale::Quarter;
    StageResolutionScale initialResolutionScale = StageResolutionScale::Full;
    std::uint32_t resolutionOverloadSamples = 3U;
    std::uint32_t resolutionRecoverySamples = 6U;
};

// Hysteretic quality governor: overload reacts quickly, while quality recovery
// is deliberately slower so live visuals do not shimmer between step budgets.
// When enabled, sustained overload downscales resolution (Full -> Half -> Quarter),
// while sustained headroom promotes resolution back up with smooth step reset.
class ImplicitQualityPolicy {
public:
    explicit ImplicitQualityPolicy(ImplicitQualityConfig config = {}) noexcept
        : config_(normalize(config)),
          raySteps_(config_.initialRaySteps),
          resolutionScale_(config_.initialResolutionScale) {}

    [[nodiscard]] bool observeKernelMilliseconds(float milliseconds) noexcept {
        if (!config_.enabled || !std::isfinite(milliseconds)
            || milliseconds <= 0.0f) {
            return false;
        }

        if (milliseconds
            > config_.targetKernelMilliseconds * config_.overloadRatio) {
            ++overloadCount_;
            recoveryCount_ = 0U;
            resolutionRecoveryCount_ = 0U;

            if (overloadCount_ < config_.overloadSamples) return false;
            overloadCount_ = 0U;

            if (raySteps_ > config_.minimumRaySteps) {
                const std::uint32_t previous = raySteps_;
                const std::uint32_t distanceToMinimum =
                    raySteps_ - config_.minimumRaySteps;
                raySteps_ = distanceToMinimum > config_.adjustmentStep
                    ? raySteps_ - config_.adjustmentStep : config_.minimumRaySteps;
                return raySteps_ != previous;
            }

            // If raySteps are already at minimum, attempt adaptive resolution demotion
            if (config_.enableAdaptiveResolution) {
                ++resolutionOverloadCount_;
                if (resolutionOverloadCount_ >= config_.resolutionOverloadSamples) {
                    resolutionOverloadCount_ = 0U;
                    const auto nextLower = nextLowerScale(resolutionScale_);
                    if (static_cast<std::uint8_t>(nextLower)
                        <= static_cast<std::uint8_t>(config_.maxResolutionScale)
                        && nextLower != resolutionScale_) {
                        resolutionScale_ = nextLower;
                        raySteps_ = config_.initialRaySteps;
                        return true;
                    }
                }
            }
            return false;
        }

        if (milliseconds
            < config_.targetKernelMilliseconds * config_.recoveryRatio) {
            ++recoveryCount_;
            overloadCount_ = 0U;
            resolutionOverloadCount_ = 0U;

            if (recoveryCount_ < config_.recoverySamples) return false;
            recoveryCount_ = 0U;

            if (raySteps_ < config_.maximumRaySteps) {
                const std::uint32_t previous = raySteps_;
                const std::uint32_t distanceToMaximum =
                    config_.maximumRaySteps - raySteps_;
                raySteps_ = distanceToMaximum > config_.adjustmentStep
                    ? raySteps_ + config_.adjustmentStep : config_.maximumRaySteps;
                return raySteps_ != previous;
            }

            // If raySteps are at maximum, attempt adaptive resolution promotion
            if (config_.enableAdaptiveResolution) {
                ++resolutionRecoveryCount_;
                if (resolutionRecoveryCount_ >= config_.resolutionRecoverySamples) {
                    resolutionRecoveryCount_ = 0U;
                    const auto nextHigher = nextHigherScale(resolutionScale_);
                    if (static_cast<std::uint8_t>(nextHigher)
                        >= static_cast<std::uint8_t>(config_.minResolutionScale)
                        && nextHigher != resolutionScale_) {
                        resolutionScale_ = nextHigher;
                        raySteps_ = config_.initialRaySteps;
                        return true;
                    }
                }
            }
            return false;
        }

        overloadCount_ = 0U;
        recoveryCount_ = 0U;
        resolutionOverloadCount_ = 0U;
        resolutionRecoveryCount_ = 0U;
        return false;
    }

    [[nodiscard]] std::uint32_t raySteps() const noexcept { return raySteps_; }
    [[nodiscard]] StageResolutionScale resolutionScale() const noexcept {
        return resolutionScale_;
    }
    [[nodiscard]] std::uint32_t scaleDivisor() const noexcept {
        return static_cast<std::uint32_t>(resolutionScale_);
    }
    [[nodiscard]] std::uint32_t effectiveWidth(std::uint32_t baseWidth) const noexcept {
        const std::uint32_t div = scaleDivisor();
        return std::max(1U, baseWidth / (div > 0U ? div : 1U));
    }
    [[nodiscard]] std::uint32_t effectiveHeight(std::uint32_t baseHeight) const noexcept {
        const std::uint32_t div = scaleDivisor();
        return std::max(1U, baseHeight / (div > 0U ? div : 1U));
    }

    void setResolutionScale(StageResolutionScale scale) noexcept {
        resolutionScale_ = scale;
    }
    void setRaySteps(std::uint32_t steps) noexcept {
        raySteps_ = std::clamp(
            steps, config_.minimumRaySteps, config_.maximumRaySteps);
    }
    void reset(StageResolutionScale scale = StageResolutionScale::Full) noexcept {
        raySteps_ = config_.initialRaySteps;
        resolutionScale_ = scale;
        overloadCount_ = 0U;
        recoveryCount_ = 0U;
        resolutionOverloadCount_ = 0U;
        resolutionRecoveryCount_ = 0U;
    }

    [[nodiscard]] const ImplicitQualityConfig& config() const noexcept {
        return config_;
    }

private:
    [[nodiscard]] static StageResolutionScale nextLowerScale(StageResolutionScale scale) noexcept {
        switch (scale) {
        case StageResolutionScale::Full: return StageResolutionScale::Half;
        case StageResolutionScale::Half: return StageResolutionScale::Quarter;
        case StageResolutionScale::Quarter: return StageResolutionScale::Eighth;
        case StageResolutionScale::Eighth: return StageResolutionScale::Eighth;
        }
        return StageResolutionScale::Quarter;
    }

    [[nodiscard]] static StageResolutionScale nextHigherScale(StageResolutionScale scale) noexcept {
        switch (scale) {
        case StageResolutionScale::Eighth: return StageResolutionScale::Quarter;
        case StageResolutionScale::Quarter: return StageResolutionScale::Half;
        case StageResolutionScale::Half: return StageResolutionScale::Full;
        case StageResolutionScale::Full: return StageResolutionScale::Full;
        }
        return StageResolutionScale::Full;
    }

    [[nodiscard]] static ImplicitQualityConfig normalize(
        ImplicitQualityConfig config) noexcept {
        config.targetKernelMilliseconds =
            std::isfinite(config.targetKernelMilliseconds)
                && config.targetKernelMilliseconds > 0.0f
            ? config.targetKernelMilliseconds : 8.0f;
        config.overloadRatio = std::isfinite(config.overloadRatio)
            ? std::max(config.overloadRatio, 1.0f) : 1.10f;
        config.recoveryRatio = std::isfinite(config.recoveryRatio)
            ? std::clamp(config.recoveryRatio, 0.05f, 0.95f) : 0.65f;
        config.minimumRaySteps = std::max(config.minimumRaySteps, 1U);
        config.maximumRaySteps = std::max(
            config.maximumRaySteps, config.minimumRaySteps);
        config.initialRaySteps = std::clamp(
            config.initialRaySteps,
            config.minimumRaySteps,
            config.maximumRaySteps);
        config.adjustmentStep = std::max(config.adjustmentStep, 1U);
        config.overloadSamples = std::max(config.overloadSamples, 1U);
        config.recoverySamples = std::max(config.recoverySamples, 1U);
        config.resolutionOverloadSamples = std::max(config.resolutionOverloadSamples, 1U);
        config.resolutionRecoverySamples = std::max(config.resolutionRecoverySamples, 1U);
        return config;
    }

    ImplicitQualityConfig config_{};
    std::uint32_t raySteps_ = 128U;
    StageResolutionScale resolutionScale_ = StageResolutionScale::Full;
    std::uint32_t overloadCount_ = 0U;
    std::uint32_t recoveryCount_ = 0U;
    std::uint32_t resolutionOverloadCount_ = 0U;
    std::uint32_t resolutionRecoveryCount_ = 0U;
};

} // namespace epi_sculpt
