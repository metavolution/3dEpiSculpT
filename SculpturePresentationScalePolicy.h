#pragma once

#include "mgfx/PresentationExtentPolicy.h"
#include "mgfx/RenderTelemetry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace epi_sculpt {

// Presentation-pixel policy only. It intentionally does not know about
// mathematical producer cadence, auxiliary field resolution, Vulkan resources,
// or RenderGraph mutation. Its sole output is an optional scene-scale request
// that must still pass through Engine's transactional target-generation seam.
struct SculpturePresentationScaleConfig {
    bool autoEnabled = false;
    double targetGpuMilliseconds = 10.0;
    double overloadRatio = 1.05;
    double recoveryRatio = 0.72;
    std::uint32_t overloadSamples = 3U;
    std::uint32_t recoverySamples = 24U;
    std::uint32_t cooldownSamples = 24U;
    float minimumAutoScale = 0.50f;
    double smoothingAlpha = 0.20;
};

struct SculpturePresentationScaleState {
    bool autoEnabled = false;
    double instantaneousGpuMilliseconds = 0.0;
    double smoothedGpuMilliseconds = 0.0;
    std::uint64_t observedDeviceGeneration = 0U;
    std::uint64_t lastSubmissionSerial = 0U;
    std::uint64_t observedTargetGeneration = 0U;
    std::uint32_t overloadCount = 0U;
    std::uint32_t recoveryCount = 0U;
    std::uint32_t cooldownRemaining = 0U;
    std::uint64_t decisionCount = 0U;
};

class SculpturePresentationScalePolicy {
public:
    SculpturePresentationScalePolicy() noexcept { setConfig({}); }

    explicit SculpturePresentationScalePolicy(
        SculpturePresentationScaleConfig config) noexcept {
        setConfig(config);
    }

    void setConfig(SculpturePresentationScaleConfig config) noexcept {
        if (!std::isfinite(config.targetGpuMilliseconds) ||
            config.targetGpuMilliseconds <= 0.0) {
            config.targetGpuMilliseconds = 10.0;
        }
        config.overloadRatio = std::clamp(config.overloadRatio, 1.0, 2.0);
        config.recoveryRatio = std::clamp(
            config.recoveryRatio, 0.25, config.overloadRatio - 0.05);
        config.overloadSamples = std::max(config.overloadSamples, 1U);
        config.recoverySamples = std::max(config.recoverySamples, 1U);
        config.minimumAutoScale = std::clamp(
            config.minimumAutoScale,
            mgfx::PresentationExtentPlan::kMinSceneRenderScale,
            mgfx::PresentationExtentPlan::kMaxSceneRenderScale);
        config.smoothingAlpha = std::clamp(config.smoothingAlpha, 0.01, 1.0);
        config_ = config;
        state_.autoEnabled = config_.autoEnabled;
        resetDecisionCounters();
    }

    [[nodiscard]] const SculpturePresentationScaleConfig& config() const noexcept {
        return config_;
    }
    [[nodiscard]] const SculpturePresentationScaleState& state() const noexcept {
        return state_;
    }

    void setAutoEnabled(bool enabled) noexcept {
        if (config_.autoEnabled != enabled) {
            state_.instantaneousGpuMilliseconds = 0.0;
            state_.smoothedGpuMilliseconds = 0.0;
        }
        config_.autoEnabled = enabled;
        state_.autoEnabled = enabled;
        resetDecisionCounters();
    }
    [[nodiscard]] bool autoEnabled() const noexcept { return config_.autoEnabled; }

    // Manual presentation control deliberately disables automatic adaptation so
    // an explicit human/script request is not immediately countermanded by an
    // older completed timing sample.
    void acceptManualOverride() noexcept {
        setAutoEnabled(false);
    }

    [[nodiscard]] static float nextManualScale(float current) noexcept {
        // Discrete perceptual/debug tiers keep repeated key/controller input
        // deterministic. Arbitrary scales remain legal through Set/CLI.
        constexpr std::array<float, 8> kTiers{
            1.00f, 0.90f, 0.80f, 0.70f, 0.60f, 0.50f, 0.375f, 0.25f};
        constexpr float kEpsilon = 0.001f;
        for (std::size_t i = 0; i < kTiers.size(); ++i) {
            if (std::fabs(current - kTiers[i]) <= kEpsilon) {
                return kTiers[(i + 1U) % kTiers.size()];
            }
            if (current > kTiers[i] + kEpsilon) {
                return kTiers[i];
            }
        }
        return kTiers.front();
    }

