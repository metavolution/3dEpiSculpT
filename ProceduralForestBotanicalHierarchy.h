/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - BOTANICAL HIERARCHY & MULTI-SPECIES ARCHITECTURE
 * ============================================================================
 *
 * [EDUCATIONAL NOTE: PHOTOTROPIC CROWN COMPETITION & STRATIFIED UNDERSTORY]
 * ----------------------------------------------------------------------------
 * A realistic Waldlichtung requires breaking botanical repetition across both
 * species and ecological strata:
 *
 * 1. Multi-Species Tree Archetypes (ForestTreeSpecies):
 *    - EuropeanBeech: Broad domed crown, buttressed root flare, smooth bark.
 *    - SilverBirch: Slender trunk, airy weeping crown, fine lenticel bark.
 *    - MossyOak: Gnarled spreading boughs, deep longitudinal bark furrows.
 *    - BlackForestSpruce: Straight trunk with conical whorled evergreen tiers.
 *
 * 2. Phototropic Crown Asymmetry & Neighbor Competition:
 *    Trees bordering the clearing lean and extend their foliage lobes toward
 *    the sunlit clearing center x_c while repelling adjacent tree crowns:
 *      v_lean(x_i) = beta_clear * d_hat_clear
 *                  + beta_repel * sum_{k != i} exp(-||x_i - x_k||^2 / 2*sigma^2) * (x_i - x_k)/||x_i - x_k||
 *
 * 3. Continuous Tree Trunk & Multi-Lobe Canopy SDFs:
 *    Evaluates tapered conical trunk capsules with root-flare buttressing
 *      delta_root(theta, y) = A_r * exp(-y / h_r) * cos^2(n_r * theta)
 *    and longitudinal bark furrows
 *      delta_bark(theta, y) = A_b * sin(n_ridges * theta + fBm(y))
 *    alongside Worley-sculpted asymmetric foliage crown lobes.
 *
 * 4. Multi-Stratum Forest-Floor Geometry:
 *    Generates explicit indexed meshes for quadratic-Bezier meadow grass clumps
 *    (TallGrass), woody multi-stem understory bushes (UnderstoryShrub), and
 *    forest-floor litter (fallen twigs, curled leaves, and pebbles for
 *    ForestDebris).
 * ============================================================================
 */

#pragma once

#include "ProceduralForestFields.h"
#include "ProceduralForestLichtungAbi.h"
#include "SculptureIsosurface.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace epi_sculpt::forest {

struct ForestTreeSpeciesProfile {
    ForestTreeSpecies species = ForestTreeSpecies::EuropeanBeech;
    float trunkTaper = 0.24f;
    float rootButtressAmplitude = 0.28f;
    float rootButtressHeight = 0.95f;
    std::uint32_t rootButtressLobes = 5U;
    float barkFurrowAmplitude = 0.025f;
    float barkRidgeCount = 10.0f;
    float crownVerticalAspect = 0.82f;
    float branchSpreadAngleRad = 0.62f;
    std::uint32_t canopyLobeCount = 6U;
    SculptureVec3 barkAlbedo{0.22f, 0.16f, 0.11f};
    SculptureVec3 foliageAlbedo{0.14f, 0.30f, 0.08f};
    SculptureVec3 foliageSssTint{0.46f, 0.72f, 0.18f};
};

struct ForestBranchSegment {
    SculptureVec3 startPos{};
    SculptureVec3 endPos{};
    float startRadius = 0.25f;
    float endRadius = 0.12f;
    std::uint32_t depth = 0U;
};

struct ForestCanopyLobe {
    SculptureVec3 center{};
    SculptureVec3 radii{2.0f, 1.6f, 2.0f};
    std::uint32_t seed = 0U;
};

struct ForestTreeSkeleton {
    ForestTreeSpecies species = ForestTreeSpecies::EuropeanBeech;
    SculptureVec3 rootPos{};
    SculptureVec3 phototropicLeanXZ{0.0f, 0.0f, 0.0f};
    std::vector<ForestBranchSegment> branches;
    std::vector<ForestCanopyLobe> canopyLobes;
};

// -----------------------------------------------------------------------------
// 1. Multi-Species Profiles & Phototropic Crown Competition
// -----------------------------------------------------------------------------
[[nodiscard]] ForestTreeSpeciesProfile getTreeSpeciesProfile(ForestTreeSpecies species) noexcept;

[[nodiscard]] SculptureVec3 computePhototropicLeanVector(
    std::size_t treeIndex,
    std::span<const ForestTreeGpu> allTrees,
    float clearingCenterX = 0.0f,
    float clearingCenterZ = 8.0f) noexcept;

[[nodiscard]] ForestTreeSkeleton generateSpeciesTreeSkeleton(
    const ForestTreeGpu& tree,
    ForestTreeSpecies species,
    const SculptureVec3& phototropicLeanXZ = {0.0f, 0.0f, 0.0f},
    std::uint32_t seed = 0x54524545U);

// -----------------------------------------------------------------------------
// 2. Continuous Tree Trunk (Root Buttress + Bark Furrows) & Canopy Foliage SDFs
// -----------------------------------------------------------------------------
[[nodiscard]] float evaluateTreeTrunkSdf(
    const SculptureVec3& p,
    const ForestTreeGpu& tree,
    std::uint32_t seed = 0U) noexcept;

[[nodiscard]] float evaluateTreeCanopySdf(
    const SculptureVec3& p,
    const ForestTreeGpu& tree,
    std::uint32_t seed = 0U) noexcept;

[[nodiscard]] float evaluateTreesSdf(
    const SculptureVec3& p,
    std::span<const ForestTreeGpu> trees,
    ForestMaterialId* outTreeMaterial = nullptr,
    int* outTreeIndex = nullptr) noexcept;

// -----------------------------------------------------------------------------
// 3. Stratified Ground-Cover & Understory Mesh Synthesis
// -----------------------------------------------------------------------------
[[nodiscard]] IsosurfaceMesh generateGrassBladeClusterMesh(
    const SculptureVec3& clumpCenter,
    float clumpRadius = 0.35f,
    std::uint32_t bladeCount = 14U,
    const SculptureVec3& windVector = {0.5f, 0.0f, 0.2f},
    float time = 0.0f,
    std::uint32_t seed = 0x47524153U);

[[nodiscard]] IsosurfaceMesh generateUnderstoryShrubMesh(
    const SculptureVec3& rootPos,
    float scale = 1.0f,
    std::uint32_t stemCount = 5U,
    std::uint32_t seed = 0x53485242U);

[[nodiscard]] IsosurfaceMesh generateForestDebrisClusterMesh(
    const SculptureVec3& centerPos,
    float clusterRadius = 0.45f,
    std::uint32_t itemCount = 8U,
    std::uint32_t seed = 0x44454252U);

} // namespace epi_sculpt::forest
