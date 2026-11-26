#include "SculptureVolumetricAtmosphere.h"
#include <algorithm>
#include <cmath>

namespace epi_sculpt {

namespace {

inline constexpr float kInvFourPi = 0.07957747154594767f;  // 1 / (4 * pi)
inline constexpr float kThreeSixteenthPi = 0.05968310365946075f; // 3 / (16 * pi)

inline float dot(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline SculptureVec3 multiply(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}

inline SculptureVec3 scale(const SculptureVec3& v, float s) noexcept {
    return {v.x * s, v.y * s, v.z * s};
}

inline SculptureVec3 add(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline SculptureVec3 expVec(const SculptureVec3& v) noexcept {
    return {std::exp(v.x), std::exp(v.y), std::exp(v.z)};
}

} // namespace

SculptureVolumetricAtmosphere::SculptureVolumetricAtmosphere() noexcept
    : SculptureVolumetricAtmosphere(VolumetricLightTransportConfig{}) {}

SculptureVolumetricAtmosphere::SculptureVolumetricAtmosphere(const VolumetricLightTransportConfig& config) noexcept
    : config_(config) {
    if (config_.medium.densityMultiplier == 1.0f && config_.medium.asymmetryFactor == 0.55f) {
        config_.medium = getProfileProperties(config_.profile);
    }
}

void SculptureVolumetricAtmosphere::setMediumProfile(MediumProfile profile) noexcept {
    config_.profile = profile;
    config_.medium = getProfileProperties(profile);
}

float SculptureVolumetricAtmosphere::henyeyGreenstein(float cosTheta, float g) noexcept {
    const float g2 = g * g;
    const float denom = 1.0f + g2 - 2.0f * g * cosTheta;
    if (denom <= 1e-4f) return kInvFourPi;
    return kInvFourPi * (1.0f - g2) / (denom * std::sqrt(denom));
}

float SculptureVolumetricAtmosphere::rayleighPhase(float cosTheta) noexcept {
    return kThreeSixteenthPi * (1.0f + cosTheta * cosTheta);
}

float SculptureVolumetricAtmosphere::dualLobePhase(float cosTheta, float g, float mieBlend) noexcept {
    const float rayleigh = rayleighPhase(cosTheta);
    const float hg = henyeyGreenstein(cosTheta, g);
    return (1.0f - mieBlend) * rayleigh + mieBlend * hg;
}

ThinFilmReflectance SculptureVolumetricAtmosphere::evaluateThinFilmInterference(
    float cosTheta,
    float filmThicknessNm,
    float filmIOR,
    float substrateIOR
) noexcept {
    ThinFilmReflectance result{};
    const float ct = std::clamp(cosTheta, 0.0f, 1.0f);
    const float sin2ThetaI = 1.0f - ct * ct;

    const float n0 = 1.0f; // Air
    const float nf = std::max(filmIOR, 1.0f);
    const float ns = std::max(substrateIOR, 1.0f);

    // Refraction angle in film: Snell's law
    const float sin2ThetaF = sin2ThetaI / (nf * nf);
    if (sin2ThetaF >= 1.0f) {
        // Total internal reflection at air-film boundary
        result.reflectance = {1.0f, 1.0f, 1.0f};
        result.averageReflectance = 1.0f;
        return result;
    }
    const float cosThetaF = std::sqrt(std::max(0.0f, 1.0f - sin2ThetaF));

    // Refraction angle in substrate
    const float sin2ThetaS = sin2ThetaI / (ns * ns);
    const float cosThetaS = std::sqrt(std::max(0.0f, 1.0f - sin2ThetaS));

    // Fresnel reflection coefficients (perpendicular/s-polarization approximation for thin films)
    const float r01 = (n0 * ct - nf * cosThetaF) / std::max(n0 * ct + nf * cosThetaF, 1e-6f);
    const float r12 = (nf * cosThetaF - ns * cosThetaS) / std::max(nf * cosThetaF + ns * cosThetaS, 1e-6f);

    const float r01_sq = r01 * r01;
    const float r12_sq = r12 * r12;
    const float r01_r12 = r01 * r12;

    // Optical Path Difference in nm
    const float opd = 2.0f * nf * std::max(filmThicknessNm, 10.0f) * cosThetaF;

    // RGB central wavelengths in nm: Red=650nm, Green=532nm, Blue=450nm
    constexpr std::array<float, 3> kWavelengths = {650.0f, 532.0f, 450.0f};
    std::array<float, 3> refl{};
    std::array<float, 3> phase{};

    for (std::size_t c = 0; c < 3; ++c) {
        const float delta = (2.0f * 3.141592653589793f / kWavelengths[c]) * opd;
        const float cosDelta = std::cos(delta);
        const float numerator = r01_sq + r12_sq + 2.0f * r01_r12 * cosDelta;
        const float denominator = 1.0f + r01_sq * r12_sq + 2.0f * r01_r12 * cosDelta;
        refl[c] = std::clamp(numerator / std::max(denominator, 1e-6f), 0.0f, 1.0f);
        phase[c] = delta;
    }

    result.reflectance = {refl[0], refl[1], refl[2]};
    result.phaseShift = {phase[0], phase[1], phase[2]};
    result.averageReflectance = (refl[0] + refl[1] + refl[2]) * 0.3333333f;
    return result;
}

CauchyDispersionResult SculptureVolumetricAtmosphere::evaluateCauchyDispersion(
    const SculptureVec3& incident,
    const SculptureVec3& normal,
    float baseIOR,
    float dispersionB
) noexcept {
    CauchyDispersionResult result{};

    // Normalizing vectors
    const float inLen = std::sqrt(incident.x * incident.x + incident.y * incident.y + incident.z * incident.z);
    const float nLen = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (inLen < 1e-6f || nLen < 1e-6f) return result;

    const SculptureVec3 I{incident.x / inLen, incident.y / inLen, incident.z / inLen};
    SculptureVec3 N{normal.x / nLen, normal.y / nLen, normal.z / nLen};

    float cosI = -(I.x * N.x + I.y * N.y + I.z * N.z);
    if (cosI < 0.0f) {
        cosI = -cosI;
        N = {-N.x, -N.y, -N.z};
    }

    // Wavelengths in micrometers: Red=0.650um, Green=0.532um, Blue=0.450um
    constexpr std::array<float, 3> kWavelengthsUm = {0.650f, 0.532f, 0.450f};
    std::array<SculptureVec3, 3> refracted{};

    for (std::size_t c = 0; c < 3; ++c) {
        const float lambdaSq = kWavelengthsUm[c] * kWavelengthsUm[c];
        const float n_lambda = baseIOR + (dispersionB / lambdaSq);
        const float eta = 1.0f / std::max(n_lambda, 1.0f);

        const float k = 1.0f - eta * eta * (1.0f - cosI * cosI);
        if (k < 0.0f) {
            result.totalInternalReflection = true;
            // Reflected fallback
            refracted[c] = {
                I.x + 2.0f * cosI * N.x,
                I.y + 2.0f * cosI * N.y,
                I.z + 2.0f * cosI * N.z
            };
        } else {
            const float cosT = std::sqrt(k);
            refracted[c] = {
                eta * I.x + (eta * cosI - cosT) * N.x,
                eta * I.y + (eta * cosI - cosT) * N.y,
                eta * I.z + (eta * cosI - cosT) * N.z
            };
        }
    }

    result.refractedRed = refracted[0];
    result.refractedGreen = refracted[1];
    result.refractedBlue = refracted[2];

    const float dotRB = refracted[0].x * refracted[2].x + refracted[0].y * refracted[2].y + refracted[0].z * refracted[2].z;
    result.angularSeparationRadians = std::acos(std::clamp(dotRB, -1.0f, 1.0f));
    return result;
}

AtmosphericMediumProperties SculptureVolumetricAtmosphere::getProfileProperties(MediumProfile profile) noexcept {
    AtmosphericMediumProperties props{};
    switch (profile) {
    case MediumProfile::SunsetAerosol:
        props.absorptionCoeff = SculptureVec3{0.05f, 0.15f, 0.38f}; // High blue absorption
        props.scatteringCoeff = SculptureVec3{0.92f, 0.72f, 0.42f}; // Warm golden forward scattering
        props.asymmetryFactor = 0.68f;
        props.mieRayleighBlend = 0.80f;
        props.densityMultiplier = 0.75f;
        props.emissionRadiance = SculptureVec3{0.02f, 0.01f, 0.00f};
        break;

    case MediumProfile::DeepOceanNebula:
        props.absorptionCoeff = SculptureVec3{0.48f, 0.14f, 0.04f}; // High red absorption
        props.scatteringCoeff = SculptureVec3{0.45f, 0.78f, 0.96f}; // Deep cyan penetration
        props.asymmetryFactor = 0.35f;
        props.mieRayleighBlend = 0.50f;
        props.densityMultiplier = 0.90f;
        props.emissionRadiance = SculptureVec3{0.00f, 0.03f, 0.06f};
        break;

    case MediumProfile::QuaternionicPlasma:
        props.absorptionCoeff = SculptureVec3{0.06f, 0.32f, 0.08f}; // Green absorption
        props.scatteringCoeff = SculptureVec3{0.95f, 0.38f, 0.98f}; // Violet-magenta scattering
        props.asymmetryFactor = 0.50f;
        props.mieRayleighBlend = 0.70f;
        props.densityMultiplier = 0.85f;
        props.emissionRadiance = SculptureVec3{0.18f, 0.04f, 0.28f};
        break;

    case MediumProfile::EtherealGold:
        props.absorptionCoeff = SculptureVec3{0.08f, 0.14f, 0.42f};
        props.scatteringCoeff = SculptureVec3{0.98f, 0.86f, 0.50f};
        props.asymmetryFactor = 0.72f;
        props.mieRayleighBlend = 0.85f;
        props.densityMultiplier = 1.10f;
        props.emissionRadiance = SculptureVec3{0.06f, 0.04f, 0.01f};
        break;

    case MediumProfile::NeutralMist:
    default:
        props.absorptionCoeff = SculptureVec3{0.12f, 0.12f, 0.12f};
        props.scatteringCoeff = SculptureVec3{0.78f, 0.78f, 0.78f};
        props.asymmetryFactor = 0.30f;
        props.mieRayleighBlend = 0.60f;
        props.densityMultiplier = 0.65f;
        props.emissionRadiance = SculptureVec3{0.0f, 0.0f, 0.0f};
        break;
    }
    return props;
}

SculptureVec3 SculptureVolumetricAtmosphere::computeShadowTransmittance(
    const SculptureVec3& samplePos,
    const SculptureVec3& lightDir,
    const SculptureProgram& program,
    float time,
    float morph,
    const std::vector<float>& spatialBuffer
) const noexcept {
    if (!config_.enableShadowRays || config_.shadowRaySteps == 0U) {
        return {1.0f, 1.0f, 1.0f};
    }

    const float shadowMarchDistance = 4.5f;
    const float stepSize = shadowMarchDistance / static_cast<float>(config_.shadowRaySteps);
    const auto& medium = config_.medium;

    const SculptureVec3 extinction = add(medium.absorptionCoeff, medium.scatteringCoeff);
    SculptureVec3 opticalDepth{0.0f, 0.0f, 0.0f};

    for (std::uint32_t step = 0; step < config_.shadowRaySteps; ++step) {
        const float t = (static_cast<float>(step) + 0.5f) * stepSize;
        const SculptureVec3 pos{
            samplePos.x + lightDir.x * t,
            samplePos.y + lightDir.y * t,
            samplePos.z + lightDir.z * t
        };

        const float density = program.sampleDensityField(pos, time, morph, spatialBuffer) * medium.densityMultiplier;
        if (density > 0.001f) {
            opticalDepth.x += extinction.x * density * stepSize;
            opticalDepth.y += extinction.y * density * stepSize;
            opticalDepth.z += extinction.z * density * stepSize;
        }

        // Fast early out if shadow ray is completely opaque
        if (opticalDepth.x > 8.0f && opticalDepth.y > 8.0f && opticalDepth.z > 8.0f) {
            return {0.0f, 0.0f, 0.0f};
        }
    }

    return expVec(scale(opticalDepth, -1.0f));
}

VolumetricRadianceResult SculptureVolumetricAtmosphere::integrateLightTransport(
    const SculptureRay& ray,
    float maxDistance,
    const SculptureProgram& program,
    float time,
    float morph,
    const std::vector<float>& spatialBuffer
) const noexcept {
    VolumetricRadianceResult result{};
    if (!config_.enableVolumetricInScattering || config_.primaryRaySteps == 0U) {
        return result;
    }

    const float marchLimit = std::min(maxDistance, config_.maxMarchDistance);
    const float stepSize = marchLimit / static_cast<float>(config_.primaryRaySteps);
    const auto& medium = config_.medium;

    const SculptureVec3 extinction = add(medium.absorptionCoeff, medium.scatteringCoeff);

    // Compute angular phase alignment cos(theta) between ray and light
    const float cosTheta = dot(ray.direction, config_.keyLightDirection);
    const float phaseFactor = dualLobePhase(cosTheta, medium.asymmetryFactor, medium.mieRayleighBlend);

    SculptureVec3 accumulatedRadiance{0.0f, 0.0f, 0.0f};
    SculptureVec3 currentTransmittance{1.0f, 1.0f, 1.0f};
    float totalOpticalDepth = 0.0f;

    for (std::uint32_t step = 0; step < config_.primaryRaySteps; ++step) {
        const float t = (static_cast<float>(step) + 0.5f) * stepSize;
        const SculptureVec3 pos{
            ray.origin.x + ray.direction.x * t,
            ray.origin.y + ray.direction.y * t,
            ray.origin.z + ray.direction.z * t
        };

        const float density = program.sampleDensityField(pos, time, morph, spatialBuffer) * medium.densityMultiplier;
        result.evaluatedSteps++;

        if (density > 0.001f) {
            // Step optical depth
            const SculptureVec3 stepOpticalDepth = scale(extinction, density * stepSize);
            const SculptureVec3 stepTransmittance = expVec(scale(stepOpticalDepth, -1.0f));

            // Shadow transmittance towards key light
            const SculptureVec3 shadowT = computeShadowTransmittance(
                pos, config_.keyLightDirection, program, time, morph, spatialBuffer
            );

            // Direct in-scattering from key light + ambient sky illumination + intrinsic emission
            const SculptureVec3 directLight = multiply(config_.keyLightRadiance, shadowT);
            const SculptureVec3 scatterSource = add(
                scale(directLight, phaseFactor),
                add(config_.ambientRadiance, medium.emissionRadiance)
            );

            // In-scattered radiance delta: L_i = T * sigma_s * rho * S * dt
            const SculptureVec3 inscatterDelta = multiply(
                currentTransmittance,
                scale(multiply(medium.scatteringCoeff, scatterSource), density * stepSize)
            );

            accumulatedRadiance = add(accumulatedRadiance, inscatterDelta);
            currentTransmittance = multiply(currentTransmittance, stepTransmittance);
            totalOpticalDepth += (stepOpticalDepth.x + stepOpticalDepth.y + stepOpticalDepth.z) * 0.33333f;

            // Early ray termination when transmittance reaches negligible levels (<0.1% luminance)
            const float lum = (currentTransmittance.x + currentTransmittance.y + currentTransmittance.z) * 0.33333f;
            if (lum < 0.001f) {
                break;
            }
        }
    }

    result.inscatteredRadiance = accumulatedRadiance;
    result.transmittance = currentTransmittance;
    result.integratedOpticalDepth = totalOpticalDepth;
    return result;
}

} // namespace epi_sculpt
