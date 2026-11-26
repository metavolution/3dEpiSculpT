#include "SculptureRibbonBraider.h"
#include "SculptureDifferentialGeometry.h"
#include "QuaternionAlgebra.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace epi_sculpt {

namespace {

constexpr float kPi = std::numbers::pi_v<float>;

SculptureVec3 normalizeVec(const SculptureVec3& v) noexcept {
    return diff_geom::normalize3(v);
}

SculptureVec3 crossVec(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return diff_geom::cross3(a, b);
}

float dotVec(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return diff_geom::dot3(a, b);
}

SculptureVec3 addVec(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

SculptureVec3 subVec(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

SculptureVec3 scaleVec(const SculptureVec3& v, float s) noexcept {
    return {v.x * s, v.y * s, v.z * s};
}

} // namespace

std::vector<std::vector<KnotFramePoint>> SculptureRibbonBraider::generateBraidFrames(
    const RibbonStrandConfig& config,
    float time
) {
    const std::size_t numStrands = std::max<std::size_t>(1U, config.numStrands);
    const std::size_t numSamples = std::max<std::size_t>(16U, config.samplesPerStrand);
    
    std::vector<std::vector<KnotFramePoint>> allStrands;
    allStrands.reserve(numStrands);

    const float thetaL = config.leftIsoclinicSpeed * time;
    const float thetaR = config.rightIsoclinicSpeed * time;

    for (std::size_t s = 0; s < numStrands; ++s) {
        std::vector<KnotFramePoint> strand;
        strand.resize(numSamples);

        const float strandPhase = (2.0f * kPi * static_cast<float>(s)) / static_cast<float>(numStrands)
                                + config.phaseOffset;

        // Step 1: Compute 3D Stereographic Positions from 4D S^3 Trajectory
        for (std::size_t i = 0; i < numSamples; ++i) {
            const float t = (2.0f * kPi * static_cast<float>(i)) / static_cast<float>(numSamples);
            
            // 4D Clifford Torus Knot trajectory in S^3
            const SculptureVec4 p4Raw{
                0.70710678f * std::cos(config.p * t),
                0.70710678f * std::sin(config.p * t),
                0.70710678f * std::cos(config.q * t + strandPhase),
                0.70710678f * std::sin(config.q * t + strandPhase)
            };

            // SO(4) Isoclinic Rotation + Stereographic Projection to R^3
            const SculptureVec4 rot4 = diff_geom::rotateSO4Isoclinic(p4Raw, thetaL, thetaR);
            strand[i].position = diff_geom::projectStereographicS3(
                SculptureVec4{rot4.x, rot4.y, rot4.z, rot4.w * 0.5f},
                config.stereographicScale,
                1.0f,
                0.15f
            );
            strand[i].parameterT = t;
        }

        // Step 2: Compute Tangents and Initial Bishop Orthonormal Frame
        for (std::size_t i = 0; i < numSamples; ++i) {
            const std::size_t prevIdx = (i + numSamples - 1U) % numSamples;
            const std::size_t nextIdx = (i + 1U) % numSamples;
            strand[i].tangent = normalizeVec(subVec(strand[nextIdx].position, strand[prevIdx].position));
        }

        // Initial normal vector perpendicular to tangent[0]
        SculptureVec3 initialNormal = {0.0f, 1.0f, 0.0f};
        if (std::abs(dotVec(strand[0].tangent, initialNormal)) > 0.9f) {
            initialNormal = {1.0f, 0.0f, 0.0f};
        }
        SculptureVec3 u_curr = normalizeVec(crossVec(strand[0].tangent, initialNormal));
        SculptureVec3 v_curr = crossVec(strand[0].tangent, u_curr);

        // Step 3: Parallel Transport Bishop Frame along the strand
        float accumulatedLength = 0.0f;
        for (std::size_t i = 0; i < numSamples; ++i) {
            if (i > 0) {
                const SculptureVec3& tPrev = strand[i - 1U].tangent;
                const SculptureVec3& tCurr = strand[i].tangent;
                const float cosAlpha = dotVec(tPrev, tCurr);
                
                if (std::abs(cosAlpha + 1.0f) > 1.0e-5f) {
                    u_curr = subVec(u_curr, scaleVec(addVec(tPrev, tCurr), dotVec(addVec(tPrev, tCurr), u_curr) / (1.0f + cosAlpha)));
                    u_curr = normalizeVec(u_curr);
                    v_curr = crossVec(tCurr, u_curr);
                }

                const SculptureVec3 segment = subVec(strand[i].position, strand[i - 1U].position);
                accumulatedLength += std::sqrt(dotVec(segment, segment));
            }

            strand[i].arcLength = accumulatedLength;

            // Surface twist modulation
            const float twistAngle = config.twistFrequency * strand[i].parameterT + time * 0.5f;
            const float cosTwist = std::cos(twistAngle);
            const float sinTwist = std::sin(twistAngle);

            strand[i].normal = normalizeVec(addVec(scaleVec(u_curr, cosTwist), scaleVec(v_curr, sinTwist)));
            strand[i].binormal = normalizeVec(subVec(scaleVec(v_curr, cosTwist), scaleVec(u_curr, sinTwist)));

            // Curvature estimation
            const std::size_t prevIdx = (i + numSamples - 1U) % numSamples;
            const std::size_t nextIdx = (i + 1U) % numSamples;
            const SculptureVec3 d1 = subVec(strand[i].position, strand[prevIdx].position);
            const SculptureVec3 d2 = subVec(strand[nextIdx].position, strand[i].position);
            const SculptureVec3 crossD = crossVec(d1, d2);
            const float d1Len = std::sqrt(dotVec(d1, d1));
            strand[i].curvature = (d1Len > 1.0e-5f) ? (std::sqrt(dotVec(crossD, crossD)) / (d1Len * d1Len)) : 0.0f;
        }

        allStrands.push_back(std::move(strand));
    }

    return allStrands;
}

IsosurfaceMesh SculptureRibbonBraider::generateRibbonMesh(
    const RibbonStrandConfig& config,
    float time
) {
    const auto strands = generateBraidFrames(config, time);
    IsosurfaceMesh mesh;

    const diff_geom::VolumetricRibbonExtrusionSpec extrudeCfg{
        .width = config.ribbonWidth,
        .thickness = config.ribbonThickness,
        .closedLoop = config.closeLoop
    };

    for (const auto& strand : strands) {
        std::vector<diff_geom::OrthonormalFrame3D> frameSamples;
        frameSamples.reserve(strand.size());
        for (std::size_t i = 0; i < strand.size(); ++i) {
            const auto& pt = strand[i];
            frameSamples.push_back(diff_geom::OrthonormalFrame3D{
                .position = pt.position,
                .tangent = pt.tangent,
                .normal = pt.normal,
                .binormal = pt.binormal,
                .curvature = pt.curvature,
                .torsion = 0.0f,
                .arcLength = pt.arcLength,
                .parameterU = static_cast<float>(i) / static_cast<float>(strand.size())
            });
        }
        diff_geom::appendVolumetricRibbonStrand(mesh, frameSamples, extrudeCfg);
    }

    return mesh;
}

float SculptureRibbonBraider::computeGaussLinkingNumber(
    const std::vector<KnotFramePoint>& strandA,
    const std::vector<KnotFramePoint>& strandB
) noexcept {
    if (strandA.empty() || strandB.empty()) return 0.0f;

    std::vector<SculptureVec3> ptsA;
    ptsA.reserve(strandA.size());
    for (const auto& pt : strandA) {
        ptsA.push_back(pt.position);
    }

    std::vector<SculptureVec3> ptsB;
    ptsB.reserve(strandB.size());
    for (const auto& pt : strandB) {
        ptsB.push_back(pt.position);
    }

    return diff_geom::computeGaussLinkingNumber(ptsA, ptsB);
}

BraidTopologicalMetrics SculptureRibbonBraider::evaluateTopologicalMetrics(
    const RibbonStrandConfig& config,
    float time
) {
    const auto strands = generateBraidFrames(config, time);
    BraidTopologicalMetrics metrics;
    metrics.allFenchelSatisfied = true;

    const std::size_t numStrands = strands.size();
    metrics.selfWrithe.resize(numStrands, 0.0f);
    metrics.totalCurvature.resize(numStrands, 0.0f);
    metrics.linkingMatrix.resize(numStrands, std::vector<float>(numStrands, 0.0f));

    for (std::size_t i = 0; i < numStrands; ++i) {
        float totCurv = 0.0f;
        for (std::size_t s = 0; s < strands[i].size(); ++s) {
            totCurv += strands[i][s].curvature;
        }
        metrics.totalCurvature[i] = totCurv;
        if (totCurv < 2.0f * kPi * 0.5f) { // Normalized check
            metrics.allFenchelSatisfied = false;
        }

        for (std::size_t j = i + 1U; j < numStrands; ++j) {
            const float lk = computeGaussLinkingNumber(strands[i], strands[j]);
            metrics.linkingMatrix[i][j] = lk;
            metrics.linkingMatrix[j][i] = lk;
        }
    }

    const auto mesh = generateRibbonMesh(config, time);
    metrics.totalVertices = mesh.vertices.size();
    metrics.totalTriangles = mesh.indices.size() / 3U;

    return metrics;
}

SculptureProgramParams SculptureRibbonBraider::makeRibbonProgramParams(
    const RibbonStrandConfig& config
) noexcept {
    SculptureProgramParams params{};
    params.inner = InnerMap::CliffordHopf;
    params.innerB = InnerMap::CalabiYauQuintic;
    params.reduction = ReductionMode::Torus;
    params.seedType = SeedType::TorusKnotPQ;
    params.colorEffect = ColorEffectType::ThinFilmIridescence;
    params.enable4D = 2U;
    params.outputMode = 4U; // Clifford-Hopf Mode
    params.topologyBlend = 0.50f;
    params.rotorAlpha = config.leftIsoclinicSpeed;
    params.rotorBeta = config.rightIsoclinicSpeed;
    params.feedbackIterations = 5U;
    params.worldScale = 1.15f;
    params.radiusScale = 0.85f;
    params.filmThickness = 550.0f;
    params.filmIOR = 1.52f;
    return params;
}

} // namespace epi_sculpt
