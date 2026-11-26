#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace epi_sculpt {

enum class SculptureTemporalFamily : std::uint8_t { Surface, Voxel };

// Source identity belongs to the producer, independently of atomic render
// compaction. The largest current source domain is the 50^3 voxel lattice.
inline constexpr std::size_t kSculptureSourceHistoryCapacity = 50U * 50U * 50U;

// Matches the private OpenCL history record, not the compact delivery ABI.
// Position w is exactly 1 for an available finite predecessor, otherwise 0.
// Velocity w is reserved; neither record carries renderer material attributes.
struct alignas(16) SculptureSourceSample {
    std::array<float, 4> positionAndValidity{};
    std::array<float, 4> velocity{};

    [[nodiscard]] bool valid() const noexcept {
        if (positionAndValidity[3] != 1.0f) return false;
        for (unsigned i = 0; i < 3; ++i)
            if (!std::isfinite(positionAndValidity[i]) || !std::isfinite(velocity[i])) return false;
        return true;
    }
};

static_assert(sizeof(SculptureSourceSample) == 32U);
static_assert(alignof(SculptureSourceSample) == 16U);
static_assert(offsetof(SculptureSourceSample, velocity) == 16U);

} // namespace epi_sculpt
