#include "ProceduralForestSpatialLodAndBridge.h"
#include "SculptureDifferentialGeometry.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace epi_sculpt::forest {

namespace {

template <typename ItemSpan, typename ExtractXZFn>
void buildCsrGridLayer(
    ItemSpan items,
    ExtractXZFn extractXZ,
    float originX,
    float originZ,
    float cellSize,
    std::uint32_t gridDim,
    std::vector<std::uint32_t>& outOffsets,
    std::vector<std::uint32_t>& outIndices)
{
    const std::size_t numCells = static_cast<std::size_t>(gridDim) * static_cast<std::size_t>(gridDim);
    outOffsets.assign(numCells + 1U, 0U);
    outIndices.clear();
    if (items.empty() || gridDim == 0U) {
        return;
    }

    const float invCell = 1.0f / std::max(cellSize, 1e-4f);
    std::vector<std::uint32_t> cellAssignment(items.size(), 0U);

    for (std::size_t i = 0U; i < items.size(); ++i) {
        float x = 0.0f;
        float z = 0.0f;
        extractXZ(items[i], x, z);
        const int cx = std::clamp(static_cast<int>(std::floor((x - originX) * invCell)), 0, static_cast<int>(gridDim) - 1);
        const int cz = std::clamp(static_cast<int>(std::floor((z - originZ) * invCell)), 0, static_cast<int>(gridDim) - 1);
        const std::uint32_t cellIdx = static_cast<std::uint32_t>(cz) * gridDim + static_cast<std::uint32_t>(cx);
        cellAssignment[i] = cellIdx;
        outOffsets[cellIdx + 1U] += 1U;
    }

    for (std::size_t c = 0U; c < numCells; ++c) {
        outOffsets[c + 1U] += outOffsets[c];
    }

    outIndices.resize(items.size(), 0U);
    std::vector<std::uint32_t> writeHeads(outOffsets.begin(), outOffsets.end() - 1);
    for (std::size_t i = 0U; i < items.size(); ++i) {
        const std::uint32_t cellIdx = cellAssignment[i];
        outIndices[writeHeads[cellIdx]++] = static_cast<std::uint32_t>(i);
    }
}

std::array<float, 4> axisAngleToQuaternion(float ax, float ay, float az, float angleRad) noexcept {
    const float len = std::sqrt(ax * ax + ay * ay + az * az);
    if (len < 1e-6f) {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }
    const float half = 0.5f * angleRad;
    const float s = std::sin(half) / len;
    return {ax * s, ay * s, az * s, std::cos(half)};
}

std::array<float, 4> alignUpVectorToDirection(const SculptureVec3& dir) noexcept {
    const SculptureVec3 u = diff_geom::normalize3(dir, {0.0f, 1.0f, 0.0f});
    const float dotUp = std::clamp(u.y, -1.0f, 1.0f);
    if (dotUp > 0.9995f) {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }
    if (dotUp < -0.9995f) {
        return {1.0f, 0.0f, 0.0f, 0.0f};
    }
    // Cross({0, 1, 0}, u) = {u.z, 0, -u.x}
    const float angle = std::acos(dotUp);
    return axisAngleToQuaternion(u.z, 0.0f, -u.x, angle);
}

} // namespace

