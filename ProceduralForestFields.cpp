#include "ProceduralForestFields.h"
#include "SculptureDifferentialGeometry.h"

#include <algorithm>
#include <cmath>

namespace epi_sculpt::forest {

std::uint32_t hash32(std::uint32_t x) noexcept {
    x ^= x >> 16U;
    x *= 0x7feb352dU;
    x ^= x >> 15U;
    x *= 0x846ca68bU;
    x ^= x >> 16U;
    return x;
}

std::uint64_t hash64(std::uint64_t x) noexcept {
    x ^= x >> 30U;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27U;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31U;
    return x;
}

float random01(std::uint32_t seed) noexcept {
    return static_cast<float>(hash32(seed) & 0x00FFFFFFU) * (1.0f / 16777216.0f);
}

void random2(std::uint32_t seed, float& outX, float& outZ) noexcept {
    outX = random01(seed ^ 0xA341316CU);
    outZ = random01(seed ^ 0xC8013EA4U);
}

float smoothstep01(float edge0, float edge1, float x) noexcept {
    const float denom = edge1 - edge0;
    if (std::abs(denom) < 1e-6f) {
        return x >= edge1 ? 1.0f : 0.0f;
    }
    const float t = std::clamp((x - edge0) / denom, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

namespace {

[[nodiscard]] constexpr float quinticFade(float t) noexcept {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

[[nodiscard]] constexpr float quinticFadeDeriv(float t) noexcept {
    return 30.0f * t * t * (t * (t - 2.0f) + 1.0f);
}

[[nodiscard]] inline std::uint32_t coordHash2D(int ix, int iz, std::uint32_t seed) noexcept {
    return hash32(
        seed ^ (static_cast<std::uint32_t>(ix) * 73856093U)
             ^ (static_cast<std::uint32_t>(iz) * 19349663U));
}

[[nodiscard]] inline std::uint32_t coordHash3D(int ix, int iy, int iz, std::uint32_t seed) noexcept {
    return hash32(
        seed ^ (static_cast<std::uint32_t>(ix) * 73856093U)
             ^ (static_cast<std::uint32_t>(iy) * 19349663U)
             ^ (static_cast<std::uint32_t>(iz) * 83492791U));
}

inline void grad2D(std::uint32_t h, float& gx, float& gz) noexcept {
    static constexpr float kGrads[8][2] = {
        { 1.0f,  0.0f}, {-1.0f,  0.0f}, { 0.0f,  1.0f}, { 0.0f, -1.0f},
        { 0.70710678f,  0.70710678f}, {-0.70710678f,  0.70710678f},
        { 0.70710678f, -0.70710678f}, {-0.70710678f, -0.70710678f}
    };
    const auto& g = kGrads[h & 7U];
    gx = g[0];
    gz = g[1];
}

} // namespace

float simplex1D(float x, std::uint32_t seed) noexcept {
    if (!std::isfinite(x)) {
        return 0.0f;
    }
    const float xFloor = std::floor(x);
    const int i0 = static_cast<int>(xFloor);
    const float fx = x - xFloor;
    const float u = quinticFade(fx);

    const float g0 = (random01(hash32(static_cast<std::uint32_t>(i0) ^ seed)) * 2.0f - 1.0f) * fx;
    const float g1 = (random01(hash32(static_cast<std::uint32_t>(i0 + 1) ^ seed)) * 2.0f - 1.0f) * (fx - 1.0f);
    return std::clamp((g0 + u * (g1 - g0)) * 2.0f, -1.0f, 1.0f);
}

ValueAndGradient2D simplex2DWithDeriv(float x, float z, std::uint32_t seed) noexcept {
    if (!std::isfinite(x) || !std::isfinite(z)) {
        return {};
    }
    const float xFloor = std::floor(x);
    const float zFloor = std::floor(z);
    const int ix = static_cast<int>(xFloor);
    const int iz = static_cast<int>(zFloor);

    const float fx = x - xFloor;
    const float fz = z - zFloor;

    const float u = quinticFade(fx);
    const float v = quinticFade(fz);
    const float du = quinticFadeDeriv(fx);
    const float dv = quinticFadeDeriv(fz);

    float g00x = 0.0f, g00z = 0.0f;
    float g10x = 0.0f, g10z = 0.0f;
    float g01x = 0.0f, g01z = 0.0f;
    float g11x = 0.0f, g11z = 0.0f;

    grad2D(coordHash2D(ix,     iz,     seed), g00x, g00z);
    grad2D(coordHash2D(ix + 1, iz,     seed), g10x, g10z);
    grad2D(coordHash2D(ix,     iz + 1, seed), g01x, g01z);
    grad2D(coordHash2D(ix + 1, iz + 1, seed), g11x, g11z);

    const float n00 = g00x * fx + g00z * fz;
    const float n10 = g10x * (fx - 1.0f) + g10z * fz;
    const float n01 = g01x * fx + g01z * (fz - 1.0f);
    const float n11 = g11x * (fx - 1.0f) + g11z * (fz - 1.0f);

    const float k0 = n00;
    const float k1 = n10 - n00;
    const float k2 = n01 - n00;
    const float k3 = n00 - n10 - n01 + n11;

    constexpr float kNorm = 1.41421356f;
    const float val = (k0 + u * k1 + v * k2 + u * v * k3) * kNorm;

    const float dndx = ((g00x + u * (g10x - g00x) + v * (g01x - g00x) + u * v * (g00x - g10x - g01x + g11x))
                        + du * (k1 + v * k3)) * kNorm;
    const float dndz = ((g00z + u * (g10z - g00z) + v * (g01z - g00z) + u * v * (g00z - g10z - g01z + g11z))
                        + dv * (k2 + u * k3)) * kNorm;

    return {std::clamp(val, -1.0f, 1.0f), dndx, dndz};
}

float simplex2D(float x, float z, std::uint32_t seed) noexcept {
    return simplex2DWithDeriv(x, z, seed).value;
}

float simplex3D(float x, float y, float z, std::uint32_t seed) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
        return 0.0f;
    }
    const float xFloor = std::floor(x);
    const float yFloor = std::floor(y);
    const float zFloor = std::floor(z);
    const int ix = static_cast<int>(xFloor);
    const int iy = static_cast<int>(yFloor);
    const int iz = static_cast<int>(zFloor);

    const float fx = x - xFloor;
    const float fy = y - yFloor;
    const float fz = z - zFloor;

    const float u = quinticFade(fx);
    const float v = quinticFade(fy);
    const float w = quinticFade(fz);

    auto sampleCorner = [&](int dx, int dy, int dz) noexcept -> float {
        const std::uint32_t h = coordHash3D(ix + dx, iy + dy, iz + dz, seed);
        return static_cast<float>(h & 0x00FFFFFFU) * (2.0f / 16777216.0f) - 1.0f;
    };

    const float c000 = sampleCorner(0, 0, 0);
    const float c100 = sampleCorner(1, 0, 0);
    const float c010 = sampleCorner(0, 1, 0);
    const float c110 = sampleCorner(1, 1, 0);
    const float c001 = sampleCorner(0, 0, 1);
    const float c101 = sampleCorner(1, 0, 1);
    const float c011 = sampleCorner(0, 1, 1);
    const float c111 = sampleCorner(1, 1, 1);

    const float x00 = c000 + u * (c100 - c000);
    const float x10 = c010 + u * (c110 - c010);
    const float x01 = c001 + u * (c101 - c001);
    const float x11 = c011 + u * (c111 - c011);

    const float y0 = x00 + v * (x10 - x00);
    const float y1 = x01 + v * (x11 - x01);
    return y0 + w * (y1 - y0);
}

float fbm1D(float x, std::uint32_t octaves, std::uint32_t seed) noexcept {
    float sum = 0.0f;
    float amp = 0.5f;
    float freq = 1.0f;
    float norm = 0.0f;
    const std::uint32_t count = std::clamp(octaves, 1U, 8U);
    for (std::uint32_t i = 0U; i < count; ++i) {
        sum += amp * simplex1D(x * freq, seed + i * 1013U);
        norm += amp;
        amp *= 0.5f;
        freq *= 2.03f;
    }
    return norm > 0.0f ? (sum / norm) : 0.0f;
}

float fbm2D(float x, float z, std::uint32_t octaves, std::uint32_t seed) noexcept {
    float sum = 0.0f;
    float amp = 0.5f;
    float px = x;
    float pz = z;
    float norm = 0.0f;
    const std::uint32_t count = std::clamp(octaves, 1U, 8U);
    for (std::uint32_t i = 0U; i < count; ++i) {
        sum += amp * simplex2D(px, pz, seed + i * 1619U);
        norm += amp;
        amp *= 0.5f;
        // Rotate by ~36.87 deg (cos=0.8, sin=0.6) and scale by 2.02
        const float nx = (0.8f * px - 0.6f * pz) * 2.02f;
        const float nz = (0.6f * px + 0.8f * pz) * 2.02f;
        px = nx;
        pz = nz;
    }
    return norm > 0.0f ? (sum / norm) : 0.0f;
}

float fbm3D(float x, float y, float z, std::uint32_t octaves, std::uint32_t seed) noexcept {
    float sum = 0.0f;
    float amp = 0.5f;
    float freq = 1.0f;
    float norm = 0.0f;
    const std::uint32_t count = std::clamp(octaves, 1U, 8U);
    for (std::uint32_t i = 0U; i < count; ++i) {
        sum += amp * simplex3D(x * freq, y * freq, z * freq, seed + i * 31337U);
        norm += amp;
        amp *= 0.5f;
        freq *= 2.03f;
    }
    return norm > 0.0f ? (sum / norm) : 0.0f;
}

float worley2D(float x, float z, std::uint32_t seed) noexcept {
    if (!std::isfinite(x) || !std::isfinite(z)) {
        return 0.0f;
    }
    const int ix = static_cast<int>(std::floor(x));
    const int iz = static_cast<int>(std::floor(z));
    float minDistSq = 10.0f;

    for (int dz = -1; dz <= 1; ++dz) {
        for (int dx = -1; dx <= 1; ++dx) {
            const std::uint32_t h = coordHash2D(ix + dx, iz + dz, seed);
            float jx = 0.0f, jz = 0.0f;
            random2(h, jx, jz);
            const float px = static_cast<float>(ix + dx) + jx - x;
            const float pz = static_cast<float>(iz + dz) + jz - z;
            minDistSq = std::min(minDistSq, px * px + pz * pz);
        }
    }
    return std::clamp(std::sqrt(minDistSq), 0.0f, 1.0f);
}

float worley3D(float x, float y, float z, std::uint32_t seed) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
        return 0.0f;
    }
    const int ix = static_cast<int>(std::floor(x));
    const int iy = static_cast<int>(std::floor(y));
    const int iz = static_cast<int>(std::floor(z));
    float minDistSq = 10.0f;

