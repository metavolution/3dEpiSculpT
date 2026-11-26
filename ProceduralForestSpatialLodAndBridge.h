/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - SPATIAL ACCELERATION, CHUNKED LOD, SEASON/WEATHER,
 *                          XR INTERACTION & SCULPTUREWORLD INSTANCE BRIDGE
 * ============================================================================
 *
 * [EDUCATIONAL NOTE: SPATIALLY & TEMPORALLY STABLE FOREST RENDERING]
 * ----------------------------------------------------------------------------
 * Bridges the procedural forest ecology and geometry layers to high-throughput
 * real-time GPU rendering and interactive environments:
 *
 * 1. 2D Spatial Hash Acceleration Grid (ForestSpatialHashGrid):
 *    Partitions trees, boulders, and rooted wildflowers into uniform 2D cells
 *    so raymarching and local bounce-light queries inspect only the 3x3 cell
 *    neighborhood instead of scanning O(N) primitives per ray step.
 *
 * 2. Hierarchical Culling, Stochastic LOD & Crack-Free Chunked Terrain:
 *    Classifies instances into LOD0 (Hero Detail), LOD1 (Mid-Ground),
 *    LOD2 (Far Proxy), or Culled using camera distance and view-cone frustum
 *    checks, with deterministic stochastic dither weights for pop-free LOD
 *    transitions. Generates chunked terrain meshes with perimeter skirts to
 *    prevent T-junction seams across adjacent LOD chunks.
 *
 * 3. Seasonal Evolution, Weather-Driven Lighting & Interactive XR Vegetation:
 *    - Seasonal cycle (Spring -> Summer -> Autumn -> Winter) shifts foliage
 *      chlorophyll/carotenoid albedo and atmospheric mist density.
 *    - Weather state modulates overcast sky softness, humidity haze, and wind.
 *    - Interactive XR repulsors bend rooted flower stems and foliage away from
 *      hands, controllers, or player capsules while keeping roots stationary.
 *
 * 4. SculptureWorld / mGFX Instance Stream Bridge:
 *    Packs the stratified Waldlichtung ecosystem into epi_sculpt::world_gpu_abi
 *    WorldSnapshotGpu (96-byte WorldInstanceGpu records with anisotropic scale,
 *    quaternions, PBR overrides, and HDR petal emission).
 * ============================================================================
 */

#pragma once

#include "ProceduralForestBotanicalHierarchy.h"
#include "ProceduralForestEcologyScatter.h"
#include "ProceduralForestFields.h"
#include "ProceduralForestLichtungAbi.h"
#include "SculptureIsosurface.h"
#include "SculptureWorldGpuAbi.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace epi_sculpt::forest {

enum class ForestLodTier : std::uint8_t {
    LOD0_HeroDetail = 0U,
    LOD1_MidGround  = 1U,
    LOD2_FarProxy   = 2U,
    Culled          = 3U
};

struct ForestCameraFrustum {
    SculptureVec3 position{0.0f, 2.2f, -18.0f};
    SculptureVec3 forward{0.0f, -0.08f, 1.0f};
    float cosHalfFov = 0.55f;       // ~113 deg horizontal+guard cone
    float nearPlane = 0.1f;
    float lod0MaxDist = 18.0f;
    float lod1MaxDist = 42.0f;
    float lod2MaxDist = 85.0f;
    float transitionBandWidth = 3.0f;
};

struct ForestSpatialHashGrid {
    std::uint32_t gridWidth = 16U;
    std::uint32_t gridHeight = 16U;
    float originX = -40.0f;
    float originZ = -40.0f;
    float cellSize = 5.0f;

    // CSR-style cell offsets (size = gridWidth * gridHeight + 1) and index lists
    std::vector<std::uint32_t> treeCellOffsets;
    std::vector<std::uint32_t> treeIndices;

    std::vector<std::uint32_t> boulderCellOffsets;
    std::vector<std::uint32_t> boulderIndices;

    std::vector<std::uint32_t> flowerCellOffsets;
    std::vector<std::uint32_t> flowerIndices;

