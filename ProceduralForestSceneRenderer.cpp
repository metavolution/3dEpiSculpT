#include "ProceduralForestSceneRenderer.h"
#include "SculptureDifferentialGeometry.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <numbers>

namespace epi_sculpt::forest {

namespace {

template <typename VisitorFn>
void forEachCellIn3x3Neighborhood(
    const ForestSpatialHashGrid& grid,
    float worldX,
    float worldZ,
    const std::vector<std::uint32_t>& offsets,
    const std::vector<std::uint32_t>& indices,
    VisitorFn&& visitor) noexcept
{
    if (offsets.empty() || grid.gridWidth == 0U || grid.gridHeight == 0U) {
        return;
    }
    const float invCell = 1.0f / std::max(grid.cellSize, 1e-4f);
    const int cx = std::clamp(
        static_cast<int>(std::floor((worldX - grid.originX) * invCell)),
        0,
        static_cast<int>(grid.gridWidth) - 1);
    const int cz = std::clamp(
        static_cast<int>(std::floor((worldZ - grid.originZ) * invCell)),
        0,
        static_cast<int>(grid.gridHeight) - 1);

    const int xMin = std::max(0, cx - 1);
    const int xMax = std::min(static_cast<int>(grid.gridWidth) - 1, cx + 1);
    const int zMin = std::max(0, cz - 1);
    const int zMax = std::min(static_cast<int>(grid.gridHeight) - 1, cz + 1);

    for (int iz = zMin; iz <= zMax; ++iz) {
        for (int ix = xMin; ix <= xMax; ++ix) {
            const std::uint32_t cellIdx = static_cast<std::uint32_t>(iz) * grid.gridWidth + static_cast<std::uint32_t>(ix);
            const std::uint32_t begin = offsets[cellIdx];
            const std::uint32_t end = offsets[cellIdx + 1U];
            for (std::uint32_t k = begin; k < end; ++k) {
                visitor(indices[k]);
            }
        }
    }
}

void evaluateSingleFlowerPetalSdf(
    const SculptureVec3& p,
    const ForestGroundFlowerGpu& fl,
    int flowerIdx,
    float& inOutMinDist,
    int& inOutBestIdx,
    float& inOutBestU,
    float& inOutBestV) noexcept
{
    const float hx = fl.headPosRadius[0];
    const float hy = fl.headPosRadius[1];
    const float hz = fl.headPosRadius[2];
    const float headR = std::max(fl.headPosRadius[3], 0.01f);

    const SculptureVec3 toP = {p.x - hx, p.y - hy, p.z - hz};
    if (diff_geom::dot3(toP, toP) > 0.16f) {
        return;
    }

    const SculptureVec3 axisY = diff_geom::normalize3(
        {fl.headAxisStiffness[0], fl.headAxisStiffness[1], fl.headAxisStiffness[2]},
        {0.0f, -1.0f, 0.0f});
    const SculptureVec3 ref = std::abs(axisY.z) < 0.9f
        ? SculptureVec3{0.0f, 0.0f, 1.0f}
        : SculptureVec3{1.0f, 0.0f, 0.0f};
    const SculptureVec3 axisX = diff_geom::normalize3(diff_geom::cross3(axisY, ref));
    const SculptureVec3 axisZ = diff_geom::cross3(axisX, axisY);

    const SculptureVec3 localP = {
        diff_geom::dot3(toP, axisX),
        diff_geom::dot3(toP, axisY),
        diff_geom::dot3(toP, axisZ)
    };

    constexpr float kSector = (2.0f * std::numbers::pi_v<float>) / 5.0f;
    const float angle = std::atan2(localP.z, localP.x);
    float wrapped = std::fmod(angle + std::numbers::pi_v<float> + 0.5f * kSector, kSector);
    if (wrapped < 0.0f) {
        wrapped += kSector;
    }
    const float localAngle = wrapped - 0.5f * kSector;

    const float rXZ = std::sqrt(localP.x * localP.x + localP.z * localP.z);
    const float u = std::clamp(rXZ / headR, 0.0f, 1.0f);
    const float v = std::clamp(localAngle / (0.5f * kSector), -1.0f, 1.0f);

    const float cupY = -0.08f * std::sin(u * 1.8f) + 0.02f * u * u;
    const float petalWidth = std::sin(u * std::numbers::pi_v<float> * 0.85f);
    const float dy = localP.y - cupY;
    const float dw = std::max(0.0f, std::abs(v) - petalWidth) * 0.04f;
    const float dPetal = std::sqrt(dy * dy + dw * dw) - 0.004f;

    if (dPetal < inOutMinDist) {
        inOutMinDist = dPetal;
        inOutBestIdx = flowerIdx;
        inOutBestU = u;
        inOutBestV = v;
    }
}

} // namespace

LichtungSdfEvaluation evaluateLichtungSceneSdf(
    const SculptureVec3& p,
    const ForestFieldConfig& fieldConfig,
    std::span<const ForestTreeGpu> trees,
    std::span<const ForestMossyBoulderGpu> boulders,
    std::span<const ForestGroundFlowerGpu> flowers,
    const ForestSpatialHashGrid* accelGrid) noexcept
{
    LichtungSdfEvaluation eval{};

    // 1. Eroded Valley Terrain & Winding Dirt Footpath
    float pathMask = 1.0f;
    const float hGround = evaluateTerrainHeight(p.x, p.z, fieldConfig, &pathMask);
    const float dTerrain = (p.y - hGround) * 0.65f;

    eval.distance = dTerrain;
    eval.material = (pathMask < 0.45f) ? ForestMaterialId::DirtPath : ForestMaterialId::ForestFloor;
    eval.pathMask = pathMask;
    eval.terrainHeight = hGround;

    // 2. Weathered Mossy Granite Boulders
    float dBoulder = 1e5f;
    int bestBoulderIdx = -1;
    if (accelGrid != nullptr && !accelGrid->boulderCellOffsets.empty()) {
        forEachCellIn3x3Neighborhood(
            *accelGrid, p.x, p.z,
            accelGrid->boulderCellOffsets, accelGrid->boulderIndices,
            [&](std::uint32_t idx) noexcept {
                if (idx < boulders.size()) {
                    const float d = evaluateBoulderSdf(p, boulders[idx]);
                    if (d < dBoulder) {
                        dBoulder = d;
                        bestBoulderIdx = static_cast<int>(idx);
                    }
                }
            });
    } else {
        for (std::size_t i = 0U; i < boulders.size(); ++i) {
            const float d = evaluateBoulderSdf(p, boulders[i]);
            if (d < dBoulder) {
                dBoulder = d;
                bestBoulderIdx = static_cast<int>(i);
            }
        }
    }
    if (dBoulder < eval.distance) {
        eval.distance = dBoulder;
        eval.material = ForestMaterialId::MossyBoulder;
        eval.primitiveIndex = bestBoulderIdx;
    }

    // 3. Deciduous Tree Trunks (Buttressed + Bark Furrows) & Canopy Foliage
    float dTree = 1e5f;
    ForestMaterialId treeMat = ForestMaterialId::TreeBark;
    int bestTreeIdx = -1;
    if (accelGrid != nullptr && !accelGrid->treeCellOffsets.empty()) {
        forEachCellIn3x3Neighborhood(
            *accelGrid, p.x, p.z,
            accelGrid->treeCellOffsets, accelGrid->treeIndices,
            [&](std::uint32_t idx) noexcept {
                if (idx < trees.size()) {
                    const ForestTreeGpu& t = trees[idx];
                    const std::uint32_t tSeed = hash32(idx * 2654435761U + 0x54524545U);
                    const float dTrunk = evaluateTreeTrunkSdf(p, t, tSeed);
                    if (dTrunk < dTree) {
                        dTree = dTrunk;
                        treeMat = ForestMaterialId::TreeBark;
                        bestTreeIdx = static_cast<int>(idx);
                    }
                    const float dCanopy = evaluateTreeCanopySdf(p, t, tSeed);
                    if (dCanopy < dTree) {
                        dTree = dCanopy;
                        treeMat = ForestMaterialId::CanopyFoliage;
                        bestTreeIdx = static_cast<int>(idx);
                    }
                }
            });
    } else {
        dTree = evaluateTreesSdf(p, trees, &treeMat, &bestTreeIdx);
    }
    if (dTree < eval.distance) {
        eval.distance = dTree;
        eval.material = treeMat;
        eval.primitiveIndex = bestTreeIdx;
    }

    // 4. Bracken Fern Undergrowth Blanket (outside worn trail core)
    if (pathMask > 0.35f && (p.y - hGround) < 0.85f) {
        const float fernEnvelope = p.y - (hGround + 0.55f * pathMask);
        const float fernWorley = 0.12f * worley3D(p.x * 4.5f, p.y * 2.0f, p.z * 4.5f) - 0.05f * pathMask;
        const float dFerns = std::max(fernEnvelope, fernWorley);
        if (dFerns < eval.distance) {
            eval.distance = dFerns;
            eval.material = ForestMaterialId::FernFoliage;
        }
    }

    // 5. Rooted Bell-Flowers & 5-Fold Cupped Petals
    if ((p.y - hGround) < 0.95f && !flowers.empty()) {
        float dFlower = 1e5f;
        int fIdx = -1;
        float pU = 0.0f;
        float pV = 0.0f;
        if (accelGrid != nullptr && !accelGrid->flowerCellOffsets.empty()) {
            forEachCellIn3x3Neighborhood(
                *accelGrid, p.x, p.z,
                accelGrid->flowerCellOffsets, accelGrid->flowerIndices,
                [&](std::uint32_t idx) noexcept {
                    if (idx < flowers.size()) {
                        evaluateSingleFlowerPetalSdf(
                            p, flowers[idx], static_cast<int>(idx), dFlower, fIdx, pU, pV);
                    }
                });
        } else {
            dFlower = evaluateRootedFlowerPetalsSdf(p, flowers, &fIdx, &pU, &pV);
        }

        if (dFlower < eval.distance) {
            eval.distance = dFlower;
            eval.material = ForestMaterialId::FlowerPetal;
            eval.primitiveIndex = fIdx;
            eval.petalU = pU;
            eval.petalV = pV;
        }
    }

    return eval;
}

SculptureVec3 evaluateLichtungSceneNormal(
    const SculptureVec3& p,
    const ForestFieldConfig& fieldConfig,
    std::span<const ForestTreeGpu> trees,
    std::span<const ForestMossyBoulderGpu> boulders,
    std::span<const ForestGroundFlowerGpu> flowers,
    const ForestSpatialHashGrid* accelGrid,
    float eps) noexcept
{
    const float h = std::max(eps, 1e-4f);
    // 4-point regular tetrahedron vertices: e0=(1,-1,-1), e1=(-1,-1,1), e2=(-1,1,-1), e3=(1,1,1)
    const float d0 = evaluateLichtungSceneSdf(
        {p.x + h, p.y - h, p.z - h}, fieldConfig, trees, boulders, flowers, accelGrid).distance;
    const float d1 = evaluateLichtungSceneSdf(
        {p.x - h, p.y - h, p.z + h}, fieldConfig, trees, boulders, flowers, accelGrid).distance;
    const float d2 = evaluateLichtungSceneSdf(
        {p.x - h, p.y + h, p.z - h}, fieldConfig, trees, boulders, flowers, accelGrid).distance;
    const float d3 = evaluateLichtungSceneSdf(
        {p.x + h, p.y + h, p.z + h}, fieldConfig, trees, boulders, flowers, accelGrid).distance;

    return diff_geom::normalize3({
        (d0 - d1 - d2 + d3),
        (-d0 - d1 + d2 + d3),
        (-d0 + d1 - d2 + d3)
    });
}

float evaluateLichtungSoftShadow(
    const SculptureVec3& origin,
    const SculptureVec3& sunDir,
    const ForestFieldConfig& fieldConfig,
    std::span<const ForestTreeGpu> trees,
    std::span<const ForestMossyBoulderGpu> boulders,
    const ForestSpatialHashGrid* accelGrid) noexcept
{
    const SculptureVec3 L = diff_geom::normalize3(sunDir);
    const float canopyTrans = evaluateCanopyShadowTransmittance(
        origin.x, origin.y, origin.z, trees, L, fieldConfig.seed);

    float shadow = 1.0f;
    float t = 0.08f;
    constexpr std::span<const ForestGroundFlowerGpu> kNoShadowFlowers{};
    for (int step = 0; step < 12 && t < 18.0f; ++step) {
        const SculptureVec3 sampleP = {
            origin.x + t * L.x,
            origin.y + t * L.y,
            origin.z + t * L.z
        };
        const float d = evaluateLichtungSceneSdf(
            sampleP, fieldConfig, trees, boulders, kNoShadowFlowers, accelGrid).distance;
        if (d < 0.002f) {
            shadow = 0.12f;
            break;
        }
        shadow = std::min(shadow, std::clamp(6.0f * d / t, 0.12f, 1.0f));
        t += std::clamp(d, 0.12f, 1.8f);
    }
    return canopyTrans * shadow;
}

float evaluateLichtungAmbientOcclusion(
    const SculptureVec3& p,
    const SculptureVec3& normal,
    const ForestFieldConfig& fieldConfig,
    std::span<const ForestTreeGpu> trees,
    std::span<const ForestMossyBoulderGpu> boulders,
    const ForestSpatialHashGrid* accelGrid) noexcept
{
    const SculptureVec3 N = diff_geom::normalize3(normal);
    constexpr std::span<const ForestGroundFlowerGpu> kNoAoFlowers{};
    float occ = 0.0f;
    float weight = 1.0f;
    for (int i = 1; i <= 4; ++i) {
        const float h = 0.06f * static_cast<float>(i);
        const SculptureVec3 sampleP = {p.x + h * N.x, p.y + h * N.y, p.z + h * N.z};
        const float d = evaluateLichtungSceneSdf(
            sampleP, fieldConfig, trees, boulders, kNoAoFlowers, accelGrid).distance;
        occ += std::max(0.0f, h - d) * weight;
        weight *= 0.5f;
    }
    return std::clamp(1.0f - 2.2f * occ, 0.15f, 1.0f);
}

SculptureVec3 tonemapAcesFilmic(const SculptureVec3& linearHdr) noexcept {
    auto acesChannel = [](float x) noexcept -> float {
        const float v = std::max(0.0f, x);
        constexpr float a = 2.51f;
        constexpr float b = 0.03f;
        constexpr float c = 2.43f;
        constexpr float d = 0.59f;
        constexpr float e = 0.14f;
        return std::clamp((v * (a * v + b)) / (v * (c * v + d) + e), 0.0f, 1.0f);
    };
    return {
        acesChannel(linearHdr.x),
        acesChannel(linearHdr.y),
        acesChannel(linearHdr.z)
    };
}

void applyThresholdedHdrPetalBloom(
    std::span<const SculptureVec3> linearHdr,
    std::uint32_t width,
    std::uint32_t height,
    float threshold,
    float softKnee,
    float intensity,
    std::vector<SculptureVec3>& outBloomedHdr)
{
    const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    outBloomedHdr.assign(linearHdr.begin(), linearHdr.end());
    if (linearHdr.size() < pixelCount || width == 0U || height == 0U || intensity <= 0.0f) {
        return;
    }

    // 1. Extract bright HDR highlights above soft-knee threshold
    const float knee = std::max(threshold * softKnee, 1e-4f);
    std::vector<SculptureVec3> brightPass(pixelCount, {0.0f, 0.0f, 0.0f});
    for (std::size_t i = 0U; i < pixelCount; ++i) {
        const SculptureVec3 c = linearHdr[i];
        const float lum = std::max({c.x, c.y, c.z, 0.0f});
        if (lum > threshold - knee) {
            float soft = std::clamp(lum - threshold + knee, 0.0f, 2.0f * knee);
            soft = (soft * soft) / (4.0f * knee);
            const float contrib = std::max(soft, lum - threshold) / std::max(lum, 1e-4f);
            brightPass[i] = {c.x * contrib, c.y * contrib, c.z * contrib};
        }
    }

    // 2. Separable 5-tap Gaussian blur (horizontal -> vertical)
    constexpr float kWeights[5] = {0.06136f, 0.24477f, 0.38774f, 0.24477f, 0.06136f};
    std::vector<SculptureVec3> tempBlur(pixelCount, {0.0f, 0.0f, 0.0f});

    for (std::uint32_t y = 0U; y < height; ++y) {
        for (std::uint32_t x = 0U; x < width; ++x) {
            SculptureVec3 sum{0.0f, 0.0f, 0.0f};
            for (int dx = -2; dx <= 2; ++dx) {
                const int sx = std::clamp(static_cast<int>(x) + dx, 0, static_cast<int>(width) - 1);
                const SculptureVec3& tap = brightPass[static_cast<std::size_t>(y) * width + static_cast<std::size_t>(sx)];
                const float w = kWeights[dx + 2];
                sum.x += tap.x * w;
                sum.y += tap.y * w;
                sum.z += tap.z * w;
            }
            tempBlur[static_cast<std::size_t>(y) * width + x] = sum;
        }
    }

    for (std::uint32_t y = 0U; y < height; ++y) {
        for (std::uint32_t x = 0U; x < width; ++x) {
            SculptureVec3 sum{0.0f, 0.0f, 0.0f};
            for (int dy = -2; dy <= 2; ++dy) {
                const int sy = std::clamp(static_cast<int>(y) + dy, 0, static_cast<int>(height) - 1);
                const SculptureVec3& tap = tempBlur[static_cast<std::size_t>(sy) * width + x];
                const float w = kWeights[dy + 2];
                sum.x += tap.x * w;
                sum.y += tap.y * w;
                sum.z += tap.z * w;
            }
            const std::size_t idx = static_cast<std::size_t>(y) * width + x;
            outBloomedHdr[idx].x += sum.x * intensity;
            outBloomedHdr[idx].y += sum.y * intensity;
            outBloomedHdr[idx].z += sum.z * intensity;
        }
    }
}

LichtungRenderBuffer renderLichtungSceneFrameCpu(
    const ForestFieldGrid& fieldGrid,
    const ForestEcosystemPopulation& population,
    const GroundBounceIrradianceGrid& bounceGiGrid,
    const ForestLichtungDescriptorGpu& descriptor,
    const LichtungCameraRig& camera,
    const LichtungRenderConfig& renderConfig,
    const ForestSpatialHashGrid* accelGrid)
{
    LichtungRenderBuffer out{};
    const std::uint32_t w = std::max(1U, renderConfig.width);
    const std::uint32_t h = std::max(1U, renderConfig.height);
    const std::size_t pixelCount = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);

