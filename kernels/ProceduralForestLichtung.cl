/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - PROCEDURAL FOREST LICHTUNG OPENCL KERNEL MODULE
 * ============================================================================
 *
 * Implements the GPU-parallel Algorithmic Nature / Waldlichtung pipeline:
 *   1. generate_forest_world_cells:
 *      Gradient-damped hydraulic erosion terrain H(x,z), irregular canopy
 *      clearing L(x,z), winding dirt footpath spline M_path(x,z), moisture,
 *      slope, and surface normals.
 *   2. scatter_forest_instances:
 *      Ecologically weighted Poisson-disk / jittered grid instance scatterer
 *      across Trees, Boulders, Shrubs, Grass, Rooted Wildflowers, Ferns, Debris.
 *   3. update_rooted_flowers:
 *      Euler-Bernoulli cantilever stem sway and cosine spectral HDR petal
 *      bioluminescence for stationary rooted wildflowers.
 *   4. accumulate_ground_bounce_irradiance:
 *      2D world-space diffuse bounce irradiance map accumulating colored light
 *      from nearby glowing flower heads onto the surrounding terrain and ferns.
 *   5. update_swirling_petals:
 *      Euler-Cromer particle-fluid simulation with 3D curl-noise wind
 *      turbulence, drag, gravity, flutter, and pink-to-violet chromodynamics.
 *   6. render_lichtung_scene:
 *      Full-scene raymarcher (eroded valley, dirt path, Worley-weathered mossy
 *      boulders, fern undergrowth, and 5-fold cupped rooted flower petals) with
 *      botanical thin-slab SSS, colored bounce GI, and Henyey-Greenstein
 *      (g = 0.74) volumetric valley mist.
 * ============================================================================
 */

// ----------------------------------------------------------------------------
// 1. ABI Structures (Mirrors 3dEpiSculpTMorph/ProceduralForestLichtungAbi.h)
// ----------------------------------------------------------------------------
typedef struct {
    float height;
    float clearing;
    float pathMask;
    float moisture;
    float slope;
    float openness;
    float normalX;
    float normalZ;
} ForestWorldCellGpu;

typedef struct {
    float posX;
    float posY;
    float posZ;
    float scale;
    uint kind;
    uint species;
    uint seed;
    uint valid;
} ForestScatterInstanceGpu;

typedef struct {
    float4 root_pos_scale;   // xyz = rooted ground position, w = stem height
    float4 head_pos_radius;  // xyz = current bent blossom center, w = blossom radius
    float4 head_axis;        // xyz = drooping blossom orientation, w = stiffness
    float4 glow_color_phase; // rgb = current multi-color petal emission, w = hue_seed
} ForestGroundFlowerGpu;

typedef struct {
    float4 center_radius;    // xyz = boulder position, w = base radius
    float4 stretch_seed;     // xyz = ellipsoid scale, w = noise seed
} ForestMossyBoulderGpu;

typedef struct {
    float4 base_radius;      // xyz = tree root position, w = trunk radius
    float4 canopy_center;    // xyz = foliage crown center, w = crown radius
} ForestTreeGpu;

typedef struct {
    float4 position;         // xyz = world position, w = lifetime age (0.0 to 1.0)
    float4 velocity;         // xyz = velocity, w = aerodynamic drag alpha
    float4 base_color;       // rgb = chromodynamic color, w = unique seed
} ForestSwirlingPetalGpu;

typedef struct {
    float2 worldExtentXZ;
    float2 clearingCenterXZ;
    float2 clearingSemiAxesXZ;
    float2 windVelocityXZ;
    float4 sunDirection;
    float4 sunRadiance;      // rgb = sun radiance, w = Henyey-Greenstein g
    float4 mistParams;       // x = base density, y = height falloff, z = petal HDR gain, w = time
} ForestLichtungDescriptorGpu;

#define FOREST_KIND_TREE       0u
#define FOREST_KIND_BOULDER    1u
#define FOREST_KIND_SHRUB      2u
#define FOREST_KIND_GRASS      3u
#define FOREST_KIND_FLOWER     4u
#define FOREST_KIND_FERN       5u
#define FOREST_KIND_DEBRIS     6u

#define MAT_SKY            0
#define MAT_DIRT_PATH      1
#define MAT_FOREST_FLOOR   2
#define MAT_MOSSY_BOULDER  3
#define MAT_TREE_BARK      4
#define MAT_FERN_FOLIAGE   5
#define MAT_FLOWER_PETAL   6
#define MAT_CANOPY_FOLIAGE 7

// ----------------------------------------------------------------------------
// 2. Deterministic Hashing, Noise, Analytic Derivatives, Worley & Curl Noise
// ----------------------------------------------------------------------------
inline uint forest_hash32(uint x) {
    x ^= x >> 16u;
    x *= 0x7feb352du;
    x ^= x >> 15u;
    x *= 0x846ca68bu;
    x ^= x >> 16u;
    return x;
}

inline float forest_random01(uint seed) {
    return (float)(forest_hash32(seed) & 0x00FFFFFFu) * (1.0f / 16777216.0f);
}

inline float2 forest_random2(uint seed) {
    return (float2)(
        forest_random01(seed ^ 0xA341316Cu),
        forest_random01(seed ^ 0xC8013EA4u));
}

inline float forest_quintic(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

inline float forest_quintic_deriv(float t) {
    return 30.0f * t * t * (t * (t - 2.0f) + 1.0f);
}

inline uint forest_coord_hash2d(int ix, int iz, uint seed) {
    return forest_hash32(seed ^ ((uint)ix * 73856093u) ^ ((uint)iz * 19349663u));
}

inline uint forest_coord_hash3d(int ix, int iy, int iz, uint seed) {
    return forest_hash32(seed ^ ((uint)ix * 73856093u) ^ ((uint)iy * 19349663u) ^ ((uint)iz * 83492791u));
}

inline float2 forest_grad2d(uint h) {
    const uint idx = h & 7u;
    if (idx == 0u) return (float2)( 1.0f,  0.0f);
    if (idx == 1u) return (float2)(-1.0f,  0.0f);
    if (idx == 2u) return (float2)( 0.0f,  1.0f);
    if (idx == 3u) return (float2)( 0.0f, -1.0f);
    if (idx == 4u) return (float2)( 0.70710678f,  0.70710678f);
    if (idx == 5u) return (float2)(-0.70710678f,  0.70710678f);
    if (idx == 6u) return (float2)( 0.70710678f, -0.70710678f);
    return (float2)(-0.70710678f, -0.70710678f);
}

// Returns x = value in [-1, 1], yz = analytic gradient (dn/dx, dn/dz)
inline float3 simplex2D_deriv(float2 xz, uint seed) {
    float2 fl = floor(xz);
    int ix = (int)fl.x;
    int iz = (int)fl.y;
    float2 f = xz - fl;

    float u = forest_quintic(f.x);
    float v = forest_quintic(f.y);
    float du = forest_quintic_deriv(f.x);
    float dv = forest_quintic_deriv(f.y);

    float2 g00 = forest_grad2d(forest_coord_hash2d(ix,     iz,     seed));
    float2 g10 = forest_grad2d(forest_coord_hash2d(ix + 1, iz,     seed));
    float2 g01 = forest_grad2d(forest_coord_hash2d(ix,     iz + 1, seed));
    float2 g11 = forest_grad2d(forest_coord_hash2d(ix + 1, iz + 1, seed));

    float n00 = dot(g00, f);
    float n10 = dot(g10, f - (float2)(1.0f, 0.0f));
    float n01 = dot(g01, f - (float2)(0.0f, 1.0f));
    float n11 = dot(g11, f - (float2)(1.0f, 1.0f));

    float k0 = n00;
    float k1 = n10 - n00;
    float k2 = n01 - n00;
    float k3 = n00 - n10 - n01 + n11;

    float norm = 1.41421356f;
    float val = clamp((k0 + u * k1 + v * k2 + u * v * k3) * norm, -1.0f, 1.0f);
    float2 dndxz = (g00 + u * (g10 - g00) + v * (g01 - g00) + u * v * (g00 - g10 - g01 + g11)
                    + (float2)(du * (k1 + v * k3), dv * (k2 + u * k3))) * norm;
    return (float3)(val, dndxz.x, dndxz.y);
}

inline float fbm1D(float x) {
    float sum = 0.0f, amp = 0.5f, freq = 1.0f, norm = 0.0f;
    for (int i = 0; i < 3; ++i) {
        float fl = floor(x * freq);
        int i0 = (int)fl;
        float fx = x * freq - fl;
        float u = forest_quintic(fx);
        uint s = 0x57414C44u + (uint)i * 1013u;
        float g0 = (forest_random01(forest_hash32((uint)i0 ^ s)) * 2.0f - 1.0f) * fx;
        float g1 = (forest_random01(forest_hash32((uint)(i0 + 1) ^ s)) * 2.0f - 1.0f) * (fx - 1.0f);
        sum += amp * clamp((g0 + u * (g1 - g0)) * 2.0f, -1.0f, 1.0f);
        norm += amp;
        amp *= 0.5f;
        freq *= 2.03f;
    }
    return sum / fmax(norm, 1e-5f);
}

inline float fbm2D(float2 xz) {
    float sum = 0.0f, amp = 0.5f, norm = 0.0f;
    float2 p = xz;
    for (int i = 0; i < 3; ++i) {
        sum += amp * simplex2D_deriv(p, 0x57414C44u + (uint)i * 1619u).x;
        norm += amp;
        amp *= 0.5f;
        p = (float2)(0.8f * p.x - 0.6f * p.y, 0.6f * p.x + 0.8f * p.y) * 2.02f;
    }
    return sum / fmax(norm, 1e-5f);
}

inline float simplex3D_val(float3 p, uint seed) {
    float3 fl = floor(p);
    int ix = (int)fl.x;
    int iy = (int)fl.y;
    int iz = (int)fl.z;
    float3 f = p - fl;
    float3 u = (float3)(forest_quintic(f.x), forest_quintic(f.y), forest_quintic(f.z));

    float c000 = forest_random01(forest_coord_hash3d(ix,     iy,     iz,     seed)) * 2.0f - 1.0f;
    float c100 = forest_random01(forest_coord_hash3d(ix + 1, iy,     iz,     seed)) * 2.0f - 1.0f;
    float c010 = forest_random01(forest_coord_hash3d(ix,     iy + 1, iz,     seed)) * 2.0f - 1.0f;
    float c110 = forest_random01(forest_coord_hash3d(ix + 1, iy + 1, iz,     seed)) * 2.0f - 1.0f;
    float c001 = forest_random01(forest_coord_hash3d(ix,     iy,     iz + 1, seed)) * 2.0f - 1.0f;
    float c101 = forest_random01(forest_coord_hash3d(ix + 1, iy,     iz + 1, seed)) * 2.0f - 1.0f;
    float c011 = forest_random01(forest_coord_hash3d(ix,     iy + 1, iz + 1, seed)) * 2.0f - 1.0f;
    float c111 = forest_random01(forest_coord_hash3d(ix + 1, iy + 1, iz + 1, seed)) * 2.0f - 1.0f;

    float x00 = mix(c000, c100, u.x);
    float x10 = mix(c010, c110, u.x);
    float x01 = mix(c001, c101, u.x);
    float x11 = mix(c011, c111, u.x);
    return mix(mix(x00, x10, u.y), mix(x01, x11, u.y), u.z);
}

inline float fbm3D(float3 p) {
    float sum = 0.0f, amp = 0.5f, freq = 1.0f, norm = 0.0f;
    for (int i = 0; i < 3; ++i) {
        sum += amp * simplex3D_val(p * freq, 0x57414C44u + (uint)i * 31337u);
        norm += amp;
        amp *= 0.5f;
        freq *= 2.03f;
    }
    return sum / fmax(norm, 1e-5f);
}

inline float worley2D(float2 xz) {
    float2 fl = floor(xz);
    int ix = (int)fl.x;
    int iz = (int)fl.y;
    float minDistSq = 10.0f;
    for (int dz = -1; dz <= 1; ++dz) {
        for (int dx = -1; dx <= 1; ++dx) {
            uint h = forest_coord_hash2d(ix + dx, iz + dz, 0x57414C44u);
            float2 j = forest_random2(h);
            float2 d = (float2)((float)(ix + dx), (float)(iz + dz)) + j - xz;
            minDistSq = fmin(minDistSq, dot(d, d));
        }
    }
    return clamp(sqrt(minDistSq), 0.0f, 1.0f);
}

inline float worley3D(float3 p) {
    float3 fl = floor(p);
    int ix = (int)fl.x;
    int iy = (int)fl.y;
    int iz = (int)fl.z;
    float minDistSq = 10.0f;
    for (int dz = -1; dz <= 1; ++dz) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                uint h = forest_coord_hash3d(ix + dx, iy + dy, iz + dz, 0x57414C44u);
                float3 j = (float3)(
                    forest_random01(h ^ 0x13579BDFu),
                    forest_random01(h ^ 0x2468ACE0u),
                    forest_random01(h ^ 0x369CF258u));
                float3 d = (float3)((float)(ix + dx), (float)(iy + dy), (float)(iz + dz)) + j - p;
                minDistSq = fmin(minDistSq, dot(d, d));
            }
        }
    }
    return clamp(sqrt(minDistSq), 0.0f, 1.0f);
}

