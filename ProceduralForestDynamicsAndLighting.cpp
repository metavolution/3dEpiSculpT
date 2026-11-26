#include "ProceduralForestDynamicsAndLighting.h"
#include "ProceduralForestGeometry.h"
#include "SculptureDifferentialGeometry.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace epi_sculpt::forest {

SculptureVec3 evaluateSpectralPetalColor(
    float hueSeed,
    float petalU,
    float time) noexcept
{
    constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;
    const float phase = hueSeed + 0.18f * petalU + 0.08f * std::sin(time * 0.7f + hueSeed * kTwoPi);

    // Cosine spectral palette shifting smoothly across cyan, magenta, gold, violet, and emerald
    const float r = 0.55f + 0.45f * std::cos(kTwoPi * (phase + 0.00f));
    const float g = 0.50f + 0.48f * std::cos(kTwoPi * (phase + 0.33f));
    const float b = 0.60f + 0.40f * std::cos(kTwoPi * (phase + 0.67f));
    return {
        std::clamp(r, 0.0f, 1.0f),
        std::clamp(g, 0.0f, 1.0f),
        std::clamp(b, 0.0f, 1.0f)
    };
}

float evaluatePetalVenation(float petalU, float petalV) noexcept {
    const float cosV = std::cos(petalV * std::numbers::pi_v<float> * 6.0f);
    const float vein = 0.75f + 0.25f * (cosV * cosV * cosV * cosV);
    const float radialFade = smoothstep01(0.08f, 0.85f, petalU);
    return vein * radialFade;
}

SculptureVec3 evaluateAttachedPetalEmission(
    float hueSeed,
    float petalU,
    float petalV,
    float time,
    float maxHdrIntensity) noexcept
{
    const SculptureVec3 hueColor = evaluateSpectralPetalColor(hueSeed, petalU, time);
    const float s = 0.5f + 0.5f * std::sin(time * 2.2f + hueSeed * 15.0f);
    const float breathingPulse = 0.35f + 0.65f * (s * s);
    const float venationMask = evaluatePetalVenation(petalU, petalV);
    const float gain = maxHdrIntensity * breathingPulse * venationMask;
    return {
        hueColor.x * gain,
        hueColor.y * gain,
        hueColor.z * gain
    };
}

void updateRootedFlowersCpu(
    std::span<ForestGroundFlowerGpu> flowers,
    float windVelX,
    float windVelZ,
    float time,
    float maxHdrIntensity) noexcept
{
    for (ForestGroundFlowerGpu& fl : flowers) {
        const float rootX = fl.rootPosScale[0];
        const float rootY = fl.rootPosScale[1];
        const float rootZ = fl.rootPosScale[2];
        const float stemLen = std::max(fl.rootPosScale[3], 0.02f);
        const float stiffness = std::max(fl.headAxisStiffness[3], 0.25f);
        const float hueSeed = fl.glowColorPhase[3];

        // 1. Rooted Stem Sway (Euler-Bernoulli Cantilever Deflection)
        const float gust = 0.7f + 0.3f * fbm2D(
            rootX * 0.2f - windVelX * time,
            rootZ * 0.2f - windVelZ * time,
            2U,
            0x7171U);
        const float osc = 0.08f * std::sin(time * 4.5f + hueSeed * 20.0f);
        const float swayX = (windVelX * gust + osc) / stiffness;
        const float swayZ = (windVelZ * gust + osc * 0.7f) / stiffness;

        const float tipOffsetX = swayX * stemLen;
        const float tipOffsetZ = swayZ * stemLen;
        const float tipDropY = -0.6f * (tipOffsetX * tipOffsetX + tipOffsetZ * tipOffsetZ) / stemLen;

        // Flower stays permanently anchored to root; head tilts gracefully with stem curvature
        fl.headPosRadius[0] = rootX + tipOffsetX;
        fl.headPosRadius[1] = rootY + stemLen + tipDropY;
        fl.headPosRadius[2] = rootZ + tipOffsetZ;

        const SculptureVec3 headAxis = diff_geom::normalize3(
            {tipOffsetX * 1.8f, -0.65f, tipOffsetZ * 1.8f},
            {0.0f, -1.0f, 0.0f});
        fl.headAxisStiffness[0] = headAxis.x;
        fl.headAxisStiffness[1] = headAxis.y;
        fl.headAxisStiffness[2] = headAxis.z;

        // 2. Multi-Color Attached Petal Glow Calculation
        const SculptureVec3 baseSpectral = evaluateSpectralPetalColor(hueSeed, 0.7f, time);
        const float s = 0.5f + 0.5f * std::sin(time * 2.2f + hueSeed * 15.0f);
        const float breathingPulse = 0.35f + 0.65f * (s * s);
        const float hdrScale = maxHdrIntensity * breathingPulse;

        fl.glowColorPhase[0] = baseSpectral.x * hdrScale;
        fl.glowColorPhase[1] = baseSpectral.y * hdrScale;
        fl.glowColorPhase[2] = baseSpectral.z * hdrScale;
    }
}