    out.width = w;
    out.height = h;
    out.linearHdr.resize(pixelCount);
    out.bloomedHdr.resize(pixelCount);
    out.displaySrgb.resize(pixelCount);
    out.materialIds.resize(pixelCount, ForestMaterialId::Sky);
    out.hitDistances.resize(pixelCount, renderConfig.maxRayDistance);
    out.metrics.totalPixels = pixelCount;

    ForestSpatialHashGrid localGrid{};
    const ForestSpatialHashGrid* activeGrid = accelGrid;
    if (activeGrid == nullptr || activeGrid->empty()) {
        localGrid = buildForestSpatialHashGrid(
            population.treePrimitives,
            population.boulderPrimitives,
            population.flowerPrimitives,
            fieldGrid.config.originX,
            fieldGrid.config.originZ,
            static_cast<float>(fieldGrid.config.gridWidth) * fieldGrid.config.cellSize,
            static_cast<float>(fieldGrid.config.gridHeight) * fieldGrid.config.cellSize,
            16U);
        activeGrid = &localGrid;
    }

    const float aspect = static_cast<float>(w) / static_cast<float>(h);
    const SculptureVec3 sunDir = diff_geom::normalize3({
        descriptor.sunDirection[0],
        descriptor.sunDirection[1],
        descriptor.sunDirection[2]
    });

