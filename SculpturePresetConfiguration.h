#pragma once

#include "SculptureModulationEngine.h"
#include "SculpturePresetAsset.h"
#include "SculptureSerializer.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace epi_sculpt {

// Project a spatial author's selected asset plus edited parameters into one
// owned configuration. Callers own spatial eligibility (canonical feedback is
// not this projection). Identity remains source provenance, not the new file's ID.
// Preserve even unsupported intent: preparation must diagnose loss, not hide it.
[[nodiscard]] inline SculptureWorldMode makeSpatialPresetConfiguration(
    const SculpturePresetAsset& asset, const SculptureProgramParams& params) {
    SculptureWorldMode world;
    world.params = params;
    world.state = asset.state;
    world.presetName = asset.id;
    world.presetRevision = asset.revision;
    world.presentationOwnership = asset.presentationOwnership;
    world.hasResolvedWorldRecipe = true;
    world.resolvedWorldRecipe = asset.worldRecipe;
    world.routingMap = asset.routingMap;
    world.macroAutomationJson = asset.macroAutomationJson;
    return world;
}

enum class PresetConfigurationError {
    None, InvalidModulationRoute, UnrepresentableConfiguration, AuthoredAutomation
};

[[nodiscard]] constexpr std::string_view presetConfigurationErrorName(PresetConfigurationError error) noexcept {
    switch (error) {
    case PresetConfigurationError::None: return "none";
    case PresetConfigurationError::InvalidModulationRoute: return "invalid-modulation-route";
    case PresetConfigurationError::UnrepresentableConfiguration: return "unrepresentable-configuration";
    case PresetConfigurationError::AuthoredAutomation: return "authored-automation";
    }
    return "unknown";
}

// Cold authoring facts, not an installed program or native capability receipt.
// On failure there is no payload; routeDiagnostic is meaningful for route errors.
// All storage is owned, so callers can inspect readiness before choosing a file.
struct PreparedPresetConfiguration {
    PresetConfigurationError error = PresetConfigurationError::UnrepresentableConfiguration;
    std::optional<std::string> encoded;
    ModulationRouteDiagnostic routeDiagnostic{};

    [[nodiscard]] bool ready() const noexcept {
        return error == PresetConfigurationError::None && encoded.has_value();
    }
};

// Admit authored configuration once at the shared authoring boundary, independent
// of mutation/morph/tour adapters. Use the exact installation parser without
// creating an engine or touching source/smoothing history. An empty authored map
// remains empty: only the eventual host may choose/materialize its defaults.
[[nodiscard]] inline PreparedPresetConfiguration preparePresetConfiguration(const SculptureWorldMode& world) {
    // WorldMode currently omits macro text. Re-encoding stability cannot detect
    // a field the writer never emitted; refuse that loss explicitly at the
    // shared boundary rather than relying on each UI/export adapter to know it.
    // The separate epimode authoring codec has a different capability contract.
    if (!world.macroAutomationJson.empty())
        return {PresetConfigurationError::AuthoredAutomation, std::nullopt, {}};
    for (std::size_t index = 0; index < world.routingMap.size(); ++index) {
        const auto inspected = SculptureModulationEngine::inspectRouteResult(world.routingMap[index]);
        if (!inspected.route)
            return {PresetConfigurationError::InvalidModulationRoute, std::nullopt, {index, inspected.error}};
    }

    // The codec remains the format authority. Stable re-encoding plus exact
    // parameter equality preserves the existing publication fidelity policy.
    auto encoded = SculptureSerializer::serializeWorldMode(world);
    const auto decoded = SculptureSerializer::deserializeWorldMode(encoded);
    if (!decoded || decoded->params != world.params
        || SculptureSerializer::serializeWorldMode(*decoded) != encoded)
        return {PresetConfigurationError::UnrepresentableConfiguration, std::nullopt, {}};
    return {PresetConfigurationError::None, std::move(encoded), {}};
}

} // namespace epi_sculpt
