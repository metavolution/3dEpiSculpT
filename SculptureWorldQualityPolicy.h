// Bounded adaptive-quality policy for dedicated procedural worlds.
//
// It changes orchestration inputs only: recipe budget and producer cadence.
// Generator identity, primitive routing, and the GPU ABI remain immutable.

#pragma once

#include "SculptureWorldRecipe.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace epi_sculpt {

class SculptureWorldQualityPolicy {
public:
    struct Telemetry {
        std::uint32_t requestedBudget = 0U;
        std::uint32_t effectiveBudget = 0U;
        float qualityScale = 1.0f;
        std::uint32_t pressureEvents = 0U;
        std::uint32_t recoveryEvents = 0U;
    };

    void configure(const WorldRecipe& recipe) noexcept {
        requestedBudget_ = recipe.instanceBudget;
        // Small topological constellations communicate through every element;
        // never remove nodes from those designs. Dense fields can fall to a
        // quarter budget, but never below a visibly useful 256 instances.
        minimumBudget_ = recipe.instanceBudget <= 256U
            ? recipe.instanceBudget
            : std::min(recipe.instanceBudget,
                       std::max(256U, recipe.instanceBudget / 4U));
        qualityScale_ = 1.0f;
        stableSamples_ = 0U;
        telemetry_ = {.requestedBudget = requestedBudget_,
                      .effectiveBudget = requestedBudget_,
                      .qualityScale = qualityScale_};
    }

    [[nodiscard]] WorldRecipe apply(const WorldRecipe& recipe) const noexcept {
        WorldRecipe adjusted = recipe;
        if (recipe.instanceBudget == 0U || requestedBudget_ != recipe.instanceBudget) {
            return adjusted;
        }
        const float scaled = std::round(static_cast<float>(recipe.instanceBudget) * qualityScale_);
        adjusted.instanceBudget = std::clamp(
            static_cast<std::uint32_t>(std::max(scaled, 1.0f)),
            minimumBudget_, recipe.instanceBudget);
        return adjusted;
    }

    // Completion time is measured from the first queued write to the readback
    // event. Two queued world slots indicate a visible backlog in the triple
    // ring, so pressure responds before it reaches total saturation.
    void observe(float completionMilliseconds,
                 float kernelMilliseconds,
                 float readbackMilliseconds,
                 std::uint32_t pendingSlots) noexcept {
        const float activeGpu = kernelMilliseconds + readbackMilliseconds;
        const float measure = std::isfinite(activeGpu) && activeGpu >= 0.0f
            ? activeGpu
            : completionMilliseconds;

        const bool overloaded = pendingSlots >= 2U
            || (std::isfinite(measure)
                && measure > kTargetMilliseconds * 1.20f);
        if (overloaded && qualityScale_ > minimumScale()) {
            qualityScale_ = std::max(minimumScale(), qualityScale_ * 0.80f);
            stableSamples_ = 0U;
            ++telemetry_.pressureEvents;
        } else if (!overloaded && std::isfinite(measure)
                   && measure > 0.0f
                   && measure < kTargetMilliseconds * 0.65f) {
            ++stableSamples_;
            if (stableSamples_ >= kSamplesBeforeRecovery && qualityScale_ < 1.0f) {
                qualityScale_ = std::min(1.0f, qualityScale_ * 1.10f);
                stableSamples_ = 0U;
                ++telemetry_.recoveryEvents;
            }
        } else {
            stableSamples_ = 0U;
        }
        telemetry_.qualityScale = qualityScale_;
        telemetry_.effectiveBudget = requestedBudget_ == 0U ? 0U
            : std::clamp(static_cast<std::uint32_t>(std::round(
                             static_cast<float>(requestedBudget_) * qualityScale_)),
                         minimumBudget_, requestedBudget_);
    }

    [[nodiscard]] const Telemetry& telemetry() const noexcept { return telemetry_; }

private:
    [[nodiscard]] float minimumScale() const noexcept {
        return requestedBudget_ == 0U ? 1.0f
            : static_cast<float>(minimumBudget_) / static_cast<float>(requestedBudget_);
    }

    static constexpr float kTargetMilliseconds = 20.0f;
    static constexpr std::uint32_t kSamplesBeforeRecovery = 24U;
    std::uint32_t requestedBudget_ = 0U;
    std::uint32_t minimumBudget_ = 0U;
    float qualityScale_ = 1.0f;
    std::uint32_t stableSamples_ = 0U;
    Telemetry telemetry_{};
};

} // namespace epi_sculpt
