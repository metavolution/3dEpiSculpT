#include "ProceduralForestGeometry.h"
#include "SculptureDifferentialGeometry.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace epi_sculpt::forest {

IsosurfaceMesh generateLichtungTerrainMesh(const ForestFieldGrid& grid) {
    IsosurfaceMesh mesh{};
    const std::uint32_t w = grid.config.gridWidth;
    const std::uint32_t h = grid.config.gridHeight;
    if (grid.empty() || w < 2U || h < 2U) {
        return mesh;
    }

    mesh.vertices.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
    mesh.indices.reserve(static_cast<std::size_t>(w - 1U) * static_cast<std::size_t>(h - 1U) * 6U);

    for (std::uint32_t iz = 0U; iz < h; ++iz) {
        const float worldZ = grid.config.originZ + (static_cast<float>(iz) + 0.5f) * grid.config.cellSize;
        for (std::uint32_t ix = 0U; ix < w; ++ix) {
            const float worldX = grid.config.originX + (static_cast<float>(ix) + 0.5f) * grid.config.cellSize;
            const ForestWorldCellGpu& cell = grid.cells[static_cast<std::size_t>(iz) * w + ix];

            const float ny = std::sqrt(std::max(0.0f, 1.0f - (cell.normalX * cell.normalX + cell.normalZ * cell.normalZ)));
            IsosurfaceVertex v{};
            v.position = {worldX, cell.height, worldZ};
            v.normal = diff_geom::normalize3({cell.normalX, ny, cell.normalZ});
            v.materialCoord = cell.pathMask;
            v.density = cell.moisture;
            mesh.vertices[static_cast<std::size_t>(iz) * w + ix] = v;
        }
    }

    for (std::uint32_t iz = 0U; iz + 1U < h; ++iz) {
        for (std::uint32_t ix = 0U; ix + 1U < w; ++ix) {
            const std::uint32_t i00 = iz * w + ix;
            const std::uint32_t i10 = iz * w + (ix + 1U);
            const std::uint32_t i01 = (iz + 1U) * w + ix;
            const std::uint32_t i11 = (iz + 1U) * w + (ix + 1U);

            mesh.indices.push_back(i00);
            mesh.indices.push_back(i01);
            mesh.indices.push_back(i10);

            mesh.indices.push_back(i10);
            mesh.indices.push_back(i01);
            mesh.indices.push_back(i11);
        }
    }

    SculptureIsosurface::computeMeshBounds(mesh);
    return mesh;
}

float evaluateBoulderSdf(
    const SculptureVec3& p,
    const ForestMossyBoulderGpu& boulder) noexcept
{
    const float cx = boulder.centerRadius[0];
    const float cy = boulder.centerRadius[1];
    const float cz = boulder.centerRadius[2];
    const float radius = std::max(boulder.centerRadius[3], 0.05f);

    const float sx = std::max(boulder.stretchSeed[0], 0.1f);
    const float sy = std::max(boulder.stretchSeed[1], 0.1f);
    const float sz = std::max(boulder.stretchSeed[2], 0.1f);
    const std::uint32_t seed = static_cast<std::uint32_t>(boulder.stretchSeed[3] * 65535.0f);

    const float qx = (p.x - cx) / sx;
    const float qy = (p.y - cy) / sy;
    const float qz = (p.z - cz) / sz;
    const float minScale = std::min({sx, sy, sz});

    float dBase = (std::sqrt(qx * qx + qy * qy + qz * qz) - radius) * minScale;
    if (dBase < 1.2f) {
        const float rockWeathering =
            0.28f * worley3D(p.x * 0.7f, p.y * 0.7f, p.z * 0.7f, seed ^ 0x41U)
          + 0.06f * fbm3D(p.x * 3.5f, p.y * 3.5f, p.z * 3.5f, 3U, seed ^ 0x82U);
        dBase += rockWeathering;
    }
    return dBase;
}

float evaluateBouldersSdf(
    const SculptureVec3& p,
    std::span<const ForestMossyBoulderGpu> boulders) noexcept
{
    float dMin = 1e5f;
    for (const ForestMossyBoulderGpu& b : boulders) {
        dMin = std::min(dMin, evaluateBoulderSdf(p, b));
    }
    return dMin;
}

