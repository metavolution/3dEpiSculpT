#pragma once

#include "SculpturePresetPathExportCheck.h"

#include <optional>
#include <span>
#include <vector>

namespace epi_sculpt {

struct PresetPathAlternativesExportCheck {
    // Owned observations in input order; no catalog/path borrows or reservation.
    std::vector<PresetPathExportCheck> paths;
    std::optional<std::size_t> recommendedIndex;

    [[nodiscard]] bool assessed() const noexcept { return !paths.empty(); }
    [[nodiscard]] bool ready() const noexcept {
        return recommendedIndex && *recommendedIndex < paths.size() && paths[*recommendedIndex].ready();
    }
};

// Assess an existing bounded set, not a new search or playback authority. Choose
// the gentlest exportable supplied path, then fewer hops, then input order. A
// frontier need not contain every exportable route: no global optimum is claimed.
// Actual export must re-admit its current inputs and destination independently.
[[nodiscard]] inline PresetPathAlternativesExportCheck checkPresetPathAlternativesExport(
    std::span<const SculpturePresetAsset> assets, std::span<const PreparedPresetPath> paths,
    float transitionSeconds = 3.0f, PresetPathTourStyle style = {}) {
    PresetPathAlternativesExportCheck result;
    // Reuse the existing frontier's work bound; refuse rather than truncate.
    if (paths.empty() || paths.size() > kMaxPresetPathHops) return result;
    result.paths.reserve(paths.size());
    for (std::size_t index = 0; index < paths.size(); ++index) {
        result.paths.push_back(checkPresetPathExport(assets, paths[index], transitionSeconds, style));
        const auto& checked = result.paths.back();
        if (!checked.ready()) continue;
        if (result.recommendedIndex) {
            const auto& current = paths[*result.recommendedIndex];
            const auto& candidate = paths[index];
            if (candidate.largestStepDistance > current.largestStepDistance
                || (candidate.largestStepDistance == current.largestStepDistance
                    && candidate.steps.size() >= current.steps.size())) continue;
        }
        // Ready paths already passed the tour owner's finite distance/aggregate,
        // current identity, compatibility and configuration checks.
        result.recommendedIndex = index;
    }
    return result;
}

} // namespace epi_sculpt
