#pragma once

#include "SculpturePresetPath.h"

namespace epi_sculpt {

struct PresetPathFrontierRequest {
    double maxDistance = kDefaultPresetPathMaxDistance;
    std::size_t maxHops = kDefaultPresetPathHops;
    bool details = false;
};

struct PreparedPresetPathFrontier {
    PresetPathFrontierRequest request{};
    PresetPathError error = PresetPathError::NoPath;
    std::size_t candidateCount = 0;
    // Owned routes, increasing hops and strictly decreasing largest step.
    // Each path's request records the smallest valid hop budget that achieved
    // it (one for a zero-hop self-query).
    std::vector<PreparedPresetPath> routes;
    [[nodiscard]] bool ready() const noexcept { return error == PresetPathError::None; }
};

// For each hop budget h, b[h] = min_path(max_edge_distance(path)). Retain only
// strict improvements: an equal bottleneck at more hops is dominated. A retained
// route cannot contain a cycle: removing it would achieve no larger bottleneck
// at fewer hops, contradicting that strict improvement. Ties use the existing
// catalog-scan policy, not total distance or an assumed symmetric metric.
//
// One directed distance grid and one layered search serve the whole frontier:
// O(N^2 * metric_cost + H*N^2), plus bounded result/explanation materialization.
// N <= 512, H <= 16. This is parameter-space inspection, not playback admission.
[[nodiscard]] inline PreparedPresetPathFrontier preparePresetPathFrontier(
    std::span<const SculpturePresetAsset> assets, std::string_view fromId,
    std::string_view toId, PresetPathFrontierRequest request = {}) {
    PreparedPresetPathFrontier result;
    result.request = request;
    const auto best = preset_path_detail::search(assets, fromId, toId,
        {request.maxDistance, request.maxHops, request.details, PresetPathObjective::GentlestSteps},
        &result.routes);
    result.error = best.error;
    result.candidateCount = best.candidateCount;
    if (!best.ready()) result.routes.clear(); // Never publish an incomplete frontier.
    return result;
}

} // namespace epi_sculpt
