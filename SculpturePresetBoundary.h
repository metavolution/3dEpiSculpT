#pragma once

#include "SculpturePresetAsset.h"
#include "SculpturePresetNeighborhood.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace epi_sculpt {

[[nodiscard]] constexpr std::string_view presentationOwnershipName(
    SculpturePresentationOwnership ownership) noexcept {
    using enum SculpturePresentationOwnership;
    switch (ownership) {
    case CompactInstances: return "compact";
    case CanonicalFeedback: return "canonical";
    case DedicatedWorld: return "world";
    case ImplicitField: return "implicit";
    case UnifiedCompositeField: return "composite";
    }
    return "unknown";
}

namespace preset_neighborhood_detail {
// Shared route vocabulary lives below reporting and query orchestration.
// Observers can short-circuit equality or collect ordered educational facts.
template<class Observer>
[[nodiscard]] inline bool visitRouteFields(
    const SculpturePresetAsset& a, const SculpturePresetAsset& b, Observer&& observe) {
    const auto field = [&](std::string_view name, auto x, auto y) {
        return observe(name, static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y));
    };
    const auto& x = a.worldRecipe;
    const auto& y = b.worldRecipe;
    return field("presentationOwnership", a.presentationOwnership, b.presentationOwnership)
        && field("canonicalFeedbackMode", a.canonicalFeedbackMode.has_value(), b.canonicalFeedbackMode.has_value())
        && field("world.generator", x.generator, y.generator)
        && field("world.primitive", x.primitive, y.primitive)
        && field("world.delivery", x.delivery, y.delivery)
        && field("world.presentation", x.presentation, y.presentation)
        && field("world.uses4DProjection", x.uses4DProjection, y.uses4DProjection)
        && field("world.mirrorTopology", x.mirrorTopology, y.mirrorTopology);
}
} // namespace preset_neighborhood_detail

// Shared inspection eligibility, not proof of native morph/field compatibility.
[[nodiscard]] inline bool sameSpatialPresetRoute(
    const SculpturePresetAsset& a, const SculpturePresetAsset& b) noexcept {
    return preset_neighborhood_detail::visitRouteFields(a, b,
        [](std::string_view, std::uint32_t x, std::uint32_t y) noexcept { return x == y; });
}

enum class PresetBoundaryDomain { Route, Selector };

[[nodiscard]] constexpr std::string_view presetBoundaryDomainName(
    PresetBoundaryDomain domain) noexcept {
    switch (domain) {
    case PresetBoundaryDomain::Route: return "route";
    case PresetBoundaryDomain::Selector: return "selector";
    }
    return "unknown";
}

struct PresetBoundaryDifference {
    PresetBoundaryDomain domain;
    std::string_view field; // Stable schema/registry name, never catalog storage.
    std::uint32_t from = 0, to = 0;
};

[[nodiscard]] inline std::string formatPresetBoundaryValue(
    const PresetBoundaryDifference& difference, std::uint32_t value) {
    const auto field = difference.field;
    if (difference.domain == PresetBoundaryDomain::Selector) {
        if (field == "inner" || field == "innerB") return std::string(name(static_cast<InnerMap>(value)));
        if (field == "reduction" || field == "reductionB") return std::string(name(static_cast<ReductionMode>(value)));
        if (field == "seedType" || field == "seedTypeB") return std::string(name(static_cast<SeedType>(value)));
        if (field == "colorEffect") return std::string(name(static_cast<ColorEffectType>(value)));
        if (field == "auxiliaryResolutionScale") {
            switch (static_cast<StageResolutionScale>(value)) {
            case StageResolutionScale::Full: return "full";
            case StageResolutionScale::Half: return "half";
            case StageResolutionScale::Quarter: return "quarter";
            case StageResolutionScale::Eighth: return "eighth";
            }
        }
    } else {
        if (field == "presentationOwnership")
            return std::string(presentationOwnershipName(static_cast<SculpturePresentationOwnership>(value)));
        if (field == "canonicalFeedbackMode") return value ? "retained" : "absent";
        if (field == "world.generator") return std::string(worldGeneratorName(static_cast<WorldGeneratorKind>(value)));
        if (field == "world.primitive") return std::string(worldPrimitiveName(static_cast<WorldPrimitiveKind>(value)));
        if (field == "world.mirrorTopology") return std::string(mirrorCellTopologyName(static_cast<MirrorCellTopology>(value)));
        if (field == "world.uses4DProjection") return value ? "true" : "false";
        if (field == "world.delivery") {
            switch (static_cast<WorldRecipeDelivery>(value)) {
            case WorldRecipeDelivery::ExistingCompactPath: return "existing-compact-path";
            case WorldRecipeDelivery::DedicatedGeneratorRequired: return "dedicated-generator-required";
            }
        }
        if (field == "world.presentation") {
            switch (static_cast<WorldPresentationKind>(value)) {
            case WorldPresentationKind::InstancedWorld: return "instanced-world";
            case WorldPresentationKind::ImplicitField: return "implicit-field";
            case WorldPresentationKind::ExperimentResources: return "experiment-resources";
            }
        }
    }
    return std::to_string(value);
}

// Owned directional facts, reusable by inspectors/editors. This explains the
// inspection predicates, not live morph safety or canonical field equivalence.
// Field work is bounded by the fixed route vocabulary and selector registry;
// allocation exceptions propagate without publishing partial output.
[[nodiscard]] inline std::vector<PresetBoundaryDifference> inspectPresetBoundaries(
    const SculpturePresetAsset& from, const SculpturePresetAsset& to) {
    std::vector<PresetBoundaryDifference> result;
    const auto collect = [&](PresetBoundaryDomain domain) {
        return [&, domain](std::string_view field, std::uint32_t a, std::uint32_t b) {
            if (a != b) result.push_back({domain, field, a, b});
            return true;
        };
    };
    (void)preset_neighborhood_detail::visitRouteFields(from, to, collect(PresetBoundaryDomain::Route));
    (void)preset_neighborhood_detail::visitSelectors(from.params, to.params, collect(PresetBoundaryDomain::Selector));
    return result;
}

struct WorldRecipeParameterDifference {
    std::string_view field;
    std::uint32_t from = 0, to = 0;
};

// Inspect non-route scalar WorldRecipe parameters (instanceBudget, generatorHz,
// materialBandCount) that affect compact morph eligibility without altering
// sameSpatialPresetRoute or tour journey route-boundary semantics.
[[nodiscard]] inline std::vector<WorldRecipeParameterDifference> inspectWorldRecipeParameterDifferences(
    const WorldRecipe& from, const WorldRecipe& to) {
    std::vector<WorldRecipeParameterDifference> result;
    const auto checkField = [&](std::string_view name, std::uint32_t a, std::uint32_t b) {
        if (a != b) result.push_back({name, a, b});
    };
    checkField("world.instanceBudget", from.instanceBudget, to.instanceBudget);
    checkField("world.generatorHz", from.generatorHz, to.generatorHz);
    checkField("world.materialBandCount", from.materialBandCount, to.materialBandCount);
    return result;
}

} // namespace epi_sculpt
