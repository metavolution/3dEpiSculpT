#pragma once

namespace epi_sculpt {

// Hardware-independent transport policy shared by all sculpture frontends.
inline constexpr float kLivePlaybackRateStep = 1.2f;
inline constexpr float kLivePlaybackRateMinimum = 0.0625f;
inline constexpr float kLivePlaybackRateMaximum = 16.0f;

} // namespace epi_sculpt
