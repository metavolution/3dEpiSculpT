#include "ProceduralForestBotanicalHierarchy.h"
#include "SculptureDifferentialGeometry.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace epi_sculpt::forest {

ForestTreeSpeciesProfile getTreeSpeciesProfile(ForestTreeSpecies species) noexcept {
    ForestTreeSpeciesProfile p{};
    p.species = species;
    switch (species) {
        case ForestTreeSpecies::EuropeanBeech:
            p.trunkTaper = 0.24f;
            p.rootButtressAmplitude = 0.30f;
            p.rootButtressHeight = 1.05f;
            p.rootButtressLobes = 5U;
            p.barkFurrowAmplitude = 0.018f;
            p.barkRidgeCount = 8.0f;
            p.crownVerticalAspect = 0.84f;
            p.branchSpreadAngleRad = 0.60f;
            p.canopyLobeCount = 6U;
            p.barkAlbedo = {0.25f, 0.23f, 0.20f};
            p.foliageAlbedo = {0.14f, 0.32f, 0.08f};
            p.foliageSssTint = {0.48f, 0.74f, 0.18f};
            break;

        case ForestTreeSpecies::SilverBirch:
            p.trunkTaper = 0.18f;
            p.rootButtressAmplitude = 0.12f;
            p.rootButtressHeight = 0.55f;
            p.rootButtressLobes = 4U;
            p.barkFurrowAmplitude = 0.012f;
            p.barkRidgeCount = 14.0f;
            p.crownVerticalAspect = 1.18f;
            p.branchSpreadAngleRad = 0.44f;
            p.canopyLobeCount = 5U;
            p.barkAlbedo = {0.72f, 0.70f, 0.66f};
            p.foliageAlbedo = {0.18f, 0.36f, 0.10f};
            p.foliageSssTint = {0.56f, 0.80f, 0.22f};
            break;

        case ForestTreeSpecies::MossyOak:
            p.trunkTaper = 0.30f;
            p.rootButtressAmplitude = 0.38f;
            p.rootButtressHeight = 1.25f;
            p.rootButtressLobes = 6U;
            p.barkFurrowAmplitude = 0.036f;
            p.barkRidgeCount = 12.0f;
            p.crownVerticalAspect = 0.72f;
            p.branchSpreadAngleRad = 0.78f;
            p.canopyLobeCount = 7U;
            p.barkAlbedo = {0.19f, 0.15f, 0.10f};
            p.foliageAlbedo = {0.12f, 0.27f, 0.07f};
            p.foliageSssTint = {0.40f, 0.64f, 0.15f};
            break;

        case ForestTreeSpecies::BlackForestSpruce:
        default:
            p.trunkTaper = 0.14f;
            p.rootButtressAmplitude = 0.20f;
            p.rootButtressHeight = 0.75f;
            p.rootButtressLobes = 5U;
            p.barkFurrowAmplitude = 0.024f;
            p.barkRidgeCount = 16.0f;
            p.crownVerticalAspect = 1.45f;
            p.branchSpreadAngleRad = 0.52f;
            p.canopyLobeCount = 6U;
            p.barkAlbedo = {0.21f, 0.14f, 0.10f};
            p.foliageAlbedo = {0.08f, 0.21f, 0.06f};
            p.foliageSssTint = {0.26f, 0.48f, 0.12f};
            break;
    }
    return p;
}

