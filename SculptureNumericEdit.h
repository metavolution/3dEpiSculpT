#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>

namespace epi_sculpt::numeric_edit {

// Prepare representable command values before changing any live owner. Clamps
// belong to each control's existing policy; clamping NaN or overflow is not
// admission. These helpers allocate nothing and retain no state or references.
[[nodiscard]] inline std::optional<float> prepareFiniteSum(float current, float delta) noexcept {
    if (!std::isfinite(current) || !std::isfinite(delta)) return std::nullopt;
    const float candidate = current + delta;
    if (!std::isfinite(candidate)) return std::nullopt;
    return candidate;
}

inline bool setFinite(float& target, float value) noexcept {
    if (!std::isfinite(value)) return false;
    target = value;
    return true;
}

inline bool addFinite(float& target, float delta) noexcept {
    const auto candidate = prepareFiniteSum(target, delta);
    if (!candidate) return false;
    target = *candidate;
    return true;
}

inline bool addFiniteClamped(float& target, float delta, float lower, float upper) noexcept {
    if (!std::isfinite(lower) || !std::isfinite(upper) || lower > upper) return false;
    const auto candidate = prepareFiniteSum(target, delta);
    if (!candidate) return false;
    target = std::clamp(*candidate, lower, upper);
    return true;
}

template <std::size_t N>
[[nodiscard]] std::optional<std::array<float, N>> prepareFiniteSums(
    const std::array<float, N>& current, const std::array<float, N>& deltas) noexcept {
    std::array<float, N> candidate{};
    for (std::size_t i = 0; i < N; ++i) {
        const auto value = prepareFiniteSum(current[i], deltas[i]);
        if (!value) return std::nullopt;
        candidate[i] = *value;
    }
    return candidate;
}

} // namespace epi_sculpt::numeric_edit
