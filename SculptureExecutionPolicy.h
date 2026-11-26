#pragma once

#include <algorithm>
#include <cstddef>

#include "SculptureTemporalHistory.h"

namespace epi_sculpt {

// CPU fallback must remain predictable for the common small surface snapshot.
// Full voxel fields are large enough to benefit from bounded parallel work.
inline constexpr std::size_t kMinParallelVoxelWorkItems = 2048U;

[[nodiscard]] inline std::size_t sculptureWorkerCount(
    SculptureTemporalFamily family,
    std::size_t workItems,
    std::size_t hardwareWorkers) noexcept {
    (void)family;
    if (workItems < kMinParallelVoxelWorkItems) {
        return 1U;
    }
    return std::min(workItems, std::max<std::size_t>(1U, hardwareWorkers));
}

} // namespace epi_sculpt