inline float3 curl_noise(float3 p, float time) {
    float x = sin(p.y * 1.2f + time) * cos(p.z * 0.8f);
    float y = cos(p.x * 1.5f - time) * sin(p.z * 1.1f);
    float z = sin(p.x * 0.9f + time) * cos(p.y * 1.4f);
    return (float3)(x, y, z);
}

// ----------------------------------------------------------------------------
// 3. Spectral Palettes & Scene SDF Evaluators
// ----------------------------------------------------------------------------
inline float3 spectral_petal_color(float hue_seed, float petal_u, float time) {
    float phase = hue_seed + 0.18f * petal_u + 0.08f * sin(time * 0.7f + hue_seed * 6.2831853f);
    float3 a = (float3)(0.55f, 0.50f, 0.60f);
    float3 b = (float3)(0.45f, 0.48f, 0.40f);
    float3 c = (float3)(1.00f, 1.00f, 1.00f);
    float3 d = (float3)(0.00f, 0.33f, 0.67f);
    return clamp(a + b * cos(6.2831853f * (c * phase + d)), 0.0f, 1.0f);
}

inline float evaluate_dirt_path_sdf(float2 xz, float* out_center_x) {
    float cx = 2.2f * sin(xz.y * 0.06f) + 0.8f * fbm1D(xz.y * 0.15f);
    *out_center_x = cx;
    float edge_irregularity = 0.35f * fbm2D(xz * 1.8f);
    return fmax(0.0f, fabs(xz.x - cx) + edge_irregularity);
}

inline float evaluate_clearing_field(float2 xz, float2 center, float2 semi_axes) {
    float2 q = (xz - center) / fmax(semi_axes, (float2)(1e-3f, 1e-3f));
    float r = length(q);
    float irregularity = 0.18f * fbm2D(xz * 0.045f);
    return 1.0f - smoothstep(0.65f, 1.25f, r + irregularity);
}

inline float evaluate_terrain_height(float2 xz, float* out_path_mask) {
    float path_cx = 0.0f;
    float path_dist = evaluate_dirt_path_sdf(xz, &path_cx);
    float path_mask = smoothstep(0.75f, 1.65f, path_dist);
    *out_path_mask = path_mask;

    float dx_path = xz.x - path_cx;
    float dx_sq = dx_path * dx_path;
    float valley_slope = (0.012f * dx_sq) / (1.0f + 0.0012f * dx_sq);

    float h = 0.0f, amp = 2.2f, freq = 0.045f;
    float2 grad_sum = (float2)(0.0f, 0.0f);
    float2 rxz = xz;
    for (int i = 0; i < 4; ++i) {
        float3 n_deriv = simplex2D_deriv(rxz * freq, 0x57414C44u + (uint)i * 4099u);
        grad_sum += n_deriv.yz * freq;
        h += (amp * n_deriv.x) / (1.0f + 1.8f * dot(grad_sum, grad_sum));
        amp *= 0.48f;
        freq *= 2.15f;
        rxz = (float2)(0.8f * rxz.x - 0.6f * rxz.y, 0.6f * rxz.x + 0.8f * rxz.y);
    }

    float on_trail = 1.0f - path_mask;
    float path_rut = -0.22f * on_trail;
    float soil_detail = 0.015f * worley2D(xz * 12.0f) * on_trail;
    return valley_slope + h * (0.25f + 0.75f * path_mask) + path_rut + soil_detail;
}

inline float evaluate_boulders_sdf(float3 p, __global const ForestMossyBoulderGpu* boulders, int num_boulders) {
    float d_min = 1e5f;
    for (int i = 0; i < num_boulders; ++i) {
        float3 c = boulders[i].center_radius.xyz;
        float  r = fmax(boulders[i].center_radius.w, 0.05f);
        float3 scale = fmax(boulders[i].stretch_seed.xyz, (float3)(0.1f));
        float3 q = (p - c) / scale;
        float d_base = (length(q) - r) * fmin(scale.x, fmin(scale.y, scale.z));
        if (d_base < 1.2f) {
            float rock_weathering = 0.28f * worley3D(p * 0.7f) + 0.06f * fbm3D(p * 3.5f);
            d_base += rock_weathering;
        }
        d_min = fmin(d_min, d_base);
    }
    return d_min;
}

inline float evaluate_rooted_flower_petals_sdf(
    float3 p,
    __global const ForestGroundFlowerGpu* flowers,
    int num_flowers,
    int* out_flower_idx,
    float2* out_petal_uv)
{
    float d_min = 1e5f;
    const float sector = 6.2831853f / 5.0f;
    for (int i = 0; i < num_flowers; ++i) {
        float4 head = flowers[i].head_pos_radius;
        float3 to_p = p - head.xyz;
        if (dot(to_p, to_p) > 0.16f) continue;

        float3 axis_y = normalize(flowers[i].head_axis.xyz);
        float3 ref_v = fabs(axis_y.z) < 0.9f ? (float3)(0.0f, 0.0f, 1.0f) : (float3)(1.0f, 0.0f, 0.0f);
        float3 axis_x = normalize(cross(axis_y, ref_v));
        float3 axis_z = cross(axis_x, axis_y);
        float3 local_p = (float3)(dot(to_p, axis_x), dot(to_p, axis_y), dot(to_p, axis_z));

        float angle = atan2(local_p.z, local_p.x);
        float wrapped = fmod(angle + 3.14159265f + sector * 0.5f, sector);
        if (wrapped < 0.0f) wrapped += sector;
        float local_angle = wrapped - sector * 0.5f;

        float r_xz = length(local_p.xz);
        float u = clamp(r_xz / fmax(head.w, 0.01f), 0.0f, 1.0f);
        float v = clamp(local_angle / (sector * 0.5f), -1.0f, 1.0f);

        float cup_y = -0.08f * sin(u * 1.8f) + 0.02f * u * u;
        float petal_width = sin(u * 3.14159265f * 0.85f);
        float d_petal = length((float2)(local_p.y - cup_y, fmax(0.0f, fabs(v) - petal_width) * 0.04f)) - 0.004f;

        if (d_petal < d_min) {
            d_min = d_petal;
            *out_flower_idx = i;
            *out_petal_uv = (float2)(u, v);
        }
    }
    return d_min;
}