GroundBounceIrradianceGrid computeGroundBounceIrradianceGridCpu(
    std::span<const ForestGroundFlowerGpu> flowers,
    const ForestFieldGrid& fieldGrid,
    std::uint32_t resolution)
{
    GroundBounceIrradianceGrid grid{};
    const std::uint32_t res = std::max(4U, resolution);
    grid.width = res;
    grid.height = res;
    grid.originX = fieldGrid.config.originX;
    grid.originZ = fieldGrid.config.originZ;
    grid.extentX = static_cast<float>(fieldGrid.config.gridWidth) * fieldGrid.config.cellSize;
    grid.extentZ = static_cast<float>(fieldGrid.config.gridHeight) * fieldGrid.config.cellSize;
    grid.irradiance.assign(static_cast<std::size_t>(res) * static_cast<std::size_t>(res), {0.0f, 0.0f, 0.0f});

    if (flowers.empty()) {
        return grid;
    }

    for (std::uint32_t iz = 0U; iz < res; ++iz) {
        const float v = (static_cast<float>(iz) + 0.5f) / static_cast<float>(res);
        const float worldZ = grid.originZ + v * grid.extentZ;

        for (std::uint32_t ix = 0U; ix < res; ++ix) {
            const float u = (static_cast<float>(ix) + 0.5f) / static_cast<float>(res);
            const float worldX = grid.originX + u * grid.extentX;
            const ForestWorldCellGpu cell = sampleWorldCell(fieldGrid, worldX, worldZ);
            const float groundY = cell.height;

            SculptureVec3 accum{0.0f, 0.0f, 0.0f};
            for (const ForestGroundFlowerGpu& fl : flowers) {
                const float dx = fl.headPosRadius[0] - worldX;
                const float dy = fl.headPosRadius[1] - groundY;
                const float dz = fl.headPosRadius[2] - worldZ;
                const float distSq = dx * dx + dy * dy + dz * dz;
                if (distSq < 12.0f) {
                    const float atten = 1.0f / (1.0f + 2.5f * distSq * distSq);
                    accum.x += fl.glowColorPhase[0] * atten;
                    accum.y += fl.glowColorPhase[1] * atten;
                    accum.z += fl.glowColorPhase[2] * atten;
                }
            }
            grid.irradiance[static_cast<std::size_t>(iz) * res + ix] = accum;
        }
    }
    return grid;
}

