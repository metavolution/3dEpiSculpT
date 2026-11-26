/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - DEDICATED PROCEDURAL FOREST LICHTUNG RENDER BRIDGE
 * ============================================================================
 */

#include "ProceduralForestLichtungRenderBridge.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

namespace epi_sculpt::forest {

namespace {

[[nodiscard]] bool isFiniteVec3(const SculptureVec3& v) noexcept {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

[[nodiscard]] float hashSeedToAngle(std::uint32_t seed) noexcept {
    std::uint32_t x = seed ^ 0x9E3779B9U;
    x ^= x >> 16U;
    x *= 0x7FEB352DU;
    x ^= x >> 15U;
    x *= 0x846CA68BU;
    x ^= x >> 16U;
    return static_cast<float>(x & 0x00FFFFFFU)
         * (6.28318530718f / static_cast<float>(0x01000000U));
}

[[nodiscard]] float hashSeedToUnit(std::uint32_t seed) noexcept {
    std::uint32_t x = seed ^ 0x85EBCA6BU;
    x ^= x >> 16U;
    x *= 0x7FEB352DU;
    x ^= x >> 15U;
    return static_cast<float>(x & 0x00FFFFFFU)
         / static_cast<float>(0x01000000U);
}

[[nodiscard]] mgfx::math::vec3 safeNormalizeVec3(
    const mgfx::math::vec3& v,
    const mgfx::math::vec3& fallback) noexcept {
    const float lenSq = glm::dot(v, v);
    if (!std::isfinite(lenSq) || lenSq <= 1.0e-10f) {
        return fallback;
    }
    return v * (1.0f / std::sqrt(lenSq));
}

[[nodiscard]] mgfx::math::vec4 computeOrthonormalTangent(
    const mgfx::math::vec3& normal) noexcept {
    const mgfx::math::vec3 n = safeNormalizeVec3(normal, {0.0f, 1.0f, 0.0f});
    const mgfx::math::vec3 refAxis =
        (std::abs(n.y) < 0.92f) ? mgfx::math::vec3(0.0f, 1.0f, 0.0f)
                                : mgfx::math::vec3(1.0f, 0.0f, 0.0f);
    const mgfx::math::vec3 tangent = safeNormalizeVec3(
        glm::cross(refAxis, n), {1.0f, 0.0f, 0.0f});
    return {tangent.x, tangent.y, tangent.z, 1.0f};
}

void appendIsosurfaceMesh(
    IsosurfaceMesh& dst,
    const IsosurfaceMesh& src) {
    if (src.vertices.empty() || src.indices.empty()) {
        return;
    }
    const std::uint32_t baseVertex = static_cast<std::uint32_t>(dst.vertices.size());
    dst.vertices.insert(dst.vertices.end(), src.vertices.begin(), src.vertices.end());
    dst.indices.reserve(dst.indices.size() + src.indices.size());
    for (std::uint32_t idx : src.indices) {
        dst.indices.push_back(baseVertex + idx);
    }
    SculptureIsosurface::computeMeshBounds(dst);
}

[[nodiscard]] mgfx::math::mat4 composeTrsMatrix(
    const mgfx::math::vec3& translation,
    float yawRadians,
    const mgfx::math::vec3& tiltAxis,
    float tiltRadians,
    const mgfx::math::vec3& scale) noexcept {
    mgfx::math::mat4 m(1.0f);
    m = glm::translate(m, translation);
    if (std::abs(tiltRadians) > 1.0e-5f) {
        const mgfx::math::vec3 axis = safeNormalizeVec3(tiltAxis, {0.0f, 0.0f, 1.0f});
        m = glm::rotate(m, tiltRadians, axis);
    }
    if (std::abs(yawRadians) > 1.0e-5f) {
        m = glm::rotate(m, yawRadians, mgfx::math::vec3(0.0f, 1.0f, 0.0f));
    }
    m = glm::scale(m, scale);
    return m;
}

} // namespace

ForestHostArchetypeGeometry
ProceduralForestLichtungRenderBridge::convertIsosurfaceToHostArchetype(
    ForestMeshArchetype archetype,
    const IsosurfaceMesh& sourceMesh,
    const mgfx::math::vec4& baseVertexColor) {
    ForestHostArchetypeGeometry out{};
    out.archetype = archetype;
    out.aabbMin = sourceMesh.aabbMin;
    out.aabbMax = sourceMesh.aabbMax;

    if (sourceMesh.vertices.empty() || sourceMesh.indices.empty()) {
        return out;
    }

    const float spanX = std::max(sourceMesh.aabbMax.x - sourceMesh.aabbMin.x, 1.0e-4f);
    const float spanY = std::max(sourceMesh.aabbMax.y - sourceMesh.aabbMin.y, 1.0e-4f);
    const float spanZ = std::max(sourceMesh.aabbMax.z - sourceMesh.aabbMin.z, 1.0e-4f);

    out.vertices.reserve(sourceMesh.vertices.size());
    for (const IsosurfaceVertex& srcVert : sourceMesh.vertices) {
        mgfx::vk::Vertex dstVert{};
        dstVert.position = {srcVert.position.x, srcVert.position.y, srcVert.position.z};
        dstVert.normal = safeNormalizeVec3(
            {srcVert.normal.x, srcVert.normal.y, srcVert.normal.z},
            {0.0f, 1.0f, 0.0f});

        const float u = std::clamp(
            (srcVert.position.x - sourceMesh.aabbMin.x) / spanX, 0.0f, 1.0f);
        const float v = std::clamp(
            (srcVert.position.z - sourceMesh.aabbMin.z) / spanZ, 0.0f, 1.0f);
        const float hNorm = std::clamp(
            (srcVert.position.y - sourceMesh.aabbMin.y) / spanY, 0.0f, 1.0f);
        dstVert.uv = {u, v};

        // Modulate vertex color slightly by height and materialCoord so moss,
        // bark, and petal tips exhibit natural organic gradient shading.
        const float matBlend = std::clamp(srcVert.materialCoord, 0.0f, 1.0f);
        const float verticalShade = 0.78f + 0.22f * hNorm;
        dstVert.color = {
            std::clamp(baseVertexColor.r * verticalShade * (0.90f + 0.15f * matBlend), 0.0f, 1.5f),
            std::clamp(baseVertexColor.g * verticalShade * (0.92f + 0.12f * matBlend), 0.0f, 1.5f),
            std::clamp(baseVertexColor.b * verticalShade * (0.88f + 0.14f * matBlend), 0.0f, 1.5f),
            baseVertexColor.a
        };
        if (archetype == ForestMeshArchetype::TerrainValleyFloor) {
            // The ecological path mask already defines dirt versus meadow.
            // Preserve it in observation color instead of painting the trail green.
            const auto albedo = glm::mix(mgfx::math::vec3(0.32f, 0.20f, 0.11f),
                mgfx::math::vec3(0.16f, 0.29f, 0.09f), matBlend);
            dstVert.color = mgfx::math::vec4(albedo * verticalShade, 1.0f);
        }
        dstVert.tangent = computeOrthonormalTangent(dstVert.normal);
        out.vertices.push_back(dstVert);
    }

    // Filter out any out-of-bounds or degenerate triangles
    const std::uint32_t vCount = static_cast<std::uint32_t>(out.vertices.size());
    out.indices.reserve(sourceMesh.indices.size());
    for (std::size_t i = 0U; i + 2U < sourceMesh.indices.size(); i += 3U) {
        const std::uint32_t i0 = sourceMesh.indices[i + 0U];
        const std::uint32_t i1 = sourceMesh.indices[i + 1U];
        const std::uint32_t i2 = sourceMesh.indices[i + 2U];
        if (i0 < vCount && i1 < vCount && i2 < vCount
            && i0 != i1 && i1 != i2 && i0 != i2) {
            out.indices.push_back(i0);
            out.indices.push_back(i1);
            out.indices.push_back(i2);
        }
    }

    return out;
}

void ProceduralForestLichtungRenderBridge::buildHostArchetypeGeometries(
    const ForestFieldGrid& fieldGrid,
    const ForestCameraFrustum& frustum,
    std::uint32_t seed) {
    if (hostArchetypesBuilt_
        && builtTerrainGridWidth_ == fieldGrid.config.gridWidth
        && builtTerrainGridHeight_ == fieldGrid.config.gridHeight
        && builtSeed_ == seed) {
        return;
    }

    // -------------------------------------------------------------------------
    // Archetype 0: TerrainValleyFloor (Crack-free skirted terrain chunk grid)
    // -------------------------------------------------------------------------
    IsosurfaceMesh stitchedTerrain{};
    if (!fieldGrid.cells.empty() && fieldGrid.config.gridWidth >= 4U && fieldGrid.config.gridHeight >= 4U) {
        const auto chunks = generateChunkedLichtungTerrainMeshes(
            fieldGrid, frustum, 4U, 0.45f);
        for (const ForestTerrainChunkMesh& chunk : chunks) {
            appendIsosurfaceMesh(stitchedTerrain, chunk.mesh);
        }
    }
    if (stitchedTerrain.empty() && !fieldGrid.cells.empty()) {
        stitchedTerrain = generateLichtungTerrainMesh(fieldGrid);
    }
    hostArchetypes_[static_cast<std::size_t>(ForestMeshArchetype::TerrainValleyFloor)] =
        convertIsosurfaceToHostArchetype(
            ForestMeshArchetype::TerrainValleyFloor,
            stitchedTerrain,
            mgfx::math::vec4(0.24f, 0.34f, 0.16f, 1.0f));

    // -------------------------------------------------------------------------
    // Archetype 1: MossyBoulder (Unit-centered Worley granite boulder)
    // -------------------------------------------------------------------------
    ForestMossyBoulderGpu unitBoulder{};
    unitBoulder.centerRadius[0] = 0.0f;
    unitBoulder.centerRadius[1] = 0.0f;
    unitBoulder.centerRadius[2] = 0.0f;
    unitBoulder.centerRadius[3] = 1.0f;
    unitBoulder.stretchSeed[0] = 1.0f;
    unitBoulder.stretchSeed[1] = 1.15f;
    unitBoulder.stretchSeed[2] = 0.92f;
    unitBoulder.stretchSeed[3] = static_cast<float>(seed & 0xFFFFU);
    const IsosurfaceMesh boulderMesh = generateMossyBoulderMesh(unitBoulder, 0.0f, 14U, 20U);
    hostArchetypes_[static_cast<std::size_t>(ForestMeshArchetype::MossyBoulder)] =
        convertIsosurfaceToHostArchetype(
            ForestMeshArchetype::MossyBoulder,
            boulderMesh,
            mgfx::math::vec4(0.38f, 0.44f, 0.34f, 1.0f));

    // -------------------------------------------------------------------------
    // Archetype 2: TreeTrunkAndBranches (Unit-normalized bifurcating trunk)
    // -------------------------------------------------------------------------
    ForestTreeGpu unitTree{};
    unitTree.baseRadius[0] = 0.0f;
    unitTree.baseRadius[1] = 0.0f;
    unitTree.baseRadius[2] = 0.0f;
    unitTree.baseRadius[3] = 0.06f;
    unitTree.canopyCenterRadius[0] = 0.0f;
    unitTree.canopyCenterRadius[1] = 1.0f;
    unitTree.canopyCenterRadius[2] = 0.0f;
    unitTree.canopyCenterRadius[3] = 0.42f;
    const IsosurfaceMesh trunkMesh = generateTreeBranchMesh(unitTree, seed ^ 0x54524545U, 2U);
    hostArchetypes_[static_cast<std::size_t>(ForestMeshArchetype::TreeTrunkAndBranches)] =
        convertIsosurfaceToHostArchetype(
            ForestMeshArchetype::TreeTrunkAndBranches,
            trunkMesh,
            mgfx::math::vec4(0.32f, 0.23f, 0.16f, 1.0f));

    // -------------------------------------------------------------------------
    // Archetype 3: CanopyFoliageCluster (Organic Worley-displaced crown lobe)
    // -------------------------------------------------------------------------
    ForestMossyBoulderGpu unitCrownLobe{};
    unitCrownLobe.centerRadius[0] = 0.0f;
    unitCrownLobe.centerRadius[1] = 0.0f;
    unitCrownLobe.centerRadius[2] = 0.0f;
    unitCrownLobe.centerRadius[3] = 1.0f;
    unitCrownLobe.stretchSeed[0] = 0.95f;
    unitCrownLobe.stretchSeed[1] = 1.22f;
    unitCrownLobe.stretchSeed[2] = 0.95f;
    unitCrownLobe.stretchSeed[3] = static_cast<float>((seed ^ 0xCA40U) & 0xFFFFU);
    const IsosurfaceMesh canopyMesh = generateMossyBoulderMesh(unitCrownLobe, -2.0f, 12U, 18U);
    hostArchetypes_[static_cast<std::size_t>(ForestMeshArchetype::CanopyFoliageCluster)] =
        convertIsosurfaceToHostArchetype(
            ForestMeshArchetype::CanopyFoliageCluster,
            canopyMesh,
            mgfx::math::vec4(0.22f, 0.48f, 0.14f, 1.0f));

    // -------------------------------------------------------------------------
    // Archetype 4: BrackenFernFrond (4-frond radial fern rosette at origin)
    // -------------------------------------------------------------------------
    IsosurfaceMesh fernRosette{};
    constexpr std::uint32_t kFrondCount = 4U;
    for (std::uint32_t f = 0U; f < kFrondCount; ++f) {
        const float heading = static_cast<float>(f) * (6.2831853f / static_cast<float>(kFrondCount))
                            + 0.25f * static_cast<float>(f & 1U);
        const IsosurfaceMesh frond = generateFernFrondMesh(
            {0.0f, 0.0f, 0.0f}, heading, 0.85f, {0.0f, 0.0f, 0.0f}, 0.0f, 10U);
        appendIsosurfaceMesh(fernRosette, frond);
    }
    hostArchetypes_[static_cast<std::size_t>(ForestMeshArchetype::BrackenFernFrond)] =
        convertIsosurfaceToHostArchetype(
            ForestMeshArchetype::BrackenFernFrond,
            fernRosette,
            mgfx::math::vec4(0.20f, 0.52f, 0.15f, 1.0f));

    // -------------------------------------------------------------------------
    // Archetype 5: RootedBellFlower (5-petal cupped bell-flower at origin)
    // -------------------------------------------------------------------------
    ForestGroundFlowerGpu unitFlower{};
    unitFlower.rootPosScale[0] = 0.0f;
    unitFlower.rootPosScale[1] = 0.0f;
    unitFlower.rootPosScale[2] = 0.0f;
    unitFlower.rootPosScale[3] = 0.45f;
    unitFlower.headPosRadius[0] = 0.02f;
    unitFlower.headPosRadius[1] = 0.44f;
    unitFlower.headPosRadius[2] = 0.0f;
    unitFlower.headPosRadius[3] = 0.14f;
    unitFlower.headAxisStiffness[0] = 0.08f;
    unitFlower.headAxisStiffness[1] = -0.99f;
    unitFlower.headAxisStiffness[2] = 0.0f;
    unitFlower.headAxisStiffness[3] = 1.5f;
    unitFlower.glowColorPhase[0] = 0.95f;
    unitFlower.glowColorPhase[1] = 0.35f;
    unitFlower.glowColorPhase[2] = 0.85f;
    unitFlower.glowColorPhase[3] = 0.35f;
    const IsosurfaceMesh flowerMesh = generateRootedFlowerMesh(unitFlower, 7U, 5U);
    hostArchetypes_[static_cast<std::size_t>(ForestMeshArchetype::RootedBellFlower)] =
        convertIsosurfaceToHostArchetype(
            ForestMeshArchetype::RootedBellFlower,
            flowerMesh,
            mgfx::math::vec4(0.96f, 0.88f, 0.98f, 1.0f));

    // -------------------------------------------------------------------------
    // Archetype 6: GrassAndUnderstoryClump (Quadratic-Bezier grass + shrub)
    // -------------------------------------------------------------------------
    IsosurfaceMesh grassAndShrub = generateGrassBladeClusterMesh(
        {0.0f, 0.0f, 0.0f}, 0.36f, 12U, {0.15f, 0.0f, 0.05f}, 0.0f, seed ^ 0x47524153U);
    const IsosurfaceMesh shrubMesh = generateUnderstoryShrubMesh(
        {0.0f, 0.0f, 0.0f}, 0.65f, 4U, seed ^ 0x53485242U);
    appendIsosurfaceMesh(grassAndShrub, shrubMesh);
    hostArchetypes_[static_cast<std::size_t>(ForestMeshArchetype::GrassAndUnderstoryClump)] =
        convertIsosurfaceToHostArchetype(
            ForestMeshArchetype::GrassAndUnderstoryClump,
            grassAndShrub,
            mgfx::math::vec4(0.24f, 0.50f, 0.16f, 1.0f));

    // -------------------------------------------------------------------------
    // Archetype 7: SwirlingPetalAndDebris (Curved petal blade & floor litter)
    // -------------------------------------------------------------------------
    const IsosurfaceMesh debrisMesh = generateForestDebrisClusterMesh(
        {0.0f, 0.0f, 0.0f}, 0.25f, 6U, seed ^ 0x44454252U);
    hostArchetypes_[static_cast<std::size_t>(ForestMeshArchetype::SwirlingPetalAndDebris)] =
        convertIsosurfaceToHostArchetype(
            ForestMeshArchetype::SwirlingPetalAndDebris,
            debrisMesh,
            mgfx::math::vec4(0.92f, 0.54f, 0.82f, 1.0f));

    builtTerrainGridWidth_ = fieldGrid.config.gridWidth;
    builtTerrainGridHeight_ = fieldGrid.config.gridHeight;
    builtSeed_ = seed;
    hostArchetypesBuilt_ = true;
}

void ProceduralForestLichtungRenderBridge::rebuild(
    std::uint64_t generation,
    const ForestFieldGrid& fieldGrid,
    const ForestEcosystemPopulation& population,
    const ForestCameraFrustum& frustum,
    const ForestSeasonWeatherConfig& seasonWeather,
    std::span<const ForestSwirlingPetalGpu> swirlingPetals,
    float timeSeconds) {
    const auto startClock = std::chrono::steady_clock::now();

    buildHostArchetypeGeometries(fieldGrid, frustum, 0x4C494348U);

    // Bucket instances per archetype before flattening into a single prefix-summed
    // contiguous arena of <= kMaxInstanceCapacity records.
    std::array<std::vector<mgfx::vk::InstanceData>, kArchetypeCount> buckets{};
    buckets[0].reserve(1U);
    buckets[1].reserve(population.boulders.size());
    buckets[2].reserve(population.trees.size());
    buckets[3].reserve(population.trees.size());
    buckets[4].reserve(population.ferns.size());
    buckets[5].reserve(population.flowers.size());
    buckets[6].reserve(population.grasses.size() + population.shrubs.size());
    buckets[7].reserve(swirlingPetals.size() + population.debris.size());

    std::uint32_t sourceCount = 1U; // Terrain Valley Floor root instance
    std::uint32_t culledCount = 0U;

    // 0. TerrainValleyFloor: 1 identity world instance
    if (!hostArchetypes_[0].empty()) {
        mgfx::vk::InstanceData terrainInst{};
        terrainInst.model = mgfx::math::mat4(1.0f);
        const float wetnessDarken = std::lerp(1.0f, 0.72f, std::clamp(seasonWeather.rainWetness, 0.0f, 1.0f));
        terrainInst.baseColorTint = {
            0.88f * wetnessDarken,
            0.94f * wetnessDarken,
            0.82f * wetnessDarken,
            1.0f
        };
        terrainInst.pbrOverride = {
            0.0f,
            std::lerp(0.88f, 0.45f, std::clamp(seasonWeather.rainWetness, 0.0f, 1.0f)),
            0.0f,
            0.0f
        };
        terrainInst.velocity = {0.0f, 0.0f, 0.0f, 0.0f};
        buckets[0].push_back(terrainInst);
    }

    auto tryAdmitLod = [&](const SculptureVec3& pos, float radius, std::uint32_t seed, float& outWeight) -> bool {
        if (!isFiniteVec3(pos)) {
            ++culledCount;
            return false;
        }
        const ForestLodTier tier = classifyForestInstanceLod(pos, radius, frustum, seed, &outWeight);
        if (tier == ForestLodTier::Culled || outWeight <= 0.02f) {
            ++culledCount;
            return false;
        }
        return true;
    };

    // 1. MossyBoulder
    sourceCount += static_cast<std::uint32_t>(population.boulders.size());
    for (std::size_t i = 0U; i < population.boulders.size(); ++i) {
        const ForestScatterInstanceGpu& b = population.boulders[i];
        const SculptureVec3 pos{b.posX, b.posY, b.posZ};
        const float radius = std::max(b.scale, 0.25f);
        float weight = 1.0f;
        if (!tryAdmitLod(pos, radius * 1.5f, b.seed, weight)) {
            continue;
        }
        const float yaw = hashSeedToAngle(b.seed);
        const float tilt = (hashSeedToUnit(b.seed ^ 0x1357U) - 0.5f) * 0.35f;
        mgfx::vk::InstanceData inst{};
        inst.model = composeTrsMatrix(
            {pos.x, pos.y, pos.z},
            yaw,
            {std::cos(yaw), 0.0f, std::sin(yaw)},
            tilt,
            {radius * 1.15f, radius * 0.85f, radius * 1.05f});
        const float wet = std::clamp(seasonWeather.rainWetness, 0.0f, 1.0f);
        inst.baseColorTint = {
            std::lerp(0.56f, 0.38f, wet),
            std::lerp(0.62f, 0.46f, wet),
            std::lerp(0.52f, 0.36f, wet),
            weight
        };
        inst.pbrOverride = {0.02f, std::lerp(0.84f, 0.38f, wet), 0.0f, 0.0f};
        inst.velocity = {0.0f, 0.0f, 0.0f, 0.0f};
        buckets[1].push_back(inst);
    }

    // 2. TreeTrunkAndBranches & 3. CanopyFoliageCluster
    sourceCount += static_cast<std::uint32_t>(population.trees.size() * 2U);
    for (std::size_t i = 0U; i < population.trees.size(); ++i) {
        const ForestScatterInstanceGpu& t = population.trees[i];
        const SculptureVec3 pos{t.posX, t.posY, t.posZ};
        // Scatter scale is a unitless ecological variation, not tree height.
        // Consume the same metre-valued primitives used by canopy feedback.
        const float variation = std::max(t.scale, 0.1f);
        const bool hasPrimitive = i < population.treePrimitives.size();
        const auto* primitive = hasPrimitive ? &population.treePrimitives[i] : nullptr;
        const float trunkHeight = primitive
            ? std::max(primitive->canopyCenterRadius[1] - primitive->baseRadius[1], 0.5f)
            : 6.2f * variation;
        const float trunkRadius = primitive ? primitive->baseRadius[3] : 0.26f * variation;
        const float crownRadius = primitive ? primitive->canopyCenterRadius[3] : 3.4f * variation;
        float weight = 1.0f;
        if (!tryAdmitLod(pos, trunkHeight + crownRadius, t.seed + static_cast<std::uint32_t>(i + 101U), weight)) {
            continue;
        }

        const ForestTreeSpecies species =
            static_cast<ForestTreeSpecies>(t.species % static_cast<std::uint32_t>(ForestTreeSpecies::Count));
        const ForestTreeSpeciesProfile profile = getTreeSpeciesProfile(species);
        SculptureVec3 leanXZ{0.0f, 0.0f, 0.0f};
        if (i < population.treePrimitives.size()) {
            leanXZ = computePhototropicLeanVector(i, population.treePrimitives);
        }
        const float leanMag = std::min(
            std::sqrt(leanXZ.x * leanXZ.x + leanXZ.z * leanXZ.z), 0.35f);
        const mgfx::math::vec3 tiltAxis{-leanXZ.z, 0.0f, leanXZ.x};
        const float yaw = hashSeedToAngle(t.seed);

        // Trunk instance
        mgfx::vk::InstanceData trunkInst{};
        trunkInst.model = composeTrsMatrix(
            {pos.x, pos.y, pos.z},
            yaw,
            tiltAxis,
            leanMag * 0.45f,
            {trunkRadius / 0.06f, trunkHeight, trunkRadius / 0.06f});
        trunkInst.baseColorTint = {
            profile.barkAlbedo.x,
            profile.barkAlbedo.y,
            profile.barkAlbedo.z,
            weight
        };
        trunkInst.pbrOverride = {0.0f, 0.88f, 0.0f, 0.0f};
        trunkInst.velocity = {0.0f, 0.0f, 0.0f, 0.0f};
        buckets[2].push_back(trunkInst);

        // Canopy Foliage Crown instance positioned at crown apex
        const SculptureVec3 crownCenter{
            (primitive ? primitive->canopyCenterRadius[0] : pos.x) + leanXZ.x * trunkHeight * 0.22f,
            primitive ? primitive->canopyCenterRadius[1] : pos.y + trunkHeight,
            (primitive ? primitive->canopyCenterRadius[2] : pos.z) + leanXZ.z * trunkHeight * 0.22f
        };
        const SculptureVec3 seasonalFoliage = evaluateSeasonalFoliageAlbedo(
            profile.foliageAlbedo, seasonWeather.seasonPhase, t.seed ^ static_cast<std::uint32_t>(i * 73U + 19U));
        mgfx::vk::InstanceData canopyInst{};
        canopyInst.model = composeTrsMatrix(
            {crownCenter.x, crownCenter.y, crownCenter.z},
            yaw + 0.5f,
            tiltAxis,
            leanMag * 0.35f,
            {crownRadius, crownRadius * profile.crownVerticalAspect, crownRadius});
        canopyInst.baseColorTint = {
            seasonalFoliage.x,
            seasonalFoliage.y,
            seasonalFoliage.z,
            weight
        };
        // pbrOverride.z stores botanical SSS translucency weight (0.65)
        canopyInst.pbrOverride = {0.0f, 0.72f, 0.65f, 0.04f};
        canopyInst.velocity = {0.0f, 0.0f, 0.0f, 0.0f};
        buckets[3].push_back(canopyInst);
    }

    // 4. BrackenFernFrond
    sourceCount += static_cast<std::uint32_t>(population.ferns.size());
    for (std::size_t i = 0U; i < population.ferns.size(); ++i) {
        const ForestScatterInstanceGpu& f = population.ferns[i];
        const SculptureVec3 pos{f.posX, f.posY, f.posZ};
        const float scale = std::max(f.scale, 0.25f);
        float weight = 1.0f;
        if (!tryAdmitLod(pos, scale * 1.1f, f.seed + static_cast<std::uint32_t>(i + 503U), weight)) {
            continue;
        }
        const float yaw = hashSeedToAngle(f.seed);
        const float tilt = (hashSeedToUnit(f.seed ^ 0x4411U) - 0.5f) * 0.25f;
        const SculptureVec3 fernTint = evaluateSeasonalFoliageAlbedo(
            {0.18f, 0.46f, 0.12f}, seasonWeather.seasonPhase, f.seed);
        mgfx::vk::InstanceData inst{};
        inst.model = composeTrsMatrix(
            {pos.x, pos.y, pos.z},
            yaw,
            {1.0f, 0.0f, 0.0f},
            tilt,
            {scale, scale, scale});
        inst.baseColorTint = {fernTint.x, fernTint.y, fernTint.z, weight};
        inst.pbrOverride = {0.0f, 0.64f, 0.75f, 0.03f};
        inst.velocity = {0.0f, 0.0f, 0.0f, 0.0f};
        buckets[4].push_back(inst);
    }

    // 5. RootedBellFlower (with Euler-Bernoulli stem sway & cosine spectral HDR glow)
    sourceCount += static_cast<std::uint32_t>(population.flowers.size());
    for (std::size_t i = 0U; i < population.flowers.size(); ++i) {
        const ForestScatterInstanceGpu& fl = population.flowers[i];
        const SculptureVec3 pos{fl.posX, fl.posY, fl.posZ};
        const float scale = std::max(fl.scale, 0.18f);
        float weight = 1.0f;
        if (!tryAdmitLod(pos, scale * 1.2f, fl.seed + static_cast<std::uint32_t>(i + 907U), weight)) {
            continue;
        }

        const float hueSeed = (i < population.flowerPrimitives.size())
            ? std::clamp(population.flowerPrimitives[i].glowColorPhase[3], 0.0f, 1.0f)
            : evaluateVoronoiFlowerHueSeed(pos.x, pos.z, fl.seed);
        const float phase = hueSeed + 0.06f * std::sin(timeSeconds * 1.3f + pos.x * 0.35f + pos.z * 0.35f);
        const SculptureVec3 spectralColor = evaluateSpectralPetalColor(phase, 0.5f, 0.0f);
        const float yaw = hashSeedToAngle(fl.seed);
        const float tilt = (hashSeedToUnit(fl.seed ^ 0x7722U) - 0.5f) * 0.2f;

        mgfx::vk::InstanceData inst{};
        inst.model = composeTrsMatrix(
            {pos.x, pos.y, pos.z},
            yaw,
            {std::sin(yaw), 0.0f, std::cos(yaw)},
            tilt,
            {scale, scale, scale});
        inst.baseColorTint = {
            spectralColor.x,
            spectralColor.y,
            spectralColor.z,
            weight
        };
        // pbrOverride.w = HDR additive emission for thresholded petal bloom
        inst.pbrOverride = {0.0f, 0.38f, hueSeed, 1.85f * weight};
        inst.velocity = {0.0f, 0.0f, 0.0f, 0.0f};
        buckets[5].push_back(inst);
    }

    // 6. GrassAndUnderstoryClump (Grass clumps + understory shrubs)
    sourceCount += static_cast<std::uint32_t>(population.grasses.size() + population.shrubs.size());
    for (std::size_t i = 0U; i < population.grasses.size(); ++i) {
        const ForestScatterInstanceGpu& g = population.grasses[i];
        const SculptureVec3 pos{g.posX, g.posY, g.posZ};
        const float scale = std::max(g.scale, 0.15f);
        float weight = 1.0f;
        if (!tryAdmitLod(pos, scale, g.seed + static_cast<std::uint32_t>(i + 2003U), weight)) {
            continue;
        }
        const float yaw = hashSeedToAngle(g.seed);
        const float tilt = (hashSeedToUnit(g.seed ^ 0x8833U) - 0.5f) * 0.18f;
        const SculptureVec3 grassTint = evaluateSeasonalFoliageAlbedo(
            {0.22f, 0.48f, 0.14f}, seasonWeather.seasonPhase, g.seed);
        mgfx::vk::InstanceData inst{};
        inst.model = composeTrsMatrix(
            {pos.x, pos.y, pos.z},
            yaw,
            {0.0f, 0.0f, 1.0f},
            tilt,
            {scale, scale, scale});
        inst.baseColorTint = {grassTint.x, grassTint.y, grassTint.z, weight};
        inst.pbrOverride = {0.0f, 0.68f, 0.55f, 0.0f};
        inst.velocity = {0.0f, 0.0f, 0.0f, 0.0f};
        buckets[6].push_back(inst);
    }
    for (std::size_t i = 0U; i < population.shrubs.size(); ++i) {
        const ForestScatterInstanceGpu& s = population.shrubs[i];
        const SculptureVec3 pos{s.posX, s.posY, s.posZ};
        const float scale = std::max(s.scale, 0.35f);
        float weight = 1.0f;
        if (!tryAdmitLod(pos, scale * 1.2f, s.seed + static_cast<std::uint32_t>(i + 3001U), weight)) {
            continue;
        }
        const float yaw = hashSeedToAngle(s.seed);
        const float tilt = (hashSeedToUnit(s.seed ^ 0x9944U) - 0.5f) * 0.2f;
        const SculptureVec3 shrubTint = evaluateSeasonalFoliageAlbedo(
            {0.17f, 0.40f, 0.11f}, seasonWeather.seasonPhase, s.seed);
        mgfx::vk::InstanceData inst{};
        inst.model = composeTrsMatrix(
            {pos.x, pos.y, pos.z},
            yaw,
            {1.0f, 0.0f, 0.0f},
            tilt,
            {scale, scale * 1.1f, scale});
        inst.baseColorTint = {shrubTint.x, shrubTint.y, shrubTint.z, weight};
        inst.pbrOverride = {0.0f, 0.74f, 0.45f, 0.0f};
        inst.velocity = {0.0f, 0.0f, 0.0f, 0.0f};
        buckets[6].push_back(inst);
    }

    // 7. SwirlingPetalAndDebris (Swirling airborne petals + forest floor litter)
    sourceCount += static_cast<std::uint32_t>(swirlingPetals.size() + population.debris.size());
    for (std::size_t i = 0U; i < swirlingPetals.size(); ++i) {
        const ForestSwirlingPetalGpu& petal = swirlingPetals[i];
        const SculptureVec3 pos{
            petal.positionAge[0],
            petal.positionAge[1],
            petal.positionAge[2]
        };
        const float age = std::clamp(petal.positionAge[3], 0.0f, 1.0f);
        float weight = 1.0f;
        if (!tryAdmitLod(pos, 0.25f, static_cast<std::uint32_t>(i + 5003U), weight)) {
            continue;
        }
        const float ageFade = std::max(0.0f, 1.0f - age * age * age * age);
        const float yaw = std::atan2(petal.velocityDrag[0], petal.velocityDrag[2])
                        + timeSeconds * 2.2f;
        mgfx::vk::InstanceData inst{};
        inst.model = composeTrsMatrix(
            {pos.x, pos.y, pos.z},
            yaw,
            {petal.velocityDrag[2], 0.25f, -petal.velocityDrag[0]},
            0.65f,
            {0.22f, 0.14f, 0.22f});
        inst.baseColorTint = {
            std::clamp(petal.baseColorSeed[0], 0.0f, 2.0f),
            std::clamp(petal.baseColorSeed[1], 0.0f, 2.0f),
            std::clamp(petal.baseColorSeed[2], 0.0f, 2.0f),
            weight * ageFade
        };
        inst.pbrOverride = {0.0f, 0.30f, petal.baseColorSeed[3], 2.25f * ageFade};
        inst.velocity = {
            petal.velocityDrag[0],
            petal.velocityDrag[1],
            petal.velocityDrag[2],
            0.0f
        };
        buckets[7].push_back(inst);
    }
    for (std::size_t i = 0U; i < population.debris.size(); ++i) {
        const ForestScatterInstanceGpu& d = population.debris[i];
        const SculptureVec3 pos{d.posX, d.posY, d.posZ};
        const float scale = std::max(d.scale, 0.15f);
        float weight = 1.0f;
        if (!tryAdmitLod(pos, scale, d.seed + static_cast<std::uint32_t>(i + 7001U), weight)) {
            continue;
        }
        const float yaw = hashSeedToAngle(d.seed);
        const float tilt = (hashSeedToUnit(d.seed ^ 0xAA55U) - 0.5f) * 0.25f;
        mgfx::vk::InstanceData inst{};
        inst.model = composeTrsMatrix(
            {pos.x, pos.y, pos.z},
            yaw,
            {0.0f, 0.0f, 1.0f},
            tilt,
            {scale, scale * 0.65f, scale});
        inst.baseColorTint = {0.34f, 0.25f, 0.16f, weight};
        inst.pbrOverride = {0.0f, 0.86f, 0.0f, 0.0f};
        inst.velocity = {0.0f, 0.0f, 0.0f, 0.0f};
        buckets[7].push_back(inst);
    }

    // Flatten buckets into a single prefix-summed contiguous buffer capped at
    // kMaxInstanceCapacity (16,384 instances = 1.75 MB/slot).
    instances_.clear();
    instances_.reserve(std::min<std::size_t>(sourceCount, kMaxInstanceCapacity));

    ForestLichtungBridgeMetrics nextMetrics{};
    nextMetrics.generation = generation;
    nextMetrics.sourceInstanceCount = sourceCount;
    nextMetrics.culledOrDitheredCount = culledCount;

    for (std::size_t k = 0U; k < kArchetypeCount; ++k) {
        const auto archetype = static_cast<ForestMeshArchetype>(k);
        const std::uint32_t firstInst = static_cast<std::uint32_t>(instances_.size());
        const std::size_t remainingBudget =
            (instances_.size() < kMaxInstanceCapacity)
                ? (kMaxInstanceCapacity - instances_.size())
                : 0U;
        const std::size_t takeCount = std::min(buckets[k].size(), remainingBudget);
        if (takeCount < buckets[k].size()) {
            nextMetrics.culledOrDitheredCount +=
                static_cast<std::uint32_t>(buckets[k].size() - takeCount);
        }

        instances_.insert(
            instances_.end(),
            buckets[k].begin(),
            buckets[k].begin() + static_cast<std::ptrdiff_t>(takeCount));

        const bool windAnimated =
            (archetype == ForestMeshArchetype::CanopyFoliageCluster
             || archetype == ForestMeshArchetype::BrackenFernFrond
             || archetype == ForestMeshArchetype::RootedBellFlower
             || archetype == ForestMeshArchetype::GrassAndUnderstoryClump);
        const bool doubleSided =
            (archetype == ForestMeshArchetype::TerrainValleyFloor
             || archetype == ForestMeshArchetype::CanopyFoliageCluster
             || archetype == ForestMeshArchetype::BrackenFernFrond
             || archetype == ForestMeshArchetype::RootedBellFlower
             || archetype == ForestMeshArchetype::GrassAndUnderstoryClump
             || archetype == ForestMeshArchetype::SwirlingPetalAndDebris);

        archetypeSlices_[k] = ForestArchetypeDrawSlice{
            .archetype = archetype,
            .firstInstance = firstInst,
            .instanceCount = static_cast<std::uint32_t>(takeCount),
            .featureFlags = windAnimated ? kFeatureFlagWindSway : 0U,
            .doubleSided = doubleSided,
        };

        nextMetrics.perArchetypeInstanceCounts[k] = static_cast<std::uint32_t>(takeCount);
        if (takeCount > 0U && !hostArchetypes_[k].empty()) {
            ++nextMetrics.activeArchetypeBatchCount;
        }

        nextMetrics.totalArchetypeVertexCount +=
            static_cast<std::uint32_t>(hostArchetypes_[k].vertices.size());
        nextMetrics.totalArchetypeIndexCount +=
            static_cast<std::uint32_t>(hostArchetypes_[k].indices.size());
        nextMetrics.archetypeGeometryBytes += hostArchetypes_[k].byteFootprint();
    }

    nextMetrics.acceptedInstanceCount = static_cast<std::uint32_t>(instances_.size());
    nextMetrics.singleSlotInstanceArenaBytes =
        static_cast<std::uint64_t>(kMaxInstanceCapacity) * sizeof(mgfx::vk::InstanceData);
    nextMetrics.tripleSlotTotalGpuBytes =
        nextMetrics.singleSlotInstanceArenaBytes * kFrameSlotCount
        + nextMetrics.archetypeGeometryBytes;

    nextMetrics.vramSavingsRatioVsSparseWorldBridge =
        1.0f - static_cast<float>(nextMetrics.tripleSlotTotalGpuBytes)
             / static_cast<float>(kSparseWorldBridgeTripleSlotBytes);

    nextMetrics.rebuildMilliseconds = std::chrono::duration<float, std::milli>(
        std::chrono::steady_clock::now() - startClock).count();

    metrics_ = nextMetrics;
}

bool ProceduralForestLichtungRenderBridge::ensureGpuArchetypeMeshes(
    mgfx::Engine& engine) {
    if (!hostArchetypesBuilt_) {
        return false;
    }
    if (boundEngine_ == &engine && gpuMeshes_[0] != nullptr) {
        return true;
    }

    std::array<std::shared_ptr<mgfx::vk::Mesh>, kArchetypeCount> candidateMeshes{};
    for (std::size_t k = 0U; k < kArchetypeCount; ++k) {
        const ForestHostArchetypeGeometry& hostGeo = hostArchetypes_[k];
        if (hostGeo.empty()) {
            continue;
        }
        candidateMeshes[k] = std::make_shared<mgfx::vk::Mesh>(
            engine.context(),
            hostGeo.vertices,
            hostGeo.indices);
    }

    gpuMeshes_ = std::move(candidateMeshes);
    boundEngine_ = &engine;
    defaultMaterialsCreated_ = false;
    return true;
}

bool ProceduralForestLichtungRenderBridge::ensureDefaultMaterials(
    mgfx::Engine& engine) {
    // Species/season colors live in vertex and instance observations. A neutral
    // material factor avoids multiplying the same dark albedo a third time.
    if (defaultMaterialsCreated_ && boundEngine_ == &engine) {
        return true;
    }
    auto* matLib = engine.materials();
    if (matLib == nullptr) {
        return false;
    }

    // 0: TerrainValleyFloor (Mossy humus & damp soil — low F0 prevents grazing Fresnel sky-glare)
    {
        mgfx::pbr::Material m{};
        m.baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
        m.metallic = 0.0f;
        m.roughness = 0.88f;
        m.specularWeight = 0.18f;
        m.sheenColorFactor = {0.14f, 0.24f, 0.09f};
        m.sheenRoughnessFactor = 0.72f;
        m.doubleSided = true;
        defaultMaterials_[0] = matLib->createMaterial(m);
    }
    // 1: MossyBoulder (Weathered granite with velvety moss sheen & damp clearcoat)
    {
        mgfx::pbr::Material m{};
        m.baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
        m.metallic = 0.02f;
        m.roughness = 0.80f;
        m.specularWeight = 0.22f;
        m.sheenColorFactor = {0.28f, 0.48f, 0.20f};
        m.sheenRoughnessFactor = 0.65f;
        m.clearcoatFactor = 0.22f;
        m.clearcoatRoughnessFactor = 0.28f;
        defaultMaterials_[1] = matLib->createMaterial(m);
    }
    // 2: TreeTrunkAndBranches (Furrowed deciduous/conifer bark)
    {
        mgfx::pbr::Material m{};
        m.baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
        m.metallic = 0.0f;
        m.roughness = 0.90f;
        m.specularWeight = 0.16f;
        defaultMaterials_[2] = matLib->createMaterial(m);
    }
    // 3: CanopyFoliageCluster (Subsurface-scattering sunlit leaves with warm chlorophyll sheen)
    {
        mgfx::pbr::Material m{};
        m.baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
        m.metallic = 0.0f;
        m.roughness = 0.68f;
        m.specularWeight = 0.20f;
        m.sheenColorFactor = {0.26f, 0.44f, 0.14f};
        m.sheenRoughnessFactor = 0.58f;
        m.doubleSided = true;
        defaultMaterials_[3] = matLib->createMaterial(m);
    }
    // 4: BrackenFernFrond (Waxy arched woodland fern with subtle cuticle clearcoat)
    {
        mgfx::pbr::Material m{};
        m.baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
        m.metallic = 0.0f;
        m.roughness = 0.56f;
        m.specularWeight = 0.22f;
        m.sheenColorFactor = {0.22f, 0.40f, 0.12f};
        m.sheenRoughnessFactor = 0.52f;
        m.clearcoatFactor = 0.18f;
        m.clearcoatRoughnessFactor = 0.32f;
        m.doubleSided = true;
        defaultMaterials_[4] = matLib->createMaterial(m);
    }
    // 5: RootedBellFlower (Luminous cosine-spectral bell-flower petals)
    {
        mgfx::pbr::Material m{};
        m.baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
        m.metallic = 0.0f;
        m.roughness = 0.34f;
        m.specularWeight = 0.24f;
        m.emissiveFactor = {0.85f, 0.32f, 0.78f};
        m.doubleSided = true;
        defaultMaterials_[5] = matLib->createMaterial(m);
    }
    // 6: GrassAndUnderstoryClump (Meadow grass & woody shrubs)
    {
        mgfx::pbr::Material m{};
        m.baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
        m.metallic = 0.0f;
        m.roughness = 0.66f;
        m.specularWeight = 0.18f;
        m.sheenColorFactor = {0.20f, 0.36f, 0.12f};
        m.sheenRoughnessFactor = 0.62f;
        m.doubleSided = true;
        defaultMaterials_[6] = matLib->createMaterial(m);
    }
    // 7: SwirlingPetalAndDebris (Swirling luminous petals & floor litter)
    {
        mgfx::pbr::Material m{};
        m.baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f};
        m.metallic = 0.0f;
        m.roughness = 0.38f;
        m.specularWeight = 0.22f;
        m.emissiveFactor = {0.75f, 0.28f, 0.68f};
        m.doubleSided = true;
        defaultMaterials_[7] = matLib->createMaterial(m);
    }

