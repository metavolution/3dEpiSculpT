#include "SculptureTwistorSpacetime.h"
#include <algorithm>
#include <numbers>

namespace epi_sculpt {

namespace {
constexpr float kPi = std::numbers::pi_v<float>;
constexpr float kTwoPi = 2.0f * kPi;
constexpr float kInvSqrt2 = 0.7071067811865475f;
}

TwistorSpinor SculptureTwistorSpacetime::evaluateIncidenceTwistor(
    float t, float x, float y, float z,
    const std::complex<float>& pi0,
    const std::complex<float>& pi1
) noexcept {
    // Penrose Incidence Relation: \\omega^A = i x^{AA\x27} \\pi_{A\x27}
    // x^{AA\x27} = 1/sqrt(2) * [[t + z, x - i*y], [x + i*y, t - z]]
    const std::complex<float> iUnit{0.0f, 1.0f};

    const std::complex<float> x00 = kInvSqrt2 * (t + z);
    const std::complex<float> x01 = kInvSqrt2 * std::complex<float>(x, -y);
    const std::complex<float> x10 = kInvSqrt2 * std::complex<float>(x, y);
    const std::complex<float> x11 = kInvSqrt2 * (t - z);

    const std::complex<float> omega0 = iUnit * (x00 * pi0 + x01 * pi1);
    const std::complex<float> omega1 = iUnit * (x10 * pi0 + x11 * pi1);

    return TwistorSpinor{
        .omega0 = omega0,
        .omega1 = omega1,
        .pi0 = pi0,
        .pi1 = pi1
    };
}

SculptureVec3 SculptureTwistorSpacetime::evaluateRobinsonVelocity(
    const SculptureVec3& pos,
    float time,
    float twistRate
) noexcept {
    const float rSq = pos.x * pos.x + pos.y * pos.y + pos.z * pos.z;
    const float denom = 1.0f + rSq;

    const float cosT = std::cos(time * 0.5f);
    const float sinT = std::sin(time * 0.5f);

    // Robinson shear-free null congruence field with conformal time precession
    const float vx = (-pos.y + twistRate * pos.z * pos.x) / denom;
    const float vy = ( pos.x + twistRate * pos.z * pos.y) / denom;
    const float vz = (1.0f - twistRate * (pos.x * pos.x + pos.y * pos.y)) / denom;

    const float rotX = vx * cosT - vy * sinT;
    const float rotY = vx * sinT + vy * cosT;

    const float len = std::sqrt(rotX * rotX + rotY * rotY + vz * vz);
    if (len < 1e-6f) return {0.0f, 0.0f, 1.0f};
    return {rotX / len, rotY / len, vz / len};
}

float SculptureTwistorSpacetime::evaluateOuterHorizonRadius(
    float mass, float spin, float charge
) noexcept {
    const float radicand = mass * mass - spin * spin - charge * charge;
    return mass + std::sqrt(std::max(0.0f, radicand));
}

float SculptureTwistorSpacetime::evaluateErgosphereRadius(
    float theta, float mass, float spin, float charge
) noexcept {
    const float cosTheta = std::cos(theta);
    const float radicand = mass * mass - (spin * spin) * (cosTheta * cosTheta) - charge * charge;
    return mass + std::sqrt(std::max(0.0f, radicand));
}

float SculptureTwistorSpacetime::evaluateFrameDraggingOmega(
    float r, float theta, float mass, float spin, float charge
) noexcept {
    const float r2 = r * r;
    const float a2 = spin * spin;
    const float sinTheta = std::sin(theta);
    const float sin2 = sinTheta * sinTheta;

    const float delta = r2 - 2.0f * mass * r + a2 + charge * charge;
    const float sigma = (r2 + a2) * (r2 + a2) - a2 * delta * sin2;

    if (sigma <= 1e-6f) return 0.0f;
    return ((2.0f * mass * r - charge * charge) * spin) / sigma;
}

SculptureVec3 SculptureTwistorSpacetime::boyerLindquistToCartesian(
    float r, float theta, float phi, float spin
) noexcept {
    const float rho = std::sqrt(r * r + spin * spin);
    const float sinTheta = std::sin(theta);
    return SculptureVec3{
        rho * sinTheta * std::cos(phi),
        rho * sinTheta * std::sin(phi),
        r * std::cos(theta)
    };
}

std::vector<SculptureSample> SculptureTwistorSpacetime::generateErgosphereOrbits(
    const KerrNewmanConfig& config,
    float time
) {
    std::vector<SculptureSample> samples;
    const std::size_t numRings = 32U;
    const std::size_t ptsPerRing = std::max(config.orbitSampleCount / numRings, 16UL);
    samples.reserve(numRings * ptsPerRing);

    const float rH = evaluateOuterHorizonRadius(config.mass, config.spin, config.charge);
    const float rE_max = evaluateErgosphereRadius(kPi * 0.5f, config.mass, config.spin, config.charge);

    for (std::size_t ring = 0; ring < numRings; ++ring) {
        const float u = static_cast<float>(ring) / static_cast<float>(numRings - 1U);
        // Radii spanning from just outside horizon up to accretion disk
        const float r = std::lerp(rH * 1.02f, config.diskOuterRadius, u);
        const float theta = (ring % 2 == 0)
            ? kPi * 0.5f // Equatorial
            : std::lerp(0.2f * kPi, 0.8f * kPi, std::sin(u * 6.28318f) * 0.5f + 0.5f); // Inclined

        const float omegaDrag = evaluateFrameDraggingOmega(r, theta, config.mass, config.spin, config.charge) * config.frameDragStrength;
        // Keplerian orbital frequency in Kerr spacetime
        const float omegaOrb = 1.0f / (std::pow(r, 1.5f) + config.spin);
        const float totalOmega = omegaDrag + omegaOrb;

        const bool inErgosphere = (r <= rE_max);

        for (std::size_t i = 0; i < ptsPerRing; ++i) {
            const float phi0 = (static_cast<float>(i) / static_cast<float>(ptsPerRing)) * kTwoPi;
            const float phi = phi0 + totalOmega * time;

            SculptureVec3 pos = boyerLindquistToCartesian(r, theta, phi, config.spin);
            pos.x *= config.horizonScale;
            pos.y *= config.horizonScale;
            pos.z *= config.horizonScale;

            SculptureSample s{};
            s.x = pos.x;
            s.y = pos.y;
            s.z = pos.z;
            s.radius = inErgosphere ? 0.055f : 0.038f;

            // Relativistic color palette: Ergosphere glow vs Accretion Doppler
            if (inErgosphere) {
                // Gold / Amber Ergoregion frame dragging
                s.r = 1.0f;
                s.g = 0.72f + 0.25f * std::sin(phi * 3.0f);
                s.b = 0.15f;
            } else {
                // Cyan-Blue accretion disk with relativistic beaming
                float beaming = std::clamp(0.5f + 0.5f * std::cos(phi), 0.0f, 1.0f);
                s.r = 0.15f + 0.35f * beaming;
                s.g = 0.55f + 0.40f * beaming;
                s.b = 0.95f;
            }
            s.a = 1.0f;

            // Velocity vector
            const float vSpd = r * totalOmega;
            s.vx = -pos.y * totalOmega;
            s.vy =  pos.x * totalOmega;
            s.vz = 0.0f;
            s.vw = vSpd;

            samples.push_back(s);
        }
    }
    return samples;
}

std::vector<SculptureSample> SculptureTwistorSpacetime::generateRobinsonCongruenceRays(
    const TwistorCongruenceConfig& config,
    float time
) {
    std::vector<SculptureSample> samples;
    samples.reserve(config.rayCount * config.samplesPerRay);

    for (std::size_t r = 0; r < config.rayCount; ++r) {
        const float u = static_cast<float>(r) / static_cast<float>(config.rayCount);
        const float theta = u * kTwoPi;
        const float ringRadius = 1.6f + 0.8f * std::sin(theta * 3.0f + time * config.timeEvolutionSpeed);

        SculptureVec3 rayOrigin{
            ringRadius * std::cos(theta),
            ringRadius * std::sin(theta),
            -config.rayLength * 0.5f
        };

        SculptureVec3 currPos = rayOrigin;
        const float step = config.rayLength / static_cast<float>(config.samplesPerRay);

        for (std::size_t s = 0; s < config.samplesPerRay; ++s) {
            SculptureVec3 vel = evaluateRobinsonVelocity(currPos, time * config.timeEvolutionSpeed, config.twistRate);

            SculptureSample sample{};
            sample.x = currPos.x;
            sample.y = currPos.y;
            sample.z = currPos.z;
            sample.radius = 0.035f;

            // Twistor helicity phase color mapping
            const float phase = static_cast<float>(s) / static_cast<float>(config.samplesPerRay);
            sample.r = std::clamp(0.5f + 0.45f * std::sin(phase * kTwoPi + config.helicity * kPi), 0.0f, 1.0f);
            sample.g = std::clamp(0.5f + 0.45f * std::sin(phase * kTwoPi + kPi * 0.66f), 0.0f, 1.0f);
            sample.b = std::clamp(0.5f + 0.45f * std::sin(phase * kTwoPi + kPi * 1.33f), 0.0f, 1.0f);
            sample.a = 1.0f;

            sample.vx = vel.x;
            sample.vy = vel.y;
            sample.vz = vel.z;
            sample.vw = 1.0f;

            samples.push_back(sample);

            currPos.x += vel.x * step;
            currPos.y += vel.y * step;
            currPos.z += vel.z * step;
        }
    }
    return samples;
}

SculptureSnapshot SculptureTwistorSpacetime::generateSnapshot(
    const KerrNewmanConfig& kerrConfig,
    const TwistorCongruenceConfig& twistorConfig,
    float time,
    float morph
) {
    SculptureSnapshot snapshot{};
    snapshot.time = time;
    snapshot.morph = morph;

    const auto ergoSamples = generateErgosphereOrbits(kerrConfig, time);
    const auto twistorSamples = generateRobinsonCongruenceRays(twistorConfig, time);

    std::size_t writeIdx = 0;
    for (const auto& s : ergoSamples) {
        if (writeIdx < snapshot.samples.size()) {
            snapshot.samples[writeIdx++] = s;
        }
    }
    for (const auto& s : twistorSamples) {
        if (writeIdx < snapshot.samples.size()) {
            snapshot.samples[writeIdx++] = s;
        }
    }

    return snapshot;
}

IsosurfaceMesh SculptureTwistorSpacetime::generateErgosphereMesh(
    const KerrNewmanConfig& config,
    std::size_t uSegments,
    std::size_t vSegments
) {
    IsosurfaceMesh mesh{};
    mesh.vertices.reserve((uSegments + 1U) * (vSegments + 1U));
    mesh.indices.reserve(uSegments * vSegments * 6U);

    for (std::size_t j = 0; j <= vSegments; ++j) {
        const float v = static_cast<float>(j) / static_cast<float>(vSegments);
        const float theta = v * kPi;
        const float rE = evaluateErgosphereRadius(theta, config.mass, config.spin, config.charge) * config.horizonScale;

        for (std::size_t i = 0; i <= uSegments; ++i) {
            const float u = static_cast<float>(i) / static_cast<float>(uSegments);
            const float phi = u * kTwoPi;

            SculptureVec3 pos = boyerLindquistToCartesian(rE, theta, phi, config.spin);

            IsosurfaceVertex vert{};
            vert.position = pos;

            // Normal vector
            const float len = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
            if (len > 1e-5f) {
                vert.normal = {pos.x / len, pos.y / len, pos.z / len};
            } else {
                vert.normal = {0.0f, 1.0f, 0.0f};
            }

            vert.materialCoord = u;
            vert.density = 1.0f;

            mesh.vertices.push_back(vert);
        }
    }

    const std::uint32_t stride = static_cast<std::uint32_t>(uSegments + 1U);
    for (std::uint32_t j = 0; j < vSegments; ++j) {
        for (std::uint32_t i = 0; i < uSegments; ++i) {
            const std::uint32_t p0 = j * stride + i;
            const std::uint32_t p1 = j * stride + (i + 1U);
            const std::uint32_t p2 = (j + 1U) * stride + i;
            const std::uint32_t p3 = (j + 1U) * stride + (i + 1U);

            mesh.indices.push_back(p0);
            mesh.indices.push_back(p2);
            mesh.indices.push_back(p1);

            mesh.indices.push_back(p1);
            mesh.indices.push_back(p2);
            mesh.indices.push_back(p3);
        }
    }

    return mesh;
}

IsosurfaceMesh SculptureTwistorSpacetime::generateEventHorizonMesh(
    const KerrNewmanConfig& config,
    std::size_t uSegments,
    std::size_t vSegments
) {
    IsosurfaceMesh mesh{};
    mesh.vertices.reserve((uSegments + 1U) * (vSegments + 1U));
    mesh.indices.reserve(uSegments * vSegments * 6U);

    const float rH = evaluateOuterHorizonRadius(config.mass, config.spin, config.charge) * config.horizonScale;

    for (std::size_t j = 0; j <= vSegments; ++j) {
        const float v = static_cast<float>(j) / static_cast<float>(vSegments);
        const float theta = v * kPi;

        for (std::size_t i = 0; i <= uSegments; ++i) {
            const float u = static_cast<float>(i) / static_cast<float>(uSegments);
            const float phi = u * kTwoPi;

            SculptureVec3 pos = boyerLindquistToCartesian(rH, theta, phi, config.spin);

            IsosurfaceVertex vert{};
            vert.position = pos;

            const float len = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
            if (len > 1e-5f) {
                vert.normal = {pos.x / len, pos.y / len, pos.z / len};
            } else {
                vert.normal = {0.0f, 1.0f, 0.0f};
            }

            vert.materialCoord = u;
            vert.density = 0.0f;

            mesh.vertices.push_back(vert);
        }
    }

    const std::uint32_t stride = static_cast<std::uint32_t>(uSegments + 1U);
    for (std::uint32_t j = 0; j < vSegments; ++j) {
        for (std::uint32_t i = 0; i < uSegments; ++i) {
            const std::uint32_t p0 = j * stride + i;
            const std::uint32_t p1 = j * stride + (i + 1U);
            const std::uint32_t p2 = (j + 1U) * stride + i;
            const std::uint32_t p3 = (j + 1U) * stride + (i + 1U);

            mesh.indices.push_back(p0);
            mesh.indices.push_back(p2);
            mesh.indices.push_back(p1);

            mesh.indices.push_back(p1);
            mesh.indices.push_back(p2);
            mesh.indices.push_back(p3);
        }
    }

    return mesh;
}

} // namespace epi_sculpt
