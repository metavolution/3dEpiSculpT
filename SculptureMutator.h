#pragma once

#include "SculptureProgramSchema.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <random>

namespace epi_sculpt {

class SculptureMutator {
public:
    explicit SculptureMutator(unsigned int seed = 42)
        : rng_(seed) {}

    void reseed(unsigned int seed = 42) noexcept {
        rng_.seed(seed);
    }

    void setStrength(float s) noexcept {
        if (std::isfinite(s)) strength_ = std::clamp(s, 0.0f, 1.0f);
    }

    [[nodiscard]] float strength() const noexcept {
        return strength_;
    }

    // An owned observation of the next candidate, not a reservation/commit
    // ticket. With unchanged source/settings/history the next mutation matches.
    [[nodiscard]] std::optional<SculptureProgramParams> previewMutation(
        const SculptureProgramParams& source, float customStrength = -1.0f) const noexcept {
        auto preview = *this;
        return preview.tryMutate(source, customStrength);
    }

    // Reject before consuming stochastic history. Only fields this operation
    // uses are admitted; this is not validation/repair of the whole program.
    [[nodiscard]] std::optional<SculptureProgramParams> tryMutate(
        const SculptureProgramParams& source, float customStrength = -1.0f) noexcept {
        if (!std::isfinite(customStrength) || !finiteFields(source)) return std::nullopt;
        const float s = (customStrength >= 0.0f) ? std::clamp(customStrength, 0.0f, 1.0f) : strength_;
        if (!std::isfinite(s)) return std::nullopt;
        auto candidateRng = rng_;
        auto p = source;
        std::uniform_real_distribution<float> dist(-s, s);

        p.worldScale = std::clamp(p.worldScale + dist(candidateRng) * 0.3f, 0.3f, 1.9f);
        p.radiusScale = std::clamp(p.radiusScale + dist(candidateRng) * 0.2f, 0.5f, 1.8f);
        p.chromaBreath = std::clamp(p.chromaBreath + dist(candidateRng) * 0.25f, 0.0f, 1.0f);
        p.lumaDrift = std::clamp(p.lumaDrift + dist(candidateRng) * 0.25f, 0.0f, 1.0f);
        p.hueRotation = std::fmod(std::abs(p.hueRotation + dist(candidateRng) * 0.2f), 1.0f);
        p.latentW = std::clamp(p.latentW + dist(candidateRng) * 0.4f, -2.0f, 2.0f);
        p.topologyBlend = std::clamp(p.topologyBlend + dist(candidateRng) * 0.3f, 0.0f, 1.0f);
        
        const int deltaIter = static_cast<int>(dist(candidateRng) * 2.0f);
        // Input is uint32; widening before addition avoids narrowing/overflow
        // even for malformed iteration counts, while retaining the [1,8] policy.
        p.feedbackIterations = static_cast<std::uint32_t>(
            std::clamp<std::int64_t>(static_cast<std::int64_t>(p.feedbackIterations) + deltaIter, 1, 8)
        );

        if (!finiteFields(p)) return std::nullopt;
        rng_ = candidateRng;
        return p;
    }

    // Existing GuidedTour/other consumers retain the hold-on-refusal value API.
    SculptureProgramParams mutate(SculptureProgramParams p, float customStrength = -1.0f) {
        return tryMutate(p, customStrength).value_or(p);
    }

private:
    static bool finiteFields(const SculptureProgramParams& p) noexcept {
        for (float value : {p.worldScale, p.radiusScale, p.chromaBreath, p.lumaDrift,
                            p.hueRotation, p.latentW, p.topologyBlend})
            if (!std::isfinite(value)) return false;
        return true;
    }
    std::mt19937 rng_;
    float strength_ = 0.35f;
};

} // namespace epi_sculpt
