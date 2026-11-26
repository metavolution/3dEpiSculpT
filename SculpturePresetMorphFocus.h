#pragma once

#include "SculpturePresetMorphStudy.h"

#include <utility>

namespace epi_sculpt {

enum class PresetMorphFocusError { None, InvalidRefinements, StudyRejected, MorphRejected, MetricUnavailable };

struct PresetMorphFocusInterval {
    float begin = 0.0f, end = 1.0f;
    SculptureProgramParams before{}, after{};
    double distance = 0.0;
};

struct PreparedPresetMorphFocus {
    PresetMorphFocusError error = PresetMorphFocusError::None;
    PresetMorphStudyError studyError = PresetMorphStudyError::None;
    PresetMorphError morphError = PresetMorphError::None;
    PresetMorphCompatibility compatibility = PresetMorphCompatibility::Ready;
    PreparedPresetMorphStudy seed;
    std::vector<PresetMorphFocusInterval> intervals;
    [[nodiscard]] bool ready() const noexcept {
        return error == PresetMorphFocusError::None && seed.ready() && !intervals.empty();
    }
};

// Bounded local search, not a global extremum or continuous-path certificate.
// At each midpoint retain the half with the larger endpoint distance; exact
// ties choose the earlier half. This can locate a discrete parameter switch
// without pretending that distance is a derivative or rendered visual motion.
[[nodiscard]] inline PreparedPresetMorphFocus preparePresetMorphFocus(
    const SculpturePresetAsset& from, const SculpturePresetAsset& to,
    std::size_t samples = kDefaultPresetMorphStudySamples,
    std::size_t refinements = kDefaultPresetMorphRefinements) {
    const auto failure = [](PresetMorphFocusError error,
                            PresetMorphStudyError studyError = PresetMorphStudyError::None,
                            PresetMorphError morphError = PresetMorphError::None,
                            PresetMorphCompatibility compatibility = PresetMorphCompatibility::Ready) {
        PreparedPresetMorphFocus result;
        result.error = error;
        result.studyError = studyError;
        result.morphError = morphError;
        result.compatibility = compatibility;
        return result;
    };
    if (refinements > kMaxPresetMorphRefinements)
        return failure(PresetMorphFocusError::InvalidRefinements);
    auto seed = preparePresetMorphStudy(from, to, samples);
    if (!seed.ready())
        return failure(PresetMorphFocusError::StudyRejected, seed.error, seed.morphError, seed.compatibility);
    PreparedPresetMorphFocus candidate;
    candidate.seed = std::move(seed);
    candidate.intervals.reserve(refinements + 1U);
    const auto ending = candidate.seed.largestStepEndingAt;
    const auto& first = candidate.seed.samples[ending - 1U];
    const auto& last = candidate.seed.samples[ending];
    candidate.intervals.push_back({first.position, last.position, first.params, last.params,
                                   last.previousDistance});
    for (std::size_t level = 0; level < refinements; ++level) {
        const auto& interval = candidate.intervals.back();
        const float midpoint = interval.begin + (interval.end - interval.begin) * 0.5f;
        const auto middle = preparePresetMorph(from, to, midpoint);
        if (!middle.ready())
            return failure(PresetMorphFocusError::MorphRejected, PresetMorphStudyError::None,
                           middle.error, middle.compatibility);
        const auto left = spatialPresetDistance(interval.before, *middle.params);
        const auto right = spatialPresetDistance(*middle.params, interval.after);
        if (!left || !right) return failure(PresetMorphFocusError::MetricUnavailable);
        // reserve above keeps interval references stable through publication.
        if (*right > *left)
            candidate.intervals.push_back({midpoint, interval.end, *middle.params, interval.after, *right});
        else
            candidate.intervals.push_back({interval.begin, midpoint, interval.before, *middle.params, *left});
    }
    return candidate;
}

// Validate copied report values at the observation boundary. Reuse the same
// metric rather than accepting stale distances or unrelated endpoint bindings.
[[nodiscard]] inline bool validPresetMorphFocus(const SculpturePresetAsset& from,
    const SculpturePresetAsset& to, const PreparedPresetMorphFocus& focus) {
    if (!focus.ready() || focus.intervals.size() > kMaxPresetMorphRefinements + 1U
        || !matchesPresetMorphStudyMath(from, to, focus.seed)) return false;
    const auto ending = focus.seed.largestStepEndingAt;
    const auto& first = focus.intervals.front();
    if (first.begin != focus.seed.samples[ending - 1U].position
        || first.end != focus.seed.samples[ending].position
        || first.before != focus.seed.samples[ending - 1U].params
        || first.after != focus.seed.samples[ending].params) return false;
    for (std::size_t level = 0; level < focus.intervals.size(); ++level) {
        const auto& interval = focus.intervals[level];
        if (!std::isfinite(interval.begin) || !std::isfinite(interval.end)
            || interval.begin < 0.0f || interval.end > 1.0f || interval.begin >= interval.end)
            return false;
        const auto distance = spatialPresetDistance(interval.before, interval.after);
        if (!distance || *distance != interval.distance) return false;
        if (level != 0U) {
            const auto& parent = focus.intervals[level - 1U];
            const float midpoint = parent.begin + (parent.end - parent.begin) * 0.5f;
            const bool left = interval.begin == parent.begin && interval.end == midpoint
                && interval.before == parent.before;
            const bool right = interval.begin == midpoint && interval.end == parent.end
                && interval.after == parent.after;
            if (!left && !right) return false;
            const auto middle = preparePresetMorph(from, to, midpoint);
            if (!middle.ready()) return false;
            const auto leftDistance = spatialPresetDistance(parent.before, *middle.params);
            const auto rightDistance = spatialPresetDistance(*middle.params, parent.after);
            if (!leftDistance || !rightDistance) return false;
            if (*rightDistance > *leftDistance) {
                if (!right || interval.before != *middle.params) return false;
            } else if (!left || interval.after != *middle.params) return false;
        }
    }
    return true;
}

} // namespace epi_sculpt
