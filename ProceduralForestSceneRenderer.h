/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - PROCEDURAL FOREST LICHTUNG COMPLETE SCENE RENDERER
 * ============================================================================
 *
 * [EDUCATIONAL NOTE: UNIFIED SDF RAYMARCHING, TETRAHEDRAL NORMALS & HDR BLOOM]
 * ----------------------------------------------------------------------------
 * Orchestrates the complete Waldlichtung rendering pipeline across all
 * geological, botanical, and atmospheric layers:
 *
 * 1. Spatially Accelerated Unified Scene SDF:
 *    Evaluates terrain heightfield H(x,z), meandering dirt trail M_path(x,z),
 *    Worley-fractured mossy granite boulders, buttressed/furrowed tree trunks,
 *    billowing canopy foliage crowns, bracken fern undergrowth, and rooted
 *    5-petal bell wildflowers using O(1) 3x3 cell lookups in ForestSpatialHashGrid.
 *
 * 2. 4-Point Tetrahedral SDF Gradient Normals:
 *      N(p) = normalize( sum_{k=0}^3 e_k * d_scene(p + eps * e_k) )
 *    where e_0=(1,-1,-1), e_1=(-1,-1,1), e_2=(-1,1,-1), e_3=(1,1,1), capturing
 *    both macro geometry and Worley/bark micro-relief on boulders, trunks,
 *    ferns, and petals.
 *
 * 3. Soft Canopy Shadows & Undergrowth Contact Ambient Occlusion:
 *    Combines SDF penumbra marching with canopy leaf transmittance V_canopy(p)
 *    and 4-step normal-aligned contact AO to ground boulders and tree roots.
 *
 * 4. Thresholded Linear-HDR Petal Bloom & ACES Filmic Tone Mapping:
 *    Extracts bright emissive flower petals above a soft knee threshold in
 *    linear HDR space, applies a separable 5-tap Gaussian halo, and compresses
 *    dynamic range via Narkowicz ACES filmic tone mapping before sRGB encoding.
 * ============================================================================
 */

#pragma once

#include "ProceduralForestBotanicalHierarchy.h"
#include "ProceduralForestDynamicsAndLighting.h"
#include "ProceduralForestEcologyScatter.h"
#include "ProceduralForestFields.h"
#include "ProceduralForestGeometry.h"
#include "ProceduralForestLichtungAbi.h"
#include "ProceduralForestSpatialLodAndBridge.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace epi_sculpt::forest {

struct LichtungSdfEvaluation {
    float distance = 1e5f;
    ForestMaterialId material = ForestMaterialId::Sky;
    float pathMask = 1.0f;
    float terrainHeight = 0.0f;
    int primitiveIndex = -1;
    float petalU = 0.0f;
    float petalV = 0.0f;
};

struct LichtungCameraRig {
    SculptureVec3 position{0.0f, 1.85f, -16.0f};
    SculptureVec3 forward{0.0f, -0.08f, 1.0f};
    SculptureVec3 right{1.0f, 0.0f, 0.0f};
    SculptureVec3 up{0.0f, 1.0f, 0.08f};
    float focalLength = 1.55f;
};

struct LichtungRenderConfig {
    std::uint32_t width = 64U;
    std::uint32_t height = 36U;
    std::uint32_t maxRaySteps = 84U;
    float maxRayDistance = 85.0f;
    std::uint32_t volumetricSteps = 20U;
    float bloomThreshold = 1.05f;
    float bloomSoftKnee = 0.35f;
    float bloomIntensity = 0.38f;
    bool enableSoftShadows = true;
};

struct LichtungFrameMetrics {
    std::size_t totalPixels = 0U;
    std::size_t skyPixelCount = 0U;
    std::size_t dirtPathPixelCount = 0U;
    std::size_t forestFloorPixelCount = 0U;
    std::size_t boulderPixelCount = 0U;
    std::size_t treeBarkPixelCount = 0U;
    std::size_t canopyFoliagePixelCount = 0U;
    std::size_t fernFoliagePixelCount = 0U;
    std::size_t flowerPetalPixelCount = 0U;
    float meanLuminance = 0.0f;
    float maxHdrLuminance = 0.0f;
    bool allFinite = true;
};