    for (std::uint32_t py = 0U; py < h; ++py) {
        // py = 0 is top of image (+1 NDC Y), py = h - 1 is bottom (-1 NDC Y)
        const float ndcY = 1.0f - 2.0f * ((static_cast<float>(py) + 0.5f) / static_cast<float>(h));
        for (std::uint32_t px = 0U; px < w; ++px) {
            const float ndcX = (2.0f * ((static_cast<float>(px) + 0.5f) / static_cast<float>(w)) - 1.0f) * aspect;
            const SculptureVec3 rayDir = diff_geom::normalize3({
                camera.forward.x * camera.focalLength + ndcX * camera.right.x + ndcY * camera.up.x,
                camera.forward.y * camera.focalLength + ndcX * camera.right.y + ndcY * camera.up.y,
                camera.forward.z * camera.focalLength + ndcX * camera.right.z + ndcY * camera.up.z
            });

            float t = 0.1f;
            LichtungSdfEvaluation hitEval{};
            hitEval.material = ForestMaterialId::Sky;

            for (std::uint32_t step = 0U; step < renderConfig.maxRaySteps && t < renderConfig.maxRayDistance; ++step) {
                const SculptureVec3 p = {
                    camera.position.x + t * rayDir.x,
                    camera.position.y + t * rayDir.y,
                    camera.position.z + t * rayDir.z
                };
                const LichtungSdfEvaluation eval = evaluateLichtungSceneSdf(
                    p,
                    fieldGrid.config,
                    population.treePrimitives,
                    population.boulderPrimitives,
                    population.flowerPrimitives,
                    activeGrid);

                if (eval.distance < 0.0025f * t) {
                    hitEval = eval;
                    break;
                }
                t += std::max(eval.distance, 0.012f);
            }

            SculptureVec3 surfaceHdr = {0.68f, 0.76f, 0.64f};
            if (hitEval.material != ForestMaterialId::Sky) {
                const SculptureVec3 hitP = {
                    camera.position.x + t * rayDir.x,
                    camera.position.y + t * rayDir.y,
                    camera.position.z + t * rayDir.z
                };
                const SculptureVec3 N = evaluateLichtungSceneNormal(
                    hitP,
                    fieldGrid.config,
                    population.treePrimitives,
                    population.boulderPrimitives,
                    population.flowerPrimitives,
                    activeGrid);

                BotanicalSurfaceSample surf{};
                surf.material = hitEval.material;
                surf.position = hitP;
                surf.normal = N;
                surf.pathMask = hitEval.pathMask;
                surf.terrainHeight = hitEval.terrainHeight;
                surf.flowerIndex = hitEval.primitiveIndex;
                surf.petalU = hitEval.petalU;
                surf.petalV = hitEval.petalV;

                surfaceHdr = evaluateBotanicalSurfaceLighting(
                    surf,
                    population.flowerPrimitives,
                    population.treePrimitives,
                    bounceGiGrid,
                    descriptor);

                if (renderConfig.enableSoftShadows && hitEval.material != ForestMaterialId::FlowerPetal) {
                    const float softShadow = evaluateLichtungSoftShadow(
                        hitP, sunDir, fieldGrid.config, population.treePrimitives, population.boulderPrimitives, activeGrid);
                    const float contactAo = evaluateLichtungAmbientOcclusion(
                        hitP, N, fieldGrid.config, population.treePrimitives, population.boulderPrimitives, activeGrid);
                    const float shadowMod = 0.35f + 0.65f * softShadow * contactAo;
                    surfaceHdr.x *= shadowMod;
                    surfaceHdr.y *= shadowMod;
                    surfaceHdr.z *= shadowMod;
                }
            }

            const SculptureVec3 volHdr = integrateVolumetricLichtungAtmosphere(
                camera.position,
                rayDir,
                t,
                surfaceHdr,
                population.treePrimitives,
                descriptor,
                renderConfig.volumetricSteps);

            const std::size_t idx = static_cast<std::size_t>(py) * w + px;
            out.linearHdr[idx] = volHdr;
            out.materialIds[idx] = hitEval.material;
            out.hitDistances[idx] = t;

            switch (hitEval.material) {
                case ForestMaterialId::Sky:           out.metrics.skyPixelCount++; break;
                case ForestMaterialId::DirtPath:      out.metrics.dirtPathPixelCount++; break;
                case ForestMaterialId::ForestFloor:   out.metrics.forestFloorPixelCount++; break;
                case ForestMaterialId::MossyBoulder:  out.metrics.boulderPixelCount++; break;
                case ForestMaterialId::TreeBark:      out.metrics.treeBarkPixelCount++; break;
                case ForestMaterialId::CanopyFoliage: out.metrics.canopyFoliagePixelCount++; break;
                case ForestMaterialId::FernFoliage:   out.metrics.fernFoliagePixelCount++; break;
                case ForestMaterialId::FlowerPetal:   out.metrics.flowerPetalPixelCount++; break;
            }
        }
    }