SculptureVec3 sampleGroundBounceIrradiance(
    const GroundBounceIrradianceGrid& giGrid,
    float worldX,
    float worldZ) noexcept
{
    if (giGrid.irradiance.empty() || giGrid.width == 0U || giGrid.height == 0U) {
        return {0.0f, 0.0f, 0.0f};
    }

    const float u = std::clamp((worldX - giGrid.originX) / std::max(giGrid.extentX, 1e-4f), 0.0f, 1.0f);
    const float v = std::clamp((worldZ - giGrid.originZ) / std::max(giGrid.extentZ, 1e-4f), 0.0f, 1.0f);

    const float gx = u * static_cast<float>(giGrid.width - 1U);
    const float gz = v * static_cast<float>(giGrid.height - 1U);

    const int x0 = static_cast<int>(std::floor(gx));
    const int z0 = static_cast<int>(std::floor(gz));
    const int x1 = std::min(x0 + 1, static_cast<int>(giGrid.width) - 1);
    const int z1 = std::min(z0 + 1, static_cast<int>(giGrid.height) - 1);

    const float tx = gx - static_cast<float>(x0);
    const float tz = gz - static_cast<float>(z0);

    const std::size_t stride = giGrid.width;
    const SculptureVec3& c00 = giGrid.irradiance[static_cast<std::size_t>(z0) * stride + static_cast<std::size_t>(x0)];
    const SculptureVec3& c10 = giGrid.irradiance[static_cast<std::size_t>(z0) * stride + static_cast<std::size_t>(x1)];
    const SculptureVec3& c01 = giGrid.irradiance[static_cast<std::size_t>(z1) * stride + static_cast<std::size_t>(x0)];
    const SculptureVec3& c11 = giGrid.irradiance[static_cast<std::size_t>(z1) * stride + static_cast<std::size_t>(x1)];

    auto bilerp = [&](float v00, float v10, float v01, float v11) noexcept -> float {
        const float a = v00 + tx * (v10 - v00);
        const float b = v01 + tx * (v11 - v01);
        return a + tz * (b - a);
    };

    return {
        bilerp(c00.x, c10.x, c01.x, c11.x),
        bilerp(c00.y, c10.y, c01.y, c11.y),
        bilerp(c00.z, c10.z, c01.z, c11.z)
    };
}

std::vector<ForestSwirlingPetalGpu> initializeSwirlingPetalsCpu(
    std::uint32_t numPetals,
    float domainWidth,
    float domainDepth,
    std::uint32_t seed)
{
    std::vector<ForestSwirlingPetalGpu> petals(numPetals);
    for (std::uint32_t i = 0U; i < numPetals; ++i) {
        const std::uint32_t h = hash32(seed ^ (i * 747796405U + 2891336453U));
        ForestSwirlingPetalGpu p{};
        p.positionAge[0] = (random01(h ^ 0x11U) - 0.5f) * domainWidth;
        p.positionAge[1] = 8.0f + random01(h ^ 0x22U) * 24.0f;
        p.positionAge[2] = (random01(h ^ 0x33U) - 0.5f) * domainDepth;
        p.positionAge[3] = random01(h ^ 0x44U); // Initial staggered lifetime phase [0, 1)

        p.velocityDrag[0] = (random01(h ^ 0x55U) - 0.5f) * 0.5f;
        p.velocityDrag[1] = -1.0f - random01(h ^ 0x66U) * 1.5f;
        p.velocityDrag[2] = (random01(h ^ 0x77U) - 0.5f) * 0.5f;
        p.velocityDrag[3] = 1.4f + random01(h ^ 0x88U) * 1.2f; // Drag coefficient alpha

        p.baseColorSeed[0] = 1.0f;
        p.baseColorSeed[1] = 0.4f;
        p.baseColorSeed[2] = 0.65f;
        p.baseColorSeed[3] = random01(h ^ 0x99U);
        petals[i] = p;
    }
    return petals;
}