    // Consume each accepted GPU sample at most once. Samples from a retired
    // target generation are intentionally ignored for decisions: their cost is
    // useful history but is not evidence about the currently effective raster
    // domain. While requested and effective scales differ, ownership is still
    // settling; adaptation pauses instead of making another decision from old
    // pixels. A blocked request likewise freezes adaptation.
    [[nodiscard]] std::optional<float> observe(
        const mgfx::RenderFrameGpuTelemetry& sample,
        float requestedScale,
        float effectiveScale,
        std::uint64_t currentTargetGeneration,
        bool targetRequestBlocked,
        std::uint64_t deviceGeneration = 1U) noexcept {
        if (deviceGeneration == 0U) return std::nullopt;
        if (state_.observedDeviceGeneration != deviceGeneration) {
            // FrameLoop submission serials restart with a reconstructed logical
            // device. Reset only native observation evidence; policy config and
            // human-selected scale remain durable application state.
            state_.observedDeviceGeneration = deviceGeneration;
            state_.lastSubmissionSerial = 0U;
            state_.observedTargetGeneration = 0U;
            state_.instantaneousGpuMilliseconds = 0.0;
            state_.smoothedGpuMilliseconds = 0.0;
            resetDecisionCounters();
        }
        if (!sample.sampled || sample.submissionSerial == 0U ||
            sample.submissionSerial <= state_.lastSubmissionSerial) {
            return std::nullopt;
        }
        state_.lastSubmissionSerial = sample.submissionSerial;

        constexpr float kScaleEpsilon = 0.0001f;
        const bool scaleRequestPending =
            std::fabs(requestedScale - effectiveScale) > kScaleEpsilon;
        if (!config_.autoEnabled || sample.truncated || targetRequestBlocked ||
            scaleRequestPending ||
            sample.targetGeneration != currentTargetGeneration ||
            !std::isfinite(sample.graphMilliseconds) ||
            sample.graphMilliseconds <= 0.0) {
            // Do not erase transition cooldown merely because completion from
            // the previous generation arrives after a new request. Cooldown is
            // intentionally counted in valid samples of the active generation.
            state_.overloadCount = 0U;
            state_.recoveryCount = 0U;
            return std::nullopt;
        }

        if (state_.observedTargetGeneration != sample.targetGeneration) {
            // Different target generations represent different pixel workloads.
            // Restart timing evidence at the publication boundary, but preserve
            // cooldown so one scale transition still gets settling time.
            state_.observedTargetGeneration = sample.targetGeneration;
            state_.instantaneousGpuMilliseconds = 0.0;
            state_.smoothedGpuMilliseconds = 0.0;
            state_.overloadCount = 0U;
            state_.recoveryCount = 0U;
        }

        state_.instantaneousGpuMilliseconds = sample.graphMilliseconds;
        if (state_.smoothedGpuMilliseconds <= 0.0) {
            state_.smoothedGpuMilliseconds = sample.graphMilliseconds;
        } else {
            const double alpha = config_.smoothingAlpha;
            state_.smoothedGpuMilliseconds =
                (1.0 - alpha) * state_.smoothedGpuMilliseconds +
                alpha * sample.graphMilliseconds;
        }

        if (state_.cooldownRemaining > 0U) {
            --state_.cooldownRemaining;
            return std::nullopt;
        }

        const double overload =
            config_.targetGpuMilliseconds * config_.overloadRatio;
        const double recovery =
            config_.targetGpuMilliseconds * config_.recoveryRatio;

        if (state_.smoothedGpuMilliseconds > overload) {
            ++state_.overloadCount;
            state_.recoveryCount = 0U;
            if (state_.overloadCount < config_.overloadSamples) {
                return std::nullopt;
            }
            resetDecisionCounters();
            if (const auto next = stepDown(effectiveScale)) {
                state_.cooldownRemaining = config_.cooldownSamples;
                ++state_.decisionCount;
                return next;
            }
            return std::nullopt;
        }

        if (state_.smoothedGpuMilliseconds < recovery) {
            ++state_.recoveryCount;
            state_.overloadCount = 0U;
            if (state_.recoveryCount < config_.recoverySamples) {
                return std::nullopt;
            }
            resetDecisionCounters();
            if (const auto next = stepUp(effectiveScale)) {
                state_.cooldownRemaining = config_.cooldownSamples;
                ++state_.decisionCount;
                return next;
            }
            return std::nullopt;
        }

        state_.overloadCount = 0U;
        state_.recoveryCount = 0U;
        return std::nullopt;
    }

private:
    [[nodiscard]] std::optional<float> stepDown(float current) const noexcept {
        constexpr std::array<float, 6> kAutoTiers{
            1.00f, 0.90f, 0.80f, 0.70f, 0.60f, 0.50f};
        constexpr float kEpsilon = 0.001f;
        for (float tier : kAutoTiers) {
            if (tier < current - kEpsilon && tier >= config_.minimumAutoScale - kEpsilon) {
                return tier;
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] static std::optional<float> stepUp(float current) noexcept {
        constexpr std::array<float, 6> kAutoTiers{
            1.00f, 0.90f, 0.80f, 0.70f, 0.60f, 0.50f};
        constexpr float kEpsilon = 0.001f;
        for (auto it = kAutoTiers.rbegin(); it != kAutoTiers.rend(); ++it) {
            if (*it > current + kEpsilon) return *it;
        }
        return std::nullopt;
    }

    void resetDecisionCounters() noexcept {
        state_.overloadCount = 0U;
        state_.recoveryCount = 0U;
        state_.cooldownRemaining = 0U;
    }

    SculpturePresentationScaleConfig config_{};
    SculpturePresentationScaleState state_{};
};

} // namespace epi_sculpt
