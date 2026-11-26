/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - PROCEDURAL FOREST SPATIAL FIELDS & GEOMORPHOLOGY
 * ============================================================================
 *
 * [EDUCATIONAL NOTE: ALGORITHMIC NATURE — OVERLAPPING ECOLOGICAL FIELDS]
 * ----------------------------------------------------------------------------
 * A convincing forest clearing ("Waldlichtung") is not a collection of random
 * props; it is an interconnected ecological field over ground coordinates
 * x = (x, z):
 *
 * 1. Irregular Canopy Clearing Field L(x, z):
 *      r(x, z) = sqrt(((x - x_c)/a)^2 + ((z - z_c)/b)^2)
 *      L(x, z) = 1 - smoothstep(r_0, r_1, r(x, z) + alpha * fBm_low(x, z))
 *    where L ~= 1 inside the sunlit clearing and L ~= 0 in the surrounding
 *    dense woodland.
 *
 * 2. Winding Dirt Footpath Spline & Edge-Warped Mask M_path(x, z):
 *      x_path(z) = A_p * sin(omega_p * z) + B_p * fBm_1D(z)
 *      d_path(x, z) = |x - x_path(z)| + eta_edge(x, z)
 *      M_path(x, z) = smoothstep(r_inner, r_outer, d_path(x, z))
 *    where M_path = 0 on the sunken dirt trail and M_path = 1 in the meadow.
 *
 * 3. Gradient-Damped Hydraulic Erosion Terrain H_terrain(x, z):
 *      H(x, z) = alpha_valley * (x - x_path(z))^2
 *              + sum_{i=0}^{K-1} [ a_i * N(omega_i R^i x) / (1 + kappa ||sum_{j<=i} grad N||^2) ] * M_path(x, z)
 *              - D_rut * (1 - M_path(x, z)) + A_pebble * Worley(x, z) * (1 - M_path(x, z))
 *    Dividing each fBm octave by the cumulative squared gradient sharpens
 *    ridges and flattens valley floors naturally, mimicking soil transport.
 *
 * 4. Canopy Transmittance Feedback:
 *    Once major trees are placed around the clearing border, their actual
 *    foliage crowns attenuate ground-level openness so understory ferns,
 *    moss, grass, and rooted wildflowers respond to true canopy shade.
 * ============================================================================
 */

#pragma once

#include "ProceduralForestLichtungAbi.h"
#include "SculptureProgramSchema.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace epi_sculpt::forest {

struct ValueAndGradient2D {
    float value = 0.0f;
    float dx = 0.0f;
    float dz = 0.0f;
};

struct ForestFieldConfig {
    std::uint32_t gridWidth = 128U;
    std::uint32_t gridHeight = 128U;
    float cellSize = 0.625f;               // 128 * 0.625 = 80m x 80m domain
    float originX = -40.0f;
    float originZ = -40.0f;

    // Clearing L(x, z) parameters
    float clearingCenterX = 0.0f;
    float clearingCenterZ = 8.0f;
    float clearingSemiAxisA = 16.0f;
    float clearingSemiAxisB = 29.0f;
    float clearingInnerRadius = 0.65f;
    float clearingOuterRadius = 1.25f;
    float clearingIrregularity = 0.18f;

    // Winding dirt path parameters
    float pathSineAmplitude = 2.2f;
    float pathSineFrequency = 0.06f;
    float pathFbmAmplitude = 0.8f;
    float pathInnerRadius = 0.75f;
    float pathOuterRadius = 1.65f;
    float pathEdgeWarp = 0.35f;
    float pathRutDepth = 0.22f;

    // Gradient-damped valley erosion terrain parameters
    float valleyCurvature = 0.012f;
    float baseAmplitude = 2.2f;
    float baseFrequency = 0.045f;
    float gradientDampingKappa = 1.8f;
    std::uint32_t erosionOctaves = 4U;
    std::uint32_t seed = 0x57414C44U;      // "WALD"
};

struct ForestFieldGrid {
    ForestFieldConfig config{};
    std::vector<ForestWorldCellGpu> cells;