    // Apply thresholded HDR bloom and ACES filmic tone mapping
    applyThresholdedHdrPetalBloom(
        out.linearHdr,
        w,
        h,
        renderConfig.bloomThreshold,
        renderConfig.bloomSoftKnee,
        renderConfig.bloomIntensity,
        out.bloomedHdr);

    double lumSum = 0.0;
    for (std::size_t i = 0U; i < pixelCount; ++i) {
        const SculptureVec3& hdr = out.bloomedHdr[i];
        if (!std::isfinite(hdr.x) || !std::isfinite(hdr.y) || !std::isfinite(hdr.z)) {
            out.metrics.allFinite = false;
        }
        const float hdrLum = 0.2126f * hdr.x + 0.7152f * hdr.y + 0.0722f * hdr.z;
        out.metrics.maxHdrLuminance = std::max(out.metrics.maxHdrLuminance, hdrLum);

        const SculptureVec3 tm = tonemapAcesFilmic(hdr);
        const SculptureVec3 srgb = {
            std::pow(tm.x, 1.0f / 2.2f),
            std::pow(tm.y, 1.0f / 2.2f),
            std::pow(tm.z, 1.0f / 2.2f)
        };
        out.displaySrgb[i] = srgb;
        lumSum += static_cast<double>(0.2126f * srgb.x + 0.7152f * srgb.y + 0.0722f * srgb.z);
    }
    out.metrics.meanLuminance = static_cast<float>(lumSum / static_cast<double>(pixelCount));

