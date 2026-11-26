// -----------------------------------------------------------------------------
// Educational Annotation: 3D VOLUMETRIC EPIMORPHISM FEEDBACK KERNEL
// -----------------------------------------------------------------------------
// This OpenCL kernel realizes Phase 3 of the 3D Higher-Dimensional World Vision
// (docu/3D-HIGHER-DIMENSIONAL-WORLD-VISION.md):
//
// "A direct 3D counterpart to the current epimorphism flow is:
//    p = A0 * p_in + b0
//    p = inner3(p, latent)
//    p = A1 * p + b1
//    p = reduce3(p)
//    history  = trilinear_sample(previous_world_field, p)
//    material = world_color(history, p, gradient, orbit_data)
//    seed     = world_seed(p, time)
//    next     = evolve(history, material, seed)"
//
// Unlike 2D screen-space feedback which diverges across multiple VR eye views,
// this 3D volumetric field maintains camera-independent persistent memory
// in world space. Raymarchers and presentation directors sample this evolving
// field trilinearly at surface hitpoints and along volumetric participating rays.
// -----------------------------------------------------------------------------

typedef struct {
    float4 colorDensity;    // xyz = Color (RGB/OKLCH), w = Density [0..1]
    float4 velocityEnergy;  // xyz = Flow vector,       w = Accumulated orbit energy
} VolumetricVoxelGpu;

typedef struct {
    uint4  identity;            // x=version, y=byteSize, z=width, w=height
    uint4  extentCadence;       // x=depth, y=resTier, z=cadence, w=flags
    float4 worldMinimum;        // xyz=worldMin, w=decayFactor
    float4 worldMaximum;        // xyz=worldMax, w=seedAlpha
    float4 transformParameters; // x=time, y=deltaSeconds, z=latentW, w=rotorAlpha
} VolumetricFieldDescriptorGpu;

#define FLAG_ENABLE_FEEDBACK    (1u << 0u)
#define FLAG_ENABLE_TORUS_WRAP  (1u << 1u)
#define FLAG_ENABLE_WCA_SEED    (1u << 2u)
#define FLAG_ENABLE_HYPER4D     (1u << 3u)

inline uint voxel_linear_index(uint x, uint y, uint z, uint width, uint height) {
    return x + width * (y + height * z);
}

