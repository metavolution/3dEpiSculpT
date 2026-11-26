#pragma once

#include <algorithm>
#include <cmath>

namespace epi_sculpt::tour_timing {

inline constexpr float kMinimumCueDurationSeconds = 0.01f;

// Playback's existing clamp, shared by morph durations and spline intervals.
// Raw-input admission is separate: a decoder may reject negative durations,
// while live authoring retains its finite short/negative-duration clamp.
[[nodiscard]] constexpr float effectiveCueDuration(float seconds) noexcept {
    return std::max(kMinimumCueDurationSeconds, seconds);
}

// An admitted cue must occupy a nonempty representable interval [total, next).
// Finite durations alone are insufficient: float addition can overflow or erase
// a short interval after a long one. Failure never changes the caller's prefix.
[[nodiscard]] inline bool tryAppendCueDuration(float& total, float seconds) noexcept {
    if (!std::isfinite(total) || total < 0.0f || !std::isfinite(seconds)) return false;
    const float next = total + effectiveCueDuration(seconds);
    if (!std::isfinite(next) || next <= total) return false;
    total = next;
    return true;
}

} // namespace epi_sculpt::tour_timing
