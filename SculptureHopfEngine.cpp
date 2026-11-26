#include "SculptureHopfEngine.h"
#include "SculptureDifferentialGeometry.h"
#include <cmath>
#include <algorithm>

namespace epi_sculpt {

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;

// Quaternionic multiplication: qA * qB
Quaternion4D quatMul(const Quaternion4D& a, const Quaternion4D& b) noexcept {
    return Quaternion4D{
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w
    };
}

} // namespace

SculptureVec3 SculptureHopfEngine::projectHopf(const Quaternion4D& q) noexcept {
    // Normalizes to ensure S^3 membership
    const float lenSq = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
    const float invLen = (lenSq > 1.0e-8f) ? (1.0f / std::sqrt(lenSq)) : 1.0f;
    const float w = q.w * invLen;
    const float x = q.x * invLen;
    const float y = q.y * invLen;
    const float z = q.z * invLen;

    return SculptureVec3{
        2.0f * (x * z + w * y),
        2.0f * (y * z - w * x),
        w * w + z * z - x * x - y * y
    };
}

SculptureVec3 SculptureHopfEngine::stereographicProject(const Quaternion4D& q, float scale) noexcept {
    return diff_geom::projectStereographicS3(SculptureVec4{q.x, q.y, q.z, q.w}, scale, 1.0f, 1.0e-5f);
}

Quaternion4D SculptureHopfEngine::inverseStereographicProject(const SculptureVec3& p, float scale) noexcept {
    const SculptureVec4 s3 = diff_geom::inverseStereographicS3(p, scale);
    return Quaternion4D{s3.w, s3.x, s3.y, s3.z};
}

HopfFiberPoint SculptureHopfEngine::evaluateFiberPoint(
    const SculptureVec3& basePointS2,
    float fiberAngle,
    float stereographicScale,
    float thetaL,
    float thetaR
) noexcept {
    // 1. Normalize S^2 base point
    const float len = std::sqrt(basePointS2.x * basePointS2.x + basePointS2.y * basePointS2.y + basePointS2.z * basePointS2.z);
    const float invLen = (len > 1.0e-6f) ? (1.0f / len) : 1.0f;
    const float ux = basePointS2.x * invLen;
    const float uy = basePointS2.y * invLen;
    const float uz = std::clamp(basePointS2.z * invLen, -1.0f, 1.0f);

    // 2. Spherical Coordinates on S^2
    const float eta = std::acos(uz);
    const float psi = std::atan2(uy, ux);

    const float r1 = std::cos(eta * 0.5f);
    const float r2 = std::sin(eta * 0.5f);

    const float theta1 = fiberAngle;
    const float theta2 = fiberAngle + psi;

    Quaternion4D q{
        r1 * std::cos(theta1),
        r1 * std::sin(theta1),
        r2 * std::cos(theta2),
        r2 * std::sin(theta2)
    };

    // 3. 4D Isoclinic Spin: q' = qL * q * qR
    if (std::abs(thetaL) > 1.0e-5f || std::abs(thetaR) > 1.0e-5f) {
        const Quaternion4D qL{std::cos(thetaL), 0.0f, 0.0f, std::sin(thetaL)};
        const Quaternion4D qR{std::cos(thetaR), 0.0f, 0.0f, std::sin(thetaR)};
        q = quatMul(quatMul(qL, q), qR);
    }

    // 4. Stereographic Projection into 3D Euclidean Space
    const SculptureVec3 pos = stereographicProject(q, stereographicScale);

    // Compute finite-difference tangent vector
    const float dXi = 0.01f;
    const float theta1_d = fiberAngle + dXi;
    const float theta2_d = fiberAngle + dXi + psi;
    Quaternion4D q_d{
        r1 * std::cos(theta1_d),
        r1 * std::sin(theta1_d),
        r2 * std::cos(theta2_d),
        r2 * std::sin(theta2_d)
    };
    if (std::abs(thetaL) > 1.0e-5f || std::abs(thetaR) > 1.0e-5f) {
        const Quaternion4D qL{std::cos(thetaL), 0.0f, 0.0f, std::sin(thetaL)};
        const Quaternion4D qR{std::cos(thetaR), 0.0f, 0.0f, std::sin(thetaR)};
        q_d = quatMul(quatMul(qL, q_d), qR);
    }
    const SculptureVec3 pos_d = stereographicProject(q_d, stereographicScale);

    const float tx = (pos_d.x - pos.x) / dXi;
    const float ty = (pos_d.y - pos.y) / dXi;
    const float tz = (pos_d.z - pos.z) / dXi;
    const float tLen = std::max(1.0e-6f, std::sqrt(tx * tx + ty * ty + tz * tz));

    return HopfFiberPoint{
        .position = pos,
        .tangent = SculptureVec3{tx / tLen, ty / tLen, tz / tLen},
        .torusRadius = r2,
        .phaseAngle = fiberAngle,
        .curvature = 1.0f / std::max(0.1f, stereographicScale * r1)
    };
}