ForestSpatialHashGrid buildForestSpatialHashGrid(
    std::span<const ForestTreeGpu> trees,
    std::span<const ForestMossyBoulderGpu> boulders,
    std::span<const ForestGroundFlowerGpu> flowers,
    float originX,
    float originZ,
    float extentX,
    float extentZ,
    std::uint32_t gridDim)
{
    ForestSpatialHashGrid grid{};
    const std::uint32_t dim = std::clamp(gridDim, 2U, 128U);
    grid.gridWidth = dim;
    grid.gridHeight = dim;
    grid.originX = originX;
    grid.originZ = originZ;
    grid.cellSize = std::max(extentX, extentZ) / static_cast<float>(dim);

    buildCsrGridLayer(
        trees,
        [](const ForestTreeGpu& t, float& x, float& z) noexcept {
            x = t.baseRadius[0];
            z = t.baseRadius[2];
        },
        originX, originZ, grid.cellSize, dim,
        grid.treeCellOffsets, grid.treeIndices);

    buildCsrGridLayer(
        boulders,
        [](const ForestMossyBoulderGpu& b, float& x, float& z) noexcept {
            x = b.centerRadius[0];
            z = b.centerRadius[2];
        },
        originX, originZ, grid.cellSize, dim,
        grid.boulderCellOffsets, grid.boulderIndices);

    buildCsrGridLayer(
        flowers,
        [](const ForestGroundFlowerGpu& f, float& x, float& z) noexcept {
            x = f.headPosRadius[0];
            z = f.headPosRadius[2];
        },
        originX, originZ, grid.cellSize, dim,
        grid.flowerCellOffsets, grid.flowerIndices);

    return grid;
}

float computeStochasticLodCrossfade(
    float distance,
    float thresholdDist,
    float bandWidth,
    std::uint32_t instanceSeed) noexcept
{
    const float halfBand = std::max(0.5f * bandWidth, 0.05f);
    const float t = smoothstep01(thresholdDist - halfBand, thresholdDist + halfBand, distance);
    const float dither = random01(hash32(instanceSeed ^ 0x4C4F4431U));
    // Smoothly weighted stochastic crossfade
    return std::clamp(1.0f - 0.7f * t - 0.3f * (t > dither ? 1.0f : 0.0f), 0.0f, 1.0f);
}

ForestLodTier classifyForestInstanceLod(
    const SculptureVec3& worldPos,
    float boundingRadius,
    const ForestCameraFrustum& frustum,
    std::uint32_t instanceSeed,
    float* outCrossfadeWeight) noexcept
{
    const SculptureVec3 toItem = {
        worldPos.x - frustum.position.x,
        worldPos.y - frustum.position.y,
        worldPos.z - frustum.position.z
    };
    const float dist = diff_geom::length3(toItem);
    const float r = std::max(boundingRadius, 0.05f);

    if (dist - r > frustum.lod2MaxDist) {
        if (outCrossfadeWeight != nullptr) {
            *outCrossfadeWeight = 0.0f;
        }
        return ForestLodTier::Culled;
    }

    // View-cone frustum test (only cull items outside near guard sphere)
    if (dist > r + 1.5f) {
        const SculptureVec3 fwd = diff_geom::normalize3(frustum.forward);
        const float proj = diff_geom::dot3(toItem, fwd);
        const float coneSin = std::sqrt(std::max(0.0f, 1.0f - frustum.cosHalfFov * frustum.cosHalfFov));
        if (proj + r < frustum.nearPlane || (proj / std::max(dist, 1e-4f)) + (r / dist) + coneSin * 0.15f < frustum.cosHalfFov) {
            if (outCrossfadeWeight != nullptr) {
                *outCrossfadeWeight = 0.0f;
            }
            return ForestLodTier::Culled;
        }
    }

    if (dist <= frustum.lod0MaxDist) {
        if (outCrossfadeWeight != nullptr) {
            *outCrossfadeWeight = computeStochasticLodCrossfade(
                dist, frustum.lod0MaxDist, frustum.transitionBandWidth, instanceSeed);
        }
        return ForestLodTier::LOD0_HeroDetail;
    }
    if (dist <= frustum.lod1MaxDist) {
        if (outCrossfadeWeight != nullptr) {
            *outCrossfadeWeight = computeStochasticLodCrossfade(
                dist, frustum.lod1MaxDist, frustum.transitionBandWidth, instanceSeed ^ 0x11U);
        }
        return ForestLodTier::LOD1_MidGround;
    }

    if (outCrossfadeWeight != nullptr) {
        *outCrossfadeWeight = computeStochasticLodCrossfade(
            dist, frustum.lod2MaxDist, frustum.transitionBandWidth, instanceSeed ^ 0x22U);
    }
    return ForestLodTier::LOD2_FarProxy;
}