    defaultMaterialsCreated_ = true;
    return true;
}

bool ProceduralForestLichtungRenderBridge::draw(
    mgfx::Engine& engine,
    mgfx::Drawable& drawable,
    std::span<const mgfx::MaterialHandle> customMaterials,
    const mgfx::math::mat4& rootTransform) {
    if (metrics_.generation == 0U || instances_.empty()) {
        return false;
    }
    if (!ensureGpuArchetypeMeshes(engine) || !ensureDefaultMaterials(engine)) {
        return false;
    }

    const VkDeviceSize capacityBytes =
        static_cast<VkDeviceSize>(kMaxInstanceCapacity) * sizeof(mgfx::vk::InstanceData);
    const VkDeviceSize activeBytes =
        static_cast<VkDeviceSize>(instances_.size()) * sizeof(mgfx::vk::InstanceData);

    if (!instanceCache_.ensure(
            engine,
            capacityBytes,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            "ForestLichtungInstances")) {
        return false;
    }

    const auto byteSpan = std::as_bytes(std::span<const mgfx::vk::InstanceData>(instances_));
    if (!instanceCache_.uploadCurrentGeneration(
            engine,
            metrics_.generation,
            byteSpan,
            VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
            VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT)) {
        return false;
    }

    const mgfx::RenderBufferView instanceView = instanceCache_.currentView(
        engine,
        metrics_.generation,
        mgfx::RenderBufferRole::Instances,
        activeBytes);

    // Prepare all active archetype streams before emitting draws
    for (std::size_t k = 0U; k < kArchetypeCount; ++k) {
        const ForestArchetypeDrawSlice& slice = archetypeSlices_[k];
        if (slice.instanceCount == 0U || gpuMeshes_[k] == nullptr) {
            continue;
        }
        if (!drawable.prepareStream(instanceView, gpuMeshes_[k])) {
            return false;
        }
    }

    // Emit one instanced draw per non-empty botanical/geological archetype slice
    for (std::size_t k = 0U; k < kArchetypeCount; ++k) {
        const ForestArchetypeDrawSlice& slice = archetypeSlices_[k];
        if (slice.instanceCount == 0U || gpuMeshes_[k] == nullptr) {
            continue;
        }
        const mgfx::MaterialHandle mat =
            (!customMaterials.empty())
                ? customMaterials[k % customMaterials.size()]
                : defaultMaterials_[k];

        if (!drawable.tryDrawInstanceStream(
                gpuMeshes_[k],
                mat,
                instanceView,
                slice.firstInstance,
                slice.instanceCount,
                rootTransform,
                mgfx::math::vec4(1.0f),
                mgfx::pbr::RenderMode::Full,
                slice.featureFlags,
                slice.doubleSided)) {
            return false;
        }
    }

    return true;
}

void ProceduralForestLichtungRenderBridge::reset() noexcept {
    hostArchetypesBuilt_ = false;
    builtTerrainGridWidth_ = 0U;
    builtTerrainGridHeight_ = 0U;
    builtSeed_ = 0U;
    for (auto& arch : hostArchetypes_) {
        arch = {};
    }
    archetypeSlices_.fill({});
    instances_.clear();
    metrics_ = {};
    boundEngine_ = nullptr;
    gpuMeshes_.fill(nullptr);
    defaultMaterials_.fill({});
    defaultMaterialsCreated_ = false;
    instanceCache_.clear();
}

} // namespace epi_sculpt::forest