// ----------------------------------------------------------------------------
// KERNEL 1: GENERATE WORLD FIELD CELLS
// ----------------------------------------------------------------------------
__kernel void generate_forest_world_cells(
    __global ForestWorldCellGpu* world,
    const uint width,
    const uint height,
    const float cellSize,
    const float2 originXZ,
    const float2 clearingCenterXZ,
    const float2 clearingSemiAxesXZ)
{
    uint id = get_global_id(0);
    if (id >= width * height) return;

    uint ix = id % width;
    uint iz = id / width;
    float2 p = originXZ + ((float2)((float)ix + 0.5f, (float)iz + 0.5f)) * cellSize;

    float path_mask = 1.0f;
    float h = evaluate_terrain_height(p, &path_mask);

    float dummy_mask = 1.0f;
    float eps = fmax(cellSize * 0.5f, 0.08f);
    float gx = (evaluate_terrain_height(p + (float2)(eps, 0.0f), &dummy_mask) -
                evaluate_terrain_height(p - (float2)(eps, 0.0f), &dummy_mask)) / (2.0f * eps);
    float gz = (evaluate_terrain_height(p + (float2)(0.0f, eps), &dummy_mask) -
                evaluate_terrain_height(p - (float2)(0.0f, eps), &dummy_mask)) / (2.0f * eps);

    float slope = length((float2)(gx, gz));
    float3 n = normalize((float3)(-gx, 1.0f, -gz));
    float clearing = evaluate_clearing_field(p, clearingCenterXZ, clearingSemiAxesXZ);
    float moisture = clamp(0.55f + 0.28f * fbm2D(p * 0.035f) - 0.045f * h, 0.0f, 1.0f);
    float openness = clamp(0.15f + 0.85f * clearing, 0.0f, 1.0f);

    ForestWorldCellGpu c;
    c.height = h;
    c.clearing = clearing;
    c.pathMask = path_mask;
    c.moisture = moisture;
    c.slope = slope;
    c.openness = openness;
    c.normalX = n.x;
    c.normalZ = n.z;
    world[id] = c;
}

// ----------------------------------------------------------------------------
// KERNEL 2: ECOLOGICALLY WEIGHTED INSTANCE SCATTERING
// ----------------------------------------------------------------------------
inline float ecological_density_gpu(uint kind, ForestWorldCellGpu c, float patchNoise) {
    float trailMask = clamp(c.pathMask, 0.0f, 1.0f);
    float slopeMask = 1.0f - smoothstep(0.25f, 0.85f, c.slope);
    float patch = 0.25f + 0.75f * smoothstep(0.2f, 0.8f, 0.5f + 0.5f * patchNoise);
    float shade = clamp(1.0f - c.openness, 0.0f, 1.0f);
    float moist = 0.45f + 0.55f * clamp(c.moisture, 0.0f, 1.0f);

    if (kind == FOREST_KIND_TREE)
        return 0.045f * fmax(0.0f, 1.0f - 0.96f * c.clearing) * moist * slopeMask * trailMask * patch;
    if (kind == FOREST_KIND_BOULDER)
        return 0.025f * (0.35f + 0.65f * (1.0f - fabs(2.0f * c.clearing - 1.0f))) * slopeMask * trailMask * patch;
    if (kind == FOREST_KIND_SHRUB)
        return 0.35f * (0.45f + 0.55f * shade) * moist * slopeMask * trailMask * patch;
    if (kind == FOREST_KIND_GRASS)
        return 8.0f * (0.40f + 0.60f * c.openness) * moist * slopeMask * trailMask * patch;
    if (kind == FOREST_KIND_FLOWER)
        return 1.80f * (0.20f + 0.80f * c.openness) * moist * slopeMask * smoothstep(0.25f, 0.65f, trailMask) * patch;
    if (kind == FOREST_KIND_FERN)
        return 1.40f * (0.35f + 0.65f * shade) * (0.30f + 0.70f * c.moisture) * slopeMask * smoothstep(0.30f, 0.75f, trailMask) * patch;
    if (kind == FOREST_KIND_DEBRIS)
        return 1.20f * (0.50f + 0.50f * shade) * slopeMask * patch;
    return 0.0f;
}

__kernel void scatter_forest_instances(
    __global const ForestWorldCellGpu* world,
    const uint worldWidth,
    const uint worldHeight,
    const float worldCellSize,
    const float2 originXZ,
    __global ForestScatterInstanceGpu* outInstances,
    const uint scatterWidth,
    const uint scatterHeight,
    const float scatterCellSize,
    const uint kind,
    const uint sceneSeed)
{
    uint id = get_global_id(0);
    if (id >= scatterWidth * scatterHeight) return;

    uint ix = id % scatterWidth;
    uint iz = id / scatterWidth;

    uint seed = forest_hash32(
        sceneSeed ^ forest_hash32(ix)
        ^ forest_hash32(iz * 73856093u)
        ^ forest_hash32(kind * 19349663u));

    float2 jitter = forest_random2(seed);
    float2 p = originXZ + ((float2)((float)ix, (float)iz) + jitter) * scatterCellSize;

    float2 gridCoord = clamp((p - originXZ) / fmax(worldCellSize, 1e-4f), (float2)(0.0f), (float2)((float)(worldWidth - 1u), (float)(worldHeight - 1u)));
    uint wx = min((uint)gridCoord.x, worldWidth - 1u);
    uint wz = min((uint)gridCoord.y, worldHeight - 1u);
    ForestWorldCellGpu c = world[wz * worldWidth + wx];

    float patchNoise = fbm2D(p * 0.11f + (float2)((float)kind * 13.7f, 5.0f));
    float density = ecological_density_gpu(kind, c, patchNoise);
    float probability = 1.0f - exp(-density * scatterCellSize * scatterCellSize);

    ForestScatterInstanceGpu v;
    v.posX = p.x;
    v.posY = c.height;
    v.posZ = p.y;
    v.scale = 1.0f;
    v.kind = kind;
    v.species = 0u;
    v.seed = seed;
    v.valid = 0u;

    if (forest_random01(seed + 13u) < probability) {
        v.scale = 0.8f + 0.4f * forest_random01(seed + 17u);
        v.species = forest_hash32(seed + 31u) % 8u;
        v.valid = 1u;
    }
    outInstances[id] = v;
}

// ----------------------------------------------------------------------------
// KERNEL 3: UPDATE ROOTED FLOWERS (CANTILEVER SWAY & HDR BIOLUMINESCENCE)
// ----------------------------------------------------------------------------
__kernel void update_rooted_flowers(
    __global ForestGroundFlowerGpu* flowers,
    const uint num_flowers,
    const float2 wind_velocity,
    const float time,
    const float max_hdr_intensity)
{
    uint gid = get_global_id(0);
    if (gid >= num_flowers) return;

    ForestGroundFlowerGpu fl = flowers[gid];
    float3 root = fl.root_pos_scale.xyz;
    float  stem_len = fmax(fl.root_pos_scale.w, 0.02f);
    float  stiffness = fmax(fl.head_axis.w, 0.25f);
    float  hue_seed = fl.glow_color_phase.w;

    // 1. Rooted Stem Sway (Euler-Bernoulli Cantilever Deflection)
    float gust = 0.7f + 0.3f * fbm2D(root.xz * 0.2f - wind_velocity * time);
    float osc = 0.08f * sin(time * 4.5f + hue_seed * 20.0f);
    float2 sway_xz = (wind_velocity * gust + (float2)(osc, osc * 0.7f)) / stiffness;
    float2 tip_offset_xz = sway_xz * stem_len;
    float  tip_drop_y = -0.6f * dot(tip_offset_xz, tip_offset_xz) / stem_len;

    fl.head_pos_radius.xyz = root + (float3)(tip_offset_xz.x, stem_len + tip_drop_y, tip_offset_xz.y);
    fl.head_axis.xyz = normalize((float3)(tip_offset_xz.x * 1.8f, -0.65f, tip_offset_xz.y * 1.8f));

    // 2. Multi-Color Attached Petal Glow Calculation
    float3 base_spectral_rgb = spectral_petal_color(hue_seed, 0.7f, time);
    float  s = 0.5f + 0.5f * sin(time * 2.2f + hue_seed * 15.0f);
    float  breathing_pulse = 0.35f + 0.65f * (s * s);
    fl.glow_color_phase.xyz = base_spectral_rgb * (breathing_pulse * max_hdr_intensity);

    flowers[gid] = fl;
}

// ----------------------------------------------------------------------------
// KERNEL 4: ACCUMULATE COLORED PETAL BOUNCE IRRADIANCE ON TERRAIN
// ----------------------------------------------------------------------------
__kernel void accumulate_ground_bounce_irradiance(
    __global const ForestGroundFlowerGpu* flowers,
    const uint num_flowers,
    __global float4* ground_bounce_irradiance_map,
    const uint grid_res,
    const float2 origin_xz,
    const float2 extent_xz)
{
    uint gid = get_global_id(0);
    if (gid >= grid_res * grid_res) return;

    uint ix = gid % grid_res;
    uint iz = gid / grid_res;

    float2 uv = ((float2)((float)ix + 0.5f, (float)iz + 0.5f)) / (float)grid_res;
    float2 world_xz = origin_xz + uv * extent_xz;
    float path_mask = 1.0f;
    float ground_y = evaluate_terrain_height(world_xz, &path_mask);
    float3 ground_p = (float3)(world_xz.x, ground_y, world_xz.y);

    float3 accumulated_bounce = (float3)(0.0f);
    for (uint i = 0u; i < num_flowers; ++i) {
        float3 f_pos = flowers[i].head_pos_radius.xyz;
        float3 diff = f_pos - ground_p;
        float dist2 = dot(diff, diff);
        if (dist2 < 12.0f) {
            float atten = 1.0f / (1.0f + 2.5f * dist2 * dist2);
            accumulated_bounce += flowers[i].glow_color_phase.xyz * atten;
        }
    }
    ground_bounce_irradiance_map[gid] = (float4)(accumulated_bounce, 1.0f);
}

