// Resource-free observation values shared by authoring, controls and presentation.
// Resolving this intent never changes mathematical fields, cadence or history.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace epi_sculpt {
inline constexpr float kMaxReliefContourDensity = 64.0f;
inline constexpr std::array<const char*, 3> kReliefContourStyleNames{
    "ink", "signed L-0.5: blue/neutral/orange", "major every 4th level"
};

struct ReliefContourSettings final {
    float density = 0.0f;
    std::uint32_t style = 0U;
    bool operator==(const ReliefContourSettings&) const = default;
};

[[nodiscard]] inline float resolveReliefContourDensity(float requested) noexcept {
    return std::isfinite(requested)
        ? std::clamp(requested, 0.0f, kMaxReliefContourDensity) : 0.0f;
}

[[nodiscard]] constexpr std::uint32_t resolveReliefContourStyle(std::uint32_t style) noexcept {
    return style < kReliefContourStyleNames.size() ? style : 0U;
}

[[nodiscard]] inline ReliefContourSettings resolveReliefContourSettings(
    ReliefContourSettings requested) noexcept {
    return {resolveReliefContourDensity(requested.density), resolveReliefContourStyle(requested.style)};
}

[[nodiscard]] constexpr const char* reliefContourStyleName(std::uint32_t style) noexcept {
    return kReliefContourStyleNames[resolveReliefContourStyle(style)];
}
} // namespace epi_sculpt
