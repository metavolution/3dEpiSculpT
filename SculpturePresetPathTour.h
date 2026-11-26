#pragma once

#include "SculpturePresetPath.h"
#include "SculpturePresetMorph.h"
#include "SculpturePresetConfiguration.h"

#include <cmath>
#include <optional>
#include <string_view>

namespace epi_sculpt {

// Authored rhythm for every cue, independent of route selection. Playback owns
// curve evaluation; the exporter only persists its existing easing vocabulary.
struct PresetPathTourStyle {
    float dwellSeconds = 1.0f;
    MorphEasing easing = MorphEasing::Smoothstep;
};

struct PreparedPresetPathTour {
    std::optional<SculptureWorldMode> world;
    std::string_view diagnostic; // Stable literal/schema vocabulary, never catalog storage.
    [[nodiscard]] bool ready() const noexcept { return world.has_value(); }
};

// Cold authoring candidate: a starting configuration plus revision-bound ID references,
// not copied per-cue params or a second playback engine. Reopening re-resolves
// those IDs against the application's catalog; this is not exact field replay.
[[nodiscard]] inline PreparedPresetPathTour preparePresetPathTour(
    std::span<const SculpturePresetAsset> assets, const PreparedPresetPath& path,
    float transitionSeconds = 3.0f, PresetPathTourStyle style = {}) {
    const auto failure = [](std::string_view why) -> PreparedPresetPathTour {
        return {std::nullopt, why};
    };
    if (!std::isfinite(transitionSeconds) || transitionSeconds < 0.001f || transitionSeconds > 3600.0f)
        return failure("invalid-transition-duration");
    if (!std::isfinite(style.dwellSeconds) || style.dwellSeconds < 0.0f || style.dwellSeconds > 3600.0f)
        return failure("invalid-dwell-duration");
    if (static_cast<std::size_t>(style.easing) >= kMorphEasingCount)
        return failure("invalid-morph-easing");
    if (!presetPathStructureAdmitted(path)) return failure("invalid-path");
    if (assets.size() > kMaxPresetPathCatalog) return failure("catalog-limit");
    const SculpturePresetIdentityIndex identities{assets};
    const SculpturePresetAsset* first = nullptr;
    const SculpturePresetAsset* previous = nullptr;
    SculpturePlaylistCaptureState playlist;
    playlist.hasLibraryTargets = true;
    playlist.active = true;
    playlist.paused = true;
    playlist.loop = false; // A planned path does not declare its closing edge.
    playlist.playbackMode = TourPlaybackMode::StepAndDwell;
    playlist.targets.reserve(path.steps.size());
    double total = 0, largest = 0;
    for (const auto& step : path.steps) {
        const auto identity = identities.resolveExactId(step.id);
        if (!identity.asset) return failure("cue-unavailable");
        const auto& asset = *identity.asset;
        if (asset.revision != step.revision) return failure("stale-cue");
        // The existing WorldMode playlist stores IDs in pipe-delimited lines.
        // Never turn an authored ID into extra fields/lines during export.
        if (std::any_of(asset.id.begin(), asset.id.end(), [](unsigned char byte) {
                return byte < 32 || byte == 127 || byte == '|';
            })) return failure("unrepresentable-cue-id");
        if (!first) first = &asset;
        const auto compatibility = assessPresetMorphCompatibility(*first, asset);
        if (compatibility != PresetMorphCompatibility::Ready)
            return failure(presetMorphCompatibilityCode(compatibility));
        const auto distance = previous ? spatialPresetDistance(previous->params, asset.params)
                                       : std::optional<double>{0.0};
        if (!distance || *distance != step.previousDistance || *distance > path.request.maxDistance)
            return failure("stale-path-distance");
        total += *distance;
        largest = std::max(largest, *distance);
        playlist.targets.push_back({asset.id, transitionSeconds, style.dwellSeconds,
                                    style.easing, asset.revision});
        previous = &asset;
    }
    if (total != path.totalDistance || largest != path.largestStepDistance)
        return failure("stale-path-aggregate");
    auto world = makeSpatialPresetConfiguration(*first, first->params);
    world.playlist = std::move(playlist);
    return {std::move(world), {}};
}

} // namespace epi_sculpt