std::vector<ForestTerrainChunkMesh> generateChunkedLichtungTerrainMeshes(
    const ForestFieldGrid& grid,
    const ForestCameraFrustum& frustum,
    std::uint32_t chunksPerAxis,
    float skirtDepth)
{
    std::vector<ForestTerrainChunkMesh> chunks;
    const std::uint32_t w = grid.config.gridWidth;
    const std::uint32_t h = grid.config.gridHeight;
    if (grid.empty() || w < 4U || h < 4U) {
        return chunks;
    }

    const std::uint32_t cAxis = std::clamp(chunksPerAxis, 1U, 16U);
    chunks.reserve(static_cast<std::size_t>(cAxis) * cAxis);

    const float domainW = static_cast<float>(w) * grid.config.cellSize;
    const float domainH = static_cast<float>(h) * grid.config.cellSize;
    const float chunkWorldW = domainW / static_cast<float>(cAxis);
    const float chunkWorldH = domainH / static_cast<float>(cAxis);
    const float chunkBoundR = 0.75f * std::sqrt(chunkWorldW * chunkWorldW + chunkWorldH * chunkWorldH);

    for (std::uint32_t cz = 0U; cz < cAxis; ++cz) {
        for (std::uint32_t cx = 0U; cx < cAxis; ++cx) {
            const float xMin = grid.config.originX + static_cast<float>(cx) * chunkWorldW;
            const float zMin = grid.config.originZ + static_cast<float>(cz) * chunkWorldH;
            const float centerX = xMin + 0.5f * chunkWorldW;
            const float centerZ = zMin + 0.5f * chunkWorldH;
            const ForestWorldCellGpu centerCell = sampleWorldCell(grid, centerX, centerZ);
            const SculptureVec3 chunkCenter = {centerX, centerCell.height, centerZ};

            const ForestLodTier lod = classifyForestInstanceLod(
                chunkCenter, chunkBoundR, frustum, (cz * 31U + cx) * 0x9E3779B9U);
            if (lod == ForestLodTier::Culled) {
                continue;
            }

            const std::uint32_t baseSubdiv = std::max(4U, (w / cAxis));
            const std::uint32_t stepDiv = (lod == ForestLodTier::LOD0_HeroDetail) ? 1U
                                        : (lod == ForestLodTier::LOD1_MidGround)  ? 2U : 4U;
            const std::uint32_t subdiv = std::max(2U, baseSubdiv / stepDiv);
            const std::uint32_t stride = subdiv + 1U;

            ForestTerrainChunkMesh outChunk{};
            outChunk.chunkX = cx;
            outChunk.chunkZ = cz;
            outChunk.lodTier = lod;
            outChunk.chunkCenter = chunkCenter;

            IsosurfaceMesh& mesh = outChunk.mesh;
            mesh.vertices.resize(static_cast<std::size_t>(stride) * stride);

            for (std::uint32_t iz = 0U; iz <= subdiv; ++iz) {
                const float vz = static_cast<float>(iz) / static_cast<float>(subdiv);
                const float wz = zMin + vz * chunkWorldH;
                for (std::uint32_t ix = 0U; ix <= subdiv; ++ix) {
                    const float vx = static_cast<float>(ix) / static_cast<float>(subdiv);
                    const float wx = xMin + vx * chunkWorldW;

                    const ForestWorldCellGpu c = sampleWorldCell(grid, wx, wz);
                    const float ny = std::sqrt(std::max(0.0f, 1.0f - (c.normalX * c.normalX + c.normalZ * c.normalZ)));

                    IsosurfaceVertex v{};
                    v.position = {wx, c.height, wz};
                    v.normal = diff_geom::normalize3({c.normalX, ny, c.normalZ});
                    v.materialCoord = c.pathMask;
                    v.density = c.moisture;
                    mesh.vertices[static_cast<std::size_t>(iz) * stride + ix] = v;
                }
            }

            mesh.indices.reserve(static_cast<std::size_t>(subdiv) * subdiv * 6U + subdiv * 24U);
            for (std::uint32_t iz = 0U; iz < subdiv; ++iz) {
                for (std::uint32_t ix = 0U; ix < subdiv; ++ix) {
                    const std::uint32_t i00 = iz * stride + ix;
                    const std::uint32_t i10 = i00 + 1U;
                    const std::uint32_t i01 = (iz + 1U) * stride + ix;
                    const std::uint32_t i11 = i01 + 1U;

                    mesh.indices.push_back(i00);
                    mesh.indices.push_back(i01);
                    mesh.indices.push_back(i10);

                    mesh.indices.push_back(i10);
                    mesh.indices.push_back(i01);
                    mesh.indices.push_back(i11);
                }
            }

            // Append perimeter skirt quads to eliminate T-junction cracks between LOD transitions
            if (skirtDepth > 1e-3f) {
                auto appendBorderSkirt = [&](std::uint32_t idxA, std::uint32_t idxB) {
                    const std::uint32_t skirtBase = static_cast<std::uint32_t>(mesh.vertices.size());
                    IsosurfaceVertex sA = mesh.vertices[idxA];
                    IsosurfaceVertex sB = mesh.vertices[idxB];
                    sA.position.y -= skirtDepth;
                    sB.position.y -= skirtDepth;
                    mesh.vertices.push_back(sA);
                    mesh.vertices.push_back(sB);

                    mesh.indices.push_back(idxA);
                    mesh.indices.push_back(skirtBase);
                    mesh.indices.push_back(idxB);

                    mesh.indices.push_back(idxB);
                    mesh.indices.push_back(skirtBase);
                    mesh.indices.push_back(skirtBase + 1U);
                };

                for (std::uint32_t i = 0U; i < subdiv; ++i) {
                    appendBorderSkirt(i, i + 1U);                                       // North edge
                    appendBorderSkirt(subdiv * stride + (i + 1U), subdiv * stride + i); // South edge
                    appendBorderSkirt((i + 1U) * stride, i * stride);                   // West edge
                    appendBorderSkirt(i * stride + subdiv, (i + 1U) * stride + subdiv); // East edge
                }
            }

            SculptureIsosurface::computeMeshBounds(mesh);
            chunks.push_back(std::move(outChunk));
        }
    }

    return chunks;
}

