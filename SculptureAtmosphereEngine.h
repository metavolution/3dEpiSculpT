#pragma once

#include "SculptureProgramSchema.h"
#include <vector>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <string_view>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: VOLUMETRIC PARTICIPATING MEDIA & ATMOSPHERIC GOD-RAYS
// -----------------------------------------------------------------------------
// Light traveling through participating media (fog, mist, aerosol haze, fluid memory)
// undergoes two physical processes:
//
// 1. EXTINCTION (Out-Scattering & Absorption):
//    Described by the Beer-Lambert law:
//        T(s) = exp( - \int_0^s \sigma_t(x(u)) du )
//    where \sigma_t = \sigma_s (scattering) + \sigma_a (absorption) is the extinction coefficient.
//
// 2. IN-SCATTERING (Direct Solar God-Rays & Ambient Radiance):
//    Light from the sun/key light scattered into the camera ray direction \omega:
//        L_in(x, \omega) = \int_0^D T(x, x') \sigma_s(x') \Phi(\omega, \omega_L) L_sun(x') ds
//    where \Phi(\theta) is the angular phase function.
//
// 3. PHASE FUNCTIONS:
//    - Henyey-Greenstein (Mie Aerosol):
//        \Phi_HG(\theta, g) = 1/(4\pi) * (1 - g^2) / (1 + g^2 - 2g \cos\theta)^{3/2}
//    - Rayleigh (Molecular Air):
//        \Phi_Rayleigh(\theta) = 3/(16\pi) * (1 + \cos^2\theta)
//
// 4. CAUCHY CHROMATIC DISPERSION:
//    Wavelength-dependent extinction generates spectral rainbow fringes in dense caustic regions.
// -----------------------------------------------------------------------------

struct AtmosphericVolumeConfig {
    SculptureVec3 sunDirection{0.57735f, 0.57735f, 0.57735f}; // Normalized sun direction
    SculptureVec3 sunRadiance{3.5f, 3.1f, 2.7f};              // Direct solar irradiance
    SculptureVec3 skyAmbient{0.25f, 0.35f, 0.55f};             // Ambient sky dome radiance
    
    float fogBaseDensity = 0.45f;         // Base aerosol density \rho_0
    float fogHeightScale = 2.5f;          // Exponential height scale H_0
    float fogAnisotropy = 0.55f;          // Henyey-Greenstein asymmetry factor g \in (-1, 1)
    float mieRayleighBlend = 0.75f;       // 0 = pure Rayleigh, 1 = pure Mie aerosol
    
    SculptureVec3 scatteringCoeff{0.75f, 0.85f, 0.95f}; // Wavelength-dependent \sigma_s
    SculptureVec3 absorptionCoeff{0.12f, 0.08f, 0.05f}; // Wavelength-dependent \sigma_a
    
    std::uint32_t marchSteps = 32;        // Primary volumetric raymarch step count
    std::uint32_t shadowSteps = 8;        // Secondary solar shadow ray step count
    float maxDistance = 15.0f;            // Maximum participating media depth
    float shadowDensityScale = 1.2f;      // Shadow extinction scale
    bool enableMultiSpectralDispersion = true;
};

struct AtmosphericScatteringResult {
    SculptureVec3 inScatteringLinear{};  // In-scattered radiance (RGB)
    SculptureVec3 transmittanceLinear{}; // Transmittance along ray (RGB)
    float opticalDepth = 0.0f;          // Integrated optical thickness
};

class SculptureAtmosphereEngine {
public:
    // Evaluates the Henyey-Greenstein phase function \Phi_HG(\theta, g)
    [[nodiscard]] static float evaluateHenyeyGreenstein(float cosTheta, float g) noexcept;

    // Evaluates the Rayleigh molecular phase function \Phi_R(\theta)
    [[nodiscard]] static float evaluateRayleigh(float cosTheta) noexcept;

    // Evaluates blended dual-phase function (Rayleigh + Mie aerosol)
    [[nodiscard]] static float evaluateDualPhase(float cosTheta, float g, float mieBlend) noexcept;

    // Evaluates multi-spectral Beer-Lambert optical transmittance
    [[nodiscard]] static SculptureVec3 evaluateTransmittance(
        const SculptureVec3& extinctionCoeff,
        float distance
    ) noexcept;

    // Evaluates height-stratified exponential fog density at point p
    [[nodiscard]] static float evaluateHeightFogDensity(
        const SculptureVec3& p,
        float baseDensity,
        float heightScale
    ) noexcept;

    // Evaluates solar shadow transmittance through 3D spatial fluid grid along light ray
    [[nodiscard]] static float evaluateSolarShadowTransmittance(
        const SculptureVec3& samplePos,
        const SculptureVec3& sunDir,
        const std::vector<float>& spatialBuffer,
        float shadowScale = 1.0f,
        std::uint32_t shadowSteps = 8
    ) noexcept;

    // Integrates total volumetric in-scattering and transmittance along a camera ray
    [[nodiscard]] static AtmosphericScatteringResult integrateInScattering(
        const SculptureVec3& rayOrigin,
        const SculptureVec3& rayDirection,
        float marchDistance,
        const AtmosphericVolumeConfig& config,
        const std::vector<float>& spatialBuffer = {},
        float time = 0.0f
    ) noexcept;

    // Evaluates analytical screen-space God-Ray radial sun shaft burst
    [[nodiscard]] static SculptureVec3 computeSunShaftBurst(
        const SculptureVec3& screenUv,
        const SculptureVec3& sunScreenPos,
        const SculptureVec3& sunColor,
        float density = 0.85f,
        float decay = 0.95f,
        float weight = 0.35f,
        std::uint32_t samples = 16
    ) noexcept;
};

} // namespace epi_sculpt