// ----------------------------------------------------------------------------
// KERNEL 5: SWIRLING FLOWER PETAL FLURRY (EULER-CROMER + CURL NOISE)
// ----------------------------------------------------------------------------
__kernel void update_swirling_petals(
    __global ForestSwirlingPetalGpu* petals,
    const float dt,
    const float time,
    const float4 wind_ambient,
    const uint num_petals)
{
    uint gid = get_global_id(0);
    if (gid >= num_petals) return;

    ForestSwirlingPetalGpu petal = petals[gid];
    float3 pos = petal.position.xyz;
    float age = petal.position.w;
    float3 vel = petal.velocity.xyz;
    float drag = petal.velocity.w;
    float seed = petal.base_color.w;

    age += dt * 0.15f;
    if (age > 1.0f) {
        float hash_val = sin((float)(gid + 1u) * 12.9898f + time * 0.1f) * 43758.5453f;
        float f0 = hash_val - floor(hash_val);
        float f1 = (hash_val * 1.414f) - floor(hash_val * 1.414f);
        float f2 = (hash_val * 3.14f) - floor(hash_val * 3.14f);
        pos.x = f0 * 100.0f - 50.0f;
        pos.z = f1 * 100.0f - 50.0f;
        pos.y = 25.0f + f2 * 15.0f;
        vel = (float3)(0.0f, -1.0f - f0 * 2.0f, 0.0f);
        age = 0.0f;
    }

    float3 turbulence = curl_noise(pos * 0.1f, time * 0.5f) * 2.5f;
    float3 current_wind = wind_ambient.xyz + turbulence;
    float3 gravity = (float3)(0.0f, -1.5f, 0.0f);
    float3 force = gravity + drag * (current_wind - vel);

    vel += force * dt;
    pos += vel * dt;

    float flutter = sin(time * 5.0f + seed * 6.2831853f);
    pos.x += flutter * 0.05f;

    float color_wave = sin(time * 2.0f + seed * 10.0f + (pos.x + pos.z) * 0.05f);
    float mix_factor = color_wave * color_wave;
    float3 start_color = (float3)(1.0f, 0.4f, 0.65f);
    float3 end_color = (float3)(0.55f, 0.05f, 0.6f);
    float3 current_color = mix(start_color, end_color, mix_factor);

    float age2 = age * age;
    float fade = clamp(1.0f - age2 * age2, 0.0f, 1.0f);
    current_color *= fade;

    petal.position = (float4)(pos, age);
    petal.velocity = (float4)(vel, drag);
    petal.base_color.xyz = current_color;
    petals[gid] = petal;
}

// ----------------------------------------------------------------------------
inline float evaluate_trees_sdf(
    float3 p,
    __global const ForestTreeGpu* trees,
    int num_trees,
    int* out_tree_mat)
{
    float d_min = 1e5f;
    int best_mat = MAT_TREE_BARK;
    for (int i = 0; i < num_trees; ++i) {
        float3 a = trees[i].base_radius.xyz;
        float  r_base = fmax(trees[i].base_radius.w, 0.08f);
        float3 b = trees[i].canopy_center.xyz;
        float  r_crown = fmax(trees[i].canopy_center.w, 0.5f);

        float2 dxz = p.xz - a.xz;
        float max_r = r_crown + 2.0f;
        if (dot(dxz, dxz) > max_r * max_r) continue;

        // 1. Tapered trunk capsule with root buttressing and bark furrows
        float3 ba = b - a;
        float3 pa = p - a;
        float h = clamp(dot(pa, ba) / fmax(dot(ba, ba), 1e-4f), 0.0f, 1.0f);
        float3 radial = p - (a + h * ba);
        float radial_dist = length(radial);
        float r_at_h = mix(r_base, r_base * 0.25f, h);
        if (radial_dist < r_at_h + 0.65f) {
            float theta = atan2(radial.z, radial.x);
            float c_but = cos(5.0f * theta + (float)i * 0.37f);
            float buttress = r_base * 0.32f * exp(-fmax(0.0f, p.y - a.y) / 0.95f) * (c_but * c_but);
            float furrow = 0.022f * sin(10.0f * theta + 2.5f * fbm1D(p.y * 1.8f));
            r_at_h += buttress + furrow;
        }
        float d_trunk = radial_dist - r_at_h;
        if (d_trunk < d_min) {
            d_min = d_trunk;
            best_mat = MAT_TREE_BARK;
        }

        // 2. Worley-eroded foliage crown
        float3 q_crown = (float3)(p.x - b.x, (p.y - b.y) / 0.82f, p.z - b.z);
        float d_crown = (length(q_crown) - r_crown) * 0.82f;
        if (d_crown < 1.35f) {
            d_crown += 0.34f * worley3D(p * 0.65f) + 0.14f * fbm3D(p * 1.8f) - 0.18f;
        }
        if (d_crown < d_min) {
            d_min = d_crown;
            best_mat = MAT_CANOPY_FOLIAGE;
        }
    }
    *out_tree_mat = best_mat;
    return d_min;
}

inline float evaluate_canopy_shadow_gpu(
    float3 p,
    float3 sun_dir,
    __global const ForestTreeGpu* trees,
    int num_trees)
{
    float dapple = smoothstep(0.25f, 0.72f, 0.5f + 0.5f * fbm2D(p.xz * 0.35f + sun_dir.xz * p.y));
    float crown_trans = 1.0f;
    for (int i = 0; i < num_trees; ++i) {
        float3 c = trees[i].canopy_center.xyz;
        float  r = fmax(trees[i].canopy_center.w, 0.5f);
        float  dy = c.y - p.y;
        if (dy <= -r) continue;
        float t_ray = fmax(0.0f, dy / fmax(sun_dir.y, 0.15f));
        float3 ray_pt = p + t_ray * sun_dir;
        float3 d = (ray_pt - c) / (float3)(r, r * 0.82f, r);
        float dist_sq = dot(d, d);
        if (dist_sq < 1.35f) {
            float crown_atten = smoothstep(0.15f, 1.35f, dist_sq);
            crown_trans *= (0.18f + 0.82f * crown_atten);
        }
    }
    return clamp(crown_trans * (0.45f + 0.55f * dapple), 0.10f, 1.0f);
}

inline float evaluate_scene_sdf_scalar(
    float3 p,
    __global const ForestMossyBoulderGpu* boulders,
    int num_boulders,
    __global const ForestTreeGpu* trees,
    int num_trees,
    __global const ForestGroundFlowerGpu* flowers,
    int num_flowers)
{
    float path_mask = 1.0f;
    float h_ground = evaluate_terrain_height(p.xz, &path_mask);
    float d_min = (p.y - h_ground) * 0.65f;

    d_min = fmin(d_min, evaluate_boulders_sdf(p, boulders, num_boulders));

    int tree_mat = MAT_TREE_BARK;
    d_min = fmin(d_min, evaluate_trees_sdf(p, trees, num_trees, &tree_mat));

    if (path_mask > 0.35f && (p.y - h_ground) < 0.85f) {
        float fern_envelope = p.y - (h_ground + 0.55f * path_mask);
        float d_ferns = fmax(fern_envelope, 0.12f * worley3D(p * (float3)(4.5f, 2.0f, 4.5f)) - 0.05f * path_mask);
        d_min = fmin(d_min, d_ferns);
    }

    if ((p.y - h_ground) < 0.95f && num_flowers > 0) {
        int f_idx = -1;
        float2 p_uv = (float2)(0.0f);
        d_min = fmin(d_min, evaluate_rooted_flower_petals_sdf(p, flowers, num_flowers, &f_idx, &p_uv));
    }
    return d_min;
}

inline float3 compute_scene_normal_tetrahedral(
    float3 p,
    __global const ForestMossyBoulderGpu* boulders,
    int num_boulders,
    __global const ForestTreeGpu* trees,
    int num_trees,
    __global const ForestGroundFlowerGpu* flowers,
    int num_flowers)
{
    const float h = 0.025f;
    float d0 = evaluate_scene_sdf_scalar(p + (float3)( h, -h, -h), boulders, num_boulders, trees, num_trees, flowers, num_flowers);
    float d1 = evaluate_scene_sdf_scalar(p + (float3)(-h, -h,  h), boulders, num_boulders, trees, num_trees, flowers, num_flowers);
    float d2 = evaluate_scene_sdf_scalar(p + (float3)(-h,  h, -h), boulders, num_boulders, trees, num_trees, flowers, num_flowers);
    float d3 = evaluate_scene_sdf_scalar(p + (float3)( h,  h,  h), boulders, num_boulders, trees, num_trees, flowers, num_flowers);
    return normalize((float3)(d0 - d1 - d2 + d3, -d0 - d1 + d2 + d3, -d0 + d1 - d2 + d3));
}

