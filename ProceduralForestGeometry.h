/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - BOTANICAL & GEOLOGICAL PROCEDURAL GEOMETRY
 * ============================================================================
 *
 * [EDUCATIONAL NOTE: HYBRID SDF + EXPLICIT MESH SYNTHESIS FOR ALGORITHMIC NATURE]
 * ----------------------------------------------------------------------------
 * Provides both continuous Signed Distance Field (SDF) evaluators (used by the
 * OpenCL raymarcher and CPU physics/collision oracles) and explicit indexed
 * IsosurfaceMesh generators (used by Vulkan rasterization pipelines):
 *
 * 1. Weathered Mossy Granite Boulders:
 *      S_boulder(p) = min_k ( ||S_k^-1 (p - c_k)|| - R_k + beta_1 W(lambda_1 p) + beta_2 fBm(lambda_2 p) )
 *      M_moss(p, N) = smoothstep(0.35, 0.75, N_y + 0.25 fBm(3p) + 0.4 exp(-Delta_y / 0.8))
 *
 * 2. Deciduous Tree Trunks & Bark Furrows:
 *      Recursive conical limbs with longitudinal bark furrow displacement:
 *      delta_bark(theta, y) = A_b * sin(n_ridges * theta + fBm(y))
 *
 * 3. Arched Bracken Fern Fronds:
 *      P_fern(s, u) = p_root + L_f * (s * T_0 - 0.5 * gamma_arch * s^2 * j)
 *                   + w_frond(s) * u * B_0 + W_wind(p_root, t) * s^2
 *
 * 4. Rooted Bell-Flowers & Attached 5-Fold Cupped Petals:
 *      Stationary ground anchor p_root with cantilever-deflected stem tip p_head
 *      and K = 5 radially arranged cupped petals parameterized by (u, v) in
 *      [0, 1] x [-1, 1] with longitudinal venation V_vein(u, v).
 * ============================================================================
 */

#pragma once

#include "ProceduralForestFields.h"
#include "ProceduralForestLichtungAbi.h"
#include "SculptureIsosurface.h"

#include <cstddef>
#include <cstdint>
#include <span>

namespace epi_sculpt::forest {

// -----------------------------------------------------------------------------
// 1. Eroded Valley Terrain Mesh Synthesis
// -----------------------------------------------------------------------------
[[nodiscard]] IsosurfaceMesh generateLichtungTerrainMesh(const ForestFieldGrid& grid);

// -----------------------------------------------------------------------------
// 2. Weathered Granite Boulders & Velvety Moss Accumulation
// -----------------------------------------------------------------------------
[[nodiscard]] float evaluateBoulderSdf(
    const SculptureVec3& p,
    const ForestMossyBoulderGpu& boulder) noexcept;

[[nodiscard]] float evaluateBouldersSdf(
    const SculptureVec3& p,
    std::span<const ForestMossyBoulderGpu> boulders) noexcept;

[[nodiscard]] float evaluateBoulderMossMask(
    const SculptureVec3& p,
    const SculptureVec3& normal,
    float terrainHeight,
    std::uint32_t seed = 0U) noexcept;

[[nodiscard]] IsosurfaceMesh generateMossyBoulderMesh(
    const ForestMossyBoulderGpu& boulder,
    float terrainHeightAtBase = 0.0f,
    std::uint32_t latSegments = 16U,
    std::uint32_t lonSegments = 24U);

// -----------------------------------------------------------------------------
// 3. Deciduous Tree Trunks, Tapering Limbs & Bark Furrows
// -----------------------------------------------------------------------------
[[nodiscard]] IsosurfaceMesh generateTreeBranchMesh(
    const ForestTreeGpu& tree,
    std::uint32_t seed = 0x54524545U,
    std::uint32_t maxBranchDepth = 3U);

// -----------------------------------------------------------------------------
// 4. Arched Fern Fronds (Rachis + Pinnae Envelope)
// -----------------------------------------------------------------------------
[[nodiscard]] SculptureVec3 evaluateFernFrondPoint(
    const SculptureVec3& rootPos,
    float headingRadians,
    float frondLength,
    float archGravity,
    float maxHalfWidth,
    float s,
    float u,
    const SculptureVec3& windVector = {0.0f, 0.0f, 0.0f},
    float time = 0.0f) noexcept;

[[nodiscard]] IsosurfaceMesh generateFernFrondMesh(
    const SculptureVec3& rootPos,
    float headingRadians,
    float frondLength = 0.75f,
    const SculptureVec3& windVector = {0.0f, 0.0f, 0.0f},
    float time = 0.0f,
    std::uint32_t axialSegments = 12U);

// -----------------------------------------------------------------------------
// 5. Rooted Bell-Flowers & Attached 5-Fold Cupped Petals
// -----------------------------------------------------------------------------
[[nodiscard]] float evaluateRootedFlowerPetalsSdf(
    const SculptureVec3& p,
    std::span<const ForestGroundFlowerGpu> flowers,
    int* outFlowerIdx = nullptr,
    float* outPetalU = nullptr,
    float* outPetalV = nullptr) noexcept;

[[nodiscard]] IsosurfaceMesh generateRootedFlowerMesh(
    const ForestGroundFlowerGpu& flower,
    std::uint32_t stemSegments = 8U,
    std::uint32_t radialSteps = 6U);

} // namespace epi_sculpt::forest