SculptureVec3 evaluateSeasonalFoliageAlbedo(
    const SculptureVec3& baseSummerAlbedo,
    float seasonPhase,
    std::uint32_t instanceSeed) noexcept
{
    const float phase = seasonPhase - std::floor(seasonPhase);
    const float treeVar = (random01(hash32(instanceSeed ^ 0x53454153U)) - 0.5f) * 0.08f;
    const float s = std::clamp(phase + treeVar, 0.0f, 1.0f);

    // Keyframes: 0.0=Spring fresh lime, 0.25=Summer emerald, 0.60=Autumn amber/crimson, 0.85=Winter frosted sage
    constexpr SculptureVec3 kSpring = {0.22f, 0.42f, 0.11f};
    const SculptureVec3 summer = baseSummerAlbedo;
    const SculptureVec3 autumn = (instanceSeed & 1U) != 0U
        ? SculptureVec3{0.52f, 0.26f, 0.06f}  // Golden amber
        : SculptureVec3{0.46f, 0.14f, 0.07f}; // Deep russet crimson
    constexpr SculptureVec3 kWinter = {0.24f, 0.28f, 0.25f};

    auto lerp3 = [](const SculptureVec3& a, const SculptureVec3& b, float t) noexcept -> SculptureVec3 {
        const float u = std::clamp(t, 0.0f, 1.0f);
        return {a.x + u * (b.x - a.x), a.y + u * (b.y - a.y), a.z + u * (b.z - a.z)};
    };

    if (s < 0.25f) {
        return lerp3(kSpring, summer, s / 0.25f);
    }
    if (s < 0.60f) {
        return lerp3(summer, autumn, (s - 0.25f) / 0.35f);
    }
    if (s < 0.85f) {
        return lerp3(autumn, kWinter, (s - 0.60f) / 0.25f);
    }
    return lerp3(kWinter, kSpring, (s - 0.85f) / 0.15f);
}