    for (int dz = -1; dz <= 1; ++dz) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                const std::uint32_t h = coordHash3D(ix + dx, iy + dy, iz + dz, seed);
                const float jx = random01(h ^ 0x13579BDFU);
                const float jy = random01(h ^ 0x2468ACE0U);
                const float jz = random01(h ^ 0x369CF258U);
                const float px = static_cast<float>(ix + dx) + jx - x;
                const float py = static_cast<float>(iy + dy) + jy - y;
                const float pz = static_cast<float>(iz + dz) + jz - z;
                minDistSq = std::min(minDistSq, px * px + py * py + pz * pz);
            }
        }
    }
    return std::clamp(std::sqrt(minDistSq), 0.0f, 1.0f);
}

SculptureVec3 curlNoise3D(const SculptureVec3& p, float time) noexcept {
    const float vx = std::sin(p.y * 1.2f + time) * std::cos(p.z * 0.8f)
                   + 0.35f * simplex3D(p.x * 0.45f, p.y * 0.45f + time * 0.3f, p.z * 0.45f, 101U);
    const float vy = std::cos(p.x * 1.5f - time) * std::sin(p.z * 1.1f)
                   + 0.35f * simplex3D(p.x * 0.45f, p.y * 0.45f, p.z * 0.45f + time * 0.3f, 202U);
    const float vz = std::sin(p.x * 0.9f + time) * std::cos(p.y * 1.4f)
                   + 0.35f * simplex3D(p.x * 0.45f + time * 0.3f, p.y * 0.45f, p.z * 0.45f, 303U);
    return {vx, vy, vz};
}

