#pragma once

#include "SculpturePresetLibrary.h"

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace epi_sculpt {

struct ComparedTourCue {
    std::string presetId;
    std::optional<std::size_t> fromIndex;
    std::optional<std::size_t> toIndex;
    std::optional<SculptureTourEntryStatus> fromStatus;
    std::optional<SculptureTourEntryStatus> toStatus;
};

// Owned inspection output: input snapshots/catalog storage may be released after
// comparison. Shared references do not certify asset availability or morphability.
struct TourCueComparison {
    std::size_t fromCount = 0, toCount = 0;
    std::size_t fromReady = 0, toReady = 0;
    std::size_t shared = 0, fromOnly = 0, toOnly = 0;
    std::vector<ComparedTourCue> cues;
};

// Occurrence pairing is intentionally not set subtraction: A,A versus A has one
// shared and one source-only reference. FIFO indices preserve that distinction in
// expected O(n+m) work, without sorting or changing either tour's declared order.
[[nodiscard]] inline TourCueComparison compareTourCues(
    std::span<const SculptureTourEntry> from, std::span<const SculptureTourEntry> to) {
    struct Occurrences {
        std::vector<std::size_t> indices;
        std::size_t next = 0;
    };
    std::unordered_map<std::string_view, Occurrences> destinations;
    destinations.reserve(to.size());
    std::vector<bool> paired(to.size(), false);
    TourCueComparison result;
    result.fromCount = from.size();
    result.toCount = to.size();
    result.cues.reserve(from.size() + to.size());
    for (std::size_t index = 0; index < to.size(); ++index) {
        if (!to[index].presetId.empty())
            destinations[to[index].presetId].indices.push_back(index);
        if (to[index].status == SculptureTourEntryStatus::Ready) ++result.toReady;
    }
    for (std::size_t index = 0; index < from.size(); ++index) {
        const auto& entry = from[index];
        ComparedTourCue cue{entry.presetId, index, std::nullopt, entry.status, std::nullopt};
        const auto found = destinations.find(entry.presetId);
        if (found != destinations.end() && found->second.next < found->second.indices.size()) {
            auto& occurrences = found->second;
            const auto target = occurrences.indices[occurrences.next++];
            cue.toIndex = target;
            cue.toStatus = to[target].status;
            paired[target] = true;
            ++result.shared;
        } else {
            ++result.fromOnly;
        }
        if (entry.status == SculptureTourEntryStatus::Ready) ++result.fromReady;
        result.cues.push_back(std::move(cue));
    }
    for (std::size_t index = 0; index < to.size(); ++index) {
        if (paired[index]) continue;
        result.cues.push_back({to[index].presetId, std::nullopt, index,
                               std::nullopt, to[index].status});
        ++result.toOnly;
    }
    return result;
}

} // namespace epi_sculpt