float evaluateBoulderMossMask(
    const SculptureVec3& p,
    const SculptureVec3& normal,
    float terrainHeight,
    std::uint32_t seed) noexcept
{
    const float deltaY = std::max(0.0f, p.y - terrainHeight);
    const float noiseTerm = 0.25f * fbm3D(p.x * 2.5f, p.y * 2.5f, p.z * 2.5f, 3U, seed ^ 0x73U);
    const float groundContactBoost = 0.40f * std::exp(-deltaY / 0.8f);
    return smoothstep01(0.35f, 0.75f, normal.y + noiseTerm + groundContactBoost);
}

IsosurfaceMesh generateMossyBoulderMesh(
    const ForestMossyBoulderGpu& boulder,
    float terrainHeightAtBase,
    std::uint32_t latSegments,
    std::uint32_t lonSegments)
{
    IsosurfaceMesh mesh{};
    const std::uint32_t latCount = std::max(4U, latSegments);
    const std::uint32_t lonCount = std::max(6U, lonSegments);

    const float cx = boulder.centerRadius[0];
    const float cy = boulder.centerRadius[1];
    const float cz = boulder.centerRadius[2];
    const float radius = std::max(boulder.centerRadius[3], 0.1f);

    const float sx = std::max(boulder.stretchSeed[0], 0.1f);
    const float sy = std::max(boulder.stretchSeed[1], 0.1f);
    const float sz = std::max(boulder.stretchSeed[2], 0.1f);
    const std::uint32_t seed = static_cast<std::uint32_t>(boulder.stretchSeed[3] * 65535.0f);

    const std::uint32_t ringStride = lonCount + 1U;
    mesh.vertices.resize(static_cast<std::size_t>(latCount + 1U) * ringStride);

    auto displaceDir = [&](float dx, float dy, float dz) noexcept -> SculptureVec3 {
        // Asymmetric flattening on bottom hemisphere for embedded ground contact
        const float bottomFlatten = dy < 0.0f ? (1.0f + 0.35f * dy) : 1.0f;
        const float worleyDisp = 1.0f - 0.22f * worley3D(dx * 1.6f, dy * 1.6f, dz * 1.6f, seed ^ 0x41U)
                               + 0.08f * fbm3D(dx * 3.2f, dy * 3.2f, dz * 3.2f, 3U, seed ^ 0x82U);
        const float r = radius * worleyDisp;
        return {
            cx + sx * dx * r,
            std::max(terrainHeightAtBase - 0.15f, cy + sy * dy * bottomFlatten * r),
            cz + sz * dz * r
        };
    };

    for (std::uint32_t ilat = 0U; ilat <= latCount; ++ilat) {
        const float v = static_cast<float>(ilat) / static_cast<float>(latCount);
        const float theta = v * std::numbers::pi_v<float>;
        const float sinTheta = std::sin(theta);
        const float cosTheta = std::cos(theta);

        for (std::uint32_t ilon = 0U; ilon <= lonCount; ++ilon) {
            const float u = static_cast<float>(ilon) / static_cast<float>(lonCount);
            const float phi = u * 2.0f * std::numbers::pi_v<float>;
            const float dx = sinTheta * std::cos(phi);
            const float dy = cosTheta;
            const float dz = sinTheta * std::sin(phi);

            const SculptureVec3 pos = displaceDir(dx, dy, dz);
            const SculptureVec3 normal = diff_geom::normalize3({dx / sx, dy / sy, dz / sz});
            const float moss = evaluateBoulderMossMask(pos, normal, terrainHeightAtBase, seed);

            IsosurfaceVertex vert{};
            vert.position = pos;
            vert.normal = normal;
            vert.materialCoord = moss;
            vert.density = 1.0f;
            mesh.vertices[static_cast<std::size_t>(ilat) * ringStride + ilon] = vert;
        }
    }

    mesh.indices.reserve(static_cast<std::size_t>(latCount) * lonCount * 6U);
    for (std::uint32_t ilat = 0U; ilat < latCount; ++ilat) {
        for (std::uint32_t ilon = 0U; ilon < lonCount; ++ilon) {
            const std::uint32_t i00 = ilat * ringStride + ilon;
            const std::uint32_t i10 = i00 + 1U;
            const std::uint32_t i01 = (ilat + 1U) * ringStride + ilon;
            const std::uint32_t i11 = i01 + 1U;

            mesh.indices.push_back(i00);
            mesh.indices.push_back(i01);
            mesh.indices.push_back(i10);

            mesh.indices.push_back(i10);
            mesh.indices.push_back(i01);
            mesh.indices.push_back(i11);
        }
    }

    SculptureIsosurface::computeMeshBounds(mesh);
    return mesh;
}