float evaluateDirtPathSdf(
    float x,
    float z,
    const ForestFieldConfig& config,
    float* outCenterX) noexcept
{
    const float centerX = config.pathSineAmplitude * std::sin(z * config.pathSineFrequency)
                        + config.pathFbmAmplitude * fbm1D(z * 0.15f, 3U, config.seed ^ 0x11U);
    if (outCenterX != nullptr) {
        *outCenterX = centerX;
    }
    const float edgeIrregularity = config.pathEdgeWarp * fbm2D(x * 1.8f, z * 1.8f, 3U, config.seed ^ 0x22U);
    return std::max(0.0f, std::abs(x - centerX) + edgeIrregularity);
}

float evaluateClearingField(
    float x,
    float z,
    const ForestFieldConfig& config) noexcept
{
    const float a = std::max(config.clearingSemiAxisA, 1e-3f);
    const float b = std::max(config.clearingSemiAxisB, 1e-3f);
    const float qx = (x - config.clearingCenterX) / a;
    const float qz = (z - config.clearingCenterZ) / b;
    const float r = std::sqrt(qx * qx + qz * qz);
    const float irregularity = config.clearingIrregularity * fbm2D(x * 0.045f, z * 0.045f, 3U, config.seed ^ 0x33U);
    return 1.0f - smoothstep01(config.clearingInnerRadius, config.clearingOuterRadius, r + irregularity);
}

