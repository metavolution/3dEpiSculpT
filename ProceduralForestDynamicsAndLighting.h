/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - FOREST DYNAMICS, CHROMODYNAMICS & LIGHT TRANSPORT
 * ============================================================================
 *
 * [EDUCATIONAL NOTE: BIOLUMINESCENT BOUNCE GI, CANTILEVER SWAY & VALLEY MIST]
 * ----------------------------------------------------------------------------
 * Unifies the dynamic physics, chromodynamic petal evolution, local bounce
 * irradiance, and atmospheric light transport of the Procedural Forest Lichtung:
 *
 * 1. Rooted Wildflower Cantilever Sway & Multi-Color Bioluminescence:
 *    Flowers remain permanently anchored at p_root; wind gusts deflect the
 *    stem tip via Euler-Bernoulli cantilever bending while each attached petal
 *    emits linear HDR radiance across a cosine spectral palette (cyan, magenta,
 *    golden-amber, violet, emerald) modulated by longitudinal venation V_vein(u,v).
 *
 * 2. Local Multi-Color Ground Bounce Irradiance L_bounce(p, N, t):
 *    Accumulates colored indirect diffuse bounce light from rooted glowing
 *    blossoms onto neighboring fern fronds, mossy boulder bases, and damp soil
 *    in a 2D world-space irradiance grid.
 *
 * 3. Swirling Flower Petal Flurry (Euler-Cromer Curl-Noise Particle Dynamics):
 *    Optional wind-advected petal flurry driven by 3D curl-noise turbulence,
 *    aerodynamic drag, gravity, sinusoidal flutter, and pink-to-violet age
 *    chromodynamics.
 *
 * 4. Botanical Thin-Slab SSS & Volumetric Valley Mist (Henyey-Greenstein):
 *    Combines waxy cuticle microfacet reflection, forward-transmitted thin-leaf
 *    subsurface scattering, canopy shadow transmittance V_canopy(x,z), and
 *    anisotropic Henyey-Greenstein (g = 0.74) humid valley mist raymarching.
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

struct GroundBounceIrradianceGrid {
    std::uint32_t width = 64U;
    std::uint32_t height = 64U;
    float originX = -40.0f;
    float originZ = -40.0f;
    float extentX = 80.0f;
    float extentZ = 80.0f;
    std::vector<SculptureVec3> irradiance; // Linear HDR RGB per grid cell
};

struct BotanicalSurfaceSample {
    ForestMaterialId material = ForestMaterialId::ForestFloor;
    SculptureVec3 position{};
    SculptureVec3 normal{0.0f, 1.0f, 0.0f};
    float pathMask = 1.0f;
    float terrainHeight = 0.0f;
    int flowerIndex = -1;
    float petalU = 0.0f;
    float petalV = 0.0f;
};

// -----------------------------------------------------------------------------
// 1. Multi-Color Spectral Palette & Attached Petal Emission
// -----------------------------------------------------------------------------
[[nodiscard]] SculptureVec3 evaluateSpectralPetalColor(
    float hueSeed,
    float petalU,
    float time) noexcept;

[[nodiscard]] float evaluatePetalVenation(float petalU, float petalV) noexcept;

[[nodiscard]] SculptureVec3 evaluateAttachedPetalEmission(
    float hueSeed,
    float petalU,
    float petalV,
    float time,
    float maxHdrIntensity = 3.2f) noexcept;

// -----------------------------------------------------------------------------
// 2. Rooted Wildflower Cantilever Sway & Ground Bounce Irradiance Grid
// -----------------------------------------------------------------------------
void updateRootedFlowersCpu(
    std::span<ForestGroundFlowerGpu> flowers,
    float windVelX,
    float windVelZ,
    float time,
    float maxHdrIntensity = 3.2f) noexcept;

[[nodiscard]] GroundBounceIrradianceGrid computeGroundBounceIrradianceGridCpu(
    std::span<const ForestGroundFlowerGpu> flowers,
    const ForestFieldGrid& fieldGrid,
    std::uint32_t resolution = 64U);

[[nodiscard]] SculptureVec3 sampleGroundBounceIrradiance(
    const GroundBounceIrradianceGrid& giGrid,
    float worldX,
    float worldZ) noexcept;

// -----------------------------------------------------------------------------
// 3. Swirling Flower Petal Flurry (Euler-Cromer Curl-Noise Particle System)
// -----------------------------------------------------------------------------
[[nodiscard]] std::vector<ForestSwirlingPetalGpu> initializeSwirlingPetalsCpu(
    std::uint32_t numPetals,
    float domainWidth = 100.0f,
    float domainDepth = 100.0f,
    std::uint32_t seed = 0x5045544CU);

void updateSwirlingPetalsCpu(
    std::span<ForestSwirlingPetalGpu> petals,
    float dt,
    float time,
    const SculptureVec3& windAmbient = {0.8f, 0.0f, 0.35f},
    float domainWidth = 100.0f,
    float domainDepth = 100.0f) noexcept;

// -----------------------------------------------------------------------------
// 4. Botanical PBR + Thin-Slab SSS & Volumetric Valley Mist Integrator
// -----------------------------------------------------------------------------
[[nodiscard]] SculptureVec3 evaluateBotanicalSurfaceLighting(
    const BotanicalSurfaceSample& sample,
    std::span<const ForestGroundFlowerGpu> flowers,
    std::span<const ForestTreeGpu> trees,
    const GroundBounceIrradianceGrid& bounceGiGrid,
    const ForestLichtungDescriptorGpu& descriptor) noexcept;

[[nodiscard]] SculptureVec3 integrateVolumetricLichtungAtmosphere(
    const SculptureVec3& cameraPos,
    const SculptureVec3& rayDir,
    float hitDistance,
    const SculptureVec3& surfaceHdrRadiance,
    std::span<const ForestTreeGpu> trees,
    const ForestLichtungDescriptorGpu& descriptor,
    std::uint32_t numSteps = 24U) noexcept;

} // namespace epi_sculpt::forest