SculptureVec3 computePhototropicCrownAsymmetry(
    const SculptureVec3& treeBase,
    std::size_t selfIdx,
    std::span<const ForestTreeGpu> allTrees,
    float clearingCenterX,
    float clearingCenterZ) noexcept
{
    // 1. Phototropic attraction toward the sunlit clearing opening
    const float toClearX = clearingCenterX - treeBase.x;
    const float toClearZ = clearingCenterZ - treeBase.z;
    const float distClear = std::sqrt(toClearX * toClearX + toClearZ * toClearZ);
    float leanX = 0.0f;
    float leanZ = 0.0f;
    if (distClear > 1e-3f) {
        const float borderWeight = std::exp(-std::pow((distClear - 18.0f) / 14.0f, 2.0f));
        leanX += 0.35f * borderWeight * (toClearX / distClear);
        leanZ += 0.35f * borderWeight * (toClearZ / distClear);
    }

    // 2. Crown competition repulsion from nearby neighbor trunks
    for (std::size_t k = 0U; k < allTrees.size(); ++k) {
        if (k == selfIdx) {
            continue;
        }
        const float dx = treeBase.x - allTrees[k].baseRadius[0];
        const float dz = treeBase.z - allTrees[k].baseRadius[2];
        const float d2 = dx * dx + dz * dz;
        if (d2 > 1e-4f && d2 < 64.0f) {
            const float d = std::sqrt(d2);
            const float repel = 0.45f * std::exp(-d2 / 18.0f);
            leanX += repel * (dx / d);
            leanZ += repel * (dz / d);
        }
    }

    const float mag = std::sqrt(leanX * leanX + leanZ * leanZ);
    if (mag > 0.85f) {
        leanX *= 0.85f / mag;
        leanZ *= 0.85f / mag;
    }
    return {leanX, 0.0f, leanZ};
}

SculptureVec3 computePhototropicLeanVector(
    std::size_t treeIndex,
    std::span<const ForestTreeGpu> allTrees,
    float clearingCenterX,
    float clearingCenterZ) noexcept
{
    if (treeIndex >= allTrees.size()) {
        return {0.0f, 0.0f, 0.0f};
    }
    const ForestTreeGpu& self = allTrees[treeIndex];
    return computePhototropicCrownAsymmetry(
        {self.baseRadius[0], self.baseRadius[1], self.baseRadius[2]},
        treeIndex,
        allTrees,
        clearingCenterX,
        clearingCenterZ);
}