void applySeasonAndWeatherToDescriptor(
    ForestLichtungDescriptorGpu& inOutDescriptor,
    const ForestSeasonWeatherConfig& seasonWeather) noexcept
{
    const float humid = std::clamp(seasonWeather.humidity, 0.0f, 1.0f);
    const float overcast = std::clamp(seasonWeather.overcastFraction, 0.0f, 1.0f);
    const float gust = std::clamp(seasonWeather.windGustMultiplier, 0.0f, 4.0f);

    // Humidity increases valley mist base density and softens height falloff
    inOutDescriptor.mistParams[0] = 0.008f + 0.028f * humid;
    inOutDescriptor.mistParams[1] = 0.16f - 0.07f * humid;

    // Overcast weather tempers direct sun intensity and shifts scattering anisotropy g
    const float sunAtten = 1.0f - 0.55f * overcast;
    inOutDescriptor.sunRadiance[0] = 2.60f * sunAtten;
    inOutDescriptor.sunRadiance[1] = 2.45f * sunAtten;
    inOutDescriptor.sunRadiance[2] = (2.10f + 0.25f * overcast) * sunAtten;
    inOutDescriptor.sunRadiance[3] = 0.78f - 0.18f * overcast;

    inOutDescriptor.windVelocityXZ[0] *= gust;
    inOutDescriptor.windVelocityXZ[1] *= gust;
}

void applyXrInteractionToRootedFlowers(
    std::span<ForestGroundFlowerGpu> inOutFlowers,
    std::span<const ForestXrRepulsor> repulsors) noexcept
{
    if (repulsors.empty()) {
        return;
    }

    for (ForestGroundFlowerGpu& fl : inOutFlowers) {
        const SculptureVec3 root = {fl.rootPosScale[0], fl.rootPosScale[1], fl.rootPosScale[2]};
        const float stemLen = std::max(fl.rootPosScale[3], 0.02f);
        SculptureVec3 head = {fl.headPosRadius[0], fl.headPosRadius[1], fl.headPosRadius[2]};

        float pushX = 0.0f;
        float pushZ = 0.0f;

        for (const ForestXrRepulsor& rep : repulsors) {
            const float radius = std::max(rep.radius, 0.05f);
            const float dx = head.x - rep.position.x;
            const float dy = head.y - rep.position.y;
            const float dz = head.z - rep.position.z;
            const float distSq = dx * dx + dy * dy + dz * dz;
            if (distSq < radius * radius) {
                const float dist = std::sqrt(std::max(distSq, 1e-6f));
                const float falloff = (1.0f - dist / radius);
                const float weight = rep.strength * falloff * falloff;
                const float invHoriz = 1.0f / std::max(std::sqrt(dx * dx + dz * dz), 1e-3f);
                pushX += weight * (dx * invHoriz * 0.45f + rep.velocity.x * 0.15f);
                pushZ += weight * (dz * invHoriz * 0.45f + rep.velocity.z * 0.15f);
            }
        }

        if (std::abs(pushX) > 1e-5f || std::abs(pushZ) > 1e-5f) {
            float offX = (head.x - root.x) + pushX * stemLen;
            float offZ = (head.z - root.z) + pushZ * stemLen;
            const float maxBend = 0.78f * stemLen;
            const float offMag = std::sqrt(offX * offX + offZ * offZ);
            if (offMag > maxBend) {
                offX *= maxBend / offMag;
                offZ *= maxBend / offMag;
            }
            const float dropY = -0.6f * (offX * offX + offZ * offZ) / stemLen;

            // Root stays strictly fixed at fl.rootPosScale; blossom head bends away from XR hand
            fl.headPosRadius[0] = root.x + offX;
            fl.headPosRadius[1] = std::max(root.y + 0.03f, root.y + stemLen + dropY);
            fl.headPosRadius[2] = root.z + offZ;

            const SculptureVec3 newAxis = diff_geom::normalize3(
                {offX * 1.8f, -0.65f, offZ * 1.8f},
                {0.0f, -1.0f, 0.0f});
            fl.headAxisStiffness[0] = newAxis.x;
            fl.headAxisStiffness[1] = newAxis.y;
            fl.headAxisStiffness[2] = newAxis.z;
        }
    }
}

