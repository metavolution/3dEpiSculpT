#include "SculptureAtmosphereEngine.h"

#include <cmath>
#include <algorithm>
#include <numbers>

namespace epi_sculpt {

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kInv4Pi = 1.0f / (4.0f * kPi);
constexpr float kRayleighFactor = 3.0f / (16.0f * kPi);

inline SculptureVec3 add(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline SculptureVec3 mul(const SculptureVec3& a, float s) noexcept {
    return {a.x * s, a.y * s, a.z * s};
}

inline SculptureVec3 mulComp(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}

inline float dot(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float length(const SculptureVec3& v) noexcept {
    return std::sqrt(dot(v, v));
}

inline SculptureVec3 normalize(const SculptureVec3& v) noexcept {
    const float len = length(v);
    if (len <= 1e-6f) return {0.0f, 1.0f, 0.0f};
    return mul(v, 1.0f / len);
}

inline float sampleSpatialGrid(const SculptureVec3& p, const std::vector<float>& buffer) noexcept {
    if (buffer.size() < 125000) return 0.0f; // 50^3 grid

    // Normalized bounds [-2.5, 2.5] -> [0, 49]
    constexpr float kHalfExtent = 2.5f;
    constexpr float kInvSpan = 49.0f / (2.0f * kHalfExtent);

    const float gx = (p.x + kHalfExtent) * kInvSpan;
    const float gy = (p.y + kHalfExtent) * kInvSpan;
    const float gz = (p.z + kHalfExtent) * kInvSpan;

    if (gx < 0.0f || gx > 49.0f || gy < 0.0f || gy > 49.0f || gz < 0.0f || gz > 49.0f) {
        return 0.0f;
    }

    const auto ix = static_cast<std::size_t>(gx);
    const auto iy = static_cast<std::size_t>(gy);
    const auto iz = static_cast<std::size_t>(gz);

    const std::size_t idx = iz * 2500U + iy * 50U + ix;
    return (idx < buffer.size()) ? buffer[idx] : 0.0f;
}

} // namespace

float SculptureAtmosphereEngine::evaluateHenyeyGreenstein(float cosTheta, float g) noexcept {
    const float g2 = g * g;
    float denom = 1.0f + g2 - 2.0f * g * cosTheta;
    denom = std::max(denom, 1e-4f);
    return kInv4Pi * (1.0f - g2) / (denom * std::sqrt(denom));
}

float SculptureAtmosphereEngine::evaluateRayleigh(float cosTheta) noexcept {
    return kRayleighFactor * (1.0f + cosTheta * cosTheta);
}

float SculptureAtmosphereEngine::evaluateDualPhase(float cosTheta, float g, float mieBlend) noexcept {
    const float u = std::clamp(mieBlend, 0.0f, 1.0f);
    const float rayleigh = evaluateRayleigh(cosTheta);
    const float mie = evaluateHenyeyGreenstein(cosTheta, g);
    return (1.0f - u) * rayleigh + u * mie;
}

SculptureVec3 SculptureAtmosphereEngine::evaluateTransmittance(
    const SculptureVec3& extinctionCoeff,
    float distance
) noexcept {
    return {
        std::exp(-extinctionCoeff.x * distance),
        std::exp(-extinctionCoeff.y * distance),
        std::exp(-extinctionCoeff.z * distance)
    };
}

float SculptureAtmosphereEngine::evaluateHeightFogDensity(
    const SculptureVec3& p,
    float baseDensity,
    float heightScale
) noexcept {
    const float h = p.y;
    const float hs = std::max(heightScale, 0.1f);
    const float rho = baseDensity * std::exp(-h / hs);
    return std::max(rho, 0.0f);
}

float SculptureAtmosphereEngine::evaluateSolarShadowTransmittance(
    const SculptureVec3& samplePos,
    const SculptureVec3& sunDir,
    const std::vector<float>& spatialBuffer,
    float shadowScale,
    std::uint32_t shadowSteps
) noexcept {
    if (spatialBuffer.empty() || shadowSteps == 0) return 1.0f;

    const std::uint32_t steps = std::min(shadowSteps, 16U);
    constexpr float maxShadowDist = 4.0f;
    const float ds = maxShadowDist / static_cast<float>(steps);

    float opticalDepth = 0.0f;
    for (std::uint32_t i = 1; i <= steps; ++i) {
        const float dist = (static_cast<float>(i) - 0.5f) * ds;
        const SculptureVec3 p = add(samplePos, mul(sunDir, dist));
        const float density = sampleSpatialGrid(p, spatialBuffer);
        opticalDepth += density * ds * shadowScale;
    }

    return std::exp(-opticalDepth);
}

AtmosphericScatteringResult SculptureAtmosphereEngine::integrateInScattering(
    const SculptureVec3& rayOrigin,
    const SculptureVec3& rayDirection,
    float marchDistance,
    const AtmosphericVolumeConfig& config,
    const std::vector<float>& spatialBuffer,
    float time
) noexcept {
    (void)time;
    AtmosphericScatteringResult result{};
    result.transmittanceLinear = {1.0f, 1.0f, 1.0f};

    const float dist = std::clamp(marchDistance, 0.0f, config.maxDistance);
    if (dist <= 1e-4f || config.marchSteps == 0) return result;

    const std::uint32_t numSteps = std::clamp(config.marchSteps, 4U, 128U);
    const float ds = dist / static_cast<float>(numSteps);

    const SculptureVec3 normalizedSun = normalize(config.sunDirection);
    const SculptureVec3 normalizedRay = normalize(rayDirection);
    const float cosTheta = dot(normalizedRay, normalizedSun);

    const float phaseValue = evaluateDualPhase(cosTheta, config.fogAnisotropy, config.mieRayleighBlend);
    const SculptureVec3 extinctionCoeff = add(config.scatteringCoeff, config.absorptionCoeff);

    SculptureVec3 accumulatedInScattering{0.0f, 0.0f, 0.0f};
    SculptureVec3 currentTransmittance{1.0f, 1.0f, 1.0f};
    float totalOpticalDepth = 0.0f;

    for (std::uint32_t i = 0; i < numSteps; ++i) {
        const float s = (static_cast<float>(i) + 0.5f) * ds;
        const SculptureVec3 p = add(rayOrigin, mul(normalizedRay, s));

        // Evaluate medium density at p (Height fog + spatial fluid memory)
        float localDensity = evaluateHeightFogDensity(p, config.fogBaseDensity, config.fogHeightScale);
        if (!spatialBuffer.empty()) {
            localDensity += 0.75f * sampleSpatialGrid(p, spatialBuffer);
        }

        if (localDensity <= 1e-5f) continue;

        // Shadow transmittance along solar ray
        const float shadowTransmittance = evaluateSolarShadowTransmittance(
            p, normalizedSun, spatialBuffer, config.shadowDensityScale, config.shadowSteps
        );

        // Direct solar in-scattering
        const SculptureVec3 directSun = mul(config.sunRadiance, phaseValue * shadowTransmittance);

        // Ambient sky dome in-scattering
        const SculptureVec3 ambientLight = mul(config.skyAmbient, kInv4Pi);

        // Total in-scattered radiance slice
        const SculptureVec3 totalSource = mulComp(add(directSun, ambientLight), config.scatteringCoeff);
        const SculptureVec3 stepInScattering = mul(mulComp(totalSource, currentTransmittance), localDensity * ds);

        accumulatedInScattering = add(accumulatedInScattering, stepInScattering);

        // Attenuate ray transmittance via Beer-Lambert
        const SculptureVec3 stepExtinction = mul(extinctionCoeff, localDensity * ds);
        currentTransmittance = mulComp(currentTransmittance, {
            std::exp(-stepExtinction.x),
            std::exp(-stepExtinction.y),
            std::exp(-stepExtinction.z)
        });

        totalOpticalDepth += (stepExtinction.x + stepExtinction.y + stepExtinction.z) * 0.333333f;
    }

    result.inScatteringLinear = accumulatedInScattering;
    result.transmittanceLinear = currentTransmittance;
    result.opticalDepth = totalOpticalDepth;
    return result;
}

SculptureVec3 SculptureAtmosphereEngine::computeSunShaftBurst(
    const SculptureVec3& screenUv,
    const SculptureVec3& sunScreenPos,
    const SculptureVec3& sunColor,
    float density,
    float decay,
    float weight,
    std::uint32_t samples
) noexcept {
    const std::uint32_t N = std::clamp(samples, 4U, 64U);
    const float du = (sunScreenPos.x - screenUv.x) / static_cast<float>(N);
    const float dv = (sunScreenPos.y - screenUv.y) / static_cast<float>(N);

    SculptureVec3 accumulated{0.0f, 0.0f, 0.0f};
    float currentDecay = 1.0f;

    for (std::uint32_t i = 0; i < N; ++i) {
        const float sampleX = screenUv.x + du * static_cast<float>(i);
        const float sampleY = screenUv.y + dv * static_cast<float>(i);

        const float distFromSun = std::sqrt(
            (sampleX - sunScreenPos.x) * (sampleX - sunScreenPos.x) +
            (sampleY - sunScreenPos.y) * (sampleY - sunScreenPos.y)
        );

        const float shaftIntensity = std::exp(-distFromSun * density * 3.0f);
        accumulated = add(accumulated, mul(sunColor, shaftIntensity * currentDecay * weight));
        currentDecay *= decay;
    }

    return accumulated;
}

} // namespace epi_sculpt
