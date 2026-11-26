/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - SCULPTURE DIFFERENTIAL GEOMETRY IMPLEMENTATION
 * ============================================================================
 */

#include "SculptureDifferentialGeometry.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace epi_sculpt::diff_geom {

namespace {
constexpr float kPi = std::numbers::pi_v<float>;
constexpr float kTwoPi = 2.0f * kPi;

inline void expandAabb(IsosurfaceMesh& mesh, const SculptureVec3& p) noexcept {
    mesh.aabbMin.x = std::min(mesh.aabbMin.x, p.x);
    mesh.aabbMin.y = std::min(mesh.aabbMin.y, p.y);
    mesh.aabbMin.z = std::min(mesh.aabbMin.z, p.z);
    mesh.aabbMax.x = std::max(mesh.aabbMax.x, p.x);
    mesh.aabbMax.y = std::max(mesh.aabbMax.y, p.y);
    mesh.aabbMax.z = std::max(mesh.aabbMax.z, p.z);
}
} // namespace

SculptureVec3 normalize3(
    const SculptureVec3& v,
    const SculptureVec3& fallback) noexcept {
    const float lenSq = dot3(v, v);
    if (lenSq > 1.0e-12f) {
        const float invLen = 1.0f / std::sqrt(lenSq);
        return {v.x * invLen, v.y * invLen, v.z * invLen};
    }
    return fallback;
}

SculptureVec4 normalize4(const SculptureVec4& v) noexcept {
    const float lenSq = v.lengthSq();
    if (lenSq > 1.0e-8f) {
        const float invLen = 1.0f / std::sqrt(lenSq);
        return {v.x * invLen, v.y * invLen, v.z * invLen, v.w * invLen};
    }
    return {0.0f, 0.0f, 0.0f, 1.0f};
}

SculptureVec4 rotateSO4Isoclinic(
    const SculptureVec4& p,
    float thetaL,
    float thetaR,
    float phiL,
    float phiR) noexcept {
    // 1. Left-isoclinic rotation in (xy) and (zw) planes
    const float cosTL = std::cos(thetaL);
    const float sinTL = std::sin(thetaL);

    const SculptureVec4 pL{
        p.x * cosTL - p.y * sinTL,
        p.x * sinTL + p.y * cosTL,
        p.z * cosTL - p.w * sinTL,
        p.z * sinTL + p.w * cosTL
    };

    // 2. Right-isoclinic rotation in (xz) and (yw) planes
    const float cosTR = std::cos(thetaR);
    const float sinTR = std::sin(thetaR);

    const SculptureVec4 pR{
        pL.x * cosTR - pL.z * sinTR,
        pL.y * cosTR + pL.w * sinTR,
        pL.x * sinTR + pL.z * cosTR,
        -pL.y * sinTR + pL.w * cosTR
    };

    // 3. Optional secondary chiral tilts
    if (phiL != 0.0f || phiR != 0.0f) {
        const float cosP = std::cos(phiL);
        const float sinP = std::sin(phiL);
        return {
            pR.x * cosP - pR.w * sinP,
            pR.y,
            pR.z,
            pR.x * sinP + pR.w * cosP
        };
    }

    return pR;
}

SculptureVec3 projectStereographicS3(
    const SculptureVec4& p4,
    float scale,
    float poleDistance,
    float minDenominator) noexcept {
    const float denom = std::max(poleDistance - p4.w, minDenominator);
    const float factor = scale / denom;
    return {
        p4.x * factor,
        p4.y * factor,
        p4.z * factor
    };
}

SculptureVec4 inverseStereographicS3(
    const SculptureVec3& p3,
    float scale) noexcept {
    const float s = std::max(1.0e-4f, scale);
    const float nx = p3.x / s;
    const float ny = p3.y / s;
    const float nz = p3.z / s;
    const float r2 = nx * nx + ny * ny + nz * nz;
    const float invDenom = 1.0f / (r2 + 1.0f);
    return {
        (2.0f * nx) * invDenom,
        (2.0f * ny) * invDenom,
        (2.0f * nz) * invDenom,
        (r2 - 1.0f) * invDenom
    };
}

