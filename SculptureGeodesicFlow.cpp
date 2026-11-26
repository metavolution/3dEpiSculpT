#include "SculptureGeodesicFlow.h"

#include <algorithm>
#include <cmath>

namespace epi_sculpt {

namespace {

inline float minkowskiDot(const SculptureVec4& a, const SculptureVec4& b) noexcept {
    return -a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float clampLength(float val, float maxVal) noexcept {
    return std::clamp(val, -maxVal, maxVal);
}

} // namespace

SculptureVec4 SculptureGeodesicFlow::poincareToLorentz(const SculptureVec3& p) noexcept {
    const float r2 = std::min(p.x * p.x + p.y * p.y + p.z * p.z, 0.9999f);
    const float invDenom = 1.0f / (1.0f - r2);
    return SculptureVec4{
        .x = 2.0f * p.x * invDenom,
        .y = 2.0f * p.y * invDenom,
        .z = 2.0f * p.z * invDenom,
        .w = (1.0f + r2) * invDenom // Timelike coordinate x0 >= 1
    };
}

SculptureVec3 SculptureGeodesicFlow::lorentzToPoincare(const SculptureVec4& h) noexcept {
    const float denom = std::max(1.0f + h.w, 1.0e-5f);
    const float invDenom = 1.0f / denom;
    return SculptureVec3{
        .x = h.x * invDenom,
        .y = h.y * invDenom,
        .z = h.z * invDenom
    };
}

float SculptureGeodesicFlow::poincareMetricScale(const SculptureVec3& p) noexcept {
    const float r2 = std::min(p.x * p.x + p.y * p.y + p.z * p.z, 0.999f);
    return 2.0f / (1.0f - r2);
}

float SculptureGeodesicFlow::hyperbolicDistance(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    const float rA2 = std::min(a.x * a.x + a.y * a.y + a.z * a.z, 0.999f);
    const float rB2 = std::min(b.x * b.x + b.y * b.y + b.z * b.z, 0.999f);
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    const float diff2 = dx * dx + dy * dy + dz * dz;

    const float delta = 2.0f * diff2 / std::max((1.0f - rA2) * (1.0f - rB2), 1.0e-6f);
    const float coshD = 1.0f + delta;
    return std::acosh(std::max(coshD, 1.0f));
}

SculptureVec4 SculptureGeodesicFlow::reflectMinkowski(
    const SculptureVec4& p,
    const SculptureVec4& normal
) noexcept {
    const float nDotN = minkowskiDot(normal, normal);
    if (std::abs(nDotN) < 1.0e-6f) return p;
    const float pDotN = minkowskiDot(p, normal);
    const float factor = 2.0f * pDotN / nDotN;
    return SculptureVec4{
        .x = p.x - factor * normal.x,
        .y = p.y - factor * normal.y,
        .z = p.z - factor * normal.z,
        .w = p.w - factor * normal.w
    };
}

SculptureVec3 SculptureGeodesicFlow::advanceGeodesic(
    const SculptureVec3& position,
    const SculptureVec3& velocity,
    float t
) noexcept {
    if (std::abs(t) < 1.0e-6f) return position;

    const SculptureVec4 P = poincareToLorentz(position);

    // Tangent mapping from Poincare to Lorentz space
    const float lambda = poincareMetricScale(position);
    const float vScale = 2.0f / (lambda * lambda);
    SculptureVec4 V{
        .x = velocity.x * vScale,
        .y = velocity.y * vScale,
        .z = velocity.z * vScale,
        .w = 0.0f
    };

    // Ensure V is orthogonal to P in Minkowski metric (<P, V>_M = 0)
    const float pDotV = minkowskiDot(P, V);
    V.w = pDotV / std::max(P.w, 1.0e-5f);

    const float vNormSq = std::max(minkowskiDot(V, V), 0.0f);
    const float speed = std::sqrt(vNormSq);

    if (speed < 1.0e-6f) return position;

    const float st = speed * t;
    const float coshTerm = std::cosh(st);
    const float sinhTerm = std::sinh(st) / speed;

    SculptureVec4 Q{
        .x = P.x * coshTerm + V.x * sinhTerm,
        .y = P.y * coshTerm + V.y * sinhTerm,
        .z = P.z * coshTerm + V.z * sinhTerm,
        .w = P.w * coshTerm + V.w * sinhTerm
    };

    return lorentzToPoincare(Q);
}

SculptureVec3 SculptureGeodesicFlow::evaluateLoxodromicFlow(
    const SculptureVec3& pos,
    float time,
    float pitch
) noexcept {
    // Loxodromic spiral: combination of hyperbolic translation along z-axis and Euclidean rotation around z
    const float r = std::sqrt(pos.x * pos.x + pos.y * pos.y);
    const float angle = std::atan2(pos.y, pos.x) + time * pitch;
    const float zDrift = std::tanh(std::atanh(std::clamp(pos.z, -0.99f, 0.99f)) + time * 0.1f);

    return SculptureVec3{
        .x = r * std::cos(angle),
        .y = r * std::sin(angle),
        .z = zDrift
    };
}

float SculptureGeodesicFlow::sampleHorosphereField(
    const SculptureVec3& p,
    float time,
    float frequency
) noexcept {
    const float r2 = std::min(p.x * p.x + p.y * p.y + p.z * p.z, 0.999f);
    const float distToIdeal = (1.0f - p.z) * (1.0f - p.z) + p.x * p.x + p.y * p.y;
    if (distToIdeal < 1.0e-6f) return 0.0f;

    const float horoHeight = (1.0f - r2) / distToIdeal;
    const float phase = std::log(std::max(horoHeight, 1.0e-5f)) * frequency - time;
    return std::sin(phase);
}

float SculptureGeodesicFlow::sampleHoneycombSDF(
    const SculptureVec3& p,
    HyperbolicHoneycombType type,
    std::uint32_t maxIterations
) noexcept {
    SculptureVec4 x = poincareToLorentz(p);

    // Coxeter mirror planes for regular hyperbolic honeycombs in Minkowski space
    const std::array<SculptureVec4, 4> mirrors = [&] {
        if (type == HyperbolicHoneycombType::Order5Cubic) {
            // {4, 3, 5} Cubic honeycomb mirrors
            return std::array<SculptureVec4, 4>{
                SculptureVec4{1.0f, 0.0f, 0.0f, 0.0f},
                SculptureVec4{0.0f, 1.0f, 0.0f, 0.0f},
                SculptureVec4{0.0f, 0.0f, 1.0f, 0.0f},
                SculptureVec4{-0.5f, -0.5f, -0.5f, 0.866025f} // Timelike mirror plane
            };
        } else if (type == HyperbolicHoneycombType::Order3Icosahedral) {
            // {3, 5, 3} Icosahedral honeycomb mirrors (Golden ratio phi = 1.618)
            constexpr float phi = 1.6180339887f;
            return std::array<SculptureVec4, 4>{
                SculptureVec4{1.0f, 0.0f, 0.0f, 0.0f},
                SculptureVec4{-0.5f, phi * 0.5f, 0.0f, 0.0f},
                SculptureVec4{0.0f, -0.5f, phi * 0.5f, 0.0f},
                SculptureVec4{0.0f, 0.0f, -1.0f, 1.17557f}
            };
        } else {
            // {5, 3, 5} Dodecahedral mirrors
            return std::array<SculptureVec4, 4>{
                SculptureVec4{1.0f, 0.0f, 0.0f, 0.0f},
                SculptureVec4{-0.7071f, 0.7071f, 0.0f, 0.0f},
                SculptureVec4{0.0f, -0.7071f, 0.7071f, 0.0f},
                SculptureVec4{0.0f, 0.0f, -0.8506f, 1.25f}
            };
        }
    }();

    float minDistance = 100.0f;
    for (std::uint32_t iter = 0; iter < maxIterations; ++iter) {
        for (const auto& mirror : mirrors) {
            const float dot = minkowskiDot(x, mirror);
            if (dot < 0.0f) {
                x = reflectMinkowski(x, mirror);
            }
            minDistance = std::min(minDistance, std::abs(dot));
        }
    }

    return minDistance;
}

void SculptureGeodesicFlow::applyGeodesicFlow(
    std::span<SculptureSample> samples,
    float time,
    const GeodesicFlowConfig& config
) noexcept {
    const float speed = config.flowSpeed;

    for (std::size_t i = 0; i < samples.size(); ++i) {
        auto& s = samples[i];
        SculptureVec3 pos{s.x, s.y, s.z};

        // Clip to boundary radius inside Poincare ball
        const float r = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
        if (r > config.boundaryRadius && r > 1.0e-5f) {
            const float scale = config.boundaryRadius / r;
            pos.x *= scale; pos.y *= scale; pos.z *= scale;
        }

        // Compute geodesic tangent vector from local potential gradient
        SculptureVec3 velocity{};
        if (config.honeycombType == HyperbolicHoneycombType::LoxodromicSpiral) {
            const auto target = evaluateLoxodromicFlow(pos, time * speed, config.loxodromicPitch);
            velocity = SculptureVec3{target.x - pos.x, target.y - pos.y, target.z - pos.z};
        } else if (config.honeycombType == HyperbolicHoneycombType::HorosphereWave) {
            const float eps = 0.01f;
            const float f0 = sampleHorosphereField(pos, time * speed, config.horosphereFrequency);
            const float fx = sampleHorosphereField({pos.x + eps, pos.y, pos.z}, time * speed, config.horosphereFrequency);
            const float fy = sampleHorosphereField({pos.x, pos.y + eps, pos.z}, time * speed, config.horosphereFrequency);
            const float fz = sampleHorosphereField({pos.x, pos.y, pos.z + eps}, time * speed, config.horosphereFrequency);
            velocity = SculptureVec3{
                -(fx - f0) / eps,
                -(fy - f0) / eps,
                -(fz - f0) / eps
            };
        } else {
            // Honeycomb Coxeter gradient flow
            const float eps = 0.01f;
            const float d0 = sampleHoneycombSDF(pos, config.honeycombType, static_cast<std::uint32_t>(config.foldingIterations));
            const float dx = sampleHoneycombSDF({pos.x + eps, pos.y, pos.z}, config.honeycombType, static_cast<std::uint32_t>(config.foldingIterations));
            const float dy = sampleHoneycombSDF({pos.x, pos.y + eps, pos.z}, config.honeycombType, static_cast<std::uint32_t>(config.foldingIterations));
            const float dz = sampleHoneycombSDF({pos.x, pos.y, pos.z + eps}, config.honeycombType, static_cast<std::uint32_t>(config.foldingIterations));
            velocity = SculptureVec3{
                -(dx - d0) / eps,
                -(dy - d0) / eps,
                -(dz - d0) / eps
            };
        }

        // Integrate geodesic streamflow
        const SculptureVec3 newPos = advanceGeodesic(pos, velocity, speed * 0.016667f);
        s.vx = newPos.x - s.x;
        s.vy = newPos.y - s.y;
        s.vz = newPos.z - s.z;
        s.x = newPos.x;
        s.y = newPos.y;
        s.z = newPos.z;

        // Optical radius and normal scaling by Poincaré conformal metric lambda = 2 / (1 - r^2)
        const float lambda = poincareMetricScale(newPos);
        s.radius = std::clamp(s.radius / (1.0f + 0.1f * lambda), 0.002f, 0.35f);
    }
}

} // namespace epi_sculpt