SculptureVec3 SculptureHopfEngine::evaluateHopfGeodesicVelocity(
    const SculptureVec3& p,
    float stereographicScale,
    float twistRate
) noexcept {
    const float s = std::max(1.0e-4f, stereographicScale);
    const float x = p.x / s;
    const float y = p.y / s;
    const float z = p.z / s;
    const float r2 = x * x + y * y + z * z;
    const float denom = (1.0f + r2);

    // Pushforward of Hopf Killing vector field under stereographic map
    const float vx = 2.0f * (-y - x * z) / denom;
    const float vy = 2.0f * (x - y * z) / denom;
    const float vz = (1.0f + z * z - x * x - y * y) / denom;

    return SculptureVec3{
        vx * s * twistRate,
        vy * s * twistRate,
        vz * s * twistRate
    };
}

std::vector<std::vector<SculptureVec3>> SculptureHopfEngine::generateFoliationStreamlines(
    const HopfFoliationConfig& config,
    float simTime
) {
    std::vector<std::vector<SculptureVec3>> streamlines;
    const std::size_t numTori = std::max<std::size_t>(1U, config.numTori);
    const std::size_t fibersPerTorus = std::max<std::size_t>(1U, config.fibersPerTorus);
    const std::size_t pts = std::max<std::size_t>(8U, config.pointsPerFiber);

    streamlines.reserve(numTori * fibersPerTorus);

    const float thetaL = simTime * config.isoclinicRotationRateLeft;
    const float thetaR = simTime * config.isoclinicRotationRateRight;

    for (std::size_t t = 0; t < numTori; ++t) {
        // Latitude on S^2 determines Clifford Torus radius ratio (r1, r2)
        const float eta = kPi * static_cast<float>(t + 1) / static_cast<float>(numTori + 1);
        const float sinEta = std::sin(eta);
        const float cosEta = std::cos(eta);

        for (std::size_t f = 0; f < fibersPerTorus; ++f) {
            // Longitude on S^2 selects individual Villarceau circle fiber
            const float psi = kTwoPi * static_cast<float>(f) / static_cast<float>(fibersPerTorus);
            const SculptureVec3 baseS2{
                sinEta * std::cos(psi),
                sinEta * std::sin(psi),
                cosEta
            };

            std::vector<SculptureVec3> fiberCurve;
            fiberCurve.reserve(pts + (config.closeLoops ? 1U : 0U));

            for (std::size_t p = 0; p < pts; ++p) {
                const float xi = kTwoPi * static_cast<float>(p) / static_cast<float>(pts);
                const auto pt = evaluateFiberPoint(baseS2, xi, config.stereographicScale, thetaL, thetaR);
                fiberCurve.push_back(pt.position);
            }

            if (config.closeLoops && !fiberCurve.empty()) {
                fiberCurve.push_back(fiberCurve.front());
            }

            streamlines.push_back(std::move(fiberCurve));
        }
    }

    return streamlines;
}

IsosurfaceMesh SculptureHopfEngine::generateFoliationMesh(
    const HopfFoliationConfig& config,
    float simTime
) {
    const auto streamlines = generateFoliationStreamlines(config, simTime);

    StreamlineTubeConfig tubeCfg{};
    tubeCfg.radius = config.tubeRadius;
    tubeCfg.radialSegments = 8;
    tubeCfg.generateCaps = !config.closeLoops;
    tubeCfg.smoothNormals = true;

    return SculptureGeometryExporter::generateStreamlineTubesMesh(streamlines, tubeCfg);
}

} // namespace epi_sculpt