    return out;
}

bool writeLichtungFramePpm(
    const LichtungRenderBuffer& frame,
    const std::string& filePath)
{
    if (frame.width == 0U || frame.height == 0U || frame.displaySrgb.size() < static_cast<std::size_t>(frame.width) * frame.height) {
        return false;
    }
    std::ofstream ofs(filePath, std::ios::binary);
    if (!ofs.good()) {
        return false;
    }

    ofs << "P6\n" << frame.width << " " << frame.height << "\n255\n";
    std::vector<std::uint8_t> rgbBytes(static_cast<std::size_t>(frame.width) * frame.height * 3U);
    for (std::size_t i = 0U; i < static_cast<std::size_t>(frame.width) * frame.height; ++i) {
        const SculptureVec3& c = frame.displaySrgb[i];
        rgbBytes[i * 3U + 0U] = static_cast<std::uint8_t>(std::clamp(c.x * 255.0f + 0.5f, 0.0f, 255.0f));
        rgbBytes[i * 3U + 1U] = static_cast<std::uint8_t>(std::clamp(c.y * 255.0f + 0.5f, 0.0f, 255.0f));
        rgbBytes[i * 3U + 2U] = static_cast<std::uint8_t>(std::clamp(c.z * 255.0f + 0.5f, 0.0f, 255.0f));
    }
    ofs.write(reinterpret_cast<const char*>(rgbBytes.data()), static_cast<std::streamsize>(rgbBytes.size()));
    return ofs.good();
}

} // namespace epi_sculpt::forest