ForestTreeSkeleton generateSpeciesTreeSkeleton(
    const ForestTreeGpu& tree,
    ForestTreeSpecies species,
    const SculptureVec3& phototropicLeanXZ,
    std::uint32_t seed)
{
    ForestTreeSkeleton skel{};
    skel.species = species;
    skel.rootPos = {tree.baseRadius[0], tree.baseRadius[1], tree.baseRadius[2]};
    skel.phototropicLeanXZ = phototropicLeanXZ;

    const ForestTreeSpeciesProfile profile = getTreeSpeciesProfile(species);
    const float baseR = std::max(tree.baseRadius[3], 0.12f);
    const SculptureVec3 crownCenter = {
        tree.canopyCenterRadius[0] + phototropicLeanXZ.x * 1.1f,
        std::max(tree.canopyCenterRadius[1], skel.rootPos.y + 3.0f),
        tree.canopyCenterRadius[2] + phototropicLeanXZ.z * 1.1f
    };
    const float crownR = std::max(tree.canopyCenterRadius[3], 1.5f);

    // Lower & upper main trunk segments with slight organic lean
    const SculptureVec3 midTrunk = {
        skel.rootPos.x + (crownCenter.x - skel.rootPos.x) * 0.45f,
        skel.rootPos.y + (crownCenter.y - skel.rootPos.y) * 0.52f,
        skel.rootPos.z + (crownCenter.z - skel.rootPos.z) * 0.45f
    };
    const float midR = baseR * 0.68f;
    const float topR = baseR * profile.trunkTaper;

    skel.branches.push_back({skel.rootPos, midTrunk, baseR, midR, 0U});
    skel.branches.push_back({midTrunk, crownCenter, midR, topR, 0U});

    // Primary and secondary structural limbs + asymmetric canopy foliage lobes
    const std::uint32_t lobeCount = std::max(3U, profile.canopyLobeCount);
    skel.canopyLobes.reserve(lobeCount + 1U);

    // Core central crown lobe
    skel.canopyLobes.push_back({
        crownCenter,
        {crownR * 0.92f, crownR * profile.crownVerticalAspect * 0.85f, crownR * 0.92f},
        seed ^ 0x101U
    });

    for (std::uint32_t i = 0U; i < lobeCount; ++i) {
        const std::uint32_t h = hash32(seed ^ (i * 0x9E3779B9U + 0x243F6A88U));
        const float baseAzimuth = (2.0f * std::numbers::pi_v<float> * static_cast<float>(i)) / static_cast<float>(lobeCount);
        const float azimuth = baseAzimuth + (random01(h ^ 0x11U) - 0.5f) * 0.55f;
        const float pitch = profile.branchSpreadAngleRad * (0.75f + 0.45f * random01(h ^ 0x22U));
        const float limbLen = crownR * (0.62f + 0.32f * random01(h ^ 0x33U));

        const float forkFraction = 0.42f + 0.38f * (static_cast<float>(i) / static_cast<float>(lobeCount));
        const SculptureVec3 branchStart = {
            skel.rootPos.x + (crownCenter.x - skel.rootPos.x) * forkFraction,
            skel.rootPos.y + (crownCenter.y - skel.rootPos.y) * forkFraction,
            skel.rootPos.z + (crownCenter.z - skel.rootPos.z) * forkFraction
        };

        const SculptureVec3 dir = diff_geom::normalize3({
            std::sin(pitch) * std::cos(azimuth) + phototropicLeanXZ.x * 0.35f,
            std::cos(pitch),
            std::sin(pitch) * std::sin(azimuth) + phototropicLeanXZ.z * 0.35f
        });

        const SculptureVec3 branchEnd = {
            branchStart.x + dir.x * limbLen,
            branchStart.y + dir.y * limbLen,
            branchStart.z + dir.z * limbLen
        };

        const float limbStartR = midR * (0.55f - 0.18f * forkFraction);
        const float limbEndR = limbStartR * 0.38f;
        skel.branches.push_back({branchStart, branchEnd, limbStartR, limbEndR, 1U});

        // Secondary twig fork
        const float subYaw = azimuth + ((i & 1U) != 0U ? 0.52f : -0.52f);
        const SculptureVec3 subDir = diff_geom::normalize3({
            std::sin(pitch * 0.85f) * std::cos(subYaw),
            std::cos(pitch * 0.85f),
            std::sin(pitch * 0.85f) * std::sin(subYaw)
        });
        const float subLen = limbLen * 0.58f;
        const SculptureVec3 subEnd = {
            branchEnd.x + subDir.x * subLen,
            branchEnd.y + subDir.y * subLen,
            branchEnd.z + subDir.z * subLen
        };
        skel.branches.push_back({branchEnd, subEnd, limbEndR, limbEndR * 0.45f, 2U});

        const float lobeR = crownR * (0.48f + 0.22f * random01(h ^ 0x44U));
        skel.canopyLobes.push_back({
            subEnd,
            {lobeR, lobeR * profile.crownVerticalAspect * 0.78f, lobeR},
            h
        });
    }

    return skel;
}

