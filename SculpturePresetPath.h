#pragma once

#include "SculpturePresetBoundary.h"
#include "SculpturePresetIdentityIndex.h"
#include "SculpturePresetInspectionLimits.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace epi_sculpt {

struct PresetPathRequest {
    double maxDistance = kDefaultPresetPathMaxDistance;
    std::size_t maxHops = kDefaultPresetPathHops;
    bool details = false;
    PresetPathObjective objective = PresetPathObjective::FewestHops;
};

enum class PresetPathError {
    None, InvalidDistance, InvalidHops, CatalogLimit, EndpointUnavailable,
    AmbiguousEndpoint, NonfiniteEndpoint, IncompatibleEndpoints, CandidateLimit, NoPath,
    UnexplainableStep, InvalidObjective
};

[[nodiscard]] constexpr std::string_view presetPathErrorName(PresetPathError error) noexcept {
    switch (error) {
    case PresetPathError::None: return "none";
    case PresetPathError::InvalidDistance: return "invalid-distance";
    case PresetPathError::InvalidHops: return "invalid-hops";
    case PresetPathError::CatalogLimit: return "catalog-limit";
    case PresetPathError::EndpointUnavailable: return "endpoint-unavailable";
    case PresetPathError::AmbiguousEndpoint: return "ambiguous-endpoint";
    case PresetPathError::NonfiniteEndpoint: return "nonfinite-endpoint";
    case PresetPathError::IncompatibleEndpoints: return "incompatible-endpoints";
    case PresetPathError::CandidateLimit: return "candidate-limit";
    case PresetPathError::NoPath: return "no-path-within-bounds";
    case PresetPathError::UnexplainableStep: return "unexplainable-step";
    case PresetPathError::InvalidObjective: return "invalid-objective";
    }
    return "unknown";
}

struct PresetPathStep {
    std::string id, revision;
    double previousDistance = 0.0;
    // Only an incoming edge can have a report. Names borrow stable registry
    // literals, never catalog storage; numeric explanations are owned values.
    std::optional<SpatialPresetDistanceReport> details;
};

// Owned, cold inspection values survive catalog reload. This is neither a
// playback plan nor proof of morph/field compatibility or visual smoothness.
struct PreparedPresetPath {
    PresetPathRequest request{};
    PresetPathError error = PresetPathError::NoPath;
    std::size_t candidateCount = 0;
    double totalDistance = 0.0;
    double largestStepDistance = 0.0;
    std::vector<PresetPathStep> steps;
    [[nodiscard]] bool ready() const noexcept { return error == PresetPathError::None; }
};

// Shared structural admission for consumers of the computed-path read model.
// Search paths are acyclic; general authored playlists may repeat cues and do
// not use this contract. Shape is separate from metric/explanation consistency,
// current-catalog identity and morph/codec readiness, which consumers still own.
[[nodiscard]] inline bool presetPathStructureAdmitted(const PreparedPresetPath& path) noexcept {
    if (!path.ready() || path.steps.empty() || path.steps.size() > kMaxPresetPathHops + 1
        || path.request.maxHops == 0 || path.request.maxHops > kMaxPresetPathHops
        || path.steps.size() - 1 > path.request.maxHops
        || path.candidateCount < path.steps.size() || path.candidateCount > kMaxPresetPathCandidates
        || !std::isfinite(path.request.maxDistance) || path.request.maxDistance < 0
        || (path.request.objective != PresetPathObjective::FewestHops
            && path.request.objective != PresetPathObjective::GentlestSteps)) return false;
    // At most seventeen steps: a bounded scan needs no identity table or heap.
    for (std::size_t i = 0; i < path.steps.size(); ++i) {
        if (path.steps[i].id.empty()) return false;
        for (std::size_t j = 0; j < i; ++j)
            if (path.steps[j].id == path.steps[i].id) return false;
    }
    return true;
}