SculptureVec4 slerpS3(
    const SculptureVec4& a,
    const SculptureVec4& b,
    float t) noexcept {
    float cosTheta = dot4(a, b);
    SculptureVec4 targetB = b;

    if (cosTheta < 0.0f) {
        targetB = {-b.x, -b.y, -b.z, -b.w};
        cosTheta = -cosTheta;
    }

    if (cosTheta > 0.9995f) {
        const float s1 = 1.0f - t;
        const float s2 = t;
        return normalize4({
            s1 * a.x + s2 * targetB.x,
            s1 * a.y + s2 * targetB.y,
            s1 * a.z + s2 * targetB.z,
            s1 * a.w + s2 * targetB.w
        });
    }

    cosTheta = std::clamp(cosTheta, -1.0f, 1.0f);
    const float theta = std::acos(cosTheta);
    const float sinTheta = std::sin(theta);
    const float s1 = std::sin((1.0f - t) * theta) / sinTheta;
    const float s2 = std::sin(t * theta) / sinTheta;

    return {
        s1 * a.x + s2 * targetB.x,
        s1 * a.y + s2 * targetB.y,
        s1 * a.z + s2 * targetB.z,
        s1 * a.w + s2 * targetB.w
    };
}

std::vector<OrthonormalFrame3D> computeBishopFrames(
    std::span<const SculptureVec3> points,
    bool closedLoop,
    float twistFrequency,
    float twistPhase) {
    const std::size_t n = points.size();
    std::vector<OrthonormalFrame3D> frames(n);
    if (n == 0U) return frames;
    if (n == 1U) {
        frames[0].position = points[0];
        return frames;
    }

    for (std::size_t i = 0; i < n; ++i) {
        frames[i].position = points[i];
        frames[i].parameterU = closedLoop
            ? (kTwoPi * static_cast<float>(i) / static_cast<float>(n))
            : (static_cast<float>(i) / static_cast<float>(n - 1U));

        SculptureVec3 delta{};
        if (closedLoop) {
            const std::size_t prev = (i == 0U) ? (n - 1U) : (i - 1U);
            const std::size_t next = (i + 1U == n) ? 0U : (i + 1U);
            delta = {
                points[next].x - points[prev].x,
                points[next].y - points[prev].y,
                points[next].z - points[prev].z
            };
        } else if (i == 0U) {
            delta = {
                points[1].x - points[0].x,
                points[1].y - points[0].y,
                points[1].z - points[0].z
            };
        } else if (i == n - 1U) {
            delta = {
                points[n - 1U].x - points[n - 2U].x,
                points[n - 1U].y - points[n - 2U].y,
                points[n - 1U].z - points[n - 2U].z
            };
        } else {
            delta = {
                points[i + 1U].x - points[i - 1U].x,
                points[i + 1U].y - points[i - 1U].y,
                points[i + 1U].z - points[i - 1U].z
            };
        }
        frames[i].tangent = normalize3(delta, {0.0f, 0.0f, 1.0f});
    }

    SculptureVec3 initialUp{0.0f, 1.0f, 0.0f};
    if (std::abs(dot3(frames[0].tangent, initialUp)) > 0.9f) {
        initialUp = {1.0f, 0.0f, 0.0f};
    }
    SculptureVec3 binormal = normalize3(cross3(frames[0].tangent, initialUp), {1.0f, 0.0f, 0.0f});
    SculptureVec3 normal = normalize3(cross3(binormal, frames[0].tangent), {0.0f, 1.0f, 0.0f});

    float accumulatedLength = 0.0f;
    for (std::size_t i = 0; i < n; ++i) {
        if (i > 0U) {
            const SculptureVec3 seg{
                points[i].x - points[i - 1U].x,
                points[i].y - points[i - 1U].y,
                points[i].z - points[i - 1U].z
            };
            const float c1 = dot3(seg, seg);
            if (c1 > 1.0e-8f) {
                accumulatedLength += std::sqrt(c1);
                const float proj = (2.0f / c1) * dot3(seg, normal);
                const SculptureVec3 reflected{
                    normal.x - proj * seg.x,
                    normal.y - proj * seg.y,
                    normal.z - proj * seg.z
                };
                normal = normalize3(reflected, normal);
            }
            binormal = normalize3(cross3(frames[i].tangent, normal), binormal);
            normal = normalize3(cross3(binormal, frames[i].tangent), normal);
        }

        frames[i].arcLength = accumulatedLength;

        if (std::abs(twistFrequency) > 1.0e-6f || std::abs(twistPhase) > 1.0e-6f) {
            const float twistAngle = twistFrequency * frames[i].parameterU + twistPhase;
            const float ct = std::cos(twistAngle);
            const float st = std::sin(twistAngle);
            frames[i].normal = normalize3({
                normal.x * ct + binormal.x * st,
                normal.y * ct + binormal.y * st,
                normal.z * ct + binormal.z * st
            }, normal);
            frames[i].binormal = normalize3({
                binormal.x * ct - normal.x * st,
                binormal.y * ct - normal.y * st,
                binormal.z * ct - normal.z * st
            }, binormal);
        } else {
            frames[i].normal = normal;
            frames[i].binormal = binormal;
        }

        const std::size_t prevIdx = closedLoop
            ? ((i + n - 1U) % n)
            : ((i > 0U) ? (i - 1U) : 0U);
        const std::size_t nextIdx = closedLoop
            ? ((i + 1U) % n)
            : ((i + 1U < n) ? (i + 1U) : (n - 1U));
        const SculptureVec3 d1{
            points[i].x - points[prevIdx].x,
            points[i].y - points[prevIdx].y,
            points[i].z - points[prevIdx].z
        };
        const SculptureVec3 d2{
            points[nextIdx].x - points[i].x,
            points[nextIdx].y - points[i].y,
            points[nextIdx].z - points[i].z
        };
        const SculptureVec3 crossD = cross3(d1, d2);
        const float d1LenSq = dot3(d1, d1);
        frames[i].curvature = (d1LenSq > 1.0e-10f)
            ? (std::sqrt(dot3(crossD, crossD)) / d1LenSq)
            : 0.0f;
    }

    return frames;
}