void updateSwirlingPetalsCpu(
    std::span<ForestSwirlingPetalGpu> petals,
    float dt,
    float time,
    const SculptureVec3& windAmbient,
    float domainWidth,
    float domainDepth) noexcept
{
    const float safeDt = std::clamp(dt, 0.0f, 0.25f);
    auto fract01 = [](float x) noexcept -> float {
        return x - std::floor(x);
    };

    for (std::size_t gid = 0U; gid < petals.size(); ++gid) {
        ForestSwirlingPetalGpu& petal = petals[gid];
        SculptureVec3 pos = {petal.positionAge[0], petal.positionAge[1], petal.positionAge[2]};
        float age = petal.positionAge[3];
        SculptureVec3 vel = {petal.velocityDrag[0], petal.velocityDrag[1], petal.velocityDrag[2]};
        const float drag = petal.velocityDrag[3];
        const float seed = petal.baseColorSeed[3];

        // 1. Increment age/lifetime (respawn at upper canopy level if expired)
        age += safeDt * 0.15f;
        if (age > 1.0f) {
            const float hashVal = std::sin(static_cast<float>(gid + 1U) * 12.9898f + time * 0.1f) * 43758.5453f;
            pos.x = (fract01(hashVal) - 0.5f) * domainWidth;
            pos.z = (fract01(hashVal * 1.414f) - 0.5f) * domainDepth;
            pos.y = 25.0f + fract01(hashVal * 3.14f) * 15.0f;
            vel = {0.0f, -1.0f - fract01(hashVal) * 2.0f, 0.0f};
            age = 0.0f;
        }

        // 2. Apply ambient wind & 3D curl-noise turbulence
        const SculptureVec3 turb = curlNoise3D({pos.x * 0.1f, pos.y * 0.1f, pos.z * 0.1f}, time * 0.5f);
        const SculptureVec3 currentWind = {
            windAmbient.x + turb.x * 2.5f,
            windAmbient.y + turb.y * 2.5f,
            windAmbient.z + turb.z * 2.5f
        };

        // 3. Euler-Cromer integration under gravity and aerodynamic drag
        constexpr SculptureVec3 kGravity = {0.0f, -1.5f, 0.0f};
        const SculptureVec3 force = {
            kGravity.x + drag * (currentWind.x - vel.x),
            kGravity.y + drag * (currentWind.y - vel.y),
            kGravity.z + drag * (currentWind.z - vel.z)
        };

        vel.x += force.x * safeDt;
        vel.y += force.y * safeDt;
        vel.z += force.z * safeDt;

        pos.x += vel.x * safeDt;
        pos.y += vel.y * safeDt;
        pos.z += vel.z * safeDt;

        // 4. Rotational flutter oscillation
        const float flutter = std::sin(time * 5.0f + seed * 6.2831853f);
        pos.x += flutter * 0.05f;

        // 5. Chromodynamics: pink -> magenta/deep violet evolution with age quartic/linear fade
        const float colorWave = std::sin(time * 2.0f + seed * 10.0f + (pos.x + pos.z) * 0.05f);
        const float mixFactor = colorWave * colorWave; // sin^2(theta_i + P_i . k)
        constexpr SculptureVec3 kPink = {1.0f, 0.40f, 0.65f};
        constexpr SculptureVec3 kViolet = {0.55f, 0.05f, 0.60f};

        const float age2 = age * age;
        const float fade = std::clamp(1.0f - age2 * age2, 0.0f, 1.0f);
        petal.baseColorSeed[0] = (kPink.x + mixFactor * (kViolet.x - kPink.x)) * fade;
        petal.baseColorSeed[1] = (kPink.y + mixFactor * (kViolet.y - kPink.y)) * fade;
        petal.baseColorSeed[2] = (kPink.z + mixFactor * (kViolet.z - kPink.z)) * fade;

        petal.positionAge[0] = pos.x;
        petal.positionAge[1] = pos.y;
        petal.positionAge[2] = pos.z;
        petal.positionAge[3] = age;

        petal.velocityDrag[0] = vel.x;
        petal.velocityDrag[1] = vel.y;
        petal.velocityDrag[2] = vel.z;
    }
}

