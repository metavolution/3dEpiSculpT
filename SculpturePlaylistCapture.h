#pragma once

#include "SculpturePresets.h"
#include "SculptureTourTiming.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace epi_sculpt {

struct SculpturePlaylistCaptureTarget {
    std::string presetId;
    float durationSeconds = 3.0f;
    float dwellSeconds = 6.0f;
    MorphEasing easing = MorphEasing::Smoothstep;
    // Empty explicitly means unversioned/legacy ID-only lookup. Otherwise
    // playback restoration requires this source revision, not just the ID.
    std::string presetRevision;
};

// A durable tour is described by stable library IDs, source revisions and timing. It
// deliberately contains no duplicate params, world recipes, or GPU state.
// Selection/flags apply to stopped tours too; exact morph/spline phase is not stored.
struct SculpturePlaylistCaptureState {
    static constexpr std::size_t kMaxTargets = 256U;
    bool hasLibraryTargets = false;
    bool active = false;
    bool paused = false;
    bool loop = true;
    std::uint32_t targetIndex = 0U;
    float dwellSeconds = 0.0f;
    std::vector<SculpturePlaylistCaptureTarget> targets;
    // Older captures omit this policy and retain the historical default.
    TourPlaybackMode playbackMode = TourPlaybackMode::StepAndDwell;
};

// Value admission is shared by decoded and directly constructed captures.
// Text/version rules belong to the codec; identity, revisions and asset routes
// belong to restoration. Neither is needed to validate the captured timeline.
[[nodiscard]] inline bool isPlaylistCaptureValid(
    const SculpturePlaylistCaptureState& state) noexcept {
    // An absent playlist carries no restoration intent; ignore its unused fields.
    if (!state.hasLibraryTargets) return true;
    if (state.targets.empty() || state.targets.size() > SculpturePlaylistCaptureState::kMaxTargets
        || state.targetIndex >= state.targets.size()
        || !std::isfinite(state.dwellSeconds) || state.dwellSeconds < 0.0f)
        return false;
    if (state.playbackMode != TourPlaybackMode::StepAndDwell
        && state.playbackMode != TourPlaybackMode::ContinuousSpline)
        return false;

    float splineDuration = 0.0f;
    for (const auto& cue : state.targets) {
        if (cue.presetId.empty()
            || !std::isfinite(cue.durationSeconds) || cue.durationSeconds < 0.0f
            || !std::isfinite(cue.dwellSeconds) || cue.dwellSeconds < 0.0f
            || static_cast<std::size_t>(cue.easing) >= kMorphEasingCount)
            return false;
        // Stopped captures must also be resumable. Use playback's duration floor
        // and require every spline cue to occupy a representable float interval.
        if (state.playbackMode == TourPlaybackMode::ContinuousSpline
            && !tour_timing::tryAppendCueDuration(splineDuration, cue.durationSeconds))
            return false;
    }
    return true;
}

} // namespace epi_sculpt
