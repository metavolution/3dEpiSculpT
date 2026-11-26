// Bounded, renderer-neutral lifecycle for a comparable world performance run.
#pragma once

#include "SculptureWorldProfile.h"

#include <algorithm>
#include <cstdint>
#include <utility>

namespace epi_sculpt {

struct WorldProfileSessionConfig {
    float settleSeconds = 2.0f;
    float sampleSeconds = 8.0f;
};

enum class WorldProfileSessionState : std::uint8_t {
    Settling,
    Sampling,
    Complete,
    Aborted,
};

// Owns timing-window policy only. Callers retain application, GPU, and output
// ownership; therefore a profile request cannot change the visual program.
class SculptureWorldProfileSession {
public:
    SculptureWorldProfileSession(WorldProfileIdentity identity,
                                 WorldProfileSessionConfig config = {})
        : profile_(std::move(identity)), config_(normalize(config)) {}

    void advance(float deltaSeconds) noexcept {
        if (state_ == WorldProfileSessionState::Complete
            || state_ == WorldProfileSessionState::Aborted) return;
        elapsedSeconds_ += std::max(0.0f, deltaSeconds);
        if (elapsedSeconds_ < config_.settleSeconds) return;
        state_ = WorldProfileSessionState::Sampling;
        if (elapsedSeconds_ >= config_.settleSeconds + config_.sampleSeconds) {
            state_ = WorldProfileSessionState::Complete;
        }
    }

    [[nodiscard]] bool observe(const WorldGenerationMetrics& generation,
                               const WorldBridgeMetrics& bridge,
                               const WorldPresentationMetrics* presentation = nullptr) noexcept {
        return state_ == WorldProfileSessionState::Sampling
            && profile_.observe(generation, bridge, presentation);
    }

    [[nodiscard]] WorldProfileSessionState state() const noexcept { return state_; }
    [[nodiscard]] bool complete() const noexcept { return state_ == WorldProfileSessionState::Complete; }
    [[nodiscard]] bool finished() const noexcept {
        return state_ == WorldProfileSessionState::Complete
            || state_ == WorldProfileSessionState::Aborted;
    }
    void abort() noexcept {
        if (finished()) return;
        profile_.markInterrupted();
        state_ = WorldProfileSessionState::Aborted;
    }
    [[nodiscard]] bool hasSamples() const noexcept {
        return profile_.summary().acceptedGenerationCount != 0U;
    }
    [[nodiscard]] const WorldProfileSummary& summary() const noexcept { return profile_.summary(); }

private:
    [[nodiscard]] static WorldProfileSessionConfig normalize(WorldProfileSessionConfig value) noexcept {
        value.settleSeconds = std::max(0.0f, value.settleSeconds);
        value.sampleSeconds = std::max(0.001f, value.sampleSeconds);
        return value;
    }
    SculptureWorldProfile profile_;
    WorldProfileSessionConfig config_{};
    WorldProfileSessionState state_ = WorldProfileSessionState::Settling;
    float elapsedSeconds_ = 0.0f;
};

} // namespace epi_sculpt