    [[nodiscard]] bool empty() const noexcept {
        return treeCellOffsets.empty() && boulderCellOffsets.empty() && flowerCellOffsets.empty();
    }
};

struct ForestTerrainChunkMesh {
    std::uint32_t chunkX = 0U;
    std::uint32_t chunkZ = 0U;
    ForestLodTier lodTier = ForestLodTier::LOD0_HeroDetail;
    SculptureVec3 chunkCenter{};
    IsosurfaceMesh mesh{};
};

struct ForestSeasonWeatherConfig {
    float seasonPhase = 0.25f;      // 0.0=Spring, 0.25=Summer, 0.60=Autumn, 0.85=Winter
    float humidity = 0.55f;         // [0, 1] controls valley mist density
    float overcastFraction = 0.30f; // [0, 1] softens direct sun & boosts ambient haze
    float rainWetness = 0.15f;      // [0, 1] darkens soil/granite & lowers roughness
    float windGustMultiplier = 1.0f;
};

struct ForestXrRepulsor {
    SculptureVec3 position{};
    SculptureVec3 velocity{};
    float radius = 0.85f;
    float strength = 1.0f;
};

// -----------------------------------------------------------------------------
// 1. 2D Spatial Hash Acceleration Grid
// -----------------------------------------------------------------------------
[[nodiscard]] ForestSpatialHashGrid buildForestSpatialHashGrid(
    std::span<const ForestTreeGpu> trees,
    std::span<const ForestMossyBoulderGpu> boulders,
    std::span<const ForestGroundFlowerGpu> flowers,
    float originX = -40.0f,
    float originZ = -40.0f,
    float extentX = 80.0f,
    float extentZ = 80.0f,
    std::uint32_t gridDim = 16U);

// -----------------------------------------------------------------------------
// 2. Hierarchical Culling, Stochastic LOD & Chunked Terrain Meshes
// -----------------------------------------------------------------------------
[[nodiscard]] ForestLodTier classifyForestInstanceLod(
    const SculptureVec3& worldPos,
    float boundingRadius,
    const ForestCameraFrustum& frustum,
    std::uint32_t instanceSeed = 0U,
    float* outCrossfadeWeight = nullptr) noexcept;

[[nodiscard]] float computeStochasticLodCrossfade(
    float distance,
    float thresholdDist,
    float bandWidth,
    std::uint32_t instanceSeed) noexcept;

[[nodiscard]] std::vector<ForestTerrainChunkMesh> generateChunkedLichtungTerrainMeshes(
    const ForestFieldGrid& grid,
    const ForestCameraFrustum& frustum,
    std::uint32_t chunksPerAxis = 4U,
    float skirtDepth = 0.45f);

// -----------------------------------------------------------------------------
// 3. Seasonal Evolution, Weather-Driven Lighting & Interactive XR Deflection
// -----------------------------------------------------------------------------
[[nodiscard]] SculptureVec3 evaluateSeasonalFoliageAlbedo(
    const SculptureVec3& baseSummerAlbedo,
    float seasonPhase,
    std::uint32_t instanceSeed = 0U) noexcept;

void applySeasonAndWeatherToDescriptor(
    ForestLichtungDescriptorGpu& inOutDescriptor,
    const ForestSeasonWeatherConfig& seasonWeather) noexcept;

void applyXrInteractionToRootedFlowers(
    std::span<ForestGroundFlowerGpu> inOutFlowers,
    std::span<const ForestXrRepulsor> repulsors) noexcept;

// -----------------------------------------------------------------------------
// 4. SculptureWorld / mGFX Instance Stream Bridge
// -----------------------------------------------------------------------------
[[nodiscard]] world_gpu_abi::WorldSnapshotGpu packForestEcosystemToWorldSnapshot(
    const ForestEcosystemPopulation& population,
    const ForestCameraFrustum& frustum = {},
    const ForestSeasonWeatherConfig& seasonWeather = {},
    std::span<const ForestSwirlingPetalGpu> swirlingPetals = {},
    float time = 0.0f,
    std::uint32_t generation = 1U);

} // namespace epi_sculpt::forest
