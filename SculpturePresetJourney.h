#pragma once

#include "SculpturePresetBoundary.h"
#include "SculpturePresetLibrary.h"
#include "SculpturePresetInspectionLimits.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace epi_sculpt {

struct PresetJourneyRequest {
    std::size_t firstStep = 0;
};

enum class PresetJourneyError { None, OffsetOutOfRange };

enum class PresetJourneyStepKind {
    Spatial, RepeatedCue, UnavailableCue, RouteBoundary,
    CanonicalInput, DiscreteBoundary, NonfiniteParameters,
};

[[nodiscard]] constexpr std::string_view presetJourneyStepName(PresetJourneyStepKind kind) noexcept {
    switch (kind) {
    case PresetJourneyStepKind::Spatial: return "spatial";
    case PresetJourneyStepKind::RepeatedCue: return "repeat";
    case PresetJourneyStepKind::UnavailableCue: return "unavailable-cue";
    case PresetJourneyStepKind::RouteBoundary: return "route-boundary";
    case PresetJourneyStepKind::CanonicalInput: return "canonical-input";
    case PresetJourneyStepKind::DiscreteBoundary: return "discrete-boundary";
    case PresetJourneyStepKind::NonfiniteParameters: return "nonfinite-parameters";
    }
    return "unknown";
}

struct PresetJourneyStep {
    std::size_t fromIndex = 0, toIndex = 0;
    std::string fromId, toId, fromRevision, toRevision;
    SculptureTourEntryStatus fromStatus = SculptureTourEntryStatus::MissingAsset;
    SculptureTourEntryStatus toStatus = SculptureTourEntryStatus::MissingAsset;
    PresetJourneyStepKind kind = PresetJourneyStepKind::UnavailableCue;
    std::optional<SpatialPresetDistanceReport> spatial;
    std::vector<PresetBoundaryDifference> boundaries;
};

// Owned derived values: catalog reload and destruction cannot invalidate IDs or
// revisions. Metric component names refer only to stable registry/literal names.
struct PresetJourneyReport {
    std::size_t cueCount = 0, totalSteps = 0;
    std::size_t firstStep = 0;
    PresetJourneyError error = PresetJourneyError::None;
    std::vector<PresetJourneyStep> steps;
    [[nodiscard]] bool ready() const noexcept { return error == PresetJourneyError::None; }
    [[nodiscard]] bool hasMore() const noexcept {
        return ready() && firstStep < totalSteps && steps.size() < totalSteps - firstStep;
    }
    [[nodiscard]] std::optional<std::size_t> nextOffset() const noexcept {
        return hasMore() ? std::optional{firstStep + steps.size()} : std::nullopt;
    }
    // Coverage of the entire requested tour, including an omitted prefix.
    [[nodiscard]] bool truncated() const noexcept { return firstStep != 0 || hasMore(); }
};

// Read requested cue adjacency, including gaps/repeats, without changing a tour,
// skipping to the next ready cue, adding loop closure, or admitting GPU work.
// The 64-step limit bounds rows/metric work, not upstream catalog discovery.
// Tour recognition/discovery remain the caller's responsibility.
[[nodiscard]] inline PresetJourneyReport preparePresetJourney(
    const SculpturePresetLibrary& library, const SculptureTourInspection& tour,
    PresetJourneyRequest request = {}) {
    PresetJourneyReport result;
    result.cueCount = tour.entries.size();
    result.totalSteps = result.cueCount == 0 ? 0 : result.cueCount - 1;
    result.firstStep = request.firstStep;
    // Check before subtraction/indexing. Equal-to-end is a successful terminal
    // page; even SIZE_MAX cannot wrap an invalid request into a valid slice.
    if (result.firstStep > result.totalSteps) {
        result.error = PresetJourneyError::OffsetOutOfRange;
        return result;
    }
    const auto count = std::min(result.totalSteps - result.firstStep, kMaxPresetJourneySteps);
    result.steps.reserve(count);
    if (count == 0) return result;

    struct Endpoint {
        SculptureTourEntryStatus status;
        const SculpturePresetAsset* asset = nullptr;
    };
    const auto resolve = [&library](const SculptureTourEntry& cue) -> Endpoint {
        if (cue.status != SculptureTourEntryStatus::Ready) return {cue.status};
        const auto identity = library.resolveExactId(cue.presetId);
        if (identity.asset) return {SculptureTourEntryStatus::Ready, identity.asset};
        return {identity.status == SculpturePresetIdStatus::Ambiguous
            ? SculptureTourEntryStatus::AmbiguousAsset : SculptureTourEntryStatus::MissingAsset};
    };
    auto from = resolve(tour.entries[result.firstStep]);
    for (std::size_t relative = 0; relative < count; ++relative) {
        const auto i = result.firstStep + relative;
        const auto to = resolve(tour.entries[i + 1]);
        PresetJourneyStep step;
        step.fromIndex = i;
        step.toIndex = i + 1;
        step.fromId = tour.entries[i].presetId;
        step.toId = tour.entries[i + 1].presetId;
        step.fromStatus = from.status;
        step.toStatus = to.status;
        if (from.asset) step.fromRevision = from.asset->revision;
        if (to.asset) step.toRevision = to.asset->revision;
        if (!from.asset || !to.asset) {
            step.kind = PresetJourneyStepKind::UnavailableCue;
        } else if (step.fromId == step.toId) {
            step.kind = PresetJourneyStepKind::RepeatedCue;
        } else if (from.asset->canonicalFeedbackMode || to.asset->canonicalFeedbackMode) {
            // A projected spatial distance cannot describe complete canonical math.
            step.kind = PresetJourneyStepKind::CanonicalInput;
        } else if (!sameSpatialPresetRoute(*from.asset, *to.asset)) {
            step.kind = PresetJourneyStepKind::RouteBoundary;
        } else if ((step.spatial = inspectSpatialPresetDistance(from.asset->params, to.asset->params))) {
            step.kind = PresetJourneyStepKind::Spatial;
        } else if (!spatialPresetParametersFinite(from.asset->params, to.asset->params)) {
            step.kind = PresetJourneyStepKind::NonfiniteParameters;
        } else {
            step.kind = PresetJourneyStepKind::DiscreteBoundary;
        }
        if (from.asset && to.asset && !step.spatial && step.kind != PresetJourneyStepKind::RepeatedCue)
            step.boundaries = inspectPresetBoundaries(*from.asset, *to.asset);
        result.steps.push_back(std::move(step));
        from = to;
    }
    return result;
}

} // namespace epi_sculpt
