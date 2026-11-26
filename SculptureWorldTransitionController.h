#pragma once

#include "SculptureWorldRecipe.h"
#include "SculptureWorldFraming.h"
#include "mgfx/Engine.h"
#include "mgfx/Camera.h"

#include <cstddef>
#include <string>
#include <vector>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// [EDUCATIONAL NOTE] Smooth Inter-World Transition & Homotopy State
// -----------------------------------------------------------------------------
// When switching between distinct procedural topologies (either via manual user
// navigation or autonomous flight tours), naive state snaps cause jarring visual
// pops across camera framing, PBR material palettes, and geometric parameters.
//
// WorldTransitionState + SculptureWorldTransitionController manage:
//  1. Continuous Geodesic Camera Flight: Radial orbital arcs with elevation domes
//     R_flight(s) = (1 - s)*R_A + s*R_B + 2.5 * sin(pi * s).
//  2. Organic PBR Material Interpolation: Smooth continuous blend of albedo,
//     metallic, roughness, and emissive factors across all 3 PBR bands.
//  3. Homotopy Topology Hand-off: Quintic smootherstep easing
//     s(t) = 6t^5 - 15t^4 + 10t^3 with canonical discrete recipe boundary
//     hand-off at the transition midpoint (t = 0.5).
// -----------------------------------------------------------------------------
struct WorldTransitionState {
    bool active = false;
    float progress = 0.0f;          // Quintic smootherstep progress in [0.0, 1.0]
    float rawProgress = 0.0f;       // Linear progress in [0.0, 1.0]
    float durationSeconds = 0.0f;
    float elapsedSeconds = 0.0f;
    std::size_t fromIndex = 0U;
    std::size_t toIndex = 0U;
    std::string fromPresetName;
    std::string toPresetName;
    std::string toPresetRevision;
    bool recipeCommitted = false;
    WorldRecipe fromRecipe{};
    WorldRecipe toRecipe{};
    SculptureWorldFrameHint fromHint{};
    SculptureWorldFrameHint toHint{};
    WorldMaterialPalette fromPalette{};
    WorldMaterialPalette toPalette{};
};

struct WorldTransitionStepResult {
    bool active = false;
    bool committedMidpointRecipe = false;
    bool completed = false;
};

class SculptureWorldTransitionController {
public:
    [[nodiscard]] static constexpr float quinticSmootherstep(float t) noexcept {
        const float clamped = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
        return clamped * clamped * clamped * (clamped * (clamped * 6.0f - 15.0f) + 10.0f);
    }

    [[nodiscard]] static WorldMaterialPalette interpolatePalette(
        const WorldMaterialPalette& fromPalette,
        const WorldMaterialPalette& toPalette,
        float progress) noexcept;

    static void applyPaletteToEngineMaterials(
        mgfx::Engine* engine,
        const std::vector<mgfx::MaterialHandle>& bands,
        const WorldMaterialPalette& palette);

    static void beginTransition(
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
        float durationSeconds);

    [[nodiscard]] static WorldTransitionStepResult stepTransition(
        WorldTransitionState& state,
        WorldRecipe& currentRecipe,
        std::string& currentPresetName,
        std::string& currentPresetRevision,
        float wallDeltaSeconds) noexcept;

    [[nodiscard]] static mgfx::Camera computeGeodesicTransitionCamera(
        const WorldTransitionState& state,
        float fovYRad) noexcept;
};

} // namespace epi_sculpt