float evaluateTerrainHeight(
    float x,
    float z,
    const ForestFieldConfig& config,
    float* outPathMask) noexcept
{
    float pathCenterX = 0.0f;
    const float pathDist = evaluateDirtPathSdf(x, z, config, &pathCenterX);
    const float pathMask = smoothstep01(config.pathInnerRadius, config.pathOuterRadius, pathDist);
    if (outPathMask != nullptr) {
        *outPathMask = pathMask;
    }

    // U-shaped Lichtung clearing valley slopes rising to rolling woodland hills on left and right
    const float dxPath = x - pathCenterX;
    const float dxSq = dxPath * dxPath;
    const float valleySlope = (config.valleyCurvature * dxSq) / (1.0f + 0.0012f * dxSq);

    // Gradient-damped multi-octave hydraulic erosion heightfield
    float h = 0.0f;
    float amp = config.baseAmplitude;
    float freq = config.baseFrequency;
    float gradSumX = 0.0f;
    float gradSumZ = 0.0f;

    float rx = x;
    float rz = z;
    const std::uint32_t octaves = std::clamp(config.erosionOctaves, 1U, 8U);
    for (std::uint32_t i = 0U; i < octaves; ++i) {
        const ValueAndGradient2D nd = simplex2DWithDeriv(rx * freq, rz * freq, config.seed + i * 4099U);
        gradSumX += nd.dx * freq;
        gradSumZ += nd.dz * freq;
        const float gradNormSq = gradSumX * gradSumX + gradSumZ * gradSumZ;
        h += (amp * nd.value) / (1.0f + config.gradientDampingKappa * gradNormSq);

        amp *= 0.48f;
        freq *= 2.15f;
        const float nrx = 0.8f * rx - 0.6f * rz;
        const float nrz = 0.6f * rx + 0.8f * rz;
        rx = nrx;
        rz = nrz;
    }

    // Carve sunken dirt footpath rut and add Worley soil micro-pebbles on the path
    const float onTrail = 1.0f - pathMask;
    const float pathRut = -config.pathRutDepth * onTrail;
    const float soilDetail = 0.015f * worley2D(x * 12.0f, z * 12.0f, config.seed ^ 0x55U) * onTrail;

    return valleySlope + h * (0.25f + 0.75f * pathMask) + pathRut + soilDetail;
}

SculptureVec3 evaluateTerrainNormal(
    float x,
    float z,
    const ForestFieldConfig& config,
    float* outSlope) noexcept
{
    const float eps = std::max(config.cellSize * 0.5f, 0.08f);
    const float hxPos = evaluateTerrainHeight(x + eps, z, config);
    const float hxNeg = evaluateTerrainHeight(x - eps, z, config);
    const float hzPos = evaluateTerrainHeight(x, z + eps, config);
    const float hzNeg = evaluateTerrainHeight(x, z - eps, config);

    const float gx = (hxPos - hxNeg) / (2.0f * eps);
    const float gz = (hzPos - hzNeg) / (2.0f * eps);
    if (outSlope != nullptr) {
        *outSlope = std::sqrt(gx * gx + gz * gz);
    }
    return diff_geom::normalize3({-gx, 1.0f, -gz});
}

