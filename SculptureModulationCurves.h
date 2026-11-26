#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace epi_sculpt {

// Stable value identities shared by runtime mathematics and authoring tools.
enum class TransferCurve : std::uint8_t {
    Linear = 0,
    QuadraticEaseIn,
    QuadraticEaseOut,
    Smoothstep,
    Smootherstep,
    BipolarCubic,
    Quantize8Step,
    Invert,
    Sigmoid,
    SinusoidalFold,
    ChebyshevT3
};

namespace modulation_curves {

inline constexpr std::array canonicalNames{
    std::string_view{"linear"},
    std::string_view{"ease_in"},
    std::string_view{"ease_out"},
    std::string_view{"smoothstep"},
    std::string_view{"smootherstep"},
    std::string_view{"cubic"},
    std::string_view{"quantize"},
    std::string_view{"invert"},
    std::string_view{"sigmoid"},
    std::string_view{"fold"},
    std::string_view{"chebyshev"},
};
static_assert(canonicalNames.size() == static_cast<std::size_t>(TransferCurve::ChebyshevT3) + 1);

[[nodiscard]] constexpr std::string_view name(TransferCurve curve) noexcept {
    const auto index = static_cast<std::size_t>(curve);
    return index < canonicalNames.size() ? canonicalNames[index] : "unknown";
}

// Alias spelling changes presentation, never the curve's mathematical identity.
[[nodiscard]] constexpr std::optional<TransferCurve> find(std::string_view text) noexcept {
    for (std::size_t index = 0; index < canonicalNames.size(); ++index)
        if (canonicalNames[index] == text) return static_cast<TransferCurve>(index);
    if (text == "tanh") return TransferCurve::Sigmoid;
    if (text == "sinusoid") return TransferCurve::SinusoidalFold;
    return std::nullopt;
}

} // namespace modulation_curves
} // namespace epi_sculpt