// ----------------------------------------------------------------------------
// KERNEL 6: FULL-SCENE LICHTUNG PBR + VOLUMETRIC RAYMARCHER
// ----------------------------------------------------------------------------
__kernel void render_lichtung_scene(
    __global float4* output_frame,
    const uint width,
    const uint height,
    __global const float4* ground_bounce_irradiance_map,
    const uint gi_res,
    __global const ForestGroundFlowerGpu* flowers,
    const int num_flowers,
    __global const ForestMossyBoulderGpu* boulders,
    const int num_boulders,
    __global const ForestTreeGpu* trees,
    const int num_trees,
    const float4 cam_pos,
    const float4 cam_forward,
    const float4 cam_right,
    const float4 cam_up,
    const ForestLichtungDescriptorGpu descriptor)
{
    uint px = get_global_id(0);
    uint py = get_global_id(1);
    if (px >= width || py >= height) return;

    float2 ndc = (((float2)((float)px + 0.5f, (float)py + 0.5f)) / (float2)((float)width, (float)height)) * 2.0f - 1.0f;
    ndc.x *= (float)width / fmax((float)height, 1.0f);
    float3 ray_dir = normalize(cam_forward.xyz * 1.6f + ndc.x * cam_right.xyz + ndc.y * cam_up.xyz);

    float t = 0.1f, t_max = 90.0f;
    int hit_mat = MAT_SKY;
    int hit_flower_id = -1;
    float2 hit_petal_uv = (float2)(0.0f);
    float hit_path_mask = 1.0f;
    float hit_ground_h = 0.0f;

    for (int step = 0; step < 96 && t < t_max; ++step) {
        float3 p = cam_pos.xyz + t * ray_dir;

        float path_mask = 1.0f;
        float h_ground = evaluate_terrain_height(p.xz, &path_mask);
        float d_terrain = (p.y - h_ground) * 0.65f;
        float d_boulder = evaluate_boulders_sdf(p, boulders, num_boulders);

        int tree_mat = MAT_TREE_BARK;
        float d_tree = evaluate_trees_sdf(p, trees, num_trees, &tree_mat);

        float fern_envelope = p.y - (h_ground + 0.55f * path_mask);
        float d_ferns = fmax(fern_envelope, 0.12f * worley3D(p * (float3)(4.5f, 2.0f, 4.5f)) - 0.05f * path_mask);

        int f_idx = -1;
        float2 p_uv = (float2)(0.0f);
        float d_flower = evaluate_rooted_flower_petals_sdf(p, flowers, num_flowers, &f_idx, &p_uv);

        float d_min = d_terrain;
        int current_mat = (path_mask < 0.45f) ? MAT_DIRT_PATH : MAT_FOREST_FLOOR;

        if (d_boulder < d_min) { d_min = d_boulder; current_mat = MAT_MOSSY_BOULDER; }
        if (d_tree < d_min)    { d_min = d_tree;    current_mat = tree_mat; }
        if (d_ferns < d_min && path_mask > 0.35f) { d_min = d_ferns; current_mat = MAT_FERN_FOLIAGE; }
        if (d_flower < d_min)  { d_min = d_flower;  current_mat = MAT_FLOWER_PETAL; hit_flower_id = f_idx; hit_petal_uv = p_uv; }

        if (d_min < 0.0025f * t) {
            hit_mat = current_mat;
            hit_path_mask = path_mask;
            hit_ground_h = h_ground;
            break;
        }
        t += fmax(d_min, 0.01f);
    }

    float3 sun_dir = normalize(descriptor.sunDirection.xyz);
    float3 sun_rad = descriptor.sunRadiance.xyz;
    float time = descriptor.mistParams.w;
    float3 surface_radiance = (float3)(0.68f, 0.76f, 0.64f);

    if (hit_mat != MAT_SKY) {
        float3 hit_p = cam_pos.xyz + t * ray_dir;
        float3 N = compute_scene_normal_tetrahedral(hit_p, boulders, num_boulders, trees, num_trees, flowers, num_flowers);

        float canopy_shadow = evaluate_canopy_shadow_gpu(hit_p, sun_dir, trees, num_trees);

        float2 gi_uv = clamp((hit_p.xz / fmax(descriptor.worldExtentXZ, (float2)(1.0f))) + 0.5f, 0.0f, 1.0f);
        uint gix = min((uint)(gi_uv.x * (float)gi_res), gi_res - 1u);
        uint giz = min((uint)(gi_uv.y * (float)gi_res), gi_res - 1u);
        float3 petal_bounce_gi = ground_bounce_irradiance_map[giz * gi_res + gix].xyz;

        float3 albedo = (float3)(0.18f, 0.26f, 0.10f);
        float3 sss_color = (float3)(0.0f);
        float3 emissive = (float3)(0.0f);
        float ao = clamp((hit_p.y - hit_ground_h) / 0.4f + 0.25f, 0.15f, 1.0f);

        if (hit_mat == MAT_DIRT_PATH) {
            albedo = mix((float3)(0.26f, 0.17f, 0.10f), (float3)(0.18f, 0.24f, 0.09f), hit_path_mask);
        } else if (hit_mat == MAT_MOSSY_BOULDER) {
            float moss_mask = smoothstep(0.35f, 0.70f, N.y + 0.25f * fbm3D(hit_p * 2.5f) + 0.35f * exp(-fmax(0.0f, hit_p.y - hit_ground_h)));
            float3 granite_col = (float3)(0.24f, 0.25f, 0.23f) * (0.7f + 0.3f * worley3D(hit_p * 4.0f));
            float3 moss_col = (float3)(0.16f, 0.28f, 0.08f);
            albedo = mix(granite_col, moss_col, moss_mask);
            sss_color = moss_col * moss_mask * 0.35f;
        } else if (hit_mat == MAT_TREE_BARK) {
            float bark_tex = 0.78f + 0.22f * fbm3D(hit_p * (float3)(3.0f, 1.4f, 3.0f));
            albedo = (float3)(0.22f, 0.16f, 0.11f) * bark_tex;
        } else if (hit_mat == MAT_CANOPY_FOLIAGE) {
            float leaf_var = 0.82f + 0.18f * fbm3D(hit_p * 1.5f);
            albedo = (float3)(0.13f, 0.29f, 0.08f) * leaf_var;
            sss_color = (float3)(0.48f, 0.72f, 0.18f) * leaf_var;
            ao = clamp(0.45f + 0.55f * N.y, 0.25f, 1.0f);
        } else if (hit_mat == MAT_FERN_FOLIAGE) {
            albedo = (float3)(0.15f, 0.31f, 0.07f);
            sss_color = (float3)(0.42f, 0.65f, 0.15f);
        } else if (hit_mat == MAT_FLOWER_PETAL && hit_flower_id >= 0) {
            albedo = (float3)(0.90f, 0.88f, 0.82f);
            float hue_seed = flowers[hit_flower_id].glow_color_phase.w;
            float u = hit_petal_uv.x;
            float v = hit_petal_uv.y;
            float c_v = cos(v * 3.14159265f * 6.0f);
            float vein_pattern = 0.75f + 0.25f * (c_v * c_v * c_v * c_v);
            float radial_fade = smoothstep(0.08f, 0.85f, u);
            float3 petal_hue = spectral_petal_color(hue_seed, u, time);
            float pulse = length(flowers[hit_flower_id].glow_color_phase.xyz) * 0.35f;
            emissive = petal_hue * pulse * radial_fade * vein_pattern;
            sss_color = mix((float3)(0.95f, 0.92f, 0.80f), petal_hue, 0.6f);
            ao = 1.0f;
        }

        float n_dot_l = dot(N, sun_dir);
        float diffuse = fmax(0.0f, n_dot_l);
        float back_sss = pow(fmax(0.0f, -n_dot_l), 2.2f);
        float3 sky_ambient = (float3)(0.20f, 0.26f, 0.18f) * (0.5f + 0.5f * N.y) * ao;
        float3 direct_sun = sun_rad * canopy_shadow * (albedo * diffuse + sss_color * back_sss);
        float3 indirect_bounce = albedo * petal_bounce_gi * (0.6f + 0.4f * fmax(0.0f, -N.y)) * ao;
        surface_radiance = direct_sun + sky_ambient * albedo + indirect_bounce + emissive;
    }

    // Volumetric Lichtung valley mist with Henyey-Greenstein forward scattering
    float3 vol_accum = (float3)(0.0f);
    float transmittance = 1.0f;
    int vol_steps = 24;
    float step_size = fmin(t, 65.0f) / (float)vol_steps;
    float cos_theta = dot(ray_dir, sun_dir);
    float g = clamp(descriptor.sunRadiance.w, -0.95f, 0.95f);
    float hg_phase = (1.0f - g * g) / (4.0f * 3.14159265f * pow(fmax(1e-4f, 1.0f + g * g - 2.0f * g * cos_theta), 1.5f));

    for (int i = 0; i < vol_steps; ++i) {
        float s = ((float)i + 0.5f) * step_size;
        float3 sample_p = cam_pos.xyz + s * ray_dir;
        float local_density = descriptor.mistParams.x * exp(-descriptor.mistParams.y * fmax(0.0f, sample_p.y))
                            * (0.7f + 0.3f * fbm3D(sample_p * 0.15f));
        if (local_density > 1e-4f) {
            float sample_canopy_vis = evaluate_canopy_shadow_gpu(sample_p, sun_dir, trees, num_trees);
            float3 mist_inscatter = (float3)(0.52f, 0.60f, 0.46f) * 0.35f + sun_rad * sample_canopy_vis * hg_phase * 1.8f;
            float step_ext = exp(-local_density * step_size);
            vol_accum += transmittance * mist_inscatter * (1.0f - step_ext);
            transmittance *= step_ext;
        }
    }

    float3 final_hdr = surface_radiance * transmittance + vol_accum;
    float3 tonemapped = final_hdr / (final_hdr + 1.0f);
    output_frame[py * width + px] = (float4)(pow(clamp(tonemapped, 0.0f, 1.0f), (float3)(1.0f / 2.2f)), (float)hit_mat);
}

// ----------------------------------------------------------------------------
// KERNEL 7: THRESHOLDED HDR PETAL BLOOM & ACES FILMIC TONE MAPPING
// ----------------------------------------------------------------------------
inline float3 aces_tonemap_gpu(float3 x) {
    float3 v = fmax(x, (float3)(0.0f));
    return clamp((v * (2.51f * v + 0.03f)) / (v * (2.43f * v + 0.59f) + 0.14f), 0.0f, 1.0f);
}