IsosurfaceMesh extrudeTubeFromPoints(
    std::span<const SculptureVec3> points,
    const TubeExtrusionSpec& spec) {
    IsosurfaceMesh mesh{};
    const std::size_t n = points.size();
    if (n < 2U) return mesh;

    const std::uint32_t segs = std::max(3U, spec.radialSegments);
    const float radius = std::max(0.001f, spec.radius);
    const auto frames = computeBishopFrames(points, spec.closedLoop);

    mesh.vertices.reserve(n * segs);
    mesh.indices.reserve((spec.closedLoop ? n : (n - 1U)) * segs * 6U);

    for (std::size_t i = 0; i < n; ++i) {
        const float uCoord = static_cast<float>(i) / static_cast<float>(n);
        const auto& f = frames[i];
        for (std::uint32_t s = 0; s < segs; ++s) {
            const float angle = static_cast<float>(s) * (kTwoPi / static_cast<float>(segs));
            const float ca = std::cos(angle);
            const float sa = std::sin(angle);
            const SculptureVec3 radialNormal = normalize3({
                f.normal.x * ca + f.binormal.x * sa,
                f.normal.y * ca + f.binormal.y * sa,
                f.normal.z * ca + f.binormal.z * sa
            });
            const SculptureVec3 pos{
                f.position.x + radialNormal.x * radius,
                f.position.y + radialNormal.y * radius,
                f.position.z + radialNormal.z * radius
            };
            mesh.vertices.push_back(IsosurfaceVertex{
                .position = pos,
                .normal = radialNormal,
                .materialCoord = uCoord,
                .density = 1.0f
            });
            expandAabb(mesh, pos);
        }
    }

    const std::size_t ringLimit = spec.closedLoop ? n : (n - 1U);
    for (std::size_t i = 0; i < ringLimit; ++i) {
        const std::size_t nextRing = (i + 1U == n) ? 0U : (i + 1U);
        const std::uint32_t baseCurrent = static_cast<std::uint32_t>(i * segs);
        const std::uint32_t baseNext = static_cast<std::uint32_t>(nextRing * segs);

        for (std::uint32_t s = 0; s < segs; ++s) {
            const std::uint32_t nextS = (s + 1U) % segs;
            const std::uint32_t v00 = baseCurrent + s;
            const std::uint32_t v01 = baseNext + s;
            const std::uint32_t v10 = baseCurrent + nextS;
            const std::uint32_t v11 = baseNext + nextS;

            mesh.indices.push_back(v00);
            mesh.indices.push_back(v01);
            mesh.indices.push_back(v10);

            mesh.indices.push_back(v01);
            mesh.indices.push_back(v11);
            mesh.indices.push_back(v10);
        }
    }

    return mesh;
}

