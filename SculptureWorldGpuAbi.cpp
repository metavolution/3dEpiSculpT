#include "SculptureWorldGpuAbi.h"

#include <algorithm>
#include <cmath>

namespace epi_sculpt::world_gpu_abi {

namespace {

GeneratorId generatorId(WorldGeneratorKind kind) noexcept {
    return static_cast<GeneratorId>(kind);
}

PrimitiveId primitiveId(WorldPrimitiveKind kind) noexcept {
    return static_cast<PrimitiveId>(kind);
}

bool finite(float value) noexcept {
    return std::isfinite(value);
}

} // namespace

WorldRecipeGpu packWorldRecipe(const WorldRecipe& recipe) noexcept {
    WorldRecipeGpu packed{};
    packed.generatorId = static_cast<std::uint32_t>(generatorId(recipe.generator));
    packed.primitiveId = static_cast<std::uint32_t>(primitiveId(recipe.primitive));
    packed.instanceBudget = std::min(recipe.instanceBudget, kMaxInstanceCount);
    packed.generatorHz = std::clamp(recipe.generatorHz, 1U, 240U);
    packed.materialBandCount = std::clamp<std::uint32_t>(recipe.materialBandCount, 1U, 16U);
    const std::uint32_t mirrorTopology = std::min<std::uint32_t>(
        static_cast<std::uint32_t>(recipe.mirrorTopology), 3U);
    packed.flags = (recipe.uses4DProjection ? kUses4DProjectionFlag : 0U)
        | (mirrorTopology << kMirrorTopologyShift);
    return packed;
}

bool isValid(const WorldRecipeGpu& recipe) noexcept {
    return recipe.abiVersion == kVersion
        && recipe.byteSize == kRecipeBytes
        && recipe.generatorId <= static_cast<std::uint32_t>(GeneratorId::NeonBubblesphere)
        && recipe.primitiveId <= static_cast<std::uint32_t>(PrimitiveId::TrussStrut)
        && recipe.instanceBudget <= kMaxInstanceCount
        && recipe.generatorHz >= 1U && recipe.generatorHz <= 240U
        && recipe.materialBandCount >= 1U && recipe.materialBandCount <= 16U
        && (recipe.flags & ~kRecipeFlagsMask) == 0U;
}

bool isValid(const WorldFrameGpu& frame, const WorldRecipeGpu& recipe) noexcept {
    return frame.abiVersion == kVersion
        && frame.byteSize == kFrameBytes
        && frame.generation != 0U
        && frame.activeInstanceCount <= recipe.instanceBudget
        && isValid(recipe)
        && finite(frame.time)
        && finite(frame.deltaSeconds)
        && finite(frame.morph);
}

bool isValid(const WorldSnapshotGpu& snapshot) noexcept {
    if (!isValid(snapshot.frame, snapshot.recipe)) return false;

    for (std::uint32_t index = 0U; index < snapshot.frame.activeInstanceCount; ++index) {
        const WorldInstanceGpu& instance = snapshot.instances[index];
        if (instance.primitiveId > static_cast<std::uint32_t>(PrimitiveId::TrussStrut)
            || instance.materialBand >= snapshot.recipe.materialBandCount
            || !finite(instance.positionScale[0]) || !finite(instance.positionScale[1])
            || !finite(instance.positionScale[2]) || !finite(instance.positionScale[3])
            || instance.positionScale[3] < 0.0f
            || !finite(instance.orientation[0]) || !finite(instance.orientation[1])
            || !finite(instance.orientation[2]) || !finite(instance.orientation[3])
            || !finite(instance.tintEmission[0]) || !finite(instance.tintEmission[1])
            || !finite(instance.tintEmission[2]) || !finite(instance.tintEmission[3])) {
            return false;
        }
        if ((instance.flags & kAnisotropicScaleFlag) != 0U
            && (!finite(instance.axisScale[0]) || !finite(instance.axisScale[1])
                || !finite(instance.axisScale[2]) || instance.axisScale[0] <= 0.0f
                || instance.axisScale[1] <= 0.0f || instance.axisScale[2] <= 0.0f)) {
            return false;
        }
    }
    return true;
}

} // namespace epi_sculpt::world_gpu_abi
