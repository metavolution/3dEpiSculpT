#include "ProceduralForestEcologyScatter.h"

#include <algorithm>
#include <cmath>

namespace epi_sculpt::forest {

float evaluateEcologicalDensity(
    ForestLayerKind kind,
    const ForestWorldCellGpu& cell,
    float patchNoise,
    const ForestEcologyScatterConfig& config) noexcept
{
    // cell.pathMask is 0 on the trail centerline and 1 outside the trail border
    const float trailMask = std::clamp(cell.pathMask, 0.0f, 1.0f);
    const float slopeMask = 1.0f - smoothstep01(0.25f, 0.85f, cell.slope);
    const float patch = 0.25f + 0.75f * smoothstep01(0.2f, 0.8f, 0.5f + 0.5f * patchNoise);
    const float shade = std::clamp(1.0f - cell.openness, 0.0f, 1.0f);
    const float moistureSuitability = 0.45f + 0.55f * std::clamp(cell.moisture, 0.0f, 1.0f);

    switch (kind) {
        case ForestLayerKind::Tree:
            // Suppressed inside the clearing opening L(x,z)
            return config.treeBaseDensity
                 * std::max(0.0f, 1.0f - config.treeClearingSuppression * cell.clearing)
                 * moistureSuitability
                 * slopeMask
                 * trailMask
                 * patch;

        case ForestLayerKind::Boulder: {
            // Boulders frame the pathway and clearing flanks without blocking the trail core
            const float flankPreference = 0.35f + 0.65f * (1.0f - std::abs(2.0f * cell.clearing - 1.0f));
            return config.boulderBaseDensity
                 * flankPreference
                 * slopeMask
                 * trailMask
                 * patch;
        }

        case ForestLayerKind::UnderstoryShrub:
            return config.shrubBaseDensity
                 * (0.45f + 0.55f * shade)
                 * moistureSuitability
                 * slopeMask
                 * trailMask
                 * patch;

        case ForestLayerKind::FernFrond:
            // Dense bracken ferns love damp slopes and dappled forest/boulder shade
            return config.fernBaseDensity
                 * (0.35f + 0.65f * shade)
                 * (0.30f + 0.70f * cell.moisture)
                 * slopeMask
                 * smoothstep01(0.30f, 0.75f, trailMask)
                 * patch;

        case ForestLayerKind::TallGrass:
            // Grass density correlates with light availability, moisture, and path exclusion
            return config.grassBaseDensity
                 * (0.40f + 0.60f * cell.openness)
                 * moistureSuitability
                 * slopeMask
                 * trailMask
                 * patch;

        case ForestLayerKind::RootedWildflower: {
            // Wildflowers thrive in the sunlit clearing and along the meadow edges of the footpath
            const float pathEdgeBoost = 1.0f + 0.65f * (1.0f - std::abs(2.0f * trailMask - 1.15f));
            return config.flowerBaseDensity
                 * (0.20f + 0.80f * cell.openness)
                 * moistureSuitability
                 * slopeMask
                 * smoothstep01(0.25f, 0.65f, trailMask)
                 * pathEdgeBoost
                 * patch;
        }

        case ForestLayerKind::ForestDebris:
            // Twigs, fallen leaves, and pebbles cluster under canopy shade and along trail edges
            return config.debrisBaseDensity
                 * (0.50f + 0.50f * shade)
                 * slopeMask
                 * patch;

        default:
            return 0.0f;
    }
}

std::vector<ForestScatterInstanceGpu> scatterEcologicalLayer(
    const ForestFieldGrid& grid,
    ForestLayerKind kind,
    float sampleCellSize,
    const ForestEcologyScatterConfig& config)
{
    std::vector<ForestScatterInstanceGpu> results;
    if (grid.empty()) {
        return results;
    }

    const float cellSz = std::max(sampleCellSize, 0.2f);
    const float domainWidth = static_cast<float>(grid.config.gridWidth) * grid.config.cellSize;
    const float domainHeight = static_cast<float>(grid.config.gridHeight) * grid.config.cellSize;

    const std::uint32_t sampleW = std::max(1U, static_cast<std::uint32_t>(std::ceil(domainWidth / cellSz)));
    const std::uint32_t sampleH = std::max(1U, static_cast<std::uint32_t>(std::ceil(domainHeight / cellSz)));
    const std::uint32_t kindId = static_cast<std::uint32_t>(kind);

    results.reserve((sampleW * sampleH) / 4U);

    for (std::uint32_t iz = 0U; iz < sampleH; ++iz) {
        for (std::uint32_t ix = 0U; ix < sampleW; ++ix) {
            const std::uint32_t seed = hash32(
                config.sceneSeed ^ hash32(ix)
                ^ hash32(iz * 73856093U)
                ^ hash32(kindId * 19349663U));

            float jx = 0.0f;
            float jz = 0.0f;
            random2(seed, jx, jz);

            const float px = grid.config.originX + (static_cast<float>(ix) + jx) * cellSz;
            const float pz = grid.config.originZ + (static_cast<float>(iz) + jz) * cellSz;

            const ForestWorldCellGpu cell = sampleWorldCell(grid, px, pz);
            const float patchNoise = fbm2D(
                px * 0.11f + static_cast<float>(kindId) * 13.7f,
                pz * 0.11f + 5.0f,
                3U,
                config.sceneSeed ^ (kindId * 4099U));

            const float density = evaluateEcologicalDensity(kind, cell, patchNoise, config);
            const float probability = 1.0f - std::exp(-density * cellSz * cellSz);

            if (random01(seed + 13U) < probability) {
                ForestScatterInstanceGpu inst{};
                inst.posX = px;
                inst.posY = cell.height;
                inst.posZ = pz;
                inst.scale = 0.8f + 0.4f * random01(seed + 17U);
                inst.kind = kindId;
                inst.species = hash32(seed + 31U) % 8U;
                inst.seed = seed;
                inst.valid = 1U;
                results.push_back(inst);
            }
        }
    }
    return results;
}

std::vector<ForestScatterInstanceGpu> enforceMinimumDistanceExclusion(
    std::span<const ForestScatterInstanceGpu> candidates,
    float minDistance) noexcept
{
    std::vector<ForestScatterInstanceGpu> accepted;
    if (candidates.empty()) {
        return accepted;
    }
    if (minDistance <= 1e-4f) {
        accepted.assign(candidates.begin(), candidates.end());
        return accepted;
    }

    const float minDistSq = minDistance * minDistance;
    accepted.reserve(candidates.size());

    for (const ForestScatterInstanceGpu& cand : candidates) {
        if (cand.valid == 0U) {
            continue;
        }
        bool conflict = false;
        for (const ForestScatterInstanceGpu& prev : accepted) {
            const float dx = cand.posX - prev.posX;
            const float dz = cand.posZ - prev.posZ;
            if (dx * dx + dz * dz < minDistSq) {
                conflict = true;
                break;
            }
        }
        if (!conflict) {
            accepted.push_back(cand);
        }
    }
    return accepted;
}

float evaluateVoronoiFlowerHueSeed(
    float worldX,
    float worldZ,
    std::uint32_t instanceSeed,
    std::uint32_t sceneSeed) noexcept
{
    // Partition meadow into ~4.5m Voronoi patches so neighboring wildflowers
    // form natural multi-color botanical colonies (cyan, magenta, gold, violet, emerald)
    const float sx = worldX * 0.22f;
    const float sz = worldZ * 0.22f;
    const int ix = static_cast<int>(std::floor(sx));
    const int iz = static_cast<int>(std::floor(sz));

    float minDistSq = 1e6f;
    std::uint32_t bestCellHash = instanceSeed;

    for (int dz = -1; dz <= 1; ++dz) {
        for (int dx = -1; dx <= 1; ++dx) {
            const std::uint32_t cellHash = hash32(
                sceneSeed
                ^ (static_cast<std::uint32_t>(ix + dx) * 73856093U)
                ^ (static_cast<std::uint32_t>(iz + dz) * 19349663U));
            float jx = 0.0f;
            float jz = 0.0f;
            random2(cellHash, jx, jz);
            const float cx = static_cast<float>(ix + dx) + jx - sx;
            const float cz = static_cast<float>(iz + dz) + jz - sz;
            const float distSq = cx * cx + cz * cz;
            if (distSq < minDistSq) {
                minDistSq = distSq;
                bestCellHash = cellHash;
            }
        }
    }

    const float clusterHue = random01(bestCellHash ^ 0x5A5A5A5AU);
    const float individualJitter = (random01(instanceSeed ^ 0x3C3C3C3CU) - 0.5f) * 0.08f;
    float hue = std::fmod(clusterHue + individualJitter + 1.0f, 1.0f);
    return std::clamp(hue, 0.0f, 1.0f);
}

ForestEcosystemPopulation synthesizeCompleteForestEcology(
    ForestFieldGrid& inOutFieldGrid,
    const ForestEcologyScatterConfig& config,
    const SculptureVec3& sunDirection)
{
    ForestEcosystemPopulation pop{};

    // Stage 1: Scatter macro structures (Canopy Trees & Weathered Granite Boulders)
    auto rawTrees = scatterEcologicalLayer(inOutFieldGrid, ForestLayerKind::Tree, config.macroCellSize, config);
    pop.trees = enforceMinimumDistanceExclusion(rawTrees, config.treeMinDistance);
    if (pop.trees.size() > config.maxTrees) {
        pop.trees.resize(config.maxTrees);
    }

    auto rawBoulders = scatterEcologicalLayer(inOutFieldGrid, ForestLayerKind::Boulder, config.macroCellSize, config);
    pop.boulders = enforceMinimumDistanceExclusion(rawBoulders, config.boulderMinDistance);
    if (pop.boulders.size() > config.maxBoulders) {
        pop.boulders.resize(config.maxBoulders);
    }

    // Build GPU tree primitives so canopy shadow feedback uses actual crown geometry
    pop.treePrimitives.reserve(pop.trees.size());
    for (const ForestScatterInstanceGpu& t : pop.trees) {
        ForestTreeGpu treeGpu{};
        const float trunkRadius = 0.26f * t.scale;
        const float trunkHeight = 6.2f * t.scale;
        const float crownRadius = 3.4f * t.scale;
        const float crownAsymmetryX = (random01(t.seed ^ 0x101U) - 0.5f) * 0.8f;
        const float crownAsymmetryZ = (random01(t.seed ^ 0x202U) - 0.5f) * 0.8f;

        treeGpu.baseRadius[0] = t.posX;
        treeGpu.baseRadius[1] = t.posY;
        treeGpu.baseRadius[2] = t.posZ;
        treeGpu.baseRadius[3] = trunkRadius;

        treeGpu.canopyCenterRadius[0] = t.posX + crownAsymmetryX;
        treeGpu.canopyCenterRadius[1] = t.posY + trunkHeight;
        treeGpu.canopyCenterRadius[2] = t.posZ + crownAsymmetryZ;
        treeGpu.canopyCenterRadius[3] = crownRadius;
        pop.treePrimitives.push_back(treeGpu);
    }

    // Build GPU boulder primitives with shared geological cluster characteristics
    pop.boulderPrimitives.reserve(pop.boulders.size());
    for (const ForestScatterInstanceGpu& b : pop.boulders) {
        ForestMossyBoulderGpu boulderGpu{};
        // Embed boulder slightly into terrain for grounded contact
        const float baseRadius = 1.05f * b.scale;
        boulderGpu.centerRadius[0] = b.posX;
        boulderGpu.centerRadius[1] = b.posY + baseRadius * 0.45f;
        boulderGpu.centerRadius[2] = b.posZ;
        boulderGpu.centerRadius[3] = baseRadius;

        // Shared geological cluster seed quantized to 8m grid cells
        const int gx = static_cast<int>(std::floor(b.posX * 0.125f));
        const int gz = static_cast<int>(std::floor(b.posZ * 0.125f));
        const std::uint32_t geoClusterHash = hash32(
            config.sceneSeed ^ static_cast<std::uint32_t>(gx * 73856093) ^ static_cast<std::uint32_t>(gz * 19349663));

        boulderGpu.stretchSeed[0] = 1.15f + 0.45f * random01(geoClusterHash ^ 0x11U);
        boulderGpu.stretchSeed[1] = 0.65f + 0.30f * random01(b.seed ^ 0x22U); // Asymmetric vertical flattening
        boulderGpu.stretchSeed[2] = 1.05f + 0.45f * random01(geoClusterHash ^ 0x33U);
        boulderGpu.stretchSeed[3] = random01(geoClusterHash ^ b.seed);
        pop.boulderPrimitives.push_back(boulderGpu);
    }

    // Stage 2: Feed placed tree canopies back into the world field grid
    applyCanopyCoverageFeedback(inOutFieldGrid, pop.treePrimitives, sunDirection);

    // Stage 3: Resample understory, ferns, grass, wildflowers, and debris using updated canopy light
    pop.shrubs = scatterEcologicalLayer(
        inOutFieldGrid, ForestLayerKind::UnderstoryShrub, config.understoryCellSize, config);
    pop.ferns = scatterEcologicalLayer(
        inOutFieldGrid, ForestLayerKind::FernFrond, config.understoryCellSize, config);
    pop.grasses = scatterEcologicalLayer(
        inOutFieldGrid, ForestLayerKind::TallGrass, config.groundCoverCellSize, config);
    pop.debris = scatterEcologicalLayer(
        inOutFieldGrid, ForestLayerKind::ForestDebris, config.groundCoverCellSize, config);

    auto rawFlowers = scatterEcologicalLayer(
        inOutFieldGrid, ForestLayerKind::RootedWildflower, config.groundCoverCellSize, config);
    pop.flowers = enforceMinimumDistanceExclusion(rawFlowers, config.flowerMinDistance);
    if (pop.flowers.size() > config.maxFlowers) {
        pop.flowers.resize(config.maxFlowers);
    }

    // Build GPU rooted wildflower primitives with Voronoi spectral hue seeding
    pop.flowerPrimitives.reserve(pop.flowers.size());
    for (const ForestScatterInstanceGpu& f : pop.flowers) {
        ForestGroundFlowerGpu flowerGpu{};
        const float stemLen = (0.28f + 0.16f * random01(f.seed ^ 0x44U)) * f.scale;
        const float blossomRadius = (0.065f + 0.035f * random01(f.seed ^ 0x55U)) * f.scale;
        const float stiffness = 1.25f + 0.85f * random01(f.seed ^ 0x66U);
        const float hueSeed = evaluateVoronoiFlowerHueSeed(f.posX, f.posZ, f.seed, config.sceneSeed);

        flowerGpu.rootPosScale[0] = f.posX;
        flowerGpu.rootPosScale[1] = f.posY;
        flowerGpu.rootPosScale[2] = f.posZ;
        flowerGpu.rootPosScale[3] = stemLen;

        flowerGpu.headPosRadius[0] = f.posX;
        flowerGpu.headPosRadius[1] = f.posY + stemLen;
        flowerGpu.headPosRadius[2] = f.posZ;
        flowerGpu.headPosRadius[3] = blossomRadius;

        flowerGpu.headAxisStiffness[0] = 0.0f;
        flowerGpu.headAxisStiffness[1] = -1.0f;
        flowerGpu.headAxisStiffness[2] = 0.0f;
        flowerGpu.headAxisStiffness[3] = stiffness;

        flowerGpu.glowColorPhase[0] = 0.85f;
        flowerGpu.glowColorPhase[1] = 0.45f;
        flowerGpu.glowColorPhase[2] = 0.92f;
        flowerGpu.glowColorPhase[3] = hueSeed;

        pop.flowerPrimitives.push_back(flowerGpu);
    }

    return pop;
}

} // namespace epi_sculpt::forest
