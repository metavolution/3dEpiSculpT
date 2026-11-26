#pragma once

#include "SculpturePresetMorph.h"
#include "SculpturePresetInspectionLimits.h"

#include <cstddef>
#include <cmath>
#include <vector>

namespace epi_sculpt {

enum class PresetMorphStudyError { None, InvalidSampleCount, MorphRejected, MetricUnavailable };

struct PresetMorphStudySample {
    float position = 0.0f;
    SculptureProgramParams params{};
    double previousDistance = 0.0;
    double fromDistance = 0.0;
    double toDistance = 0.0;
};

struct PreparedPresetMorphStudy {
    PresetMorphStudyError error = PresetMorphStudyError::None;
    PresetMorphError morphError = PresetMorphError::None;
    PresetMorphCompatibility compatibility = PresetMorphCompatibility::Ready;
    std::size_t failedSample = 0;
    std::vector<PresetMorphStudySample> samples;
    double sampledDistance = 0.0;
    std::size_t largestStepEndingAt = 0;
    SpatialPresetDistanceReport largestStep;
    [[nodiscard]] bool ready() const noexcept {
        return error == PresetMorphStudyError::None && samples.size() >= kMinPresetMorphStudySamples;
    }
};

// Format-neutral admission of a copied inspection view. Serializers share these
// bounds and endpoint bindings; neither format becomes a second math authority.
[[nodiscard]] inline bool validPresetMorphStudy(
    const SculpturePresetAsset& from, const SculpturePresetAsset& to,
    const PreparedPresetMorphStudy& study) noexcept {
    const auto nonnegativeFinite = [](double value) noexcept {
        return std::isfinite(value) && value >= 0.0;
    };
    if (!study.ready() || study.samples.size() > kMaxPresetMorphStudySamples
        || study.largestStepEndingAt == 0U || study.largestStepEndingAt >= study.samples.size()
        || !nonnegativeFinite(study.sampledDistance) || !nonnegativeFinite(study.largestStep.distance)
        || study.largestStep.differenceCount > study.largestStep.differences.size()
        || study.samples.front().params != from.params || study.samples.back().params != to.params)
        return false;
    for (std::size_t index = 0; index < study.samples.size(); ++index) {
        const auto& sample = study.samples[index];
        const float expectedPosition = static_cast<float>(index)
            / static_cast<float>(study.samples.size() - 1U);
        if (sample.position != expectedPosition || !spatialPresetParametersFinite(sample.params)
            || !nonnegativeFinite(sample.previousDistance) || !nonnegativeFinite(sample.fromDistance)
            || !nonnegativeFinite(sample.toDistance)) return false;
    }
    for (std::size_t index = 0; index < study.largestStep.differenceCount; ++index) {
        const auto& delta = study.largestStep.differences[index];
        if (!std::isfinite(delta.from) || !std::isfinite(delta.to)
            || !nonnegativeFinite(delta.contribution)) return false;
    }
    return true;
}

// Strong copied-view admission without rebuilding an owned sample vector.
// Replay the existing interpolation/metric as a stream, then verify the selected
// maximum as a property: earlier equal steps win. This checks mathematical
// consistency, not historical execution or presentation provenance.
[[nodiscard]] inline bool matchesPresetMorphStudyMath(
    const SculpturePresetAsset& from, const SculpturePresetAsset& to,
    const PreparedPresetMorphStudy& study) {
    if (!validPresetMorphStudy(from, to, study)) return false;
    const auto ending = study.largestStepEndingAt;
    const double selectedDistance = study.samples[ending].previousDistance;
    double sampledDistance = 0.0;
    for (std::size_t index = 0; index < study.samples.size(); ++index) {
        const auto& sample = study.samples[index];
        const auto actual = preparePresetMorph(from, to, sample.position);
        if (!actual.ready() || sample.params != *actual.params) return false;
        const auto& previous = index == 0U ? sample.params : study.samples[index - 1U].params;
        const auto step = spatialPresetDistance(previous, sample.params);
        const auto fromDistance = spatialPresetDistance(from.params, sample.params);
        const auto toDistance = spatialPresetDistance(sample.params, to.params);
        if (!step || !fromDistance || !toDistance || sample.previousDistance != *step
            || sample.fromDistance != *fromDistance || sample.toDistance != *toDistance)
            return false;
        if (index != 0U && ((index < ending && *step >= selectedDistance)
            || (index > ending && *step > selectedDistance))) return false;
        sampledDistance += *step;
    }
    if (study.sampledDistance != sampledDistance) return false;
    const auto actualStep = inspectSpatialPresetDistance(study.samples[ending - 1U].params,
                                                        study.samples[ending].params);
    if (!actualStep) return false;
    const auto& supplied = study.largestStep;
    if (supplied.distance != actualStep->distance || supplied.componentCount != actualStep->componentCount
        || supplied.changedCount != actualStep->changedCount
        || supplied.differenceCount != actualStep->differenceCount) return false;
    for (std::size_t index = 0; index < actualStep->differenceCount; ++index) {
        const auto& a = supplied.differences[index];
        const auto& b = actualStep->differences[index];
        if (a.parameter != b.parameter || a.component != b.component || a.from != b.from
            || a.to != b.to || a.contribution != b.contribution) return false;
    }
    return true;
}

// Cold read model over the existing interpolation and metric. Prepare every
// sample privately: a rejected interior must never publish a truncated study.
// Distances describe this finite grid, not unsampled intervals or rendered motion.
[[nodiscard]] inline PreparedPresetMorphStudy preparePresetMorphStudy(
    const SculpturePresetAsset& from, const SculpturePresetAsset& to,
    std::size_t sampleCount = kDefaultPresetMorphStudySamples) {
    const auto failure = [](PresetMorphStudyError error, std::size_t index = 0,
                            PresetMorphError morphError = PresetMorphError::None,
                            PresetMorphCompatibility compatibility = PresetMorphCompatibility::Ready) {
        PreparedPresetMorphStudy result;
        result.error = error;
        result.failedSample = index;
        result.morphError = morphError;
        result.compatibility = compatibility;
        return result;
    };
    if (sampleCount < kMinPresetMorphStudySamples || sampleCount > kMaxPresetMorphStudySamples)
        return failure(PresetMorphStudyError::InvalidSampleCount);
    const auto first = preparePresetMorph(from, to, 0.0f);
    if (!first.ready()) return failure(PresetMorphStudyError::MorphRejected, 0, first.error, first.compatibility);

    PreparedPresetMorphStudy candidate;
    candidate.samples.reserve(sampleCount);
    for (std::size_t index = 0; index < sampleCount; ++index) {
        const float position = static_cast<float>(index) / static_cast<float>(sampleCount - 1U);
        const auto prepared = index == 0U ? first : preparePresetMorph(from, to, position);
        if (!prepared.ready())
            return failure(PresetMorphStudyError::MorphRejected, index, prepared.error, prepared.compatibility);
        const auto& previous = index == 0U ? *prepared.params : candidate.samples.back().params;
        const auto step = spatialPresetDistance(previous, *prepared.params);
        const auto fromDistance = spatialPresetDistance(from.params, *prepared.params);
        const auto toDistance = spatialPresetDistance(*prepared.params, to.params);
        if (!step || !fromDistance || !toDistance)
            return failure(PresetMorphStudyError::MetricUnavailable, index);
        candidate.samples.push_back({position, *prepared.params, *step, *fromDistance, *toDistance});
        candidate.sampledDistance += *step;
        // The first segment wins exact ties, including a completely static path.
        if (index == 1U || (index > 1U
            && *step > candidate.samples[candidate.largestStepEndingAt].previousDistance))
            candidate.largestStepEndingAt = index;
    }
    const auto last = candidate.largestStepEndingAt;
    const auto report = inspectSpatialPresetDistance(candidate.samples[last - 1U].params,
                                                     candidate.samples[last].params);
    if (!report) return failure(PresetMorphStudyError::MetricUnavailable, last);
    candidate.largestStep = *report;
    return candidate;
}

} // namespace epi_sculpt