__kernel void postprocess_lichtung_bloom_and_tonemap(
    __global const float4* in_hdr_frame,
    __global float4* out_srgb_frame,
    const uint width,
    const uint height,
    const float bloom_threshold,
    const float bloom_intensity)
{
    uint px = get_global_id(0);
    uint py = get_global_id(1);
    if (px >= width || py >= height) return;

    float4 center_sample = in_hdr_frame[py * width + px];
    float3 center_rgb = center_sample.xyz;

    float3 bloom_sum = (float3)(0.0f);
    float weight_sum = 0.0f;
    const float knee = fmax(bloom_threshold * 0.5f, 0.08f);
    for (int dy = -2; dy <= 2; ++dy) {
        int sy = clamp((int)py + dy, 0, (int)height - 1);
        for (int dx = -2; dx <= 2; ++dx) {
            int sx = clamp((int)px + dx, 0, (int)width - 1);
            float3 tap = in_hdr_frame[(uint)sy * width + (uint)sx].xyz;
            float lum = fmax(tap.x, fmax(tap.y, tap.z));
            float soft = clamp(lum - bloom_threshold + knee, 0.0f, 2.0f * knee);
            soft = (soft * soft) / (4.0f * knee + 1e-4f);
            float excess = fmax(lum - bloom_threshold, soft);
            float w = exp(-0.35f * (float)(dx * dx + dy * dy));
            if (lum > 1e-4f) {
                bloom_sum += tap * (excess / lum) * w;
            }
            weight_sum += w;
        }
    }

    float3 combined = center_rgb + (bloom_sum / fmax(weight_sum, 1e-4f)) * bloom_intensity;
    float3 srgb = pow(aces_tonemap_gpu(combined), (float3)(1.0f / 2.2f));
    out_srgb_frame[py * width + px] = (float4)(srgb, center_sample.w);
}

// ============================================================================
// CSR 2D SPATIAL HASH GRID ACCELERATION (3x3 CELL NEIGHBORHOOD QUERIES)
// ============================================================================
// Eliminates O(N_trees + N_boulders + N_flowers) full-array scans at every
// raymarch step and GI texel by querying flat CSR cellOffsets / cellIndices.
// ============================================================================

typedef struct {
    uint  gridWidth;
    uint  gridHeight;
    float originX;
    float originZ;
    float invCellSize;
    float maxSceneElevation;
    uint  pad0;
    uint  pad1;
} ForestSpatialGridHeaderGpu;

inline int2 forest_grid_cell_coord(float2 xz, ForestSpatialGridHeaderGpu grid) {
    int cx = clamp((int)floor((xz.x - grid.originX) * grid.invCellSize), 0, (int)grid.gridWidth - 1);
    int cz = clamp((int)floor((xz.y - grid.originZ) * grid.invCellSize), 0, (int)grid.gridHeight - 1);
    return (int2)(cx, cz);
}

inline float evaluate_boulders_sdf_csr(
    float3 p,
    __global const ForestMossyBoulderGpu* boulders,
    __global const uint* boulder_cell_offsets,
    __global const uint* boulder_indices,
    ForestSpatialGridHeaderGpu grid)
{
    float d_min = 1e5f;
    int2 c0 = forest_grid_cell_coord(p.xz, grid);
    for (int dz = -1; dz <= 1; ++dz) {
        int cz = c0.y + dz;
        if (cz < 0 || cz >= (int)grid.gridHeight) continue;
        for (int dx = -1; dx <= 1; ++dx) {
            int cx = c0.x + dx;
            if (cx < 0 || cx >= (int)grid.gridWidth) continue;
            uint cell_idx = (uint)cz * grid.gridWidth + (uint)cx;
            uint begin = boulder_cell_offsets[cell_idx];
            uint end   = boulder_cell_offsets[cell_idx + 1u];
            for (uint k = begin; k < end; ++k) {
                uint i = boulder_indices[k];
                float3 c = boulders[i].center_radius.xyz;
                float  r = fmax(boulders[i].center_radius.w, 0.05f);
                float3 scale = fmax(boulders[i].stretch_seed.xyz, (float3)(0.1f));
                float3 q = (p - c) / scale;
                float d_base = (length(q) - r) * fmin(scale.x, fmin(scale.y, scale.z));
                if (d_base < 1.2f) {
                    d_base += 0.28f * worley3D(p * 0.7f) + 0.06f * fbm3D(p * 3.5f);
                }
                d_min = fmin(d_min, d_base);
            }
        }
    }
    return d_min;
}

inline float evaluate_trees_sdf_csr(
    float3 p,
    __global const ForestTreeGpu* trees,
    __global const uint* tree_cell_offsets,
    __global const uint* tree_indices,
    ForestSpatialGridHeaderGpu grid,
    int* out_tree_mat)
{
    float d_min = 1e5f;
    int best_mat = MAT_TREE_BARK;
    int2 c0 = forest_grid_cell_coord(p.xz, grid);
    for (int dz = -1; dz <= 1; ++dz) {
        int cz = c0.y + dz;
        if (cz < 0 || cz >= (int)grid.gridHeight) continue;
        for (int dx = -1; dx <= 1; ++dx) {
            int cx = c0.x + dx;
            if (cx < 0 || cx >= (int)grid.gridWidth) continue;
            uint cell_idx = (uint)cz * grid.gridWidth + (uint)cx;
            uint begin = tree_cell_offsets[cell_idx];
            uint end   = tree_cell_offsets[cell_idx + 1u];
            for (uint k = begin; k < end; ++k) {
                uint i = tree_indices[k];
                float3 a = trees[i].base_radius.xyz;
                float  r_base = fmax(trees[i].base_radius.w, 0.08f);
                float3 b = trees[i].canopy_center.xyz;
                float  r_crown = fmax(trees[i].canopy_center.w, 0.5f);

                float2 dxz = p.xz - a.xz;
                float max_r = r_crown + 2.0f;
                if (dot(dxz, dxz) > max_r * max_r) continue;

                float3 ba = b - a;
                float3 pa = p - a;
                float h = clamp(dot(pa, ba) / fmax(dot(ba, ba), 1e-4f), 0.0f, 1.0f);
                float3 radial = p - (a + h * ba);
                float radial_dist = length(radial);
                float r_at_h = mix(r_base, r_base * 0.25f, h);
                if (radial_dist < r_at_h + 0.65f) {
                    float theta = atan2(radial.z, radial.x);
                    float c_but = cos(5.0f * theta + (float)i * 0.37f);
                    float buttress = r_base * 0.32f * exp(-fmax(0.0f, p.y - a.y) / 0.95f) * (c_but * c_but);
                    float furrow = 0.022f * sin(10.0f * theta + 2.5f * fbm1D(p.y * 1.8f));
                    r_at_h += buttress + furrow;
                }
                float d_trunk = radial_dist - r_at_h;
                if (d_trunk < d_min) {
                    d_min = d_trunk;
                    best_mat = MAT_TREE_BARK;
                }

                float3 q_crown = (float3)(p.x - b.x, (p.y - b.y) / 0.82f, p.z - b.z);
                float d_crown = (length(q_crown) - r_crown) * 0.82f;
                if (d_crown < 1.35f) {
                    d_crown += 0.34f * worley3D(p * 0.65f) + 0.14f * fbm3D(p * 1.8f) - 0.18f;
                }
                if (d_crown < d_min) {
                    d_min = d_crown;
                    best_mat = MAT_CANOPY_FOLIAGE;
                }
            }
        }
    }
    *out_tree_mat = best_mat;
    return d_min;
}

inline float evaluate_rooted_flower_petals_sdf_csr(
    float3 p,
    __global const ForestGroundFlowerGpu* flowers,
    __global const uint* flower_cell_offsets,
    __global const uint* flower_indices,
    ForestSpatialGridHeaderGpu grid,
    int* out_flower_idx,
    float2* out_petal_uv)
{
    float d_min = 1e5f;
    const float sector = 6.2831853f / 5.0f;
    int2 c0 = forest_grid_cell_coord(p.xz, grid);
    for (int dz = -1; dz <= 1; ++dz) {
        int cz = c0.y + dz;
        if (cz < 0 || cz >= (int)grid.gridHeight) continue;
        for (int dx = -1; dx <= 1; ++dx) {
            int cx = c0.x + dx;
            if (cx < 0 || cx >= (int)grid.gridWidth) continue;
            uint cell_idx = (uint)cz * grid.gridWidth + (uint)cx;
            uint begin = flower_cell_offsets[cell_idx];
            uint end   = flower_cell_offsets[cell_idx + 1u];
            for (uint k = begin; k < end; ++k) {
                uint i = flower_indices[k];
                float4 head = flowers[i].head_pos_radius;
                float3 to_p = p - head.xyz;
                if (dot(to_p, to_p) > 0.16f) continue;

                float3 axis_y = normalize(flowers[i].head_axis.xyz);
                float3 ref_v = fabs(axis_y.z) < 0.9f ? (float3)(0.0f, 0.0f, 1.0f) : (float3)(1.0f, 0.0f, 0.0f);
                float3 axis_x = normalize(cross(axis_y, ref_v));
                float3 axis_z = cross(axis_x, axis_y);
                float3 local_p = (float3)(dot(to_p, axis_x), dot(to_p, axis_y), dot(to_p, axis_z));

                float angle = atan2(local_p.z, local_p.x);
                float wrapped = fmod(angle + 3.14159265f + sector * 0.5f, sector);
                if (wrapped < 0.0f) wrapped += sector;
                float local_angle = wrapped - sector * 0.5f;

                float r_xz = length(local_p.xz);
                float u = clamp(r_xz / fmax(head.w, 0.01f), 0.0f, 1.0f);
                float v = clamp(local_angle / (sector * 0.5f), -1.0f, 1.0f);

                float cup_y = -0.08f * sin(u * 1.8f) + 0.02f * u * u;
                float petal_width = sin(u * 3.14159265f * 0.85f);
                float d_petal = length((float2)(local_p.y - cup_y, fmax(0.0f, fabs(v) - petal_width) * 0.04f)) - 0.004f;

                if (d_petal < d_min) {
                    d_min = d_petal;
                    *out_flower_idx = (int)i;
                    *out_petal_uv = (float2)(u, v);
                }
            }
        }
    }
    return d_min;
}

