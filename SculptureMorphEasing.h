#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace epi_sculpt {

// Shared authored easing vocabulary. Numeric values are persisted; keep stable.
enum class MorphEasing : std::uint8_t {
    Smoothstep = 0,
    Linear = 1,
    Sinusoidal = 2,
    Exponential = 3,
    ElasticPulse = 4,
    SpringUnderdamped = 5,
    SpringCriticallyDamped = 6,
    SpringOverdamped = 7,
    CubicBezier = 8,
    BackOvershoot = 9,
    BounceOut = 10
};

inline constexpr std::size_t kMorphEasingCount = 11U;

[[nodiscard]] constexpr std::string_view name(MorphEasing easing) noexcept {
    switch (easing) {
    case MorphEasing::Smoothstep: return "Smoothstep";
    case MorphEasing::Linear: return "Linear";
    case MorphEasing::Sinusoidal: return "Sinusoidal";
    case MorphEasing::Exponential: return "Exponential";
    case MorphEasing::ElasticPulse: return "ElasticPulse";
    case MorphEasing::SpringUnderdamped: return "SpringUnderdamped";
    case MorphEasing::SpringCriticallyDamped: return "SpringCriticallyDamped";
    case MorphEasing::SpringOverdamped: return "SpringOverdamped";
    case MorphEasing::CubicBezier: return "CubicBezier";
    case MorphEasing::BackOvershoot: return "BackOvershoot";
    case MorphEasing::BounceOut: return "BounceOut";
    }
    return "Smoothstep";
}

// Exact authored names; invalid input never silently becomes Smoothstep.
[[nodiscard]] constexpr std::optional<MorphEasing> parseMorphEasing(std::string_view text) noexcept {
    for (std::size_t index = 0; index < kMorphEasingCount; ++index) {
        const auto easing = static_cast<MorphEasing>(index);
        if (name(easing) == text) return easing;
    }
    return std::nullopt;
}

} // namespace epi_sculpt