SculptureVec3 evaluateBotanicalSurfaceLighting(
    const BotanicalSurfaceSample& sample,
    std::span<const ForestGroundFlowerGpu> flowers,
    std::span<const ForestTreeGpu> trees,
    const GroundBounceIrradianceGrid& bounceGiGrid,
    const ForestLichtungDescriptorGpu& descriptor) noexcept
{
    if (sample.material == ForestMaterialId::Sky) {
        return {0.68f, 0.76f, 0.64f};
    }

    const SculptureVec3 N = diff_geom::normalize3(sample.normal);
    const SculptureVec3 sunDir = diff_geom::normalize3({
        descriptor.sunDirection[0],
        descriptor.sunDirection[1],
        descriptor.sunDirection[2]
    });
    const SculptureVec3 sunRadiance = {
        descriptor.sunRadiance[0],
        descriptor.sunRadiance[1],
        descriptor.sunRadiance[2]
    };
    const float time = descriptor.mistParams[3];

    // Dappled overhead tree canopy shadow transmittance
    const float canopyShadow = evaluateCanopyShadowTransmittance(
        sample.position.x,
        sample.position.y,
        sample.position.z,
        trees,
        sunDir);

    // Sample colored bounce light cast by nearby glowing flowers onto the environment
    const SculptureVec3 petalBounceGi = sampleGroundBounceIrradiance(
        bounceGiGrid,
        sample.position.x,
        sample.position.z);

    SculptureVec3 albedo{0.18f, 0.26f, 0.10f};
    SculptureVec3 sssColor{0.0f, 0.0f, 0.0f};
    SculptureVec3 emissive{0.0f, 0.0f, 0.0f};
    float ao = std::clamp((sample.position.y - sample.terrainHeight) / 0.4f + 0.25f, 0.15f, 1.0f);

    switch (sample.material) {
        case ForestMaterialId::DirtPath: {
            const float m = std::clamp(sample.pathMask, 0.0f, 1.0f);
            albedo = {
                0.26f + m * (0.18f - 0.26f),
                0.17f + m * (0.24f - 0.17f),
                0.10f + m * (0.09f - 0.10f)
            };
            break;
        }
        case ForestMaterialId::MossyBoulder: {
            const float mossMask = evaluateBoulderMossMask(sample.position, N, sample.terrainHeight);
            const float worleyTex = 0.7f + 0.3f * worley3D(
                sample.position.x * 4.0f,
                sample.position.y * 4.0f,
                sample.position.z * 4.0f);
            const SculptureVec3 graniteCol = {0.24f * worleyTex, 0.25f * worleyTex, 0.23f * worleyTex};
            constexpr SculptureVec3 kMossCol = {0.16f, 0.28f, 0.08f};
            albedo = {
                graniteCol.x + mossMask * (kMossCol.x - graniteCol.x),
                graniteCol.y + mossMask * (kMossCol.y - graniteCol.y),
                graniteCol.z + mossMask * (kMossCol.z - graniteCol.z)
            };
            sssColor = {kMossCol.x * mossMask * 0.35f, kMossCol.y * mossMask * 0.35f, kMossCol.z * mossMask * 0.35f};
            break;
        }
        case ForestMaterialId::TreeBark: {
            const float barkTex = 0.78f + 0.22f * fbm3D(
                sample.position.x * 3.0f,
                sample.position.y * 1.4f,
                sample.position.z * 3.0f);
            albedo = {0.22f * barkTex, 0.16f * barkTex, 0.11f * barkTex};
            break;
        }
        case ForestMaterialId::CanopyFoliage: {
            const float leafVar = 0.82f + 0.18f * fbm3D(
                sample.position.x * 1.5f,
                sample.position.y * 1.5f,
                sample.position.z * 1.5f);
            albedo = {0.13f * leafVar, 0.29f * leafVar, 0.08f * leafVar};
            sssColor = {0.48f * leafVar, 0.72f * leafVar, 0.18f * leafVar};
            ao = std::clamp(0.45f + 0.55f * N.y, 0.25f, 1.0f);
            break;
        }
        case ForestMaterialId::FernFoliage: {
            albedo = {0.15f, 0.31f, 0.07f};
            sssColor = {0.42f, 0.65f, 0.15f};
            break;
        }
        case ForestMaterialId::FlowerPetal: {
            albedo = {0.90f, 0.88f, 0.82f};
            float hueSeed = 0.25f;
            if (sample.flowerIndex >= 0 && static_cast<std::size_t>(sample.flowerIndex) < flowers.size()) {
                hueSeed = flowers[static_cast<std::size_t>(sample.flowerIndex)].glowColorPhase[3];
            }
            emissive = evaluateAttachedPetalEmission(
                hueSeed,
                sample.petalU,
                sample.petalV,
                time,
                descriptor.mistParams[2]);
            const SculptureVec3 petalHue = evaluateSpectralPetalColor(hueSeed, sample.petalU, time);
            sssColor = {
                0.95f * 0.4f + petalHue.x * 0.6f,
                0.92f * 0.4f + petalHue.y * 0.6f,
                0.80f * 0.4f + petalHue.z * 0.6f
            };
            ao = 1.0f;
            break;
        }
        default:
            break;
    }

    const float nDotL = diff_geom::dot3(N, sunDir);
    const float diffuse = std::max(0.0f, nDotL);
    const float backSss = std::pow(std::max(0.0f, -nDotL), 2.2f);
    const float skyFactor = (0.5f + 0.5f * N.y) * ao;
    const SculptureVec3 skyAmbient = {0.20f * skyFactor, 0.26f * skyFactor, 0.18f * skyFactor};
    const float bounceGeom = (0.6f + 0.4f * std::max(0.0f, -N.y)) * ao;

    return {
        sunRadiance.x * canopyShadow * (albedo.x * diffuse + sssColor.x * backSss)
            + skyAmbient.x * albedo.x + albedo.x * petalBounceGi.x * bounceGeom + emissive.x,
        sunRadiance.y * canopyShadow * (albedo.y * diffuse + sssColor.y * backSss)
            + skyAmbient.y * albedo.y + albedo.y * petalBounceGi.y * bounceGeom + emissive.y,
        sunRadiance.z * canopyShadow * (albedo.z * diffuse + sssColor.z * backSss)
            + skyAmbient.z * albedo.z + albedo.z * petalBounceGi.z * bounceGeom + emissive.z
    };
}

