/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - ECOLOGICAL DISTRIBUTION & POISSON-DISK SCATTERER
 * ============================================================================
 *
 * [EDUCATIONAL NOTE: STRATIFIED VEGETATION ECOLOGY & CANOPY FEEDBACK]
 * ----------------------------------------------------------------------------
 * Implements the 5-tier botanical and geological distribution hierarchy of the
 * Procedural Forest Lichtung:
 *   1. Canopy Trees (ForestLayerKind::Tree):
 *      Suppressed inside the clearing opening L(x,z):
 *        rho_T(x) = rho_T0 * (1 - eta * L(x)) * M_moist(x) * S_slope(x) * M_path(x)
 *      Filtered with minimum-distance Poisson-disk exclusion so trunks never
 *      interpenetrate.
 *   2. Weathered Granite Boulders (ForestLayerKind::Boulder):
 *      Frame the meandering footpath and clearing flanks without blocking the
 *      trail core, sharing local geological cluster seeds.
 *   3. Canopy Feedback Pass:
 *      Placed trees project canopy shadow transmittance back into the
 *      ForestFieldGrid before understory and ground-cover layers are sampled.
 *   4. Understory Shrubs & Bracken Ferns (UnderstoryShrub, FernFrond):
 *      Prefer dappled shade (1 - openness) and damp soil near boulder bases.
 *   5. Meadow Grass & Rooted Bioluminescent Wildflowers (TallGrass, RootedWildflower):
 *      Respond to canopy openness, soil moisture, and path border proximity,
 *      with Voronoi cluster seeding assigning coherent multi-color spectral
 *      hues (cyan, magenta, gold, violet, emerald) to flower patches.
 * ============================================================================
 */

#pragma once

#include "ProceduralForestFields.h"
#include "ProceduralForestLichtungAbi.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace epi_sculpt::forest {

struct ForestEcologyScatterConfig {
    std::uint32_t sceneSeed = 0x4C494348U; // "LICH"

    // Base ecological densities (instances per m^2 before environmental masks)
    float treeBaseDensity = 0.045f;
    float treeClearingSuppression = 0.96f;
    float boulderBaseDensity = 0.025f;
    float shrubBaseDensity = 0.35f;
    float fernBaseDensity = 1.40f;
    float grassBaseDensity = 8.0f;
    float flowerBaseDensity = 1.80f;
    float debrisBaseDensity = 1.20f;

    // Sampling cell sizes per stratum (meters)
    float macroCellSize = 2.5f;      // Trees & boulders
    float understoryCellSize = 1.25f; // Shrubs & ferns
    float groundCoverCellSize = 0.8f; // Grass, wildflowers, debris

    // Minimum-distance Poisson exclusion radii (meters)
    float treeMinDistance = 3.2f;
    float boulderMinDistance = 2.6f;
    float flowerMinDistance = 0.45f;

    // Maximum caps for GPU buffers
    std::uint32_t maxTrees = 256U;
    std::uint32_t maxBoulders = 128U;
    std::uint32_t maxFlowers = 1024U;
};

struct ForestEcosystemPopulation {
    std::vector<ForestScatterInstanceGpu> trees;
    std::vector<ForestScatterInstanceGpu> boulders;
    std::vector<ForestScatterInstanceGpu> shrubs;
    std::vector<ForestScatterInstanceGpu> ferns;
    std::vector<ForestScatterInstanceGpu> grasses;
    std::vector<ForestScatterInstanceGpu> flowers;
    std::vector<ForestScatterInstanceGpu> debris;

    // Specialized GPU-ready primitive buffers derived from scattered instances
    std::vector<ForestTreeGpu> treePrimitives;
    std::vector<ForestMossyBoulderGpu> boulderPrimitives;
    std::vector<ForestGroundFlowerGpu> flowerPrimitives;

    [[nodiscard]] std::size_t totalInstanceCount() const noexcept {
        return trees.size() + boulders.size() + shrubs.size() + ferns.size()
             + grasses.size() + flowers.size() + debris.size();
    }
};

// Evaluates ecological density rho(x, z) for a specific stratum and WorldCell
[[nodiscard]] float evaluateEcologicalDensity(
    ForestLayerKind kind,
    const ForestWorldCellGpu& cell,
    float patchNoise,
    const ForestEcologyScatterConfig& config = {}) noexcept;

// Scatters candidate instances for a single ecological layer over the field grid
[[nodiscard]] std::vector<ForestScatterInstanceGpu> scatterEcologicalLayer(
    const ForestFieldGrid& grid,
    ForestLayerKind kind,
    float sampleCellSize,
    const ForestEcologyScatterConfig& config = {});

// Enforces minimum-distance Poisson-disk exclusion in the XZ plane
[[nodiscard]] std::vector<ForestScatterInstanceGpu> enforceMinimumDistanceExclusion(
    std::span<const ForestScatterInstanceGpu> candidates,
    float minDistance) noexcept;

// Computes Voronoi-clustered spectral hue seed in [0, 1) for rooted wildflowers
[[nodiscard]] float evaluateVoronoiFlowerHueSeed(
    float worldX,
    float worldZ,
    std::uint32_t instanceSeed,
    std::uint32_t sceneSeed = 0x4C494348U) noexcept;

// Executes the full two-stage Lichtung ecosystem synthesis (macro scatter ->
// canopy shadow feedback -> understory & wildflower scatter -> GPU primitives)
[[nodiscard]] ForestEcosystemPopulation synthesizeCompleteForestEcology(
    ForestFieldGrid& inOutFieldGrid,
    const ForestEcologyScatterConfig& config = {},
    const SculptureVec3& sunDirection = {0.42f, 0.78f, 0.46f});

} // namespace epi_sculpt::forest