struct LichtungRenderBuffer {
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    std::vector<SculptureVec3> linearHdr;     // Pre-tonemap linear HDR RGB (with volumetric mist)
    std::vector<SculptureVec3> bloomedHdr;    // Linear HDR RGB after thresholded petal bloom
    std::vector<SculptureVec3> displaySrgb;   // ACES filmic tonemapped + gamma-2.2 sRGB in [0, 1]
    std::vector<ForestMaterialId> materialIds;
    std::vector<float> hitDistances;
    LichtungFrameMetrics metrics{};
};

// -----------------------------------------------------------------------------
// 1. Unified Scene SDF, 4-Point Tetrahedral Normals & Raymarching
// -----------------------------------------------------------------------------
[[nodiscard]] LichtungSdfEvaluation evaluateLichtungSceneSdf(
    const SculptureVec3& p,
    const ForestFieldConfig& fieldConfig,
    std::span<const ForestTreeGpu> trees,
    std::span<const ForestMossyBoulderGpu> boulders,
    std::span<const ForestGroundFlowerGpu> flowers,
    const ForestSpatialHashGrid* accelGrid = nullptr) noexcept;

[[nodiscard]] SculptureVec3 evaluateLichtungSceneNormal(
    const SculptureVec3& p,
    const ForestFieldConfig& fieldConfig,
    std::span<const ForestTreeGpu> trees,
    std::span<const ForestMossyBoulderGpu> boulders,
    std::span<const ForestGroundFlowerGpu> flowers,
    const ForestSpatialHashGrid* accelGrid = nullptr,
    float eps = 0.025f) noexcept;

[[nodiscard]] float evaluateLichtungSoftShadow(
    const SculptureVec3& origin,
    const SculptureVec3& sunDir,
    const ForestFieldConfig& fieldConfig,
    std::span<const ForestTreeGpu> trees,
    std::span<const ForestMossyBoulderGpu> boulders,
    const ForestSpatialHashGrid* accelGrid = nullptr) noexcept;

[[nodiscard]] float evaluateLichtungAmbientOcclusion(
    const SculptureVec3& p,
    const SculptureVec3& normal,
    const ForestFieldConfig& fieldConfig,
    std::span<const ForestTreeGpu> trees,
    std::span<const ForestMossyBoulderGpu> boulders,
    const ForestSpatialHashGrid* accelGrid = nullptr) noexcept;

// -----------------------------------------------------------------------------
// 2. Thresholded HDR Petal Bloom, ACES Tone Mapping & Frame Synthesis
// -----------------------------------------------------------------------------
[[nodiscard]] SculptureVec3 tonemapAcesFilmic(const SculptureVec3& linearHdr) noexcept;

void applyThresholdedHdrPetalBloom(
    std::span<const SculptureVec3> linearHdr,
    std::uint32_t width,
    std::uint32_t height,
    float threshold,
    float softKnee,
    float intensity,
    std::vector<SculptureVec3>& outBloomedHdr);

[[nodiscard]] LichtungRenderBuffer renderLichtungSceneFrameCpu(
    const ForestFieldGrid& fieldGrid,
    const ForestEcosystemPopulation& population,
    const GroundBounceIrradianceGrid& bounceGiGrid,
    const ForestLichtungDescriptorGpu& descriptor,
    const LichtungCameraRig& camera = {},
    const LichtungRenderConfig& renderConfig = {},
    const ForestSpatialHashGrid* accelGrid = nullptr);

[[nodiscard]] bool writeLichtungFramePpm(
    const LichtungRenderBuffer& frame,
    const std::string& filePath);

} // namespace epi_sculpt::forest
