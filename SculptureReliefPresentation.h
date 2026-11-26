#pragma once

#include <cstdint>

namespace epi_sculpt {

// Fixed construction-owned quality; changing it requires a new bridge.
enum class SculptureReliefQuality : std::uint8_t { Standard128, Detailed384 };

[[nodiscard]] constexpr bool validSculptureReliefQuality(SculptureReliefQuality quality) noexcept {
    return quality == SculptureReliefQuality::Standard128 || quality == SculptureReliefQuality::Detailed384;
}

[[nodiscard]] constexpr std::uint32_t sculptureReliefGridResolution(SculptureReliefQuality quality) noexcept {
    switch (quality) {
    case SculptureReliefQuality::Standard128: return 128U;
    case SculptureReliefQuality::Detailed384: return 384U;
    }
    return 0U;
}

} // namespace epi_sculpt
