#include "SculptureWorldTransitionController.h"

#include <algorithm>
#include <cmath>

namespace epi_sculpt {

WorldMaterialPalette SculptureWorldTransitionController::interpolatePalette(
    const WorldMaterialPalette& pA,
    const WorldMaterialPalette& pB,
    float progress) noexcept {
    const float s = std::clamp(progress, 0.0f, 1.0f);
    WorldMaterialPalette result{};
    for (std::size_t b = 0; b < result.size(); ++b) {
        result[b].baseColor = {
            (1.0f - s) * pA[b].baseColor[0] + s * pB[b].baseColor[0],
            (1.0f - s) * pA[b].baseColor[1] + s * pB[b].baseColor[1],
            (1.0f - s) * pA[b].baseColor[2] + s * pB[b].baseColor[2],
            (1.0f - s) * pA[b].baseColor[3] + s * pB[b].baseColor[3],
        };
        result[b].metallic = (1.0f - s) * pA[b].metallic + s * pB[b].metallic;
        result[b].roughness = (1.0f - s) * pA[b].roughness + s * pB[b].roughness;
        result[b].emission = {
            (1.0f - s) * pA[b].emission[0] + s * pB[b].emission[0],
            (1.0f - s) * pA[b].emission[1] + s * pB[b].emission[1],
            (1.0f - s) * pA[b].emission[2] + s * pB[b].emission[2],
        };
    }
    return result;
}

void SculptureWorldTransitionController::applyPaletteToEngineMaterials(
    mgfx::Engine* engine,
    const std::vector<mgfx::MaterialHandle>& bands,
    const WorldMaterialPalette& palette) {
    if (engine == nullptr || engine->materials() == nullptr) {
        return;
    }
    for (std::size_t i = 0; i < bands.size() && i < palette.size(); ++i) {
        mgfx::pbr::Material m{};
        m.baseColorFactor = mgfx::math::vec4(
            palette[i].baseColor[0],
            palette[i].baseColor[1],
            palette[i].baseColor[2],
            palette[i].baseColor[3]);
        m.metallic = palette[i].metallic;
        m.roughness = palette[i].roughness;
        m.emissiveFactor = mgfx::math::vec3(
            palette[i].emission[0],
            palette[i].emission[1],
            palette[i].emission[2]);
        engine->materials()->updateMaterial(bands[i], m);
    }
}

void SculptureWorldTransitionController::beginTransition(
    WorldTransitionState& state,
    std::size_t fromIndex,
    std::size_t toIndex,
    const std::string& fromPresetName,
    const std::string& toPresetName,
    const std::string& toPresetRevision,
    const WorldRecipe& fromRecipe,
    const WorldRecipe& toRecipe,
    const SculptureWorldFrameHint& activeHint,
    const mgfx::Camera& currentCamera,
    float durationSeconds) {
    if (state.active) {
        state.fromHint = SculptureWorldFrameHint{
            .eyeX = currentCamera.eye.x,
            .eyeY = currentCamera.eye.y,
            .eyeZ = currentCamera.eye.z,
            .targetX = currentCamera.target.x,
            .targetY = currentCamera.target.y,
            .targetZ = currentCamera.target.z,
        };
        state.fromPalette = interpolatePalette(
            state.fromPalette,
            state.toPalette,
            state.progress);
    } else {
        state.fromHint = activeHint;
        state.fromPalette = lookupWorldPresentationPreset(
            fromRecipe.generator,
            fromRecipe.mirrorTopology).materials;
    }

    state.fromIndex = fromIndex;
    state.toIndex = toIndex;
    state.fromPresetName = fromPresetName;
    state.toPresetName = toPresetName;
    state.toPresetRevision = toPresetRevision;
    state.fromRecipe = fromRecipe;
    state.toRecipe = toRecipe;
    state.recipeCommitted = false;
    state.toHint = makeWorldFrameHint(toRecipe);
    state.toPalette = lookupWorldPresentationPreset(
        toRecipe.generator,
        toRecipe.mirrorTopology).materials;
    state.durationSeconds = durationSeconds;
    state.elapsedSeconds = 0.0f;
    state.rawProgress = 0.0f;
    state.progress = 0.0f;
    state.active = true;
}

WorldTransitionStepResult SculptureWorldTransitionController::stepTransition(
    WorldTransitionState& state,
    WorldRecipe& currentRecipe,
    std::string& currentPresetName,
    std::string& currentPresetRevision,
    float wallDeltaSeconds) noexcept {
    WorldTransitionStepResult result{};
    if (!state.active) {
        return result;
    }
    result.active = true;

    state.elapsedSeconds += wallDeltaSeconds;
    state.rawProgress = state.durationSeconds > 0.0f
        ? std::clamp(state.elapsedSeconds / state.durationSeconds, 0.0f, 1.0f)
        : 1.0f;
    state.progress = quinticSmootherstep(state.rawProgress);

    if (state.rawProgress >= 0.5f && !state.recipeCommitted) {
        currentRecipe = state.toRecipe;
        currentPresetName = state.toPresetName;
        currentPresetRevision = state.toPresetRevision;
        state.recipeCommitted = true;
        result.committedMidpointRecipe = true;
    }

    if (state.rawProgress >= 1.0f) {
        currentRecipe = state.toRecipe;
        currentPresetName = state.toPresetName;
        currentPresetRevision = state.toPresetRevision;
        state.active = false;
        result.completed = true;
    }

    return result;
}

mgfx::Camera SculptureWorldTransitionController::computeGeodesicTransitionCamera(
    const WorldTransitionState& state,
    float fovYRad) noexcept {
    const float s = state.progress;
    const auto& fromH = state.fromHint;
    const auto& toH = state.toHint;

    const mgfx::math::vec3 fromTarget(fromH.targetX, fromH.targetY, fromH.targetZ);
    const mgfx::math::vec3 toTarget(toH.targetX, toH.targetY, toH.targetZ);
    const mgfx::math::vec3 target = (1.0f - s) * fromTarget + s * toTarget;

    const mgfx::math::vec3 fromEye(fromH.eyeX, fromH.eyeY, fromH.eyeZ);
    const mgfx::math::vec3 toEye(toH.eyeX, toH.eyeY, toH.eyeZ);

    const mgfx::math::vec3 fromDir = fromEye - fromTarget;
    const mgfx::math::vec3 toDir = toEye - toTarget;
    const float fromRadius = std::sqrt(fromDir.x * fromDir.x + fromDir.y * fromDir.y + fromDir.z * fromDir.z);
    const float toRadius = std::sqrt(toDir.x * toDir.x + toDir.y * toDir.y + toDir.z * toDir.z);

    auto safeNorm = [](const mgfx::math::vec3& v, const mgfx::math::vec3& fallback = mgfx::math::vec3(0.0f, 0.0f, 1.0f)) {
        const float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        return len > 1e-5f ? v * (1.0f / len) : fallback;
    };

    const mgfx::math::vec3 normFrom = safeNorm(fromDir);
    const mgfx::math::vec3 normTo = safeNorm(toDir);
    const mgfx::math::vec3 flightDir = safeNorm((1.0f - s) * normFrom + s * normTo, normFrom);

    const float clearanceDome = 2.5f * std::sin(s * 3.1415926535f);
    const float flightRadius = (1.0f - s) * fromRadius + s * toRadius + clearanceDome;

    return mgfx::Camera{
        .eye = target + flightDir * flightRadius,
        .target = target,
        .up = mgfx::math::vec3(0.0f, 1.0f, 0.0f),
        .fovYRad = fovYRad,
    };
}

} // namespace epi_sculpt
