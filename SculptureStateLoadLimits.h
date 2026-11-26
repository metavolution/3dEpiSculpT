#pragma once

#include <cstddef>

namespace epi_sculpt {

// Cold host policy for state-file acquisition, separate from format semantics.
// Bounds source bytes, not decoded object overhead or retained GPU history.
// Zero disables file input; hosts may explicitly choose another positive budget.
struct StateLoadLimits {
    std::size_t maxBytes = 8U * 1024U * 1024U;
};

} // namespace epi_sculpt
