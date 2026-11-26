#pragma once

#include "SculpturePresetBoundary.h"
#include "SculpturePresetIdentityIndex.h"
#include "SculptureBoundedSelection.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace epi_sculpt {

inline constexpr std::size_t kMaxPresetBridgeResults = 5;

enum class PresetBridgeError {
    None, InvalidEndpoint, CanonicalEndpoint, NonfiniteEndpoint, RouteMismatch, SelectorMismatch,
    UnexplainableBridge
};

struct PresetBridgeHit {
    std::string id;
    std::string title;
    std::string revision;
    double fromDistance = 0.0;
    double toDistance = 0.0;
    // Owned numeric observations; parameter names refer only to stable registry
    // literals. Neither report borrows the catalog's parameter/asset storage.
    std::optional<SpatialPresetDistanceReport> fromReport;
    std::optional<SpatialPresetDistanceReport> toReport;

    [[nodiscard]] double worstDistance() const noexcept {
        return std::max(fromDistance, toDistance);
    }
    [[nodiscard]] double totalDistance() const noexcept { return fromDistance + toDistance; }
};

// Owned catalog observations, safe after catalog replacement. This report is
// neither a generated preset nor a live transition/canonical-field admission.
struct PreparedPresetBridges {
    PresetBridgeError error = PresetBridgeError::None;
    std::string fromId, toId, fromRevision, toRevision;
    double directDistance = 0.0;
    std::size_t matchCount = 0;
    std::vector<PresetBridgeHit> hits;

    [[nodiscard]] bool ready() const noexcept { return error == PresetBridgeError::None; }
};

// Caller resolves exact unique endpoints, as for the existing neighborhood query.
// Search only existing assets: minimize the larger of two metric steps, rather
// than claim that inserting a preset shortens an additive path or enables morphing.
[[nodiscard]] inline PreparedPresetBridges preparePresetBridges(
    const SculpturePresetAsset& from, const SculpturePresetAsset& to,
    std::span<const SculpturePresetAsset> assets, bool details = false) {
    PreparedPresetBridges result;
    result.fromId = from.id;
    result.toId = to.id;
    result.fromRevision = from.revision;
    result.toRevision = to.revision;
    if (from.id.empty() || to.id.empty()) {
        result.error = PresetBridgeError::InvalidEndpoint;
    } else if (from.canonicalFeedbackMode || to.canonicalFeedbackMode) {
        result.error = PresetBridgeError::CanonicalEndpoint;
    } else if (!spatialPresetParametersFinite(from.params, to.params)) {
        result.error = PresetBridgeError::NonfiniteEndpoint;
    } else if (!sameSpatialPresetRoute(from, to)) {
        result.error = PresetBridgeError::RouteMismatch;
    } else if (const auto distance = spatialPresetDistance(from.params, to.params)) {
        result.directDistance = *distance;
    } else {
        result.error = PresetBridgeError::SelectorMismatch;
    }
    if (!result.ready() || result.directDistance == 0.0) return result;

    const SculpturePresetIdentityIndex identities{assets};
    // Rank only borrowed references and cached costs. One record carries both
    // ranking and explanation identity; no parallel array needs alignment.
    struct Candidate {
        const SculpturePresetAsset* asset;
        double fromDistance, toDistance;
    };
    std::vector<Candidate> selected;
    selected.reserve(kMaxPresetBridgeResults);
    for (const auto& asset : assets) {
        if (asset.id == from.id || asset.id == to.id
            || !identities.isUnique(asset.id) || !sameSpatialPresetRoute(from, asset)) continue;
        const auto first = spatialPresetDistance(from.params, asset.params);
        if (!first) continue;
        const auto second = spatialPresetDistance(asset.params, to.params);
        if (!second) continue;
        const double worst = std::max(*first, *second);
        if (worst >= result.directDistance) continue;
        ++result.matchCount;

        // Domain policy stays here: minimize worst leg, then total, then exact
        // ID, independently of discovery order. Total is not a path objective.
        retainBoundedSorted(selected, Candidate{&asset, *first, *second}, kMaxPresetBridgeResults,
            [](const Candidate& a, const Candidate& b) {
                const double aWorst = std::max(a.fromDistance, a.toDistance);
                const double bWorst = std::max(b.fromDistance, b.toDistance);
                if (aWorst != bWorst) return aWorst < bWorst;
                const double aTotal = a.fromDistance + a.toDistance;
                const double bTotal = b.fromDistance + b.toDistance;
                if (aTotal != bTotal) return aTotal < bTotal;
                return a.asset->id < b.asset->id;
            });
    }
    // Only final winners acquire owned text/reports. All catalog borrows end
    // before return, so the public report remains safe after catalog reload.
    result.hits.reserve(selected.size());
    for (const auto& candidate : selected) {
        const auto& asset = *candidate.asset;
        PresetBridgeHit hit{asset.id, asset.title, asset.revision,
                            candidate.fromDistance, candidate.toDistance};
        if (details) {
            hit.fromReport = inspectSpatialPresetDistance(from.params, asset.params);
            hit.toReport = inspectSpatialPresetDistance(asset.params, to.params);
            if (!hit.fromReport || !hit.toReport) {
                result.error = PresetBridgeError::UnexplainableBridge;
                result.hits.clear();
                return result;
            }
        }
        result.hits.push_back(std::move(hit));
    }
    return result;
}

} // namespace epi_sculpt