IsosurfaceMesh extrudeFlatRibbonFromPoints(
    std::span<const SculptureVec3> points,
    const FlatRibbonExtrusionSpec& spec) {
    IsosurfaceMesh mesh{};
    const std::size_t n = points.size();
    if (n < 2U) return mesh;

    const auto frames = computeBishopFrames(
        points, spec.closedLoop, spec.twistFrequency, spec.twistPhase);
    const float halfWidth = spec.width * 0.5f;

    mesh.vertices.reserve(n * 2U);
    mesh.indices.reserve((spec.closedLoop ? n : (n - 1U)) * 6U);

    for (std::size_t i = 0; i < n; ++i) {
        const auto& f = frames[i];
        const float uCoord = (n > 1U)
            ? (static_cast<float>(i) / static_cast<float>(spec.closedLoop ? n : (n - 1U)))
            : 0.0f;
        const SculptureVec3& span = spec.useBinormalSpan ? f.binormal : f.normal;
        const SculptureVec3& surfNormal = spec.useBinormalSpan ? f.normal : f.binormal;

        const SculptureVec3 posLeft{
            f.position.x - span.x * halfWidth,
            f.position.y - span.y * halfWidth,
            f.position.z - span.z * halfWidth
        };
        const SculptureVec3 posRight{
            f.position.x + span.x * halfWidth,
            f.position.y + span.y * halfWidth,
            f.position.z + span.z * halfWidth
        };

        mesh.vertices.push_back(IsosurfaceVertex{
            .position = posLeft,
            .normal = surfNormal,
            .materialCoord = uCoord,
            .density = 1.0f
        });
        mesh.vertices.push_back(IsosurfaceVertex{
            .position = posRight,
            .normal = surfNormal,
            .materialCoord = uCoord,
            .density = 1.0f
        });

        expandAabb(mesh, {f.position.x - halfWidth, f.position.y - halfWidth, f.position.z - halfWidth});
        expandAabb(mesh, {f.position.x + halfWidth, f.position.y + halfWidth, f.position.z + halfWidth});

        if (i + 1U < n || spec.closedLoop) {
            const std::size_t nextI = (i + 1U) % n;
            const std::uint32_t base0 = static_cast<std::uint32_t>(i * 2U);
            const std::uint32_t base1 = static_cast<std::uint32_t>(nextI * 2U);

            mesh.indices.push_back(base0 + 0U);
            mesh.indices.push_back(base0 + 1U);
            mesh.indices.push_back(base1 + 0U);

            mesh.indices.push_back(base0 + 1U);
            mesh.indices.push_back(base1 + 1U);
            mesh.indices.push_back(base1 + 0U);
        }
    }

    return mesh;
}

