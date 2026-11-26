#include "SculptureOctonionFibration.h"
#include "SculptureGeometryExporter.h"
#include "SculptureDifferentialGeometry.h"

#include <cmath>
#include <algorithm>
#include <numbers>

namespace epi_sculpt {

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;
} // namespace

SculptureVec3 SculptureOctonionFibration::projectOctonionHopf(const Octonion& o, float scale) noexcept {
    const Quaternion A = o.lo();
    const Quaternion B = o.hi();

    // Octonionic Hopf map S^7 -> S^4
    // Z = 2 * A * B_conj, W = |A|^2 - |B|^2
    const Quaternion Z = (A * B.conjugate()) * 2.0f;
    const float normASq = A.normSq();
    const float normBSq = B.normSq();
    const float W = normASq - normBSq;

    // Stereographic projection S^4 -> R^3 via unified pole-regularized chart
    const float s = std::max(0.1f, scale);
    return diff_geom::projectStereographicS3(SculptureVec4{Z.x, Z.y, Z.z, -W * 0.5f}, s, 1.0f, 0.15f);
}

float SculptureOctonionFibration::computeAssociatorCurvature(
    const Octonion& x, const Octonion& y, const Octonion& z
) noexcept {
    const Octonion assoc = Octonion::associator(x, y, z);
    return assoc.norm();
}

OctonionFiberPoint SculptureOctonionFibration::evaluateFiberPoint(
    const Octonion& baseOctonion,
    float fiberAngle,
    float g2Angle,
    float associatorGain,
    float scale
) noexcept {
    // 1. Apply 14-parameter G2 Automorphism on the Fano plane
    const Octonion g2Rotated = Octonion::rotateG2(baseOctonion, g2Angle);

    // 2. Generate S^3 fiber trajectory in 8D
    const float cFiber = std::cos(fiberAngle);
    const float sFiber = std::sin(fiberAngle);
    
    // Fiber generator along the imaginary octonion triad (e3, e5, e7)
    const Octonion fiberGenerator(
        0.0f,
        0.0f, 0.0f, sFiber * 0.577f,
        0.0f, sFiber * 0.577f, 0.0f, sFiber * 0.577f
    );

    const Octonion fiberPoint8D = (g2Rotated * cFiber + fiberGenerator).normalized();

    // 3. Evaluate non-associative curvature with canonical Fano units e1 and e2
    const Octonion unitE1(0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    const Octonion unitE2(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    const float assocCurvature = computeAssociatorCurvature(fiberPoint8D, unitE1, unitE2);

    // 4. Project into 3D Euclidean space
    const SculptureVec3 pos = projectOctonionHopf(fiberPoint8D, scale);

    // 5. Evaluate finite-difference tangent vector
    const float dXi = 0.015f;
    const float cNext = std::cos(fiberAngle + dXi);
    const float sNext = std::sin(fiberAngle + dXi);
    const Octonion genNext(
        0.0f,
        0.0f, 0.0f, sNext * 0.577f,
        0.0f, sNext * 0.577f, 0.0f, sNext * 0.577f
    );
    const Octonion nextPoint8D = (g2Rotated * cNext + genNext).normalized();
    const SculptureVec3 posNext = projectOctonionHopf(nextPoint8D, scale);

    const float tx = (posNext.x - pos.x) / dXi;
    const float ty = (posNext.y - pos.y) / dXi;
    const float tz = (posNext.z - pos.z) / dXi;
    const float tLen = std::max(1.0e-6f, std::sqrt(tx * tx + ty * ty + tz * tz));

    return OctonionFiberPoint{
        .position = pos,
        .tangent = SculptureVec3{tx / tLen, ty / tLen, tz / tLen},
        .associatorCurvature = assocCurvature * associatorGain,
        .g2InvariantPhase = g2Angle,
        .materialCoord = std::clamp(assocCurvature * associatorGain, 0.0f, 1.0f)
    };
}

std::vector<std::vector<SculptureVec3>> SculptureOctonionFibration::generateFoliationStreamlines(
    const OctonionFibrationConfig& config,
    float simTime
) {
    std::vector<std::vector<SculptureVec3>> streamlines;
    const std::size_t shells = std::max<std::size_t>(1U, config.numHyperShells);
    const std::size_t fibersPerShell = std::max<std::size_t>(1U, config.fibersPerShell);
    const std::size_t pts = std::max<std::size_t>(8U, config.pointsPerFiber);

    streamlines.reserve(shells * fibersPerShell);

    const float g2Angle = simTime * config.g2RotationRate;

    for (std::size_t s = 0; s < shells; ++s) {
        const float shellRadius = 0.6f + 0.4f * static_cast<float>(s + 1) / static_cast<float>(shells);

        for (std::size_t f = 0; f < fibersPerShell; ++f) {
            const float psi = kTwoPi * static_cast<float>(f) / static_cast<float>(fibersPerShell);
            const float phi = kPi * static_cast<float>(f + 1) / static_cast<float>(fibersPerShell + 1);

            // Construct 8D base octonion on S^7
            const Octonion base(
                shellRadius * std::cos(psi) * std::sin(phi),
                shellRadius * std::sin(psi) * std::sin(phi),
                shellRadius * std::cos(phi),
                0.25f * std::sin(psi * 2.0f),
                shellRadius * 0.35f * std::cos(psi + phi),
                shellRadius * 0.35f * std::sin(psi - phi),
                0.20f * std::cos(phi * 3.0f),
                0.15f * std::sin(psi * 3.0f)
            );

            std::vector<SculptureVec3> fiberCurve;
            fiberCurve.reserve(pts + (config.closeLoops ? 1U : 0U));

            for (std::size_t p = 0; p < pts; ++p) {
                const float xi = kTwoPi * static_cast<float>(p) / static_cast<float>(pts);
                const auto pt = evaluateFiberPoint(
                    base, xi, g2Angle, config.associatorGain, config.projectionScale
                );
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

IsosurfaceMesh SculptureOctonionFibration::generateFoliationMesh(
    const OctonionFibrationConfig& config,
    float simTime
) {
    const auto streamlines = generateFoliationStreamlines(config, simTime);

    StreamlineTubeConfig tubeCfg{};
    tubeCfg.radius = config.tubeRadius;
    tubeCfg.radialSegments = std::max(3U, config.radialSegments);
    tubeCfg.generateCaps = !config.closeLoops;
    tubeCfg.smoothNormals = true;

    return SculptureGeometryExporter::generateStreamlineTubesMesh(streamlines, tubeCfg);
}

} // namespace epi_sculpt