namespace {

void appendTaperedBarkBranch(
    IsosurfaceMesh& mesh,
    const SculptureVec3& startPos,
    const SculptureVec3& endPos,
    float startRadius,
    float endRadius,
    std::uint32_t seed,
    std::uint32_t depth,
    std::uint32_t maxDepth)
{
    constexpr std::uint32_t kAxialSteps = 7U;
    constexpr std::uint32_t kRadialSegments = 8U;

    std::vector<SculptureVec3> spine(kAxialSteps);
    const SculptureVec3 delta = {
        endPos.x - startPos.x,
        endPos.y - startPos.y,
        endPos.z - startPos.z
    };
    const float branchLen = std::sqrt(diff_geom::dot3(delta, delta));

    for (std::uint32_t i = 0U; i < kAxialSteps; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kAxialSteps - 1U);
        const float bend = std::sin(t * std::numbers::pi_v<float>) * 0.08f * branchLen;
        const float bendX = (random01(seed ^ 0x11U) - 0.5f) * 2.0f * bend;
        const float bendZ = (random01(seed ^ 0x22U) - 0.5f) * 2.0f * bend;
        spine[i] = {
            startPos.x + delta.x * t + bendX,
            startPos.y + delta.y * t,
            startPos.z + delta.z * t + bendZ
        };
    }

    const auto frames = diff_geom::computeBishopFrames(spine, false);
    const std::uint32_t baseVertex = static_cast<std::uint32_t>(mesh.vertices.size());
    const std::uint32_t ringStride = kRadialSegments + 1U;

    for (std::uint32_t i = 0U; i < kAxialSteps; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kAxialSteps - 1U);
        const float baseR = startRadius + t * (endRadius - startRadius);
        const auto& frame = frames[i];

        for (std::uint32_t j = 0U; j <= kRadialSegments; ++j) {
            const float u = static_cast<float>(j) / static_cast<float>(kRadialSegments);
            const float theta = u * 2.0f * std::numbers::pi_v<float>;
            const float cosT = std::cos(theta);
            const float sinT = std::sin(theta);

            // Bark furrow displacement: delta_bark(theta, y) = A_b * sin(n_ridges * theta + fBm(y))
            const float barkFurrow = 0.08f * baseR * std::sin(
                7.0f * theta + 2.5f * fbm1D(frame.position.y * 1.4f, 2U, seed));
            const float r = std::max(0.01f, baseR + barkFurrow);

            const SculptureVec3 radialDir = diff_geom::normalize3({
                frame.normal.x * cosT + frame.binormal.x * sinT,
                frame.normal.y * cosT + frame.binormal.y * sinT,
                frame.normal.z * cosT + frame.binormal.z * sinT
            });

            IsosurfaceVertex v{};
            v.position = {
                frame.position.x + radialDir.x * r,
                frame.position.y + radialDir.y * r,
                frame.position.z + radialDir.z * r
            };
            v.normal = radialDir;
            v.materialCoord = t;
            v.density = static_cast<float>(depth) / static_cast<float>(std::max(1U, maxDepth));
            mesh.vertices.push_back(v);
        }
    }

    for (std::uint32_t i = 0U; i + 1U < kAxialSteps; ++i) {
        for (std::uint32_t j = 0U; j < kRadialSegments; ++j) {
            const std::uint32_t i00 = baseVertex + i * ringStride + j;
            const std::uint32_t i10 = i00 + 1U;
            const std::uint32_t i01 = baseVertex + (i + 1U) * ringStride + j;
            const std::uint32_t i11 = i01 + 1U;

            mesh.indices.push_back(i00);
            mesh.indices.push_back(i01);
            mesh.indices.push_back(i10);

            mesh.indices.push_back(i10);
            mesh.indices.push_back(i01);
            mesh.indices.push_back(i11);
        }
    }

    if (depth < maxDepth) {
        const std::uint32_t childBranches = 2U + (hash32(seed ^ 0x77U) & 1U);
        for (std::uint32_t c = 0U; c < childBranches; ++c) {
            const std::uint32_t childSeed = hash32(seed + c * 9973U + depth * 313U);
            const float yaw = (static_cast<float>(c) + 0.35f * random01(childSeed))
                            * (2.0f * std::numbers::pi_v<float> / static_cast<float>(childBranches));
            const float spread = (0.35f + 0.25f * random01(childSeed ^ 0x44U)) * branchLen * 0.68f;
            const float childHeight = branchLen * (0.55f + 0.18f * random01(childSeed ^ 0x88U));

            const SculptureVec3 childEnd = {
                endPos.x + std::cos(yaw) * spread,
                endPos.y + childHeight,
                endPos.z + std::sin(yaw) * spread
            };
            appendTaperedBarkBranch(
                mesh,
                endPos,
                childEnd,
                endRadius * 0.85f,
                endRadius * 0.42f,
                childSeed,
                depth + 1U,
                maxDepth);
        }
    }
}

} // namespace