ForestWorldCellGpu evaluateWorldCellAt(
    float x,
    float z,
    const ForestFieldConfig& config) noexcept
{
    float pathMask = 1.0f;
    const float h = evaluateTerrainHeight(x, z, config, &pathMask);
    float slope = 0.0f;
    const SculptureVec3 normal = evaluateTerrainNormal(x, z, config, &slope);
    const float clearing = evaluateClearingField(x, z, config);

    // Soil moisture pools in lower elevations and shaded woodland fringes
    const float moisture = std::clamp(
        0.55f + 0.28f * fbm2D(x * 0.035f, z * 0.035f, 3U, config.seed ^ 0x77U) - 0.045f * h,
        0.0f,
        1.0f);

    // Provisional canopy openness prior to tree placement feedback
    const float openness = std::clamp(0.15f + 0.85f * clearing, 0.0f, 1.0f);

    ForestWorldCellGpu cell{};
    cell.height = h;
    cell.clearing = clearing;
    cell.pathMask = pathMask;
    cell.moisture = moisture;
    cell.slope = slope;
    cell.openness = openness;
    cell.normalX = normal.x;
    cell.normalZ = normal.z;
    return cell;
}

ForestFieldGrid generateWorldFieldGrid(const ForestFieldConfig& config) {
    ForestFieldGrid grid{};
    grid.config = config;
    const std::uint32_t w = std::max(1U, config.gridWidth);
    const std::uint32_t h = std::max(1U, config.gridHeight);
    grid.cells.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));

    for (std::uint32_t iz = 0U; iz < h; ++iz) {
        const float worldZ = config.originZ + (static_cast<float>(iz) + 0.5f) * config.cellSize;
        for (std::uint32_t ix = 0U; ix < w; ++ix) {
            const float worldX = config.originX + (static_cast<float>(ix) + 0.5f) * config.cellSize;
            grid.cells[static_cast<std::size_t>(iz) * w + ix] =
                evaluateWorldCellAt(worldX, worldZ, config);
        }
    }
    return grid;
}

ForestWorldCellGpu sampleWorldCell(
    const ForestFieldGrid& grid,
    float worldX,
    float worldZ) noexcept
{
    if (grid.empty() || grid.config.gridWidth == 0U || grid.config.gridHeight == 0U) {
        return evaluateWorldCellAt(worldX, worldZ, grid.config);
    }

    const float gx = (worldX - grid.config.originX) / std::max(grid.config.cellSize, 1e-4f) - 0.5f;
    const float gz = (worldZ - grid.config.originZ) / std::max(grid.config.cellSize, 1e-4f) - 0.5f;

    const int maxX = static_cast<int>(grid.config.gridWidth) - 1;
    const int maxZ = static_cast<int>(grid.config.gridHeight) - 1;

    const int x0 = std::clamp(static_cast<int>(std::floor(gx)), 0, maxX);
    const int z0 = std::clamp(static_cast<int>(std::floor(gz)), 0, maxZ);
    const int x1 = std::min(x0 + 1, maxX);
    const int z1 = std::min(z0 + 1, maxZ);

    const float tx = std::clamp(gx - std::floor(gx), 0.0f, 1.0f);
    const float tz = std::clamp(gz - std::floor(gz), 0.0f, 1.0f);

    const std::size_t stride = grid.config.gridWidth;
    const ForestWorldCellGpu& c00 = grid.cells[static_cast<std::size_t>(z0) * stride + static_cast<std::size_t>(x0)];
    const ForestWorldCellGpu& c10 = grid.cells[static_cast<std::size_t>(z0) * stride + static_cast<std::size_t>(x1)];
    const ForestWorldCellGpu& c01 = grid.cells[static_cast<std::size_t>(z1) * stride + static_cast<std::size_t>(x0)];
    const ForestWorldCellGpu& c11 = grid.cells[static_cast<std::size_t>(z1) * stride + static_cast<std::size_t>(x1)];

    auto bilerp = [&](float v00, float v10, float v01, float v11) noexcept -> float {
        const float a = v00 + tx * (v10 - v00);
        const float b = v01 + tx * (v11 - v01);
        return a + tz * (b - a);
    };

    ForestWorldCellGpu out{};
    out.height   = bilerp(c00.height,   c10.height,   c01.height,   c11.height);
    out.clearing = bilerp(c00.clearing, c10.clearing, c01.clearing, c11.clearing);
    out.pathMask = bilerp(c00.pathMask, c10.pathMask, c01.pathMask, c11.pathMask);
    out.moisture = bilerp(c00.moisture, c10.moisture, c01.moisture, c11.moisture);
    out.slope    = bilerp(c00.slope,    c10.slope,    c01.slope,    c11.slope);
    out.openness = bilerp(c00.openness, c10.openness, c01.openness, c11.openness);
    out.normalX  = bilerp(c00.normalX,  c10.normalX,  c01.normalX,  c11.normalX);
    out.normalZ  = bilerp(c00.normalZ,  c10.normalZ,  c01.normalZ,  c11.normalZ);
    return out;
}