inline float evaluate_canopy_shadow_gpu_csr(
    float3 p,
    float3 sun_dir,
    __global const ForestTreeGpu* trees,
    __global const uint* tree_cell_offsets,
    __global const uint* tree_indices,
    ForestSpatialGridHeaderGpu grid)
{
    float dapple = smoothstep(0.25f, 0.72f, 0.5f + 0.5f * fbm2D(p.xz * 0.35f + sun_dir.xz * p.y));
    float crown_trans = 1.0f;
    float2 proj_xz = p.xz + sun_dir.xz * fmax(0.0f, (7.0f - p.y) / fmax(sun_dir.y, 0.15f));
    int2 c0 = forest_grid_cell_coord(proj_xz, grid);
    for (int dz = -1; dz <= 1; ++dz) {
        int cz = c0.y + dz;
        if (cz < 0 || cz >= (int)grid.gridHeight) continue;
        for (int dx = -1; dx <= 1; ++dx) {
            int cx = c0.x + dx;
            if (cx < 0 || cx >= (int)grid.gridWidth) continue;
            uint cell_idx = (uint)cz * grid.gridWidth + (uint)cx;
            uint begin = tree_cell_offsets[cell_idx];
            uint end   = tree_cell_offsets[cell_idx + 1u];
            for (uint k = begin; k < end; ++k) {
                uint i = tree_indices[k];
                float3 c = trees[i].canopy_center.xyz;
                float  r = fmax(trees[i].canopy_center.w, 0.5f);
                float  dy = c.y - p.y;
                if (dy <= -r) continue;
                float t_ray = fmax(0.0f, dy / fmax(sun_dir.y, 0.15f));
                float3 ray_pt = p + t_ray * sun_dir;
                float3 d = (ray_pt - c) / (float3)(r, r * 0.82f, r);
                float dist_sq = dot(d, d);
                if (dist_sq < 1.35f) {
                    float crown_atten = smoothstep(0.15f, 1.35f, dist_sq);
                    crown_trans *= (0.18f + 0.82f * crown_atten);
                }
            }
        }
    }
    return clamp(crown_trans * (0.45f + 0.55f * dapple), 0.10f, 1.0f);
}

inline float evaluate_scene_sdf_scalar_csr(
    float3 p,
    __global const ForestMossyBoulderGpu* boulders,
    __global const uint* boulder_cell_offsets,
    __global const uint* boulder_indices,
    __global const ForestTreeGpu* trees,
    __global const uint* tree_cell_offsets,
    __global const uint* tree_indices,
    __global const ForestGroundFlowerGpu* flowers,
    __global const uint* flower_cell_offsets,
    __global const uint* flower_indices,
    ForestSpatialGridHeaderGpu grid)
{
    float path_mask = 1.0f;
    float h_ground = evaluate_terrain_height(p.xz, &path_mask);
    float d_min = (p.y - h_ground) * 0.65f;

    d_min = fmin(d_min, evaluate_boulders_sdf_csr(p, boulders, boulder_cell_offsets, boulder_indices, grid));

    int tree_mat = MAT_TREE_BARK;
    d_min = fmin(d_min, evaluate_trees_sdf_csr(p, trees, tree_cell_offsets, tree_indices, grid, &tree_mat));

    if (path_mask > 0.35f && (p.y - h_ground) < 0.85f) {
        float fern_envelope = p.y - (h_ground + 0.55f * path_mask);
        float d_ferns = fmax(fern_envelope, 0.12f * worley3D(p * (float3)(4.5f, 2.0f, 4.5f)) - 0.05f * path_mask);
        d_min = fmin(d_min, d_ferns);
    }

    if ((p.y - h_ground) < 0.95f) {
        int f_idx = -1;
        float2 p_uv = (float2)(0.0f);
        d_min = fmin(d_min, evaluate_rooted_flower_petals_sdf_csr(
            p, flowers, flower_cell_offsets, flower_indices, grid, &f_idx, &p_uv));
    }
    return d_min;
}

inline float3 compute_scene_normal_tetrahedral_csr(
    float3 p,
    __global const ForestMossyBoulderGpu* boulders,
    __global const uint* boulder_cell_offsets,
    __global const uint* boulder_indices,
    __global const ForestTreeGpu* trees,
    __global const uint* tree_cell_offsets,
    __global const uint* tree_indices,
    __global const ForestGroundFlowerGpu* flowers,
    __global const uint* flower_cell_offsets,
    __global const uint* flower_indices,
    ForestSpatialGridHeaderGpu grid)
{
    const float h = 0.025f;
    float d0 = evaluate_scene_sdf_scalar_csr(p + (float3)( h, -h, -h), boulders, boulder_cell_offsets, boulder_indices, trees, tree_cell_offsets, tree_indices, flowers, flower_cell_offsets, flower_indices, grid);
    float d1 = evaluate_scene_sdf_scalar_csr(p + (float3)(-h, -h,  h), boulders, boulder_cell_offsets, boulder_indices, trees, tree_cell_offsets, tree_indices, flowers, flower_cell_offsets, flower_indices, grid);
    float d2 = evaluate_scene_sdf_scalar_csr(p + (float3)(-h,  h, -h), boulders, boulder_cell_offsets, boulder_indices, trees, tree_cell_offsets, tree_indices, flowers, flower_cell_offsets, flower_indices, grid);
    float d3 = evaluate_scene_sdf_scalar_csr(p + (float3)( h,  h,  h), boulders, boulder_cell_offsets, boulder_indices, trees, tree_cell_offsets, tree_indices, flowers, flower_cell_offsets, flower_indices, grid);
    return normalize((float3)(d0 - d1 - d2 + d3, -d0 - d1 + d2 + d3, -d0 + d1 - d2 + d3));
}

// ----------------------------------------------------------------------------
// KERNEL 8: CSR-ACCELERATED GROUND BOUNCE IRRADIANCE ACCUMULATION
// ----------------------------------------------------------------------------
__kernel void accumulate_ground_bounce_irradiance_accelerated(
    __global const ForestGroundFlowerGpu* flowers,
    __global const uint* flower_cell_offsets,
    __global const uint* flower_indices,
    const ForestSpatialGridHeaderGpu grid,
    __global float4* ground_bounce_irradiance_map,
    const uint grid_res,
    const float2 origin_xz,
    const float2 extent_xz)
{
    uint gid = get_global_id(0);
    if (gid >= grid_res * grid_res) return;

    uint ix = gid % grid_res;
    uint iz = gid / grid_res;

    float2 uv = ((float2)((float)ix + 0.5f, (float)iz + 0.5f)) / (float)grid_res;
    float2 world_xz = origin_xz + uv * extent_xz;
    float path_mask = 1.0f;
    float ground_y = evaluate_terrain_height(world_xz, &path_mask);
    float3 ground_p = (float3)(world_xz.x, ground_y, world_xz.y);

    float3 accumulated_bounce = (float3)(0.0f);
    int2 c0 = forest_grid_cell_coord(world_xz, grid);
    for (int dz = -1; dz <= 1; ++dz) {
        int cz = c0.y + dz;
        if (cz < 0 || cz >= (int)grid.gridHeight) continue;
        for (int dx = -1; dx <= 1; ++dx) {
            int cx = c0.x + dx;
            if (cx < 0 || cx >= (int)grid.gridWidth) continue;
            uint cell_idx = (uint)cz * grid.gridWidth + (uint)cx;
            uint begin = flower_cell_offsets[cell_idx];
            uint end   = flower_cell_offsets[cell_idx + 1u];
            for (uint k = begin; k < end; ++k) {
                uint i = flower_indices[k];
                float3 f_pos = flowers[i].head_pos_radius.xyz;
                float3 diff = f_pos - ground_p;
                float dist2 = dot(diff, diff);
                if (dist2 < 12.0f) {
                    float atten = 1.0f / (1.0f + 2.5f * dist2 * dist2);
                    accumulated_bounce += flowers[i].glow_color_phase.xyz * atten;
                }
            }
        }
    }
    ground_bounce_irradiance_map[gid] = (float4)(accumulated_bounce, 1.0f);
}