IsosurfaceMesh generateTreeBranchMesh(
    const ForestTreeGpu& tree,
    std::uint32_t seed,
    std::uint32_t maxBranchDepth)
{
    IsosurfaceMesh mesh{};
    const SculptureVec3 rootPos = {
        tree.baseRadius[0],
        tree.baseRadius[1],
        tree.baseRadius[2]
    };
    const float trunkRadius = std::max(tree.baseRadius[3], 0.08f);
    const SculptureVec3 crownPos = {
        tree.canopyCenterRadius[0],
        tree.canopyCenterRadius[1],
        tree.canopyCenterRadius[2]
    };
    const SculptureVec3 trunkTop = {
        rootPos.x + (crownPos.x - rootPos.x) * 0.45f,
        rootPos.y + (crownPos.y - rootPos.y) * 0.65f,
        rootPos.z + (crownPos.z - rootPos.z) * 0.45f
    };

    const std::uint32_t clampedDepth = std::clamp(maxBranchDepth, 1U, 4U);
    appendTaperedBarkBranch(
        mesh,
        rootPos,
        trunkTop,
        trunkRadius,
        trunkRadius * 0.48f,
        seed,
        1U,
        clampedDepth);

    SculptureIsosurface::computeMeshBounds(mesh);
    return mesh;
}

SculptureVec3 evaluateFernFrondPoint(
    const SculptureVec3& rootPos,
    float headingRadians,
    float frondLength,
    float archGravity,
    float maxHalfWidth,
    float s,
    float u,
    const SculptureVec3& windVector,
    float time) noexcept
{
    const float sc = std::clamp(s, 0.0f, 1.0f);
    const float uc = std::clamp(u, -1.0f, 1.0f);

    // Initial upward-and-outward tangent T_0 and lateral binormal B_0
    constexpr float kLaunchElevation = 0.68f;
    const float cosElev = std::cos(kLaunchElevation);
    const float sinElev = std::sin(kLaunchElevation);
    const SculptureVec3 T0 = {
        std::cos(headingRadians) * cosElev,
        sinElev,
        std::sin(headingRadians) * cosElev
    };
    const SculptureVec3 B0 = {
        -std::sin(headingRadians),
        0.0f,
        std::cos(headingRadians)
    };

    // Tapered fern frond pinnae width profile w_frond(s)
    const float widthProfile = maxHalfWidth * std::sin(std::pow(sc, 0.75f) * std::numbers::pi_v<float>);
    const float windFlutter = 0.03f * std::sin(time * 3.4f + rootPos.x * 1.7f + sc * 4.0f);
    const float sSq = sc * sc;

    return {
        rootPos.x + frondLength * (sc * T0.x) + widthProfile * uc * B0.x + (windVector.x + windFlutter) * sSq,
        rootPos.y + frondLength * (sc * T0.y - 0.5f * archGravity * sSq) + windVector.y * sSq,
        rootPos.z + frondLength * (sc * T0.z) + widthProfile * uc * B0.z + (windVector.z + windFlutter) * sSq
    };
}

