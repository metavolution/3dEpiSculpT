#pragma once

#include <cstddef>

namespace epi_sculpt {

// Cold host admission policy, independent of numerical and presentation state.
// Text size alone cannot bound expansion into full parameter/routing keyframes.
// maxBytes covers sequence text, not referenced preset files or resolver work.
// Either zero value disables import; callers may choose other positive limits.
struct SequenceLoadLimits {
    std::size_t maxBytes = 4U * 1024U * 1024U;
    std::size_t maxKeyframes = 4096U;
};

} // namespace epi_sculpt
