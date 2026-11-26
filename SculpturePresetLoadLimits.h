#pragma once

#include <cstddef>

namespace epi_sculpt {

// Cold host acquisition policy, independent of format/math and query limits.
// All catalog budgets are inclusive and shared across both discovery roots.
// Exhaustion rejects the candidate as a whole; never publish a truncated catalog.
// These bound discovery and source snapshots, not wall time or exact decoded RSS.
struct PresetLoadLimits {
    // Per-file limit. Zero disables content reads while retaining discovery.
    std::size_t maxBytes = 8U * 1024U * 1024U;
    // Every visited entry counts, including directories and unrelated files.
    std::size_t maxDirectoryEntries = 65536U;
    // Root children have depth zero, matching recursive_directory_iterator.
    std::size_t maxDirectoryDepth = 64U;
    // All regular .epimode candidates count, even rejected/duplicate-ID files.
    // Built-in and root-pattern assets do not consume this budget.
    std::size_t maxPresetFiles = 4096U;
    // Successfully acquired snapshot bytes, charged before decode/deduplication.
    // One bounded file is acquired before its aggregate charge can be decided.
    // This is not a count of physical IO, failed reads or allocator overhead.
    std::size_t maxTotalSourceBytes = 256U * 1024U * 1024U;
};

} // namespace epi_sculpt
