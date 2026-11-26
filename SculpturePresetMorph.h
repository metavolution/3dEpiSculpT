#pragma once

#include "SculptureMorphChoreographer.h"
#include "SculpturePresetLibrary.h"
#include "SculpturePresetNeighborhood.h"

#include <cmath>
#include <optional>
#include <string_view>

namespace epi_sculpt {

enum class PresetMorphError {
    None, InvalidPosition, UnsupportedEndpoint, AuthoredAutomation,
    IncompatibleConfiguration, NonfiniteEndpoint, NonfiniteCandidate,
};

// Fine-grained read-model assessment for compact spatial morph eligibility
// between two preset endpoints. Unlike PresetMorphError::IncompatibleConfiguration,
// this distinguishes discrete selector differences from WorldRecipe or
// modulation routing-map differences for passive inspection and comparison.
enum class PresetMorphCompatibility {
    Ready,
    UnsupportedEndpoint,
    AuthoredAutomation,
    SelectorMismatch,
    WorldRecipeMismatch,
    RoutingMapMismatch,
    NonfiniteEndpoint,
};

[[nodiscard]] constexpr std::string_view presetMorphCompatibilityCode(
    PresetMorphCompatibility compatibility) noexcept {
    switch (compatibility) {
    case PresetMorphCompatibility::Ready: return "ready";
    case PresetMorphCompatibility::UnsupportedEndpoint: return "unsupported-endpoint";
    case PresetMorphCompatibility::AuthoredAutomation: return "authored-automation";
    case PresetMorphCompatibility::SelectorMismatch: return "selector-mismatch";
    case PresetMorphCompatibility::WorldRecipeMismatch: return "world-recipe-mismatch";
    case PresetMorphCompatibility::RoutingMapMismatch: return "routing-map-mismatch";
    case PresetMorphCompatibility::NonfiniteEndpoint: return "nonfinite-endpoint";
    }
    return "unknown";
}

[[nodiscard]] constexpr PresetMorphError presetMorphErrorFromCompatibility(
    PresetMorphCompatibility compatibility) noexcept {
    switch (compatibility) {
    case PresetMorphCompatibility::Ready: return PresetMorphError::None;
    case PresetMorphCompatibility::UnsupportedEndpoint: return PresetMorphError::UnsupportedEndpoint;
    case PresetMorphCompatibility::AuthoredAutomation: return PresetMorphError::AuthoredAutomation;
    case PresetMorphCompatibility::SelectorMismatch:
    case PresetMorphCompatibility::WorldRecipeMismatch:
    case PresetMorphCompatibility::RoutingMapMismatch:
        return PresetMorphError::IncompatibleConfiguration;
    case PresetMorphCompatibility::NonfiniteEndpoint: return PresetMorphError::NonfiniteEndpoint;
    }
    return PresetMorphError::IncompatibleConfiguration;
}

struct PreparedPresetMorph {
    PresetMorphError error = PresetMorphError::None;
    std::optional<SculptureProgramParams> params;
    PresetMorphCompatibility compatibility = PresetMorphCompatibility::Ready;
    [[nodiscard]] bool ready() const noexcept { return error == PresetMorphError::None && params.has_value(); }
};

[[nodiscard]] inline bool compactSpatialPreset(const SculpturePresetAsset& asset) noexcept {
    return !asset.canonicalFeedbackMode
        && asset.presentationOwnership == SculpturePresentationOwnership::CompactInstances
        && !asset.worldRecipe.needsDedicatedGenerator() && !asset.worldRecipe.usesImplicitField();
}

[[nodiscard]] inline PresetMorphCompatibility assessPresetMorphCompatibility(
    const SculpturePresetAsset& from, const SculpturePresetAsset& to) noexcept {
    if (!compactSpatialPreset(from) || !compactSpatialPreset(to))
        return PresetMorphCompatibility::UnsupportedEndpoint;
    if (!from.macroAutomationJson.empty() || !to.macroAutomationJson.empty())
        return PresetMorphCompatibility::AuthoredAutomation;
    if (!sameSpatialPresetSelectors(from.params, to.params))
        return PresetMorphCompatibility::SelectorMismatch;
    if (from.worldRecipe != to.worldRecipe)
        return PresetMorphCompatibility::WorldRecipeMismatch;
    if (from.routingMap != to.routingMap)
        return PresetMorphCompatibility::RoutingMapMismatch;
    if (!spatialPresetParametersFinite(from.params, to.params))
        return PresetMorphCompatibility::NonfiniteEndpoint;
    return PresetMorphCompatibility::Ready;
}

// Cold authoring preparation, shared by inspection and export. Owns only copied
// values; no catalog mutation, native capability claim, IO or live clock advance.
// Compatibility checks short-circuit without allocating explanation reports.
[[nodiscard]] inline PreparedPresetMorph preparePresetMorph(
    const SculpturePresetAsset& from, const SculpturePresetAsset& to, float position) {
    const auto failure = [](PresetMorphError error,
                            PresetMorphCompatibility compatibility = PresetMorphCompatibility::Ready) -> PreparedPresetMorph {
        return {error, std::nullopt, compatibility};
    };
    if (!std::isfinite(position) || position < 0.0f || position > 1.0f)
        return failure(PresetMorphError::InvalidPosition);
    const auto compatibility = assessPresetMorphCompatibility(from, to);
    if (compatibility != PresetMorphCompatibility::Ready)
        return failure(presetMorphErrorFromCompatibility(compatibility), compatibility);

    // Exact authored endpoints avoid interpolation normalization. Interior
    // points retain the existing live spatial morph mathematics and field policy.
    const auto params = position == 0.0f ? from.params : position == 1.0f ? to.params
        : SculptureMorphChoreographer::interpolateParams(from.params, to.params, position);
    if (!spatialPresetParametersFinite(params, params))
        return failure(PresetMorphError::NonfiniteCandidate);
    return {PresetMorphError::None, params, PresetMorphCompatibility::Ready};
}

} // namespace epi_sculpt
