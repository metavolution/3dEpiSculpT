#pragma once

#include "SculpturePresetBoundary.h"
#include "SculpturePresetIdentityIndex.h"
#include "SculpturePresetInspectionLimits.h"
#include "SculptureBoundedSelection.h"

#include <cstddef>
#include <cmath>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace epi_sculpt {

struct PresetNeighborhoodRequest {
    std::size_t limit = kDefaultPresetNeighborLimit;
    bool details = false;
    // Inclusive bound on the existing normalized spatial distance, when set.
    std::optional<double> maxDistance = std::nullopt;
    std::optional<double> minDistance = std::nullopt;
};

enum class PresetNeighborhoodError {
    None, InvalidLimit, InvalidMaxDistance, InvalidMinDistance, InvalidDistanceRange,
    NonfiniteSource, UnexplainableNeighbor
};

struct PresetNeighborHit {
    const SculpturePresetAsset* asset = nullptr;
    double distance = 0.0;
};

// A derived query result, never a catalog owner or simulation authority. Hit and
// error pointers borrow input storage, which must outlive consumption and stay
// unchanged. Reports own values and stable registry/literal names only.
struct PreparedPresetNeighborhood {
    PresetNeighborhoodRequest request{};
    PresetNeighborhoodError error = PresetNeighborhoodError::None;
    const SculpturePresetAsset* errorAsset = nullptr;
    std::size_t matchCount = 0;
    std::vector<PresetNeighborHit> hits;
    std::vector<SpatialPresetDistanceReport> reports;

    [[nodiscard]] bool ready() const noexcept { return error == PresetNeighborhoodError::None; }
};

// The caller resolves the exact unique source identity. Candidate identities,
// route eligibility and stable nearest-K selection have this one query owner.
// Allocation exceptions propagate; error results never publish partial rows.
[[nodiscard]] inline PreparedPresetNeighborhood preparePresetNeighborhood(
    const SculpturePresetAsset& source, std::span<const SculpturePresetAsset> assets,
    PresetNeighborhoodRequest request = {}) {
    const auto failure = [&](PresetNeighborhoodError error, const SculpturePresetAsset* asset = nullptr) {
        PreparedPresetNeighborhood result;
        result.request = request;
        result.error = error;
        result.errorAsset = asset;
        return result;
    };
    if (request.limit == 0 || request.limit > kMaxPresetNeighborLimit)
        return failure(PresetNeighborhoodError::InvalidLimit);
    if (request.maxDistance && (!std::isfinite(*request.maxDistance) || *request.maxDistance < 0.0))
        return failure(PresetNeighborhoodError::InvalidMaxDistance);
    if (request.minDistance && (!std::isfinite(*request.minDistance) || *request.minDistance < 0.0))
        return failure(PresetNeighborhoodError::InvalidMinDistance);
    if (request.minDistance && request.maxDistance && *request.minDistance > *request.maxDistance)
        return failure(PresetNeighborhoodError::InvalidDistanceRange);
    if (!spatialPresetParametersFinite(source.params))
        return failure(PresetNeighborhoodError::NonfiniteSource, &source);

    const SculpturePresetIdentityIndex identities{assets};
    PreparedPresetNeighborhood result;
    result.request = request;
    result.hits.reserve(request.limit);
    for (const auto& asset : assets) {
        if (asset.id == source.id || !identities.isUnique(asset.id)
            || !sameSpatialPresetRoute(source, asset)) continue;
        if (const auto distance = spatialPresetDistance(source.params, asset.params)) {
            if (request.minDistance && *distance < *request.minDistance) continue;
            if (request.maxDistance && *distance > *request.maxDistance) continue;
            ++result.matchCount;
            retainBoundedSorted(result.hits, PresetNeighborHit{&asset, *distance}, request.limit,
                [](const auto& a, const auto& b) { return a.distance < b.distance; });
        }
    }
    if (request.details) {
        result.reports.reserve(result.hits.size());
        for (const auto& hit : result.hits) {
            const auto report = inspectSpatialPresetDistance(source.params, hit.asset->params);
            if (!report) return failure(PresetNeighborhoodError::UnexplainableNeighbor, hit.asset);
            result.reports.push_back(*report);
        }
    }
    return result;
}

} // namespace epi_sculpt
