// -----------------------------------------------------------------------------
// Educational Annotation: RUNTIME TIME-FLOW & STATE GOVERNANCE
// -----------------------------------------------------------------------------
// Decouples wall-clock display refresh rates (e.g. 60Hz, 120Hz, 144Hz VR) from
// mathematical simulation time. Provides deterministic simulation advancement,
// variable playback rate scaling, pause/resume state management, interactive
// timeline steering, and exact restored world-state overrides.
// -----------------------------------------------------------------------------

#pragma once

#include "SculpturePlaybackRate.h"
#include "SculptureWorldStateOverride.h"
#include <cmath>
#include <cstdint>
#include <limits>

namespace epi_sculpt {

class SculptureRuntimeFlow {
public:
    [[nodiscard]] float advance(bool cinematic, float wallDeltaSeconds) noexcept {
        if (paused_ || !std::isfinite(wallDeltaSeconds)) return 0.0f;
        const float simulationDelta = playbackRate_.scale(wallDeltaSeconds);
        if (!std::isfinite(simulationDelta)) return 0.0f;
        const double nextSimulationTime = simulationTime_ + static_cast<double>(simulationDelta);
        const double nextCinematicTime = cinematic
            ? cinematicTime_ + static_cast<double>(simulationDelta) : cinematicTime_;
        // Persisted state, producer inputs and GPU ABI use float. Reject an
        // unrepresentable transition before changing either clock or generation.
        if (!representableTime(nextSimulationTime) || !representableTime(nextCinematicTime))
            return 0.0f;
        simulationTime_ = nextSimulationTime;
        cinematicTime_ = nextCinematicTime;
        ++frameGeneration_;
        return simulationDelta;
    }

    void togglePause() noexcept {
        paused_ = !paused_;
        if (!paused_) releaseCapturedWorld();
    }

    void restoreCapturedWorld(const SculptureWorldState& state) noexcept {
        simulationTime_ = state.time;
        restoredWorldState_.restore(state);
        paused_ = true;
    }

    void releaseCapturedWorld() noexcept { restoredWorldState_.clear(); }

    // Report admission so a command can commit a paired manual edit only when
    // its clock transition also succeeds. Ignoring the result remains valid.
    bool steer(float deltaSeconds) noexcept {
        if (!std::isfinite(deltaSeconds)) return false;
        const double nextTime = simulationTime_ + static_cast<double>(deltaSeconds);
        if (!representableTime(nextTime)) return false;
        simulationTime_ = nextTime;
        if (deltaSeconds != 0.0f) releaseCapturedWorld();
        return true;
    }

    [[nodiscard]] SculptureWorldState resolve(
        const SculptureWorldState& liveState) const noexcept {
        return restoredWorldState_.resolve(liveState);
    }

    [[nodiscard]] bool paused() const noexcept { return paused_; }
    [[nodiscard]] float simulationTime() const noexcept { return static_cast<float>(simulationTime_); }
    [[nodiscard]] float cinematicTime() const noexcept { return static_cast<float>(cinematicTime_); }
    [[nodiscard]] std::uint64_t frameGeneration() const noexcept { return frameGeneration_; }
    [[nodiscard]] SculpturePlaybackRate& playbackRate() noexcept { return playbackRate_; }
    [[nodiscard]] const SculpturePlaybackRate& playbackRate() const noexcept { return playbackRate_; }

private:
    [[nodiscard]] static bool representableTime(double value) noexcept {
        constexpr double limit = std::numeric_limits<float>::max();
        return std::isfinite(value) && value >= -limit && value <= limit;
    }

    bool paused_ = false;
    // Keep sub-float increments in the application clock; conversion happens
    // only at the existing persisted-state and producer-facing boundary.
    double simulationTime_ = 0.0;
    double cinematicTime_ = 0.0;
    std::uint64_t frameGeneration_ = 0;
    SculpturePlaybackRate playbackRate_{};
    SculptureWorldStateOverride restoredWorldState_{};
};

} // namespace epi_sculpt