float evaluateTreeTrunkSdf(
    const SculptureVec3& p,
    const ForestTreeGpu& tree,
    std::uint32_t seed) noexcept
{
    const SculptureVec3 a = {tree.baseRadius[0], tree.baseRadius[1], tree.baseRadius[2]};
    const SculptureVec3 b = {tree.canopyCenterRadius[0], tree.canopyCenterRadius[1], tree.canopyCenterRadius[2]};
    const float rBase = std::max(tree.baseRadius[3], 0.08f);
    const float rTop = rBase * 0.25f;

    const SculptureVec3 ba = {b.x - a.x, b.y - a.y, b.z - a.z};
    const SculptureVec3 pa = {p.x - a.x, p.y - a.y, p.z - a.z};
    const float baDotBa = std::max(diff_geom::dot3(ba, ba), 1e-4f);
    const float h = std::clamp(diff_geom::dot3(pa, ba) / baDotBa, 0.0f, 1.0f);

    const SculptureVec3 axisPt = {a.x + h * ba.x, a.y + h * ba.y, a.z + h * ba.z};
    const SculptureVec3 radial = {p.x - axisPt.x, p.y - axisPt.y, p.z - axisPt.z};
    const float radialDist = diff_geom::length3(radial);

    // Tapered trunk radius + root-flare buttressing + longitudinal bark furrows
    float radiusAtH = rBase + h * (rTop - rBase);
    if (radialDist < radiusAtH + 0.65f) {
        const float theta = std::atan2(radial.z, radial.x);
        const float heightAboveRoot = std::max(0.0f, p.y - a.y);
        const float cosButtress = std::cos(5.0f * theta + static_cast<float>(seed & 0xFFU) * 0.05f);
        const float rootButtress = rBase * 0.32f * std::exp(-heightAboveRoot / 0.95f) * (cosButtress * cosButtress);
        const float barkNoise = fbm1D(p.y * 1.8f, 2U, seed ^ 0x55U);
        const float barkFurrow = 0.022f * std::sin(10.0f * theta + 2.5f * barkNoise);
        radiusAtH += rootButtress + barkFurrow;
    }

    return radialDist - radiusAtH;
}

float evaluateTreeCanopySdf(
    const SculptureVec3& p,
    const ForestTreeGpu& tree,
    std::uint32_t seed) noexcept
{
    const SculptureVec3 c = {
        tree.canopyCenterRadius[0],
        tree.canopyCenterRadius[1],
        tree.canopyCenterRadius[2]
    };
    const float r = std::max(tree.canopyCenterRadius[3], 0.5f);

    const float dx = p.x - c.x;
    const float dy = (p.y - c.y) / 0.82f;
    const float dz = p.z - c.z;
    float dBase = (std::sqrt(dx * dx + dy * dy + dz * dz) - r) * 0.82f;
    if (dBase < 1.35f) {
        // Worley + fBm leaf-cluster billowing silhouette
        const float leafClusters =
            0.34f * worley3D(p.x * 0.65f, p.y * 0.65f, p.z * 0.65f, seed ^ 0x91U)
          + 0.14f * fbm3D(p.x * 1.8f, p.y * 1.8f, p.z * 1.8f, 2U, seed ^ 0x37U);
        dBase += leafClusters - 0.18f;
    }
    return dBase;
}

float evaluateTreesSdf(
    const SculptureVec3& p,
    std::span<const ForestTreeGpu> trees,
    ForestMaterialId* outTreeMaterial,
    int* outTreeIndex) noexcept
{
    float dMin = 1e5f;
    ForestMaterialId bestMat = ForestMaterialId::TreeBark;
    int bestIdx = -1;

    for (std::size_t i = 0U; i < trees.size(); ++i) {
        const ForestTreeGpu& t = trees[i];
        // Quick XZ cylinder bounding check
        const float dx = p.x - t.baseRadius[0];
        const float dz = p.z - t.baseRadius[2];
        const float maxCrownR = t.canopyCenterRadius[3] + 2.0f;
        if (dx * dx + dz * dz > maxCrownR * maxCrownR) {
            continue;
        }

        const std::uint32_t treeSeed = hash32(static_cast<std::uint32_t>(i) * 2654435761U + 0x54524545U);
        const float dTrunk = evaluateTreeTrunkSdf(p, t, treeSeed);
        if (dTrunk < dMin) {
            dMin = dTrunk;
            bestMat = ForestMaterialId::TreeBark;
            bestIdx = static_cast<int>(i);
        }

        const float dCanopy = evaluateTreeCanopySdf(p, t, treeSeed);
        if (dCanopy < dMin) {
            dMin = dCanopy;
            bestMat = ForestMaterialId::CanopyFoliage;
            bestIdx = static_cast<int>(i);
        }
    }

    if (outTreeMaterial != nullptr) {
        *outTreeMaterial = bestMat;
    }
    if (outTreeIndex != nullptr) {
        *outTreeIndex = bestIdx;
    }
    return dMin;
}