// Default BFS minimizes hop count, retaining catalog discovery order on ties.
// GentlestSteps minimizes the maximum edge distance, then hops. A bounded layered
// search keeps separate labels per hop: one best label per node is insufficient
// when a cheaper prefix may exhaust the hop budget. Neither minimizes total cost.
// Explicit bounds refuse oversized inputs before graph traversal, rather than
// silently truncating the candidate graph and calling a partial search complete.
namespace preset_path_detail {
// Optional improvements are collected only by the cold frontier query. Public
// callers publish them transactionally after this complete search succeeds.
[[nodiscard]] inline PreparedPresetPath search(
    std::span<const SculpturePresetAsset> assets, std::string_view fromId,
    std::string_view toId, PresetPathRequest request,
    std::vector<PreparedPresetPath>* improvements) {
    const auto failure = [&](PresetPathError error) {
        PreparedPresetPath result; result.request = request; result.error = error;
        return result;
    };
    if (!std::isfinite(request.maxDistance) || request.maxDistance < 0.0)
        return failure(PresetPathError::InvalidDistance);
    if (request.maxHops == 0 || request.maxHops > kMaxPresetPathHops)
        return failure(PresetPathError::InvalidHops);
    if (request.objective != PresetPathObjective::FewestHops
        && request.objective != PresetPathObjective::GentlestSteps)
        return failure(PresetPathError::InvalidObjective);
    if (assets.size() > kMaxPresetPathCatalog) return failure(PresetPathError::CatalogLimit);
    const SculpturePresetIdentityIndex identities{assets};
    const auto fromIdentity = identities.resolveExactId(fromId), toIdentity = identities.resolveExactId(toId);
    if (fromIdentity.status == SculpturePresetIdStatus::Missing || toIdentity.status == SculpturePresetIdStatus::Missing)
        return failure(PresetPathError::EndpointUnavailable);
    if (fromIdentity.status != SculpturePresetIdStatus::Found || toIdentity.status != SculpturePresetIdStatus::Found)
        return failure(PresetPathError::AmbiguousEndpoint);
    const auto& from = *fromIdentity.asset;
    const auto& to = *toIdentity.asset;
    if (!spatialPresetParametersFinite(from.params) || !spatialPresetParametersFinite(to.params))
        return failure(PresetPathError::NonfiniteEndpoint);
    if (!sameSpatialPresetRoute(from, to) || !sameSpatialPresetSelectors(from.params, to.params))
        return failure(PresetPathError::IncompatibleEndpoints);

    std::vector<const SculpturePresetAsset*> candidates;
    std::size_t start = 0, goal = 0;
    for (const auto& asset : assets) {
        if (!identities.isUnique(asset.id)
            || !sameSpatialPresetRoute(from, asset) || !sameSpatialPresetSelectors(from.params, asset.params)
            || !spatialPresetParametersFinite(asset.params)) continue;
        if (candidates.size() == kMaxPresetPathCandidates) return failure(PresetPathError::CandidateLimit);
        if (asset.id == fromId) start = candidates.size();
        if (asset.id == toId) goal = candidates.size();
        candidates.push_back(&asset);
    }
    const auto unseen = std::numeric_limits<std::size_t>::max();
    using PathNode = std::pair<std::size_t, double>;
    const auto finish = [&](const std::vector<PathNode>& route) {
        PreparedPresetPath result; result.request = request;
        result.error = PresetPathError::None; result.candidateCount = candidates.size();
        for (std::size_t i = 0; i < route.size(); ++i) {
            const auto [index, distance] = route[i];
            PresetPathStep step{candidates[index]->id, candidates[index]->revision, distance, std::nullopt};
            if (request.details && i != 0) {
                step.details = inspectSpatialPresetDistance(candidates[route[i - 1].first]->params, candidates[index]->params);
                if (!step.details || step.details->distance != distance)
                    return failure(PresetPathError::UnexplainableStep);
            }
            result.steps.push_back(std::move(step));
            result.totalDistance += distance;
            result.largestStepDistance = std::max(result.largestStepDistance, distance);
        }
        return result;
    };
    if (start == goal) {
        auto result = finish({{start, 0.0}});
        if (improvements && result.ready()) {
            improvements->push_back(result);
            improvements->back().request.maxHops = 1U; // Smallest valid budget for a zero-hop query.
        }
        return result;
    }
    if (request.objective == PresetPathObjective::GentlestSteps) {
        const auto count = candidates.size();
        const double infinity = std::numeric_limits<double>::infinity();
        // At most 512^2 doubles (~2 MiB), with no native resources or readback.
        // Cache actual directed distances once; do not assume metric symmetry.
        std::vector<double> edges(count * count, infinity);
        for (std::size_t fromIndex = 0; fromIndex < count; ++fromIndex) {
            for (std::size_t toIndex = 0; toIndex < count; ++toIndex) {
                if (fromIndex == toIndex) continue;
                const auto distance = spatialPresetDistance(candidates[fromIndex]->params, candidates[toIndex]->params);
                if (distance && *distance <= request.maxDistance)
                    edges[fromIndex * count + toIndex] = *distance;
            }
        }
        std::vector<double> previous(count, infinity), next(count, infinity);
        std::vector<std::size_t> parents((request.maxHops + 1) * count, unseen);
        const auto reconstruct = [&](std::size_t hops) {
            std::vector<PathNode> route;
            auto index = goal;
            for (auto hop = hops; hop != 0; --hop) {
                const auto parent = parents[hop * count + index];
                route.emplace_back(index, edges[parent * count + index]);
                index = parent;
            }
            route.emplace_back(start, 0.0);
            std::reverse(route.begin(), route.end());
            return route;
        };
        previous[start] = 0.0;
        double best = infinity;
        std::size_t bestHops = 0;
        for (std::size_t hop = 1; hop <= request.maxHops; ++hop) {
            std::fill(next.begin(), next.end(), infinity);
            for (std::size_t current = 0; current < count; ++current) {
                if (!std::isfinite(previous[current])) continue;
                for (std::size_t index = 0; index < count; ++index) {
                    const double edge = edges[current * count + index];
                    const double peak = std::max(previous[current], edge);
                    if (peak < next[index]) {
                        next[index] = peak;
                        parents[hop * count + index] = current;
                    }
                }
            }
            // Strict improvement preserves fewer hops on equal bottlenecks;
            // equal labels within a layer retain first catalog-scan discovery.
            if (next[goal] < best) {
                best = next[goal]; bestHops = hop;
                if (improvements) {
                    auto path = finish(reconstruct(hop));
                    if (!path.ready()) return path;
                    path.request.maxHops = hop;
                    improvements->push_back(std::move(path));
                }
            }
            previous.swap(next);
            if (best == 0.0) break; // No nonnegative distance can improve this.
        }
        if (bestHops != 0) {
            return finish(reconstruct(bestHops));
        }
        auto result = failure(PresetPathError::NoPath);
        result.candidateCount = candidates.size();
        return result;
    }
    std::vector<std::size_t> parent(candidates.size(), unseen);
    std::vector<double> incoming(candidates.size(), 0.0);
    parent[start] = start;
    std::vector<std::size_t> frontier{start}, next;
    for (std::size_t hop = 0; hop < request.maxHops && !frontier.empty(); ++hop) {
        next.clear();
        for (const auto current : frontier) {
            for (std::size_t index = 0; index < candidates.size(); ++index) {
                if (parent[index] != unseen) continue;
                const auto distance = spatialPresetDistance(candidates[current]->params, candidates[index]->params);
                if (!distance || *distance > request.maxDistance) continue;
                parent[index] = current; incoming[index] = *distance;
                if (index == goal) {
                    std::vector<PathNode> route;
                    for (auto node = goal;; node = parent[node]) {
                        route.emplace_back(node, incoming[node]);
                        if (node == start) break;
                    }
                    std::reverse(route.begin(), route.end());
                    return finish(route);
                }
                next.push_back(index);
            }
        }
        frontier.swap(next);
    }
    auto result = failure(PresetPathError::NoPath);
    result.candidateCount = candidates.size();
    return result;
}
} // namespace preset_path_detail

[[nodiscard]] inline PreparedPresetPath preparePresetPath(
    std::span<const SculpturePresetAsset> assets, std::string_view fromId,
    std::string_view toId, PresetPathRequest request = {}) {
    return preset_path_detail::search(assets, fromId, toId, request, nullptr);
}

} // namespace epi_sculpt