world_gpu_abi::WorldSnapshotGpu packForestEcosystemToWorldSnapshot(
    const ForestEcosystemPopulation& population,
    const ForestCameraFrustum& frustum,
    const ForestSeasonWeatherConfig& seasonWeather,
    std::span<const ForestSwirlingPetalGpu> swirlingPetals,
    float time,
    std::uint32_t generation)
{
    using namespace world_gpu_abi;
    WorldSnapshotGpu snapshot{};
    snapshot.frame.abiVersion = kVersion;
    snapshot.frame.byteSize = kFrameBytes;
    snapshot.frame.generation = generation;
    snapshot.frame.time = time;
    snapshot.frame.deltaSeconds = 1.0f / 60.0f;
    snapshot.frame.morph = seasonWeather.seasonPhase;

    snapshot.recipe.abiVersion = kVersion;
    snapshot.recipe.byteSize = kRecipeBytes;
    snapshot.recipe.generatorId = static_cast<std::uint32_t>(GeneratorId::ProceduralWoodland);
    snapshot.recipe.primitiveId = static_cast<std::uint32_t>(PrimitiveId::Sphere);
    snapshot.recipe.generatorHz = 60U;
    snapshot.recipe.materialBandCount = 3U;
    snapshot.recipe.flags = 0U;

    std::uint32_t count = 0U;
    const float wetnessRoughMod = 1.0f - 0.45f * std::clamp(seasonWeather.rainWetness, 0.0f, 1.0f);

    auto pushInstance = [&](const WorldInstanceGpu& inst) noexcept -> bool {
        if (count >= kMaxInstanceCount) {
            return false;
        }
        snapshot.instances[count++] = inst;
        return true;
    };

    // 1. Pack Trees (Trunk Cylinder + Asymmetric Canopy Sphere Lobes)
    for (std::size_t i = 0U; i < population.treePrimitives.size() && count < kMaxInstanceCount; ++i) {
        const ForestTreeGpu& t = population.treePrimitives[i];
        const SculptureVec3 basePos = {t.baseRadius[0], t.baseRadius[1], t.baseRadius[2]};
        const SculptureVec3 crownPos = {t.canopyCenterRadius[0], t.canopyCenterRadius[1], t.canopyCenterRadius[2]};
        const float crownR = std::max(t.canopyCenterRadius[3], 1.0f);

        const ForestLodTier lod = classifyForestInstanceLod(crownPos, crownR * 1.5f, frustum, static_cast<std::uint32_t>(i));
        if (lod == ForestLodTier::Culled) {
            continue;
        }

        const SculptureVec3 trunkVec = {crownPos.x - basePos.x, crownPos.y - basePos.y, crownPos.z - basePos.z};
        const float trunkLen = std::max(diff_geom::length3(trunkVec), 0.5f);
        const float trunkR = std::max(t.baseRadius[3], 0.1f);

        WorldInstanceGpu trunk{};
        trunk.positionScale = {
            basePos.x + 0.5f * trunkVec.x,
            basePos.y + 0.5f * trunkVec.y,
            basePos.z + 0.5f * trunkVec.z,
            std::max(trunkR, trunkLen)
        };
        trunk.axisScale = {trunkR, trunkLen, trunkR, 1.0f};
        trunk.orientation = alignUpVectorToDirection(trunkVec);
        trunk.tintEmission = {0.22f, 0.16f, 0.11f, 0.0f};
        trunk.pbrOverride = {0.0f, 0.82f * wetnessRoughMod, 0.0f, 0.0f};
        trunk.primitiveId = static_cast<std::uint32_t>(PrimitiveId::Cylinder);
        trunk.materialBand = 0U;
        trunk.flags = kAnisotropicScaleFlag;
        trunk.sourceIndex = static_cast<std::uint32_t>(i);
        pushInstance(trunk);

        const SculptureVec3 leafCol = evaluateSeasonalFoliageAlbedo(
            {0.14f, 0.30f, 0.08f}, seasonWeather.seasonPhase, static_cast<std::uint32_t>(i));

        WorldInstanceGpu canopy{};
        canopy.positionScale = {crownPos.x, crownPos.y, crownPos.z, crownR};
        canopy.axisScale = {crownR, crownR * 0.82f, crownR, 1.0f};
        canopy.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
        canopy.tintEmission = {leafCol.x, leafCol.y, leafCol.z, 0.0f};
        canopy.pbrOverride = {0.0f, 0.65f * wetnessRoughMod, 0.0f, 0.0f};
        canopy.primitiveId = static_cast<std::uint32_t>(PrimitiveId::Sphere);
        canopy.materialBand = 1U;
        canopy.flags = kAnisotropicScaleFlag;
        canopy.sourceIndex = static_cast<std::uint32_t>(i);
        pushInstance(canopy);
    }

    // 2. Pack Weathered Mossy Granite Boulders
    for (std::size_t i = 0U; i < population.boulderPrimitives.size() && count < kMaxInstanceCount; ++i) {
        const ForestMossyBoulderGpu& b = population.boulderPrimitives[i];
        const SculptureVec3 pos = {b.centerRadius[0], b.centerRadius[1], b.centerRadius[2]};
        const float r = std::max(b.centerRadius[3], 0.2f);
        if (classifyForestInstanceLod(pos, r * 1.5f, frustum, static_cast<std::uint32_t>(i) ^ 0xB0U) == ForestLodTier::Culled) {
            continue;
        }

        const float sx = r * std::max(b.stretchSeed[0], 0.2f);
        const float sy = r * std::max(b.stretchSeed[1], 0.2f);
        const float sz = r * std::max(b.stretchSeed[2], 0.2f);

        WorldInstanceGpu inst{};
        inst.positionScale = {pos.x, pos.y, pos.z, std::max({sx, sy, sz})};
        inst.axisScale = {sx, sy, sz, 1.0f};
        inst.orientation = axisAngleToQuaternion(0.0f, 1.0f, 0.0f, b.stretchSeed[3] * 6.2831853f);
        inst.tintEmission = {0.20f, 0.26f, 0.16f, 0.0f};
        inst.pbrOverride = {0.02f, 0.68f * wetnessRoughMod, 0.0f, 0.0f};
        inst.primitiveId = static_cast<std::uint32_t>(PrimitiveId::Sphere);
        inst.materialBand = 0U;
        inst.flags = kAnisotropicScaleFlag;
        inst.sourceIndex = static_cast<std::uint32_t>(i);
        pushInstance(inst);
    }

    // 3. Pack Rooted Bioluminescent Wildflowers (Stationary stem + glowing blossom head)
    for (std::size_t i = 0U; i < population.flowerPrimitives.size() && count < kMaxInstanceCount; ++i) {
        const ForestGroundFlowerGpu& fl = population.flowerPrimitives[i];
        const SculptureVec3 headPos = {fl.headPosRadius[0], fl.headPosRadius[1], fl.headPosRadius[2]};
        const float headR = std::max(fl.headPosRadius[3], 0.04f);

        const ForestLodTier lod = classifyForestInstanceLod(headPos, headR * 2.0f, frustum, static_cast<std::uint32_t>(i) ^ 0xF1U);
        if (lod == ForestLodTier::Culled) {
            continue;
        }

        const SculptureVec3 glow = {fl.glowColorPhase[0], fl.glowColorPhase[1], fl.glowColorPhase[2]};
        const float maxChannel = std::max({glow.x, glow.y, glow.z, 1e-4f});

        WorldInstanceGpu blossom{};
        blossom.positionScale = {headPos.x, headPos.y, headPos.z, headR};
        blossom.axisScale = {headR, headR * 0.75f, headR, 1.0f};
        blossom.orientation = alignUpVectorToDirection({
            -fl.headAxisStiffness[0],
            -fl.headAxisStiffness[1],
            -fl.headAxisStiffness[2]
        });
        blossom.tintEmission = {
            glow.x / maxChannel,
            glow.y / maxChannel,
            glow.z / maxChannel,
            maxChannel
        };
        blossom.pbrOverride = {0.0f, 0.28f, 0.0f, 0.0f};
        blossom.primitiveId = static_cast<std::uint32_t>(PrimitiveId::Sphere);
        blossom.materialBand = 2U;
        blossom.flags = kAnisotropicScaleFlag;
        blossom.sourceIndex = static_cast<std::uint32_t>(i);
        pushInstance(blossom);
    }

    // 4. Pack Understory Ferns & Shrubs
    for (std::size_t i = 0U; i < population.ferns.size() && count < kMaxInstanceCount; ++i) {
        const ForestScatterInstanceGpu& f = population.ferns[i];
        const SculptureVec3 pos = {f.posX, f.posY + 0.18f * f.scale, f.posZ};
        const ForestLodTier lod = classifyForestInstanceLod(pos, 0.6f * f.scale, frustum, f.seed);
        if (lod == ForestLodTier::Culled || lod == ForestLodTier::LOD2_FarProxy) {
            continue;
        }

        const SculptureVec3 fernCol = evaluateSeasonalFoliageAlbedo(
            {0.15f, 0.31f, 0.07f}, seasonWeather.seasonPhase, f.seed);
        WorldInstanceGpu inst{};
        inst.positionScale = {pos.x, pos.y, pos.z, 0.45f * f.scale};
        inst.axisScale = {0.45f * f.scale, 0.22f * f.scale, 0.45f * f.scale, 1.0f};
        inst.orientation = axisAngleToQuaternion(0.0f, 1.0f, 0.0f, random01(f.seed) * 6.2831853f);
        inst.tintEmission = {fernCol.x, fernCol.y, fernCol.z, 0.0f};
        inst.pbrOverride = {0.0f, 0.38f * wetnessRoughMod, 0.0f, 0.0f};
        inst.primitiveId = static_cast<std::uint32_t>(PrimitiveId::Sphere);
        inst.materialBand = 1U;
        inst.flags = kAnisotropicScaleFlag;
        inst.sourceIndex = static_cast<std::uint32_t>(i);
        pushInstance(inst);
    }

    // 5. Optional Swirling Flower Petals
    for (std::size_t i = 0U; i < swirlingPetals.size() && count < kMaxInstanceCount; ++i) {
        const ForestSwirlingPetalGpu& p = swirlingPetals[i];
        const SculptureVec3 pos = {p.positionAge[0], p.positionAge[1], p.positionAge[2]};
        if (classifyForestInstanceLod(pos, 0.1f, frustum, static_cast<std::uint32_t>(i) ^ 0x55U) == ForestLodTier::Culled) {
            continue;
        }

        WorldInstanceGpu inst{};
        inst.positionScale = {pos.x, pos.y, pos.z, 0.045f};
        inst.axisScale = {0.045f, 0.012f, 0.035f, 1.0f};
        inst.orientation = alignUpVectorToDirection({p.velocityDrag[0], p.velocityDrag[1], p.velocityDrag[2]});
        inst.tintEmission = {p.baseColorSeed[0], p.baseColorSeed[1], p.baseColorSeed[2], 1.25f};
        inst.pbrOverride = {0.0f, 0.32f, 0.0f, 0.0f};
        inst.primitiveId = static_cast<std::uint32_t>(PrimitiveId::Tile);
        inst.materialBand = 2U;
        inst.flags = kAnisotropicScaleFlag;
        inst.sourceIndex = static_cast<std::uint32_t>(i);
        pushInstance(inst);
    }

    snapshot.frame.activeInstanceCount = count;
    snapshot.recipe.instanceBudget = std::max(count, 1U);
    return snapshot;
}

} // namespace epi_sculpt::forest