IsosurfaceMesh generateFernFrondMesh(
    const SculptureVec3& rootPos,
    float headingRadians,
    float frondLength,
    const SculptureVec3& windVector,
    float time,
    std::uint32_t axialSegments)
{
    IsosurfaceMesh mesh{};
    const std::uint32_t steps = std::max(4U, axialSegments);
    const float maxHalfWidth = frondLength * 0.24f;
    const float archGravity = 0.85f;

    mesh.vertices.resize(static_cast<std::size_t>(steps + 1U) * 3U);
    mesh.indices.reserve(static_cast<std::size_t>(steps) * 12U);

    for (std::uint32_t i = 0U; i <= steps; ++i) {
        const float s = static_cast<float>(i) / static_cast<float>(steps);
        const SculptureVec3 leftP = evaluateFernFrondPoint(
            rootPos, headingRadians, frondLength, archGravity, maxHalfWidth, s, -1.0f, windVector, time);
        const SculptureVec3 midP = evaluateFernFrondPoint(
            rootPos, headingRadians, frondLength, archGravity, maxHalfWidth, s, 0.0f, windVector, time);
        const SculptureVec3 rightP = evaluateFernFrondPoint(
            rootPos, headingRadians, frondLength, archGravity, maxHalfWidth, s, 1.0f, windVector, time);

        const float sNext = std::min(1.0f, s + 0.05f);
        const SculptureVec3 midNext = evaluateFernFrondPoint(
            rootPos, headingRadians, frondLength, archGravity, maxHalfWidth, sNext, 0.0f, windVector, time);

        const SculptureVec3 tangent = diff_geom::normalize3({
            midNext.x - midP.x,
            midNext.y - midP.y,
            midNext.z - midP.z
        });
        const SculptureVec3 span = diff_geom::normalize3({
            rightP.x - leftP.x,
            rightP.y - leftP.y,
            rightP.z - leftP.z
        });
        const SculptureVec3 normal = diff_geom::normalize3(diff_geom::cross3(span, tangent));

        const std::size_t base = static_cast<std::size_t>(i) * 3U;
        mesh.vertices[base + 0U] = {leftP, normal, s, 1.0f};
        mesh.vertices[base + 1U] = {{midP.x, midP.y + 0.012f, midP.z}, normal, s, 0.5f};
        mesh.vertices[base + 2U] = {rightP, normal, s, 1.0f};
    }

    for (std::uint32_t i = 0U; i < steps; ++i) {
        const std::uint32_t b0 = i * 3U;
        const std::uint32_t b1 = (i + 1U) * 3U;

        // Left leaflet strip
        mesh.indices.push_back(b0 + 0U);
        mesh.indices.push_back(b1 + 0U);
        mesh.indices.push_back(b0 + 1U);

        mesh.indices.push_back(b0 + 1U);
        mesh.indices.push_back(b1 + 0U);
        mesh.indices.push_back(b1 + 1U);

        // Right leaflet strip
        mesh.indices.push_back(b0 + 1U);
        mesh.indices.push_back(b1 + 1U);
        mesh.indices.push_back(b0 + 2U);

        mesh.indices.push_back(b0 + 2U);
        mesh.indices.push_back(b1 + 1U);
        mesh.indices.push_back(b1 + 2U);
    }

    SculptureIsosurface::computeMeshBounds(mesh);
    return mesh;
}