// Complex arithmetic helper functions
inline float2 c_mul(float2 a, float2 b) {
    return (float2)(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

inline float2 c_sqr(float2 z) {
    return (float2)(z.x * z.x - z.y * z.y, 2.0f * z.x * z.y);
}

inline float2 c_sin(float2 z) {
    return (float2)(sin(z.x) * cosh(z.y), cos(z.x) * sinh(z.y));
}

inline float2 c_cos(float2 z) {
    return (float2)(cos(z.x) * cosh(z.y), -sin(z.x) * sinh(z.y));
}

inline float2 c_exp(float2 z) {
    float r = exp(z.x);
    return (float2)(r * cos(z.y), r * sin(z.y));
}

inline float2 c_sinh(float2 z) {
    return (float2)(sinh(z.x) * cos(z.y), cosh(z.x) * sin(z.y));
}

inline float2 evaluate_complex_inner(float2 z, uint innerType) {
    switch (innerType) {
    case 0u: return z;
    case 1u: return c_sqr(z);
    case 2u: return c_mul(c_sqr(z), z);
    case 3u: return c_sin(z);
    case 4u: return c_cos(z);
    case 5u: return c_sinh(z);
    case 6u: return (float2)(cosh(z.x)*cos(z.y), sinh(z.x)*sin(z.y));
    case 7u: {
        float2 num = (float2)(sinh(z.x)*cos(z.y), cosh(z.x)*sin(z.y));
        float2 den = (float2)(cosh(z.x)*cos(z.y), sinh(z.x)*sin(z.y));
        float d = fmax(den.x*den.x + den.y*den.y, 1.0e-5f);
        return (float2)((num.x*den.x + num.y*den.y)/d, (num.y*den.x - num.x*den.y)/d);
    }
    case 8u: return c_exp(z);
    case 9u: {
        // Inversion: z / |z|^2
        float denom = fmax(z.x * z.x + z.y * z.y, 1.0e-5f);
        return (float2)(z.x / denom, -z.y / denom);
    }
    case 10u: {
        float r = length(z);
        float th = atan2(z.y, z.x);
        float w = fmod(th + 3.14159265f, 6.2831853f / 6.0f);
        if (w < 0.0f) w += 6.2831853f / 6.0f;
        float f = fabs(w - 3.14159265f / 6.0f) - 3.14159265f;
        return (float2)(r * cos(f), r * sin(f));
    }
    case 14u: return (float2)(sin(1.5f*z.x+0.5f), sin(2.0f*z.y));
    case 16u: return (float2)(sin(z.x)*cos(z.y) + sin(z.y)*cos(z.x), sin(z.x)*sin(z.y) - cos(z.x)*cos(z.y));
    case 22u: {
        float r2 = dot(z, z);
        if (r2 >= 1.0f) { z = (float2)(z.x/r2, -z.y/r2); r2 = dot(z, z); }
        float r = sqrt(fmax(r2, 1e-8f));
        float th = atan2(z.y, z.x);
        float d = 0.5f * log((1.0f + clamp(r, 1e-4f, 0.999f)) / (1.0f - clamp(r, 1e-4f, 0.999f)));
        float r_out = tanh(1.5f * d);
        return (float2)(r_out * cos(th), r_out * sin(th));
    }
    case 24u: {
        float r = length(z); float th = atan2(z.y, z.x);
        float r5 = pow(clamp(r, 0.0f, 2.5f), 5.0f);
        float2 z5 = (float2)(r5*cos(5.0f*th), r5*sin(5.0f*th));
        float2 w_cy = (float2)(1.0f - z5.x, -z5.y);
        float rw = length(w_cy); float phi_w = atan2(w_cy.y, w_cy.x);
        float out_r = pow(fmax(rw, 1e-6f), 0.2f);
        return (float2)(out_r * cos(phi_w * 0.2f), out_r * sin(phi_w * 0.2f));
    }
    case 27u: {
        float dx = 10.0f * (z.y - z.x);
        float dy = z.x * (28.0f - (z.x*z.x + z.y*z.y)*0.04f) - z.y;
        float2 nz = z + 0.02f * (float2)(dx, dy);
        float m = length(nz); return (m > 4.0f) ? (nz * (4.0f / m)) : nz;
    }
    case 28u: {
        float dx = -z.y - (z.x*z.x + z.y*z.y)*0.08f;
        float dy = z.x + 0.2f * z.y;
        float2 nz = z + 0.025f * (float2)(dx, dy);
        float m = length(nz); return (m > 4.0f) ? (nz * (4.0f / m)) : nz;
    }
    case 31u: {
        float r2 = dot(z, z);
        float u = clamp(1.0f - 0.5f * exp(-r2 * 2.0f) * cos(4.0f * z.x) * cos(4.0f * z.y), 0.0f, 1.0f);
        float v = clamp(0.25f * exp(-r2 * 2.0f) * (1.0f + sin(3.0f * z.x + z.y)), 0.0f, 1.0f);
        float ang = atan2(z.y, z.x) + (v - u) * 2.5f;
        float rad = sqrt(r2) * (0.85f + 0.35f * v);
        return (float2)(rad * cos(ang), rad * sin(ang));
    }
    case 33u: {
        float2 z_sh = (float2)(sinh(z.x)*cos(z.y), cosh(z.x)*sin(z.y));
        float m = exp(z.x);
        float2 z_ex = m * (float2)(cos(z.y), sin(z.y));
        float2 h = 0.5f * z_sh + 0.5f * z_ex;
        float len = length(h); return (len > 6.0f) ? (h * (6.0f / len)) : h;
    }
    case 37u: {
        float th = atan2(z.y, z.x);
        float c = 0.65f * sin(3.0f * th) + 0.35f * sin(5.0f * th);
        float r = tanh(1.2f * length(z));
        return (float2)(r * cos(th + 0.45f * c), r * sin(th + 0.45f * c));
    }
    case 38u: {
        float r2 = dot(z, z);
        float env = exp(-0.35f * r2);
        return env * (float2)((2.0f*z.x*z.x - 1.0f)*cos(2.5f*z.y), (2.0f*z.y*z.y - 1.0f)*sin(2.5f*z.x));
    }
    case 41u: {
        float th1 = 3.14159265f * 0.4f; float th2 = 3.14159265f * 0.8f;
        float2 v1 = (float2)(cos(th1)*z.x + sin(th1)*z.y, cos(th2)*z.y - sin(th2)*z.x);
        float2 v2 = (float2)(cos(th2)*z.x + sin(th2)*z.y, cos(th1)*z.y - sin(th1)*z.x);
        float2 p = (float2)(sin(v1.x)*cosh(v2.y), cos(v2.x)*sinh(v1.y));
        float m = length(p); return (m > 3.0f) ? (p * (3.0f / m)) : p;
    }
    case 42u: {
        float r2 = dot(z, z); float ang = atan2(z.y, z.x);
        float twist = ang + 1.5f * sin(r2 * 4.0f);
        float rad = 0.5f * log(1.0f + r2) + 0.15f * cos(3.0f * ang);
        return (float2)(rad * cos(twist), rad * sin(twist));
    }
    case 43u: {
        float2 p = z; p.x = fabs(p.x); p.y = fabs(p.y);
        float m = dot(p, p);
        if (m < 0.25f) p *= 4.0f; else if (m < 1.0f) p *= (1.0f / m);
        p = p * 1.5f - (float2)(0.5f, 0.5f); return p;
    }
    default: return c_sin(z);
    }
}

// HSV to RGB conversion
inline float3 volumetric_hsv2rgb(float h, float s, float v) {
    float c = v * s;
    float h_prime = fmod(fabs(h) * 6.0f, 6.0f);
    float x = c * (1.0f - fabs(fmod(h_prime, 2.0f) - 1.0f));
    float3 rgb = (float3)(0.0f);
    if (h_prime < 1.0f)      rgb = (float3)(c, x, 0.0f);
    else if (h_prime < 2.0f) rgb = (float3)(x, c, 0.0f);
    else if (h_prime < 3.0f) rgb = (float3)(0.0f, c, x);
    else if (h_prime < 4.0f) rgb = (float3)(0.0f, x, c);
    else if (h_prime < 5.0f) rgb = (float3)(x, 0.0f, c);
    else                     rgb = (float3)(c, 0.0f, x);
    float m = v - c;
    return rgb + (float3)(m);
}

// -----------------------------------------------------------------------------
// Trilinear Volume Sampler
// -----------------------------------------------------------------------------
// Evaluates continuous spatial memory by sampling the previous generation's
// 3D volume with hardware-equivalent trilinear filter weights.
// -----------------------------------------------------------------------------
inline VolumetricVoxelGpu sample_volume_trilinear(
    __global const VolumetricVoxelGpu* volume,
    float3 p_norm,
    uint width,
    uint height,
    uint depth,
    bool torusWrap
) {
    // Map normalized [-1, 1]^3 to grid float coordinates [0, Dim-1]
    float3 gridPos = (p_norm * 0.5f + 0.5f) * (float3)(
        convert_float(width - 1u),
        convert_float(height - 1u),
        convert_float(depth - 1u)
    );

    float3 f = floor(gridPos);
    float3 frac = gridPos - f;

    int3 i0 = convert_int3(f);
    int3 i1 = i0 + (int3)(1);

    if (torusWrap) {
        i0.x = (i0.x % (int)width  + (int)width)  % (int)width;
        i0.y = (i0.y % (int)height + (int)height) % (int)height;
        i0.z = (i0.z % (int)depth  + (int)depth)  % (int)depth;

        i1.x = (i1.x % (int)width  + (int)width)  % (int)width;
        i1.y = (i1.y % (int)height + (int)height) % (int)height;
        i1.z = (i1.z % (int)depth  + (int)depth)  % (int)depth;
    } else {
        i0 = clamp(i0, (int3)(0), (int3)((int)width - 1, (int)height - 1, (int)depth - 1));
        i1 = clamp(i1, (int3)(0), (int3)((int)width - 1, (int)height - 1, (int)depth - 1));
    }

    // Read 8 surrounding corners
    const VolumetricVoxelGpu c000 = volume[voxel_linear_index(i0.x, i0.y, i0.z, width, height)];
    const VolumetricVoxelGpu c100 = volume[voxel_linear_index(i1.x, i0.y, i0.z, width, height)];
    const VolumetricVoxelGpu c010 = volume[voxel_linear_index(i0.x, i1.y, i0.z, width, height)];
    const VolumetricVoxelGpu c110 = volume[voxel_linear_index(i1.x, i1.y, i0.z, width, height)];
    const VolumetricVoxelGpu c001 = volume[voxel_linear_index(i0.x, i0.y, i1.z, width, height)];
    const VolumetricVoxelGpu c101 = volume[voxel_linear_index(i1.x, i0.y, i1.z, width, height)];
    const VolumetricVoxelGpu c011 = volume[voxel_linear_index(i0.x, i1.y, i1.z, width, height)];
    const VolumetricVoxelGpu c111 = volume[voxel_linear_index(i1.x, i1.y, i1.z, width, height)];

    // Trilinear blend for colorDensity
    float4 c00 = mix(c000.colorDensity, c100.colorDensity, frac.x);
    float4 c10 = mix(c010.colorDensity, c110.colorDensity, frac.x);
    float4 c01 = mix(c001.colorDensity, c101.colorDensity, frac.x);
    float4 c11 = mix(c011.colorDensity, c111.colorDensity, frac.x);
    float4 c0 = mix(c00, c10, frac.y);
    float4 c1 = mix(c01, c11, frac.y);
    float4 colorDensity = mix(c0, c1, frac.z);

    // Trilinear blend for velocityEnergy
    float4 v00 = mix(c000.velocityEnergy, c100.velocityEnergy, frac.x);
    float4 v10 = mix(c010.velocityEnergy, c110.velocityEnergy, frac.x);
    float4 v01 = mix(c001.velocityEnergy, c101.velocityEnergy, frac.x);
    float4 v11 = mix(c011.velocityEnergy, c111.velocityEnergy, frac.x);
    float4 v0 = mix(v00, v10, frac.y);
    float4 v1 = mix(v01, v11, frac.y);
    float4 velocityEnergy = mix(v0, v1, frac.z);

    VolumetricVoxelGpu result;
    result.colorDensity = colorDensity;
    result.velocityEnergy = velocityEnergy;
    return result;
}

// -----------------------------------------------------------------------------
// Stage 1 Seed Kernel: evaluate_volumetric_seed_3d
// -----------------------------------------------------------------------------
__kernel void evaluate_volumetric_seed_3d(
    __global VolumetricVoxelGpu* aux_volume,
    __constant const VolumetricFieldDescriptorGpu* desc,
    uint innerFunctionId,
    float4 audioSpectrum
) {
    (void)innerFunctionId;
    const uint gx = get_global_id(0);
    const uint gy = get_global_id(1);
    const uint gz = get_global_id(2);

    const uint width  = desc->extentCadence.z; // auxDim stored in extentCadence.z
    const uint height = desc->extentCadence.z;
    const uint depth  = desc->extentCadence.z;

    if (gx >= width || gy >= height || gz >= depth) return;
    const uint outIndex = voxel_linear_index(gx, gy, gz, width, height);

    float3 p = (float3)(
        (2.0f * (convert_float(gx) + 0.5f) / convert_float(width))  - 1.0f,
        (2.0f * (convert_float(gy) + 0.5f) / convert_float(height)) - 1.0f,
        (2.0f * (convert_float(gz) + 0.5f) / convert_float(depth))  - 1.0f
    );
    const float time = desc->transformParameters.x;
    const float latentW = desc->transformParameters.z;
    const uint flags = desc->extentCadence.w;
    const float seedAlphaParam = desc->worldMaximum.w;
    const float audioBass   = audioSpectrum.x;
    const float audioFlux   = audioSpectrum.w;
    const bool hasAcousticEnergy = (audioBass > 1e-4f || audioFlux > 1e-4f);

    float hueShift = time * 0.08f + latentW * 0.15f;
    if (hasAcousticEnergy) hueShift += audioFlux * 0.35f;
    float hue = fmod(fabs(hueShift + length(p) * 0.25f), 1.0f);

    float w_seed = 0.65f;
    float3 absP = fabs(p);
    float wx = (absP.x + w_seed - 1.0f) / w_seed;
    float wy = (absP.y + w_seed - 1.0f) / w_seed;
    float wz = (absP.z + w_seed - 1.0f) / w_seed;
    float w_val = clamp(fmax(wx, fmax(wy, wz)), 0.0f, 1.0f);
    if (hasAcousticEnergy) {
        w_val = clamp(w_val + audioBass * 0.30f + audioFlux * 0.25f, 0.0f, 1.0f);
    }
    float3 seedColor = volumetric_hsv2rgb(fmod(hue + 0.33f, 1.0f), 1.0f, w_val);
    float seedAlpha = (flags & FLAG_ENABLE_WCA_SEED) ? (w_val * seedAlphaParam) : 0.0f;

    VolumetricVoxelGpu outputVoxel;
    outputVoxel.colorDensity = (float4)(seedColor, seedAlpha);
    outputVoxel.velocityEnergy = (float4)(0.0f);
    aux_volume[outIndex] = outputVoxel;
}

// -----------------------------------------------------------------------------
// Primary Kernel: evolve_volumetric_epimorphism_3d
// -----------------------------------------------------------------------------
// Educational Annotation: PARALLEL 3D VOLUMETRIC FEEDBACK WORKGROUP SCHEDULING
// Executed across a 3-dimensional NDRange (width x height x depth).
// Each work item calculates one voxel in the double-buffered 3D memory core.
// Supports multi-spectral acoustic coupling (bass, mid, treble, flux) to enable
// real-time cymatic standing waves and transient seed pulse modulation.
// -----------------------------------------------------------------------------
__kernel void evolve_volumetric_epimorphism_3d(
    __global const VolumetricVoxelGpu* prev_volume,
    __global VolumetricVoxelGpu* next_volume,
    __global const VolumetricVoxelGpu* aux_volume,
    __constant const VolumetricFieldDescriptorGpu* desc,
    uint innerFunctionId,
    float4 audioSpectrum
) {
    const uint gx = get_global_id(0);
    const uint gy = get_global_id(1);
    const uint gz = get_global_id(2);

    const uint width  = desc->identity.z;
    const uint height = desc->identity.w;
    const uint depth  = desc->extentCadence.x;

    if (gx >= width || gy >= height || gz >= depth) return;

    const uint outIndex = voxel_linear_index(gx, gy, gz, width, height);

    // 1. Normalized coordinate in [-1, 1]^3
    float3 p = (float3)(
        (2.0f * (convert_float(gx) + 0.5f) / convert_float(width))  - 1.0f,
        (2.0f * (convert_float(gy) + 0.5f) / convert_float(height)) - 1.0f,
        (2.0f * (convert_float(gz) + 0.5f) / convert_float(depth))  - 1.0f
    );

    const float time = desc->transformParameters.x;
    const float latentW = desc->transformParameters.z;
    const float rotorAlpha = desc->transformParameters.w;
    const uint flags = desc->extentCadence.w;
    const float decay = desc->worldMinimum.w;
    const uint auxDim = desc->extentCadence.z;

    // Acoustic synesthesia spectral bands
    const float audioBass   = audioSpectrum.x;
    const float audioMid    = audioSpectrum.y;
    const float audioTreble = audioSpectrum.z;
    const float audioFlux   = audioSpectrum.w;
    const bool hasAcousticEnergy = (audioBass > 1e-4f || audioMid > 1e-4f || audioFlux > 1e-4f);

    // 2. Pre-affine rotation (SO(3) + Left rotor angle alpha)
    float cosA = cos(rotorAlpha + time * 0.15f);
    float sinA = sin(rotorAlpha + time * 0.15f);
    float3 p_rot = (float3)(
        p.x * cosA - p.y * sinA,
        p.x * sinA + p.y * cosA,
        p.z
    );

    // Incorporate 4D latent coordinate into Z-axis foliation phase
    p_rot.z += sin(latentW * 2.0f + p_rot.x * 1.5f) * 0.12f;

    // 3. Cyclic Non-Linear Inner Map Lift (XY, YZ, ZX planes)
    float2 z_xy = (float2)(p_rot.x, p_rot.y);
    float2 z_yz = (float2)(p_rot.y, p_rot.z);
    float2 z_zx = (float2)(p_rot.z, p_rot.x);

    float2 w_xy = evaluate_complex_inner(z_xy, innerFunctionId);
    float2 w_yz = evaluate_complex_inner(z_yz, innerFunctionId);
    float2 w_zx = evaluate_complex_inner(z_zx, innerFunctionId);

    // Reconstruct 3D position by averaging coupled plane contributions
    float3 p_warped = (float3)(
        0.5f * (w_xy.x + w_zx.y),
        0.5f * (w_yz.x + w_xy.y),
        0.5f * (w_zx.x + w_yz.y)
    );

    // 4. Domain Reduction: 3D Torus fold [-1, 1]^3
    float3 p_reduced = p_warped;
    const bool torusWrap = (flags & FLAG_ENABLE_TORUS_WRAP) != 0u;
    if (torusWrap) {
        p_reduced = p_warped - 2.0f * floor((p_warped + 1.0f) * 0.5f);
    } else {
        p_reduced = clamp(p_warped, -1.0f, 1.0f);
    }

    // 5. Sample Previous Generation Volume Trilinearly
    VolumetricVoxelGpu sampledHistory;
    if (flags & FLAG_ENABLE_FEEDBACK) {
        sampledHistory = sample_volume_trilinear(
            prev_volume, p_reduced, width, height, depth, torusWrap);
    } else {
        sampledHistory.colorDensity = (float4)(0.0f);
        sampledHistory.velocityEnergy = (float4)(0.0f);
    }

    // 6. Color & Material Rotation with Acoustic Cymatics
    float3 flowVec = p_warped - p_rot;
    const float radius3D = length(p);

    if (hasAcousticEnergy) {
        const float cymaticPulse = cos(radius3D * (7.0f + audioMid * 6.0f) - time * 4.0f) * audioBass * 0.25f;
        const float invR = 1.0f / (radius3D + 1e-3f);
        flowVec += p * invR * cymaticPulse;
    }

    float orbitDelta = length(flowVec);
    float accumulatedEnergy = sampledHistory.velocityEnergy.w * 0.92f + orbitDelta * 0.08f;
    if (hasAcousticEnergy) {
        accumulatedEnergy += (audioBass * 0.05f + audioFlux * 0.08f);
    }
    
    // Perceptual hue rotation over time and local orbit tension
    float hueShift = time * 0.08f + accumulatedEnergy * 0.45f + latentW * 0.15f;
    if (hasAcousticEnergy) {
        hueShift += (audioFlux * 0.35f + audioTreble * 0.20f);
    }
    
    // Convert back/forth for smooth hue drift
    float hue = fmod(fabs(hueShift + radius3D * 0.25f), 1.0f);
    float sat = clamp(0.75f + 0.25f * sin(time * 0.3f) + audioMid * 0.15f, 0.0f, 1.0f);
    float val = clamp(sampledHistory.colorDensity.w * 1.2f + audioBass * 0.15f, 0.0f, 1.0f);
    float3 evolvedColor = volumetric_hsv2rgb(hue, sat, val) * decay;

    // 7. Sample Pre-Evaluated 3D Volumetric WCA Seed Auxiliary Volume
    VolumetricVoxelGpu sampledSeed = sample_volume_trilinear(
        aux_volume, p, auxDim, auxDim, auxDim, false);

    float3 seedColor = sampledSeed.colorDensity.xyz;
    float seedAlpha = sampledSeed.colorDensity.w;

    // 8. Blend Memory and Fresh Seed
    float3 finalColor = mix(evolvedColor, seedColor, seedAlpha);
    float finalDensity = clamp(sampledHistory.colorDensity.w * decay + seedAlpha * 0.75f, 0.0f, 1.0f);

    VolumetricVoxelGpu outputVoxel;
    outputVoxel.colorDensity = (float4)(finalColor, finalDensity);
    outputVoxel.velocityEnergy = (float4)(flowVec, accumulatedEnergy);

    next_volume[outIndex] = outputVoxel;
}