    [[nodiscard]] bool empty() const noexcept { return cells.empty(); }
    [[nodiscard]] std::size_t cellCount() const noexcept { return cells.size(); }
};

// -----------------------------------------------------------------------------
// 1. Deterministic Hashing & Pseudo-Random Generators
// -----------------------------------------------------------------------------
[[nodiscard]] std::uint32_t hash32(std::uint32_t x) noexcept;
[[nodiscard]] std::uint64_t hash64(std::uint64_t x) noexcept;
[[nodiscard]] float random01(std::uint32_t seed) noexcept;
void random2(std::uint32_t seed, float& outX, float& outZ) noexcept;
[[nodiscard]] float smoothstep01(float edge0, float edge1, float x) noexcept;

// -----------------------------------------------------------------------------
// 2. Continuous Noise, Analytic Gradients, Worley & 3D Curl Noise
// -----------------------------------------------------------------------------
[[nodiscard]] float simplex1D(float x, std::uint32_t seed = 0U) noexcept;
[[nodiscard]] float simplex2D(float x, float z, std::uint32_t seed = 0U) noexcept;
[[nodiscard]] ValueAndGradient2D simplex2DWithDeriv(float x, float z, std::uint32_t seed = 0U) noexcept;
[[nodiscard]] float simplex3D(float x, float y, float z, std::uint32_t seed = 0U) noexcept;

[[nodiscard]] float fbm1D(float x, std::uint32_t octaves = 4U, std::uint32_t seed = 0U) noexcept;
[[nodiscard]] float fbm2D(float x, float z, std::uint32_t octaves = 4U, std::uint32_t seed = 0U) noexcept;
[[nodiscard]] float fbm3D(float x, float y, float z, std::uint32_t octaves = 4U, std::uint32_t seed = 0U) noexcept;

[[nodiscard]] float worley2D(float x, float z, std::uint32_t seed = 0U) noexcept;
[[nodiscard]] float worley3D(float x, float y, float z, std::uint32_t seed = 0U) noexcept;

[[nodiscard]] SculptureVec3 curlNoise3D(const SculptureVec3& p, float time) noexcept;

// -----------------------------------------------------------------------------
// 3. Lichtung Geomorphology & Spatial Field Evaluators
// -----------------------------------------------------------------------------
[[nodiscard]] float evaluateDirtPathSdf(
    float x,
    float z,
    const ForestFieldConfig& config = {},
    float* outCenterX = nullptr) noexcept;

[[nodiscard]] float evaluateClearingField(
    float x,
    float z,
    const ForestFieldConfig& config = {}) noexcept;

[[nodiscard]] float evaluateTerrainHeight(
    float x,
    float z,
    const ForestFieldConfig& config = {},
    float* outPathMask = nullptr) noexcept;

[[nodiscard]] SculptureVec3 evaluateTerrainNormal(
    float x,
    float z,
    const ForestFieldConfig& config = {},
    float* outSlope = nullptr) noexcept;

[[nodiscard]] ForestWorldCellGpu evaluateWorldCellAt(
    float x,
    float z,
    const ForestFieldConfig& config = {}) noexcept;

// -----------------------------------------------------------------------------
// 4. World Field Grid Construction, Sampling & Canopy Feedback
// -----------------------------------------------------------------------------
[[nodiscard]] ForestFieldGrid generateWorldFieldGrid(const ForestFieldConfig& config = {});

[[nodiscard]] ForestWorldCellGpu sampleWorldCell(
    const ForestFieldGrid& grid,
    float worldX,
    float worldZ) noexcept;

// Updates cell.openness in-place using actual placed tree crowns and sun angle
void applyCanopyCoverageFeedback(
    ForestFieldGrid& grid,
    std::span<const ForestTreeGpu> trees,
    const SculptureVec3& sunDirection = {0.42f, 0.78f, 0.46f}) noexcept;

[[nodiscard]] float evaluateCanopyShadowTransmittance(
    float worldX,
    float worldY,
    float worldZ,
    std::span<const ForestTreeGpu> trees,
    const SculptureVec3& sunDirection = {0.42f, 0.78f, 0.46f},
    std::uint32_t seed = 0U) noexcept;

} // namespace epi_sculpt::forest