IsosurfaceMesh generateGrassBladeClusterMesh(
    const SculptureVec3& clumpCenter,
    float clumpRadius,
    std::uint32_t bladeCount,
    const SculptureVec3& windVector,
    float time,
    std::uint32_t seed)
{
    IsosurfaceMesh mesh{};
    const std::uint32_t count = std::max(1U, bladeCount);
    constexpr std::uint32_t kBladeSteps = 5U;

    for (std::uint32_t b = 0U; b < count; ++b) {
        const std::uint32_t h = hash32(seed ^ (b * 747796405U + 0x9E3779B9U));
        float jx = 0.0f;
        float jz = 0.0f;
        random2(h, jx, jz);
        const float angle = random01(h ^ 0x11U) * 2.0f * std::numbers::pi_v<float>;
        const float r = std::sqrt(jx) * clumpRadius;

        const SculptureVec3 root = {
            clumpCenter.x + r * std::cos(angle),
            clumpCenter.y,
            clumpCenter.z + r * std::sin(angle)
        };

        const float bladeHeight = 0.22f + 0.26f * random01(h ^ 0x22U);
        const float bladeHalfWidth = 0.012f + 0.008f * random01(h ^ 0x33U);
        const float heading = random01(h ^ 0x44U) * 2.0f * std::numbers::pi_v<float>;
        const float bend = 0.10f + 0.14f * random01(h ^ 0x55U);
        const float windOsc = 0.04f * std::sin(time * 3.6f + static_cast<float>(b) * 1.1f);

        const SculptureVec3 sideDir = {-std::sin(heading), 0.0f, std::cos(heading)};
        const std::uint32_t baseVertex = static_cast<std::uint32_t>(mesh.vertices.size());

        for (std::uint32_t sIdx = 0U; sIdx <= kBladeSteps; ++sIdx) {
            const float t = static_cast<float>(sIdx) / static_cast<float>(kBladeSteps);
            const float t2 = t * t;
            const float widthProfile = bladeHalfWidth * (1.0f - 0.88f * t * t);

            const SculptureVec3 spine = {
                root.x + (std::cos(heading) * bend + (windVector.x + windOsc) * 0.18f) * t2,
                root.y + bladeHeight * (t - 0.18f * t2),
                root.z + (std::sin(heading) * bend + (windVector.z + windOsc) * 0.18f) * t2
            };
            const SculptureVec3 normal = diff_geom::normalize3({
                -std::cos(heading) * 0.35f,
                0.92f,
                -std::sin(heading) * 0.35f
            });

            for (int side = -1; side <= 1; side += 2) {
                const float sign = static_cast<float>(side);
                IsosurfaceVertex v{};
                v.position = {
                    spine.x + sideDir.x * widthProfile * sign,
                    spine.y,
                    spine.z + sideDir.z * widthProfile * sign
                };
                v.normal = normal;
                v.materialCoord = t;
                v.density = 1.0f;
                mesh.vertices.push_back(v);
            }
        }

        for (std::uint32_t sIdx = 0U; sIdx < kBladeSteps; ++sIdx) {
            const std::uint32_t i00 = baseVertex + sIdx * 2U;
            const std::uint32_t i10 = i00 + 1U;
            const std::uint32_t i01 = baseVertex + (sIdx + 1U) * 2U;
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

IsosurfaceMesh generateUnderstoryShrubMesh(
    const SculptureVec3& rootPos,
    float scale,
    std::uint32_t stemCount,
    std::uint32_t seed)
{
    IsosurfaceMesh mesh{};
    const std::uint32_t stems = std::max(2U, stemCount);
    const float safeScale = std::max(scale, 0.2f);

    auto appendSubMesh = [](IsosurfaceMesh& dst, const IsosurfaceMesh& src) {
        const std::uint32_t base = static_cast<std::uint32_t>(dst.vertices.size());
        dst.vertices.insert(dst.vertices.end(), src.vertices.begin(), src.vertices.end());
        for (std::uint32_t idx : src.indices) {
            dst.indices.push_back(base + idx);
        }
    };

    for (std::uint32_t s = 0U; s < stems; ++s) {
        const std::uint32_t h = hash32(seed ^ (s * 0x85EBCA6BU + 0xC2B2AE35U));
        const float yaw = (2.0f * std::numbers::pi_v<float> * static_cast<float>(s)) / static_cast<float>(stems)
                        + (random01(h ^ 0x17U) - 0.5f) * 0.45f;
        const float len = (0.55f + 0.35f * random01(h ^ 0x29U)) * safeScale;
        const float spread = (0.35f + 0.25f * random01(h ^ 0x3BU)) * safeScale;

        std::vector<SculptureVec3> pts(5U);
        for (std::uint32_t i = 0U; i < 5U; ++i) {
            const float t = static_cast<float>(i) / 4.0f;
            const float rHoriz = spread * std::pow(t, 1.3f);
            pts[i] = {
                rootPos.x + rHoriz * std::cos(yaw),
                rootPos.y + len * (t - 0.15f * t * t),
                rootPos.z + rHoriz * std::sin(yaw)
            };
        }
        diff_geom::TubeExtrusionSpec spec{};
        spec.radius = 0.022f * safeScale;
        spec.radialSegments = 6U;
        spec.closedLoop = false;
        appendSubMesh(mesh, diff_geom::extrudeTubeFromPoints(pts, spec));
    }

    SculptureIsosurface::computeMeshBounds(mesh);
    return mesh;
}

IsosurfaceMesh generateForestDebrisClusterMesh(
    const SculptureVec3& centerPos,
    float clusterRadius,
    std::uint32_t itemCount,
    std::uint32_t seed)
{
    IsosurfaceMesh mesh{};
    const std::uint32_t count = std::max(1U, itemCount);

    auto appendSubMesh = [](IsosurfaceMesh& dst, const IsosurfaceMesh& src) {
        const std::uint32_t base = static_cast<std::uint32_t>(dst.vertices.size());
        dst.vertices.insert(dst.vertices.end(), src.vertices.begin(), src.vertices.end());
        for (std::uint32_t idx : src.indices) {
            dst.indices.push_back(base + idx);
        }
    };

    for (std::uint32_t i = 0U; i < count; ++i) {
        const std::uint32_t h = hash32(seed ^ (i * 0x27D4EB2DU + 0x165667B1U));
        const float angle = random01(h ^ 0x11U) * 2.0f * std::numbers::pi_v<float>;
        const float dist = std::sqrt(random01(h ^ 0x22U)) * clusterRadius;
        const float yaw = random01(h ^ 0x33U) * 2.0f * std::numbers::pi_v<float>;
        const float twigLen = 0.14f + 0.22f * random01(h ^ 0x44U);
        const float twigR = 0.008f + 0.010f * random01(h ^ 0x55U);

        const SculptureVec3 p0 = {
            centerPos.x + dist * std::cos(angle),
            centerPos.y + twigR,
            centerPos.z + dist * std::sin(angle)
        };
        const SculptureVec3 p1 = {
            p0.x + 0.5f * twigLen * std::cos(yaw),
            p0.y + 0.015f * random01(h ^ 0x66U),
            p0.z + 0.5f * twigLen * std::sin(yaw)
        };
        const SculptureVec3 p2 = {
            p0.x + twigLen * std::cos(yaw + 0.18f),
            p0.y + twigR * 0.6f,
            p0.z + twigLen * std::sin(yaw + 0.18f)
        };

        const std::vector<SculptureVec3> pts = {p0, p1, p2};
        diff_geom::TubeExtrusionSpec spec{};
        spec.radius = twigR;
        spec.radialSegments = 5U;
        spec.closedLoop = false;
        appendSubMesh(mesh, diff_geom::extrudeTubeFromPoints(pts, spec));
    }

    SculptureIsosurface::computeMeshBounds(mesh);
    return mesh;
}

} // namespace epi_sculpt::forest
