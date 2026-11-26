#pragma once

#include "SculptureSerializer.h"
#include "SculptureWorldGpuAbi.h"

#include <cmath>
#include <optional>
#include <string>

namespace epi_sculpt {

// Owned by the application, borrowed only until the current frame settles.
// This is a captured effective recipe, not retained mathematical field data.
struct SculptureCapturedWorldRecipe {
    WorldRecipe recipe{};
    float effectiveUpdateHz = 0.0f;
    std::string presetId;
    std::string presetRevision;
};

[[nodiscard]] inline std::optional<SculptureCapturedWorldRecipe>
prepareCapturedWorldRecipe(const SculptureWorldMode& mode) {
    if (mode.version < 11 || !mode.hasResolvedWorldRecipe
        || mode.presentationOwnership != SculpturePresentationOwnership::DedicatedWorld
        || mode.playlist.active || mode.morphSpace.active) return std::nullopt;
    const WorldRecipe& recipe = mode.resolvedWorldRecipe;
    if (recipe.delivery != WorldRecipeDelivery::DedicatedGeneratorRequired
        || recipe.presentation != WorldPresentationKind::InstancedWorld
        || recipe.instanceBudget == 0U || recipe.instanceBudget > world_gpu_abi::kMaxInstanceCount
        || recipe.instanceBudget != mode.effectiveWorldBudget
        || recipe.generatorHz == 0U || recipe.generatorHz > 240U
        || recipe.materialBandCount == 0U || recipe.materialBandCount > 16U
        || static_cast<std::size_t>(recipe.mirrorTopology) >= kMirrorCellTopologyCount
        || !std::isfinite(mode.effectiveWorldUpdateHz) || mode.effectiveWorldUpdateHz < 1.0f
        || mode.effectiveWorldUpdateHz > static_cast<float>(recipe.generatorHz)) return std::nullopt;

    // A generator/primitive pair must exist in the current implemented catalog;
    // the captured preset itself may be absent or changed. Never silently clamp
    // a captured budget or admit a route only because its enum is in range.
    for (const auto& entry : worldCatalog()) {
        if (entry.hasCompleteRoute() && entry.delivery == recipe.delivery
            && entry.presentation == recipe.presentation
            && entry.generator == recipe.generator && entry.primitive == recipe.primitive
            && entry.materialBandCount == recipe.materialBandCount
            && entry.uses4DProjection == recipe.uses4DProjection
            && recipe.instanceBudget <= entry.instanceBudget) {
            return SculptureCapturedWorldRecipe{recipe, mode.effectiveWorldUpdateHz,
                                                mode.presetName, mode.presetRevision};
        }
    }
    return std::nullopt;
}

} // namespace epi_sculpt
