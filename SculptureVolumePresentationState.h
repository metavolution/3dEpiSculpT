#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace epi_sculpt {

// Portable presentation intent, not evidence of an actually rendered payload.
// Numeric IDs are stable serialized values; native conversion checks them again.
struct SculptureVolumePresentationState {
    std::uint32_t recipe = 0U;
    std::uint32_t colorChannel = 3U;
    std::array<float, 4> lowColorAndMinimum{0.1f, 0.5f, 1.0f, 0.0f};
    std::array<float, 4> highColorAndMaximum{1.0f, 0.2f, 0.5f, 1.0f};
    std::array<float, 4> optics{0.12f, 2.5f, 0.65f, 0.2f};

    [[nodiscard]] bool valid() const noexcept {
        for (std::size_t i = 0; i < 4; ++i) {
            if (!std::isfinite(lowColorAndMinimum[i]) || !std::isfinite(highColorAndMaximum[i]) ||
                !std::isfinite(optics[i])) return false;
            if (i < 3 && (lowColorAndMinimum[i] < 0.0f || lowColorAndMinimum[i] > 1.0f ||
                          highColorAndMaximum[i] < 0.0f || highColorAndMaximum[i] > 1.0f)) return false;
        }
        const float range = highColorAndMaximum[3] - lowColorAndMinimum[3];
        return recipe <= 4U && colorChannel <= 3U && std::isfinite(range) &&
            range >= std::numeric_limits<float>::min() &&
            optics[0] >= 0.0f && optics[0] <= 16.0f && optics[1] >= 1.0f && optics[1] <= 16.0f &&
            std::abs(optics[2]) <= 0.95f && optics[3] >= 0.0f && optics[3] <= 0.5f;
    }
    [[nodiscard]] bool operator==(const SculptureVolumePresentationState&) const noexcept = default;
};

} // namespace epi_sculpt
