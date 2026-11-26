#pragma once

#include <cmath>
#include <cstddef>

namespace epi_sculpt::detail {

// Keep the sampler's historical rounded-cube inference, including trailing data.
// Prove N^3 fits the backing count by division before constructing any strides.
inline std::size_t voxelSamplingDimension(std::size_t sampleCount) noexcept {
    if (sampleCount < 8U) return 0U;
    const auto dimension = static_cast<std::size_t>(
        std::round(std::cbrt(static_cast<double>(sampleCount))));
    if (dimension < 2U || dimension > sampleCount / dimension / dimension) return 0U;
    return dimension;
}

} // namespace epi_sculpt::detail