SculptureVec3 integrateVolumetricLichtungAtmosphere(
    const SculptureVec3& cameraPos,
    const SculptureVec3& rayDir,
    float hitDistance,
    const SculptureVec3& surfaceHdrRadiance,
    std::span<const ForestTreeGpu> trees,
    const ForestLichtungDescriptorGpu& descriptor,
    std::uint32_t numSteps) noexcept
{
    const SculptureVec3 V = diff_geom::normalize3(rayDir);
    const SculptureVec3 sunDir = diff_geom::normalize3({
        descriptor.sunDirection[0],
        descriptor.sunDirection[1],
        descriptor.sunDirection[2]
    });
    const SculptureVec3 sunRadiance = {
        descriptor.sunRadiance[0],
        descriptor.sunRadiance[1],
        descriptor.sunRadiance[2]
    };

    const float g = std::clamp(descriptor.sunRadiance[3], -0.95f, 0.95f);
    const float baseDensity = std::max(0.0f, descriptor.mistParams[0]);
    const float heightFalloff = std::max(0.0f, descriptor.mistParams[1]);
    const std::uint32_t steps = std::clamp(numSteps, 4U, 128U);
    const float marchDist = std::min(std::max(hitDistance, 0.1f), 65.0f);
    const float stepSize = marchDist / static_cast<float>(steps);

    // Henyey-Greenstein forward scattering phase function
    const float cosTheta = diff_geom::dot3(V, sunDir);
    const float g2 = g * g;
    const float hgDenom = std::pow(std::max(1e-4f, 1.0f + g2 - 2.0f * g * cosTheta), 1.5f);
    const float hgPhase = (1.0f - g2) / (4.0f * std::numbers::pi_v<float> * hgDenom);

    SculptureVec3 volAccum{0.0f, 0.0f, 0.0f};
    float transmittance = 1.0f;

    for (std::uint32_t i = 0U; i < steps; ++i) {
        const float s = (static_cast<float>(i) + 0.5f) * stepSize;
        const SculptureVec3 sampleP = {
            cameraPos.x + s * V.x,
            cameraPos.y + s * V.y,
            cameraPos.z + s * V.z
        };

        const float mistNoise = 0.7f + 0.3f * fbm3D(sampleP.x * 0.15f, sampleP.y * 0.15f, sampleP.z * 0.15f, 2U);
        const float localDensity = baseDensity * std::exp(-heightFalloff * std::max(0.0f, sampleP.y)) * mistNoise;
        if (localDensity > 1e-5f) {
            const float sampleCanopyVis = evaluateCanopyShadowTransmittance(
                sampleP.x, sampleP.y, sampleP.z, trees, sunDir);
            const SculptureVec3 mistInscatter = {
                0.52f * 0.35f + sunRadiance.x * sampleCanopyVis * hgPhase * 1.8f,
                0.60f * 0.35f + sunRadiance.y * sampleCanopyVis * hgPhase * 1.8f,
                0.46f * 0.35f + sunRadiance.z * sampleCanopyVis * hgPhase * 1.8f
            };
            const float stepExt = std::exp(-localDensity * stepSize);
            const float scatterWeight = transmittance * (1.0f - stepExt);
            volAccum.x += scatterWeight * mistInscatter.x;
            volAccum.y += scatterWeight * mistInscatter.y;
            volAccum.z += scatterWeight * mistInscatter.z;
            transmittance *= stepExt;
        }
    }

    return {
        surfaceHdrRadiance.x * transmittance + volAccum.x,
        surfaceHdrRadiance.y * transmittance + volAccum.y,
        surfaceHdrRadiance.z * transmittance + volAccum.z
    };
}

} // namespace epi_sculpt::forest