void appendVolumetricRibbonStrand(
    IsosurfaceMesh& mesh,
    std::span<const OrthonormalFrame3D> frames,
    const VolumetricRibbonExtrusionSpec& spec) {
    const std::size_t sampleCount = frames.size();
    if (sampleCount < 2U) return;

    const float halfWidth = spec.width * 0.5f;
    const float halfThick = spec.thickness * 0.5f;
    const std::size_t vertexBase = mesh.vertices.size();

    for (std::size_t i = 0; i < sampleCount; ++i) {
        const auto& pt = frames[i];
        const SculptureVec3 offsetN_pos{pt.normal.x * halfWidth, pt.normal.y * halfWidth, pt.normal.z * halfWidth};
        const SculptureVec3 offsetN_neg{-offsetN_pos.x, -offsetN_pos.y, -offsetN_pos.z};
        const SculptureVec3 offsetB_pos{pt.binormal.x * halfThick, pt.binormal.y * halfThick, pt.binormal.z * halfThick};
        const SculptureVec3 offsetB_neg{-offsetB_pos.x, -offsetB_pos.y, -offsetB_pos.z};
        const SculptureVec3 negBinormal{-pt.binormal.x, -pt.binormal.y, -pt.binormal.z};

        IsosurfaceVertex v0{};
        v0.position = {
            pt.position.x + offsetN_pos.x + offsetB_pos.x,
            pt.position.y + offsetN_pos.y + offsetB_pos.y,
            pt.position.z + offsetN_pos.z + offsetB_pos.z
        };
        v0.normal = pt.binormal;
        mesh.vertices.push_back(v0);

        IsosurfaceVertex v1{};
        v1.position = {
            pt.position.x + offsetN_neg.x + offsetB_pos.x,
            pt.position.y + offsetN_neg.y + offsetB_pos.y,
            pt.position.z + offsetN_neg.z + offsetB_pos.z
        };
        v1.normal = pt.binormal;
        mesh.vertices.push_back(v1);

        IsosurfaceVertex v2{};
        v2.position = {
            pt.position.x + offsetN_neg.x + offsetB_neg.x,
            pt.position.y + offsetN_neg.y + offsetB_neg.y,
            pt.position.z + offsetN_neg.z + offsetB_neg.z
        };
        v2.normal = negBinormal;
        mesh.vertices.push_back(v2);

        IsosurfaceVertex v3{};
        v3.position = {
            pt.position.x + offsetN_pos.x + offsetB_neg.x,
            pt.position.y + offsetN_pos.y + offsetB_neg.y,
            pt.position.z + offsetN_pos.z + offsetB_neg.z
        };
        v3.normal = negBinormal;
        mesh.vertices.push_back(v3);
    }

    for (std::size_t i = 0; i < sampleCount; ++i) {
        const std::size_t nextI = (i + 1U) % sampleCount;
        if (!spec.closedLoop && nextI == 0U) continue;

        const std::uint32_t c0 = static_cast<std::uint32_t>(vertexBase + i * 4U);
        const std::uint32_t c1 = static_cast<std::uint32_t>(vertexBase + nextI * 4U);

        // Top Face (V0, V1)
        mesh.indices.push_back(c0 + 0U);
        mesh.indices.push_back(c1 + 0U);
        mesh.indices.push_back(c1 + 1U);
        mesh.indices.push_back(c0 + 0U);
        mesh.indices.push_back(c1 + 1U);
        mesh.indices.push_back(c0 + 1U);

        // Bottom Face (V2, V3)
        mesh.indices.push_back(c0 + 2U);
        mesh.indices.push_back(c1 + 2U);
        mesh.indices.push_back(c1 + 3U);
        mesh.indices.push_back(c0 + 2U);
        mesh.indices.push_back(c1 + 3U);
        mesh.indices.push_back(c0 + 3U);

        // Right Edge (V1, V2)
        mesh.indices.push_back(c0 + 1U);
        mesh.indices.push_back(c1 + 1U);
        mesh.indices.push_back(c1 + 2U);
        mesh.indices.push_back(c0 + 1U);
        mesh.indices.push_back(c1 + 2U);
        mesh.indices.push_back(c0 + 2U);

        // Left Edge (V3, V0)
        mesh.indices.push_back(c0 + 3U);
        mesh.indices.push_back(c1 + 3U);
        mesh.indices.push_back(c1 + 0U);
        mesh.indices.push_back(c0 + 3U);
        mesh.indices.push_back(c1 + 0U);
        mesh.indices.push_back(c0 + 0U);
    }
}

float computeGaussLinkingNumber(
    std::span<const SculptureVec3> curveA,
    std::span<const SculptureVec3> curveB) noexcept {
    if (curveA.empty() || curveB.empty()) return 0.0f;

    const std::size_t nA = curveA.size();
    const std::size_t nB = curveB.size();
    float linkingSum = 0.0f;

    for (std::size_t i = 0; i < nA; ++i) {
        const std::size_t nextA = (i + 1U) % nA;
        const SculptureVec3 drA{
            curveA[nextA].x - curveA[i].x,
            curveA[nextA].y - curveA[i].y,
            curveA[nextA].z - curveA[i].z
        };
        const SculptureVec3 rA_mid{
            (curveA[i].x + curveA[nextA].x) * 0.5f,
            (curveA[i].y + curveA[nextA].y) * 0.5f,
            (curveA[i].z + curveA[nextA].z) * 0.5f
        };

        for (std::size_t j = 0; j < nB; ++j) {
            const std::size_t nextB = (j + 1U) % nB;
            const SculptureVec3 drB{
                curveB[nextB].x - curveB[j].x,
                curveB[nextB].y - curveB[j].y,
                curveB[nextB].z - curveB[j].z
            };
            const SculptureVec3 rB_mid{
                (curveB[j].x + curveB[nextB].x) * 0.5f,
                (curveB[j].y + curveB[nextB].y) * 0.5f,
                (curveB[j].z + curveB[nextB].z) * 0.5f
            };

            const SculptureVec3 rDiff{
                rA_mid.x - rB_mid.x,
                rA_mid.y - rB_mid.y,
                rA_mid.z - rB_mid.z
            };
            const float rDistSq = dot3(rDiff, rDiff);
            if (rDistSq < 1.0e-8f) continue;

            const float rDist = std::sqrt(rDistSq);
            const float rDist3 = rDistSq * rDist;
            const SculptureVec3 crossDr = cross3(drA, drB);
            linkingSum += dot3(rDiff, crossDr) / rDist3;
        }
    }

    return linkingSum / (4.0f * kPi);
}

} // namespace epi_sculpt::diff_geom