float evaluateRootedFlowerPetalsSdf(
    const SculptureVec3& p,
    std::span<const ForestGroundFlowerGpu> flowers,
    int* outFlowerIdx,
    float* outPetalU,
    float* outPetalV) noexcept
{
    float dMin = 1e5f;
    constexpr float kSector = 2.0f * std::numbers::pi_v<float> / 5.0f;

    for (std::size_t i = 0U; i < flowers.size(); ++i) {
        const ForestGroundFlowerGpu& fl = flowers[i];
        const SculptureVec3 headPos = {
            fl.headPosRadius[0],
            fl.headPosRadius[1],
            fl.headPosRadius[2]
        };
        const float headRadius = std::max(fl.headPosRadius[3], 0.01f);

        const SculptureVec3 toP = {
            p.x - headPos.x,
            p.y - headPos.y,
            p.z - headPos.z
        };
        const float distSq = diff_geom::dot3(toP, toP);
        if (distSq > 0.16f) {
            continue;
        }

        // Local drooping blossom coordinate frame
        const SculptureVec3 axisY = diff_geom::normalize3(
            {fl.headAxisStiffness[0], fl.headAxisStiffness[1], fl.headAxisStiffness[2]},
            {0.0f, -1.0f, 0.0f});
        const SculptureVec3 refVec = std::abs(axisY.z) < 0.9f
            ? SculptureVec3{0.0f, 0.0f, 1.0f}
            : SculptureVec3{1.0f, 0.0f, 0.0f};
        const SculptureVec3 axisX = diff_geom::normalize3(diff_geom::cross3(axisY, refVec));
        const SculptureVec3 axisZ = diff_geom::cross3(axisX, axisY);

        const float localX = diff_geom::dot3(toP, axisX);
        const float localY = diff_geom::dot3(toP, axisY);
        const float localZ = diff_geom::dot3(toP, axisZ);

        // 5-fold radial symmetry for attached bell-flower petals
        const float angle = std::atan2(localZ, localX);
        const float wrapped = std::fmod(angle + std::numbers::pi_v<float> + kSector * 0.5f, kSector);
        const float localAngle = (wrapped < 0.0f ? wrapped + kSector : wrapped) - kSector * 0.5f;

        const float rXZ = std::sqrt(localX * localX + localZ * localZ);
        const float u = std::clamp(rXZ / headRadius, 0.0f, 1.0f);
        const float v = std::clamp(localAngle / (kSector * 0.5f), -1.0f, 1.0f);

        // Cupped bell profile curve + delicate petal thickness
        const float cupY = -0.08f * std::sin(u * 1.8f) + 0.02f * u * u;
        const float petalWidth = std::sin(u * std::numbers::pi_v<float> * 0.85f);
        const float dy = localY - cupY;
        const float dv = std::max(0.0f, std::abs(v) - petalWidth) * 0.04f;
        const float dPetal = std::sqrt(dy * dy + dv * dv) - 0.004f;

        if (dPetal < dMin) {
            dMin = dPetal;
            if (outFlowerIdx != nullptr) {
                *outFlowerIdx = static_cast<int>(i);
            }
            if (outPetalU != nullptr) {
                *outPetalU = u;
            }
            if (outPetalV != nullptr) {
                *outPetalV = v;
            }
        }
    }
    return dMin;
}

