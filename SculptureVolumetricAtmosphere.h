#pragma once

#include "SculptureProgram.h"
#include "SculptureRaymarcher.h"
#include <array>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <string_view>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: VOLUMETRIC PARTICIPATING MEDIA & SPECTRAL TRANSPORT
// -----------------------------------------------------------------------------
// Models microscopic light interaction with participating atmospheric media
// surrounding the mathematical sculpture manifold:
//
// 1. Beer-Lambert Exponential Multi-Wavelength Attenuation:
//    - T(lambda, s) = exp(-int_0^s (sigma_a(lambda) + sigma_s(lambda)) * rho(x(t)) dt)
//    - Wavelength-dependent absorption sigma_a and scattering sigma_s coefficients
//      create authentic physical chromatic color shifts (e.g. Rayleigh sky blue,
//      sunset golden twilight, or deep-ocean cyan penetration).
//
// 2. Dual-Lobe Anisotropic Phase Function:
//    - Combines isotropic molecular Rayleigh scattering for ambient fog with
//      forward-peaked Henyey-Greenstein Mie scattering for sharp solar halos:
//        P_dual(theta) = alpha * P_Rayleigh(theta) + (1 - alpha) * P_HG(theta, g)
//
// 3. Deep Light Transmittance & Shadow Marching:
//    - For each primary sample along camera rays, secondary shadow rays march
//      towards directional/point light sources to compute light attenuation T_light,
//      producing photorealistic god rays and soft volumetric shadow shafts.
// -----------------------------------------------------------------------------

enum class MediumProfile : std::uint8_t {
    SunsetAerosol = 0,     // Warm golden forward scattering with strong blue Rayleigh extinction
    DeepOceanNebula = 1,   // Deep cyan-blue penetration with high red absorption
    QuaternionicPlasma = 2,// High-energy violet-magenta emission and nonlinear dispersion
    EtherealGold = 3,      // Dense metallic particulate suspension with high albedo
    NeutralMist = 4        // Balanced grey atmospheric haze
};

[[nodiscard]] constexpr std::string_view name(MediumProfile profile) noexcept {
    switch (profile) {
    case MediumProfile::SunsetAerosol: return "Sunset Aerosol";
    case MediumProfile::DeepOceanNebula: return "Deep Ocean Nebula";
    case MediumProfile::QuaternionicPlasma: return "Quaternionic Plasma";
    case MediumProfile::EtherealGold: return "Ethereal Gold";
    case MediumProfile::NeutralMist: return "Neutral Mist";
    }
    return "Neutral Mist";
}

struct AtmosphericMediumProperties {
    SculptureVec3 absorptionCoeff{0.10f, 0.15f, 0.25f}; // sigma_a per RGB wavelength
    SculptureVec3 scatteringCoeff{0.85f, 0.75f, 0.65f}; // sigma_s per RGB wavelength
    float asymmetryFactor = 0.55f;                      // Henyey-Greenstein g in (-1, 1)
    float mieRayleighBlend = 0.75f;                     // Blend between Mie (HG) and Rayleigh
    float densityMultiplier = 1.0f;                     // Global media density scale
    SculptureVec3 emissionRadiance{0.0f, 0.0f, 0.0f};  // Intrinsic volumetric radiance
};

struct VolumetricLightTransportConfig {
    bool enableVolumetricInScattering = true;
    bool enableShadowRays = true;
    std::uint32_t primaryRaySteps = 48U;
    std::uint32_t shadowRaySteps = 8U;
    float maxMarchDistance = 15.0f;
    float stepJitter = 0.25f; // Dithering factor to break regular wood-grain stepping artifacts
    MediumProfile profile = MediumProfile::SunsetAerosol;
    AtmosphericMediumProperties medium{};
    SculptureVec3 keyLightDirection{0.577f, 0.577f, 0.577f}; // Normalized key light direction
    SculptureVec3 keyLightRadiance{3.0f, 2.7f, 2.2f};        // Direct solar irradiance
    SculptureVec3 ambientRadiance{0.15f, 0.18f, 0.22f};      // Omnidirectional sky irradiance
};

struct VolumetricRadianceResult {
    SculptureVec3 inscatteredRadiance{0.0f, 0.0f, 0.0f};
    SculptureVec3 transmittance{1.0f, 1.0f, 1.0f};
    float integratedOpticalDepth = 0.0f;
    std::uint32_t evaluatedSteps = 0U;
};

struct ThinFilmReflectance {
    SculptureVec3 reflectance{0.0f, 0.0f, 0.0f}; // RGB spectral reflectance in [0, 1]
    SculptureVec3 phaseShift{0.0f, 0.0f, 0.0f};   // Spectral phase delta in radians
    float averageReflectance = 0.0f;
};

struct CauchyDispersionResult {
    SculptureVec3 refractedRed{};
    SculptureVec3 refractedGreen{};
    SculptureVec3 refractedBlue{};
    float angularSeparationRadians = 0.0f;
    bool totalInternalReflection = false;
};

class SculptureVolumetricAtmosphere {
public:
    SculptureVolumetricAtmosphere() noexcept;
    explicit SculptureVolumetricAtmosphere(const VolumetricLightTransportConfig& config) noexcept;

    void setConfig(const VolumetricLightTransportConfig& config) noexcept { config_ = config; }
    [[nodiscard]] const VolumetricLightTransportConfig& config() const noexcept { return config_; }

    // Sets medium profile from pre-calibrated physical presets
    void setMediumProfile(MediumProfile profile) noexcept;

    // Evaluates full volumetric radiative transfer along a primary camera ray
    [[nodiscard]] VolumetricRadianceResult integrateLightTransport(
        const SculptureRay& ray,
        float maxDistance,
        const SculptureProgram& program,
        float time,
        float morph,
        const std::vector<float>& spatialBuffer = {}
    ) const noexcept;

    // Computes light attenuation from a point in space towards the directional key light
    [[nodiscard]] SculptureVec3 computeShadowTransmittance(
        const SculptureVec3& samplePos,
        const SculptureVec3& lightDir,
        const SculptureProgram& program,
        float time,
        float morph,
        const std::vector<float>& spatialBuffer = {}
    ) const noexcept;

    // Static Physical Phase Functions
    [[nodiscard]] static float henyeyGreenstein(float cosTheta, float g) noexcept;
    [[nodiscard]] static float rayleighPhase(float cosTheta) noexcept;
    [[nodiscard]] static float dualLobePhase(float cosTheta, float g, float mieBlend) noexcept;

    // Multi-Layer Thin-Film Airy Wave Interference (650nm R, 532nm G, 450nm B)
    [[nodiscard]] static ThinFilmReflectance evaluateThinFilmInterference(
        float cosTheta,
        float filmThicknessNm,
        float filmIOR = 1.33f,
        float substrateIOR = 1.52f
    ) noexcept;

    // Wavelength-Dependent Cauchy Refraction & Chromatic Dispersion
    [[nodiscard]] static CauchyDispersionResult evaluateCauchyDispersion(
        const SculptureVec3& incident,
        const SculptureVec3& normal,
        float baseIOR = 1.50f,
        float dispersionB = 0.0042f
    ) noexcept;

    // Generates pre-calibrated medium properties for standard physical profiles
    [[nodiscard]] static AtmosphericMediumProperties getProfileProperties(MediumProfile profile) noexcept;

private:
    VolumetricLightTransportConfig config_{};
};

} // namespace epi_sculpt