float evaluateCanopyShadowTransmittance(
    float worldX,
    float worldY,
    float worldZ,
    std::span<const ForestTreeGpu> trees,
    const SculptureVec3& sunDirection,
    std::uint32_t seed) noexcept
{
    const SculptureVec3 sunDir = diff_geom::normalize3(sunDirection, {0.42f, 0.78f, 0.46f});
    const float sunY = std::max(sunDir.y, 0.15f);

    float transmittance = 1.0f;
    for (const ForestTreeGpu& tree : trees) {
        const float crownX = tree.canopyCenterRadius[0];
        const float crownY = tree.canopyCenterRadius[1];
        const float crownZ = tree.canopyCenterRadius[2];
        const float crownR = std::max(tree.canopyCenterRadius[3], 0.1f);

        if (crownY <= worldY) {
            continue;
        }
        // Project crown shadow along sun ray onto horizontal plane at worldY
        const float tRay = (crownY - worldY) / sunY;
        const float projX = worldX + sunDir.x * tRay;
        const float projZ = worldZ + sunDir.z * tRay;

        const float dx = projX - crownX;
        const float dz = projZ - crownZ;
        const float distSq = dx * dx + dz * dz;
        const float radiusSq = crownR * crownR;
        if (distSq < radiusSq * 1.44f) {
            const float rNorm = std::sqrt(distSq) / crownR;
            const float crownCoverage = 1.0f - smoothstep01(0.45f, 1.2f, rNorm);
            // Dappled leaf gaps inside the crown silhouette
            const float leafGap = 0.22f + 0.38f * smoothstep01(
                -0.2f,
                0.35f,
                fbm2D(projX * 0.85f, projZ * 0.85f, 3U, seed ^ 0x99U));
            transmittance *= (1.0f - crownCoverage * (1.0f - leafGap));
        }
    }
    return std::clamp(transmittance, 0.08f, 1.0f);
}

void applyCanopyCoverageFeedback(
    ForestFieldGrid& grid,
    std::span<const ForestTreeGpu> trees,
    const SculptureVec3& sunDirection) noexcept
{
    if (grid.empty() || trees.empty()) {
        return;
    }
    const std::uint32_t w = grid.config.gridWidth;
    const std::uint32_t h = grid.config.gridHeight;

    for (std::uint32_t iz = 0U; iz < h; ++iz) {
        const float worldZ = grid.config.originZ + (static_cast<float>(iz) + 0.5f) * grid.config.cellSize;
        for (std::uint32_t ix = 0U; ix < w; ++ix) {
            const float worldX = grid.config.originX + (static_cast<float>(ix) + 0.5f) * grid.config.cellSize;
            ForestWorldCellGpu& cell = grid.cells[static_cast<std::size_t>(iz) * w + ix];
            const float canopyTrans = evaluateCanopyShadowTransmittance(
                worldX,
                cell.height,
                worldZ,
                trees,
                sunDirection,
                grid.config.seed);
            const float baseOpenness = std::clamp(0.15f + 0.85f * cell.clearing, 0.0f, 1.0f);
            cell.openness = std::clamp(baseOpenness * (0.25f + 0.75f * canopyTrans), 0.05f, 1.0f);
            // Shaded ground under dense tree canopies retains slightly higher soil moisture
            cell.moisture = std::clamp(cell.moisture + 0.12f * (1.0f - canopyTrans), 0.0f, 1.0f);
        }
    }
}

} // namespace epi_sculpt::forest