IsosurfaceMesh generateRootedFlowerMesh(
    const ForestGroundFlowerGpu& flower,
    std::uint32_t stemSegments,
    std::uint32_t radialSteps)
{
    const SculptureVec3 rootPos = {
        flower.rootPosScale[0],
        flower.rootPosScale[1],
        flower.rootPosScale[2]
    };
    const SculptureVec3 headPos = {
        flower.headPosRadius[0],
        flower.headPosRadius[1],
        flower.headPosRadius[2]
    };
    const float petalLen = std::max(flower.headPosRadius[3], 0.03f);

    // 1. Extrude slender curved stem from rootPos to headPos
    const std::uint32_t sCount = std::max(4U, stemSegments);
    std::vector<SculptureVec3> stemPoints(sCount);
    for (std::uint32_t i = 0U; i < sCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(sCount - 1U);
        const float archY = t * (headPos.y - rootPos.y) + 0.04f * std::sin(t * std::numbers::pi_v<float>);
        stemPoints[i] = {
            rootPos.x + (headPos.x - rootPos.x) * (t * t),
            rootPos.y + archY,
            rootPos.z + (headPos.z - rootPos.z) * (t * t)
        };
    }

    diff_geom::TubeExtrusionSpec stemSpec{};
    stemSpec.radius = 0.006f;
    stemSpec.radialSegments = 5U;
    stemSpec.closedLoop = false;
    IsosurfaceMesh mesh = diff_geom::extrudeTubeFromPoints(stemPoints, stemSpec);

    // 2. Append K = 5 attached cupped petals around headPos
    const SculptureVec3 axisY = diff_geom::normalize3(
        {flower.headAxisStiffness[0], flower.headAxisStiffness[1], flower.headAxisStiffness[2]},
        {0.0f, -1.0f, 0.0f});
    const SculptureVec3 refVec = std::abs(axisY.z) < 0.9f
        ? SculptureVec3{0.0f, 0.0f, 1.0f}
        : SculptureVec3{1.0f, 0.0f, 0.0f};
    const SculptureVec3 axisX = diff_geom::normalize3(diff_geom::cross3(axisY, refVec));
    const SculptureVec3 axisZ = diff_geom::cross3(axisX, axisY);

    const std::uint32_t uSteps = std::max(3U, radialSteps);
    constexpr std::uint32_t kPetalCount = 5U;

    for (std::uint32_t k = 0U; k < kPetalCount; ++k) {
        const float phiK = static_cast<float>(k) * (2.0f * std::numbers::pi_v<float> / static_cast<float>(kPetalCount));
        const float cosPhi = std::cos(phiK);
        const float sinPhi = std::sin(phiK);

        const SculptureVec3 petalRadialDir = {
            axisX.x * cosPhi + axisZ.x * sinPhi,
            axisX.y * cosPhi + axisZ.y * sinPhi,
            axisX.z * cosPhi + axisZ.z * sinPhi
        };
        const SculptureVec3 petalTangentialDir = {
            -axisX.x * sinPhi + axisZ.x * cosPhi,
            -axisX.y * sinPhi + axisZ.y * cosPhi,
            -axisX.z * sinPhi + axisZ.z * cosPhi
        };

        const std::uint32_t baseVertex = static_cast<std::uint32_t>(mesh.vertices.size());
        for (std::uint32_t iu = 0U; iu <= uSteps; ++iu) {
            const float u = static_cast<float>(iu) / static_cast<float>(uSteps);
            const float thetaCup = 0.35f + 0.65f * u;
            const float rOut = u * petalLen * std::sin(thetaCup);
            const float yDrop = -u * petalLen * std::cos(thetaCup) * 0.45f;
            const float halfWidth = petalLen * 0.38f * std::sin(u * std::numbers::pi_v<float> * 0.88f);

            for (int iv = -1; iv <= 1; ++iv) {
                const float v = static_cast<float>(iv);
                const SculptureVec3 pos = {
                    headPos.x + petalRadialDir.x * rOut + axisY.x * yDrop + petalTangentialDir.x * (v * halfWidth),
                    headPos.y + petalRadialDir.y * rOut + axisY.y * yDrop + petalTangentialDir.y * (v * halfWidth),
                    headPos.z + petalRadialDir.z * rOut + axisY.z * yDrop + petalTangentialDir.z * (v * halfWidth)
                };
                const SculptureVec3 normal = diff_geom::normalize3({
                    petalRadialDir.x * 0.4f - axisY.x * 0.9f,
                    petalRadialDir.y * 0.4f - axisY.y * 0.9f,
                    petalRadialDir.z * 0.4f - axisY.z * 0.9f
                });
                const float vein = 0.75f + 0.25f * std::pow(std::cos(v * std::numbers::pi_v<float> * 6.0f), 4.0f);

                mesh.vertices.push_back({pos, normal, u, vein});
            }
        }

        for (std::uint32_t iu = 0U; iu < uSteps; ++iu) {
            const std::uint32_t r0 = baseVertex + iu * 3U;
            const std::uint32_t r1 = baseVertex + (iu + 1U) * 3U;

            mesh.indices.push_back(r0 + 0U);
            mesh.indices.push_back(r1 + 0U);
            mesh.indices.push_back(r0 + 1U);

            mesh.indices.push_back(r0 + 1U);
            mesh.indices.push_back(r1 + 0U);
            mesh.indices.push_back(r1 + 1U);

            mesh.indices.push_back(r0 + 1U);
            mesh.indices.push_back(r1 + 1U);
            mesh.indices.push_back(r0 + 2U);

            mesh.indices.push_back(r0 + 2U);
            mesh.indices.push_back(r1 + 1U);
            mesh.indices.push_back(r1 + 2U);
        }
    }

    SculptureIsosurface::computeMeshBounds(mesh);
    return mesh;
}

} // namespace epi_sculpt::forest