// ----------------------------------------------------------------------------
// KERNEL 9: CSR-ACCELERATED LINEAR-HDR LICHTUNG RAYMARCHER
// ----------------------------------------------------------------------------
// Uses 3x3 CSR spatial hash lookups for trees, boulders, and flowers, skips
// empty sky above maxSceneElevation, and outputs TRUE LINEAR HDR radiance in
// output_linear_hdr_frame.xyz (for postprocess_lichtung_bloom_and_tonemap).
// ----------------------------------------------------------------------------
__kernel void render_lichtung_scene_accelerated(
    __global float4* output_linear_hdr_frame,
    const uint width,
    const uint height,
    __global const float4* ground_bounce_irradiance_map,
    const uint gi_res,
    __global const ForestGroundFlowerGpu* flowers,
    __global const uint* flower_cell_offsets,
    __global const uint* flower_indices,
    __global const ForestMossyBoulderGpu* boulders,
    __global const uint* boulder_cell_offsets,
    __global const uint* boulder_indices,
    __global const ForestTreeGpu* trees,
    __global const uint* tree_cell_offsets,
    __global const uint* tree_indices,
    const ForestSpatialGridHeaderGpu grid,
    const float4 cam_pos,
    const float4 cam_forward,
    const float4 cam_right,
    const float4 cam_up,
    const ForestLichtungDescriptorGpu descriptor)
{
    uint px = get_global_id(0);
    uint py = get_global_id(1);
    if (px >= width || py >= height) return;

    float2 ndc = (((float2)((float)px + 0.5f, (float)py + 0.5f)) / (float2)((float)width, (float)height)) * 2.0f - 1.0f;
    ndc.x *= (float)width / fmax((float)height, 1.0f);
    float3 ray_dir = normalize(cam_forward.xyz * 1.6f + ndc.x * cam_right.xyz + ndc.y * cam_up.xyz);

    float t = 0.1f, t_max = 90.0f;
    int hit_mat = MAT_SKY;
    int hit_flower_id = -1;
    float2 hit_petal_uv = (float2)(0.0f);
    float hit_path_mask = 1.0f;
    float hit_ground_h = 0.0f;

    for (int step = 0; step < 96 && t < t_max; ++step) {
        float3 p = cam_pos.xyz + t * ray_dir;

        // Ceiling envelope early-out when marching upward above highest tree canopy
        if (p.y > grid.maxSceneElevation && ray_dir.y >= 0.0f) {
            break;
        }

        float path_mask = 1.0f;
        float h_ground = evaluate_terrain_height(p.xz, &path_mask);
        float d_terrain = (p.y - h_ground) * 0.65f;
        float d_boulder = evaluate_boulders_sdf_csr(p, boulders, boulder_cell_offsets, boulder_indices, grid);

        int tree_mat = MAT_TREE_BARK;
        float d_tree = evaluate_trees_sdf_csr(p, trees, tree_cell_offsets, tree_indices, grid, &tree_mat);

        float fern_envelope = p.y - (h_ground + 0.55f * path_mask);
        float d_ferns = fmax(fern_envelope, 0.12f * worley3D(p * (float3)(4.5f, 2.0f, 4.5f)) - 0.05f * path_mask);

        int f_idx = -1;
        float2 p_uv = (float2)(0.0f);
        float d_flower = 1e5f;
        if ((p.y - h_ground) < 0.95f && t < 36.0f) {
            d_flower = evaluate_rooted_flower_petals_sdf_csr(
                p, flowers, flower_cell_offsets, flower_indices, grid, &f_idx, &p_uv);
        }

        float d_min = d_terrain;
        int current_mat = (path_mask < 0.45f) ? MAT_DIRT_PATH : MAT_FOREST_FLOOR;

        if (d_boulder < d_min) { d_min = d_boulder; current_mat = MAT_MOSSY_BOULDER; }
        if (d_tree < d_min)    { d_min = d_tree;    current_mat = tree_mat; }
        if (d_ferns < d_min && path_mask > 0.35f) { d_min = d_ferns; current_mat = MAT_FERN_FOLIAGE; }
        if (d_flower < d_min)  { d_min = d_flower;  current_mat = MAT_FLOWER_PETAL; hit_flower_id = f_idx; hit_petal_uv = p_uv; }

        if (d_min < 0.0025f * t) {
            hit_mat = current_mat;
            hit_path_mask = path_mask;
            hit_ground_h = h_ground;
            break;
        }
        t += fmax(d_min, 0.01f);
    }

    float3 sun_dir = normalize(descriptor.sunDirection.xyz);
    float3 sun_rad = descriptor.sunRadiance.xyz;
    float time = descriptor.mistParams.w;
    float3 surface_radiance = (float3)(0.68f, 0.76f, 0.64f);

    if (hit_mat != MAT_SKY) {
        float3 hit_p = cam_pos.xyz + t * ray_dir;
        float3 N = compute_scene_normal_tetrahedral_csr(
            hit_p, boulders, boulder_cell_offsets, boulder_indices,
            trees, tree_cell_offsets, tree_indices,
            flowers, flower_cell_offsets, flower_indices, grid);

        float canopy_shadow = evaluate_canopy_shadow_gpu_csr(
            hit_p, sun_dir, trees, tree_cell_offsets, tree_indices, grid);

        float2 gi_uv = clamp((hit_p.xz / fmax(descriptor.worldExtentXZ, (float2)(1.0f))) + 0.5f, 0.0f, 1.0f);
        uint gix = min((uint)(gi_uv.x * (float)gi_res), gi_res - 1u);
        uint giz = min((uint)(gi_uv.y * (float)gi_res), gi_res - 1u);
        float3 petal_bounce_gi = ground_bounce_irradiance_map[giz * gi_res + gix].xyz;

        float3 albedo = (float3)(0.18f, 0.26f, 0.10f);
        float3 sss_color = (float3)(0.0f);
        float3 emissive = (float3)(0.0f);
        float ao = clamp((hit_p.y - hit_ground_h) / 0.4f + 0.25f, 0.15f, 1.0f);

        if (hit_mat == MAT_DIRT_PATH) {
            albedo = mix((float3)(0.26f, 0.17f, 0.10f), (float3)(0.18f, 0.24f, 0.09f), hit_path_mask);
        } else if (hit_mat == MAT_MOSSY_BOULDER) {
            float moss_mask = smoothstep(0.35f, 0.70f, N.y + 0.25f * fbm3D(hit_p * 2.5f) + 0.35f * exp(-fmax(0.0f, hit_p.y - hit_ground_h)));
            float3 granite_col = (float3)(0.24f, 0.25f, 0.23f) * (0.7f + 0.3f * worley3D(hit_p * 4.0f));
            float3 moss_col = (float3)(0.16f, 0.28f, 0.08f);
            albedo = mix(granite_col, moss_col, moss_mask);
            sss_color = moss_col * moss_mask * 0.35f;
        } else if (hit_mat == MAT_TREE_BARK) {
            float bark_tex = 0.78f + 0.22f * fbm3D(hit_p * (float3)(3.0f, 1.4f, 3.0f));
            albedo = (float3)(0.22f, 0.16f, 0.11f) * bark_tex;
        } else if (hit_mat == MAT_CANOPY_FOLIAGE) {
            float leaf_var = 0.82f + 0.18f * fbm3D(hit_p * 1.5f);
            albedo = (float3)(0.13f, 0.29f, 0.08f) * leaf_var;
            sss_color = (float3)(0.48f, 0.72f, 0.18f) * leaf_var;
            ao = clamp(0.45f + 0.55f * N.y, 0.25f, 1.0f);
        } else if (hit_mat == MAT_FERN_FOLIAGE) {
            albedo = (float3)(0.15f, 0.31f, 0.07f);
            sss_color = (float3)(0.42f, 0.65f, 0.15f);
        } else if (hit_mat == MAT_FLOWER_PETAL && hit_flower_id >= 0) {
            albedo = (float3)(0.90f, 0.88f, 0.82f);
            float hue_seed = flowers[hit_flower_id].glow_color_phase.w;
            float u = hit_petal_uv.x;
            float v = hit_petal_uv.y;
            float c_v = cos(v * 3.14159265f * 6.0f);
            float vein_pattern = 0.75f + 0.25f * (c_v * c_v * c_v * c_v);
            float radial_fade = smoothstep(0.08f, 0.85f, u);
            float3 petal_hue = spectral_petal_color(hue_seed, u, time);
            float pulse = length(flowers[hit_flower_id].glow_color_phase.xyz) * 0.35f;
            emissive = petal_hue * pulse * radial_fade * vein_pattern;
            sss_color = mix((float3)(0.95f, 0.92f, 0.80f), petal_hue, 0.6f);
            ao = 1.0f;
        }

        float n_dot_l = dot(N, sun_dir);
        float diffuse = fmax(0.0f, n_dot_l);
        float back_sss = pow(fmax(0.0f, -n_dot_l), 2.2f);
        float3 sky_ambient = (float3)(0.20f, 0.26f, 0.18f) * (0.5f + 0.5f * N.y) * ao;
        float3 direct_sun = sun_rad * canopy_shadow * (albedo * diffuse + sss_color * back_sss);
        float3 indirect_bounce = albedo * petal_bounce_gi * (0.6f + 0.4f * fmax(0.0f, -N.y)) * ao;
        surface_radiance = direct_sun + sky_ambient * albedo + indirect_bounce + emissive;
    }

    float3 vol_accum = (float3)(0.0f);
    float transmittance = 1.0f;
    int vol_steps = 20;
    float step_size = fmin(t, 65.0f) / (float)vol_steps;
    float cos_theta = dot(ray_dir, sun_dir);
    float g = clamp(descriptor.sunRadiance.w, -0.95f, 0.95f);
    float hg_phase = (1.0f - g * g) / (4.0f * 3.14159265f * pow(fmax(1e-4f, 1.0f + g * g - 2.0f * g * cos_theta), 1.5f));

    for (int i = 0; i < vol_steps; ++i) {
        float s = ((float)i + 0.5f) * step_size;
        float3 sample_p = cam_pos.xyz + s * ray_dir;
        float local_density = descriptor.mistParams.x * exp(-descriptor.mistParams.y * fmax(0.0f, sample_p.y))
                            * (0.7f + 0.3f * fbm3D(sample_p * 0.15f));
        if (local_density > 1e-4f) {
            float sample_canopy_vis = evaluate_canopy_shadow_gpu_csr(
                sample_p, sun_dir, trees, tree_cell_offsets, tree_indices, grid);
            float3 mist_inscatter = (float3)(0.52f, 0.60f, 0.46f) * 0.35f + sun_rad * sample_canopy_vis * hg_phase * 1.8f;
            float step_ext = exp(-local_density * step_size);
            vol_accum += transmittance * mist_inscatter * (1.0f - step_ext);
            transmittance *= step_ext;
        }
    }

    // Store true linear HDR radiance in .xyz and hit material ID in .w
    float3 final_linear_hdr = fmax(surface_radiance * transmittance + vol_accum, (float3)(0.0f));
    output_linear_hdr_frame[py * width + px] = (float4)(final_linear_hdr, (float)hit_mat);
}

