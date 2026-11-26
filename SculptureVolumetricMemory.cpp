// -----------------------------------------------------------------------------
// Educational Annotation: 3D VOLUMETRIC RECURSIVE MEMORY IMPLEMENTATION
// -----------------------------------------------------------------------------
// Realizes Phase 3 of the 3D Higher-Dimensional Procedural World Vision:
// Camera-Independent Recursive Memory Core (docu/3D-HIGHER-DIMENSIONAL-WORLD-VISION.md).
//
// Bridges the canonical 6-stage epimorphic feedback loop into 3-dimensional
// Euclidean world space, offering:
// 1. GPU OpenCL Parallel Dispatch: 3D NDRange compute workgroups executing
//    `evolve_volumetric_epimorphism_3d` with ping-pong VRAM buffers.
// 2. Headless CPU Oracle: Explicit reference/recovery support for environments
//    without dedicated OpenCL hardware; full numerical parity is not claimed.
// 3. Hierarchical 3D Macro-Block Occupancy: Coarse 8x8x8 bitmask enabling
//    O(1) vacancy testing for empty-space leaping in raymarchers.
// 4. Closed-Loop Acoustic Cymatic Synesthesia: Real-time resonance coupling
//    from FFT spectral analysis into volumetric standing waves and seed pulses.
// -----------------------------------------------------------------------------

#include "SculptureVolumetricMemory.h"
#include "SculptureMathOracle.h"
#include "SculptureFilePublication.h"

#include <cmath>
#include <cstring>
#include <complex>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>
#include <bit>
#include <filesystem>
#include <atomic>
#include <stdexcept>
#include <utility>

namespace epi_sculpt {

namespace {

// One numeric admission rule serves initialization and live configuration.
// UI ranges remain in adapters; finite artistic coefficients retain their value.
bool isValidVolumeConfig(const VolumetricMemoryConfig& config) noexcept {
    return std::isfinite(config.decayFactor) && std::isfinite(config.seedAlpha) &&
        std::isfinite(config.acousticGain) && std::isfinite(config.worldRadius) &&
        config.worldRadius > 0.0f;
}

// Tokens stay distinct across resets, loads, moves and replacement objects.
// They identify local content, not accepted producer generations or saved state.
std::uint64_t nextVolumeContentRevision() noexcept {
    static std::atomic<std::uint64_t> next{1U};
    return next.fetch_add(1U, std::memory_order_relaxed);
}

inline float positiveFmod(float val, float m) noexcept {
    float r = std::fmod(val, m);
    return r < 0.0f ? r + m : r;
}

inline void hsvToRgb(float h, float s, float v, float& r, float& g, float& b) noexcept {
    float c = v * s;
    float hPrime = std::fmod(std::fabs(h) * 6.0f, 6.0f);
    float x = c * (1.0f - std::fabs(std::fmod(hPrime, 2.0f) - 1.0f));
    float r1 = 0.0f, g1 = 0.0f, b1 = 0.0f;
    if (hPrime < 1.0f)      { r1 = c; g1 = x; b1 = 0.0f; }
    else if (hPrime < 2.0f) { r1 = x; g1 = c; b1 = 0.0f; }
    else if (hPrime < 3.0f) { r1 = 0.0f; g1 = c; b1 = x; }
    else if (hPrime < 4.0f) { r1 = 0.0f; g1 = x; b1 = c; }
    else if (hPrime < 5.0f) { r1 = x; g1 = 0.0f; b1 = c; }
    else                    { r1 = c; g1 = 0.0f; b1 = x; }
    float m = v - c;
    r = r1 + m;
    g = g1 + m;
    b = b1 + m;
}

inline std::complex<float> evaluateInner(const std::complex<float>& z, InnerMap type) noexcept {
    const epi::Complexf result = oracle::applyInner(type, epi::Complexf{z.real(), z.imag()});
    return {result.re, result.im};
}

// Embedded fallback OpenCL C source code ensuring compilation succeeds even if disk files are missing
const char* kEmbeddedVolumetricFeedbackKernel = R"opencl(
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
inline float2 c_sqr(float2 z) {
    return (float2)(z.x * z.x - z.y * z.y, 2.0f * z.x * z.y);
}

inline float2 c_mul(float2 a, float2 b) {
    return (float2)(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
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

inline VolumetricVoxelGpu sample_volume_trilinear(
    __global const VolumetricVoxelGpu* volume,
    float3 p_norm,
    uint width,
    uint height,
    uint depth,
    bool torusWrap
) {
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

    const VolumetricVoxelGpu c000 = volume[voxel_linear_index(i0.x, i0.y, i0.z, width, height)];
    const VolumetricVoxelGpu c100 = volume[voxel_linear_index(i1.x, i0.y, i0.z, width, height)];
    const VolumetricVoxelGpu c010 = volume[voxel_linear_index(i0.x, i1.y, i0.z, width, height)];
    const VolumetricVoxelGpu c110 = volume[voxel_linear_index(i1.x, i1.y, i0.z, width, height)];
    const VolumetricVoxelGpu c001 = volume[voxel_linear_index(i0.x, i0.y, i1.z, width, height)];
    const VolumetricVoxelGpu c101 = volume[voxel_linear_index(i1.x, i0.y, i1.z, width, height)];
    const VolumetricVoxelGpu c011 = volume[voxel_linear_index(i0.x, i1.y, i1.z, width, height)];
    const VolumetricVoxelGpu c111 = volume[voxel_linear_index(i1.x, i1.y, i1.z, width, height)];

    float4 c00 = mix(c000.colorDensity, c100.colorDensity, frac.x);
    float4 c10 = mix(c010.colorDensity, c110.colorDensity, frac.x);
    float4 c01 = mix(c001.colorDensity, c101.colorDensity, frac.x);
    float4 c11 = mix(c011.colorDensity, c111.colorDensity, frac.x);
    float4 c0 = mix(c00, c10, frac.y);
    float4 c1 = mix(c01, c11, frac.y);
    float4 colorDensity = mix(c0, c1, frac.z);

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

__kernel void evaluate_volumetric_seed_3d(
    __global VolumetricVoxelGpu* aux_volume,
    __constant const VolumetricFieldDescriptorGpu* desc,
    uint innerFunctionId,
    float4 audioSpectrum
) {
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

    const float audioBass   = audioSpectrum.x;
    const float audioMid    = audioSpectrum.y;
    const float audioTreble = audioSpectrum.z;
    const float audioFlux   = audioSpectrum.w;
    const bool hasAcousticEnergy = (audioBass > 1e-4f || audioMid > 1e-4f || audioFlux > 1e-4f);

    float cosA = cos(rotorAlpha + time * 0.15f);
    float sinA = sin(rotorAlpha + time * 0.15f);
    float3 p_rot = (float3)(
        p.x * cosA - p.y * sinA,
        p.x * sinA + p.y * cosA,
        p.z
    );

    p_rot.z += sin(latentW * 2.0f + p_rot.x * 1.5f) * 0.12f;

    float2 z_xy = (float2)(p_rot.x, p_rot.y);
    float2 z_yz = (float2)(p_rot.y, p_rot.z);
    float2 z_zx = (float2)(p_rot.z, p_rot.x);

    float2 w_xy = evaluate_complex_inner(z_xy, innerFunctionId);
    float2 w_yz = evaluate_complex_inner(z_yz, innerFunctionId);
    float2 w_zx = evaluate_complex_inner(z_zx, innerFunctionId);

    float3 p_warped = (float3)(
        0.5f * (w_xy.x + w_zx.y),
        0.5f * (w_yz.x + w_xy.y),
        0.5f * (w_zx.x + w_yz.y)
    );

    float3 p_reduced = p_warped;
    const bool torusWrap = (flags & FLAG_ENABLE_TORUS_WRAP) != 0u;
    if (torusWrap) {
        p_reduced = p_warped - 2.0f * floor((p_warped + 1.0f) * 0.5f);
    } else {
        p_reduced = clamp(p_warped, -1.0f, 1.0f);
    }

    VolumetricVoxelGpu sampledHistory;
    if (flags & FLAG_ENABLE_FEEDBACK) {
        sampledHistory = sample_volume_trilinear(
            prev_volume, p_reduced, width, height, depth, torusWrap);
    } else {
        sampledHistory.colorDensity = (float4)(0.0f);
        sampledHistory.velocityEnergy = (float4)(0.0f);
    }

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

    float hueShift = time * 0.08f + accumulatedEnergy * 0.45f + latentW * 0.15f;
    if (hasAcousticEnergy) {
        hueShift += (audioFlux * 0.35f + audioTreble * 0.20f);
    }

    float hue = fmod(fabs(hueShift + radius3D * 0.25f), 1.0f);
    float sat = clamp(0.75f + 0.25f * sin(time * 0.3f) + audioMid * 0.15f, 0.0f, 1.0f);
    float val = clamp(sampledHistory.colorDensity.w * 1.2f + audioBass * 0.15f, 0.0f, 1.0f);
    float3 evolvedColor = volumetric_hsv2rgb(hue, sat, val) * decay;

    VolumetricVoxelGpu sampledSeed = sample_volume_trilinear(
        aux_volume, p, auxDim, auxDim, auxDim, false);

    float3 seedColor = sampledSeed.colorDensity.xyz;
    float seedAlpha = sampledSeed.colorDensity.w;

    float3 finalColor = mix(evolvedColor, seedColor, seedAlpha);
    float finalDensity = clamp(sampledHistory.colorDensity.w * decay + seedAlpha * 0.75f, 0.0f, 1.0f);

    VolumetricVoxelGpu outputVoxel;
    outputVoxel.colorDensity = (float4)(finalColor, finalDensity);
    outputVoxel.velocityEnergy = (float4)(flowVec, accumulatedEnergy);

    next_volume[outIndex] = outputVoxel;
}
)opencl";

} // namespace

SculptureVolumetricMemory::SculptureVolumetricMemory(const VolumetricMemoryConfig& config) {
    // Pre-allocate maximum capacity (128^3 voxels = 64 MiB total) to prevent heap thrashing
    const std::size_t maxVoxels = static_cast<std::size_t>(volumetric_gpu_abi::kFullGridDimension)
        * volumetric_gpu_abi::kFullGridDimension * volumetric_gpu_abi::kFullGridDimension;
    volumeA_.resize(maxVoxels);
    volumeB_.resize(maxVoxels);

    // There is no predecessor domain during construction. Initialize once,
    // instead of taking the live replacement path before the initial reset.
    if (isValidVolumeConfig(config)) {
        config_ = config;
    }
    activeDim_ = std::clamp(config_.baseDimension,
        volumetric_gpu_abi::kQuarterGridDimension,
        volumetric_gpu_abi::kFullGridDimension);
    updateDescriptor();
    reset();
}

SculptureVolumetricMemory::~SculptureVolumetricMemory() {
    releaseGpu();
}

SculptureVolumetricMemory::SculptureVolumetricMemory(SculptureVolumetricMemory&& other) noexcept {
    // Member defaults are resource-empty: reuse the complete assignment path
    // without invoking the ordinary constructor's full-size voxel allocations.
    *this = std::move(other);
}

SculptureVolumetricMemory& SculptureVolumetricMemory::operator=(SculptureVolumetricMemory&& other) noexcept {
    if (this != &other) {
#if EPI_SCULPT_HAS_OPENCL
        // Nonblocking uploads borrow source-owned staging. Complete queued work
        // while those addresses are alive before relocating their owner.
        // Like releaseGpu(), this noexcept boundary retains best-effort cleanup
        // on native errors; moving is not a device-loss recovery transaction.
        if (other.queue_ != nullptr) {
            clFinish(other.queue_);
        }
#endif
        other.releaseDescriptorUploads();
        releaseGpu();

        config_ = other.config_;
        currentScale_ = other.currentScale_;
        activeDim_ = other.activeDim_;
        readBufferA_ = other.readBufferA_;
        volumeA_ = std::move(other.volumeA_);
        volumeB_ = std::move(other.volumeB_);
        descriptor_ = other.descriptor_;
        telemetry_ = other.telemetry_;
        macroOccupancy_ = other.macroOccupancy_;
        macroOccupancyRatio_ = other.macroOccupancyRatio_;

        // One transfer list pairs every device handle with source disarming.
        // Context/device are borrowed; queue/program/kernels/buffers are owned.
        context_ = std::exchange(other.context_, nullptr);
        device_ = std::exchange(other.device_, nullptr);
        queue_ = std::exchange(other.queue_, nullptr);
        program_ = std::exchange(other.program_, nullptr);
        kernel_ = std::exchange(other.kernel_, nullptr);
        auxKernel_ = std::exchange(other.auxKernel_, nullptr);
        volumeGpuA_ = std::exchange(other.volumeGpuA_, nullptr);
        volumeGpuB_ = std::exchange(other.volumeGpuB_, nullptr);
        auxVolumeGpu_ = std::exchange(other.auxVolumeGpu_, nullptr);
        descriptorBuffer_ = std::exchange(other.descriptorBuffer_, nullptr);
        auxDim_ = other.auxDim_;
        auxVolumeCpu_ = std::move(other.auxVolumeCpu_);
        gpuReadA_ = other.gpuReadA_;
        gpuActive_ = other.gpuActive_;
        gpuInitialized_ = std::exchange(other.gpuInitialized_, false);
        gpuEnabled_ = other.gpuEnabled_;
        hostBuffersDirty_ = std::exchange(other.hostBuffersDirty_, false);
        contentRevision_ = nextVolumeContentRevision();
        other.contentRevision_ = nextVolumeContentRevision();

        other.deactivateGpu("CPU-Oracle (Moved From)");
    }
    return *this;
}

void SculptureVolumetricMemory::setConfig(const VolumetricMemoryConfig& config) {
    (void)trySetConfig(config);
}

bool SculptureVolumetricMemory::trySetConfig(const VolumetricMemoryConfig& config) {
    // Validate the complete candidate before assigning any part of it. This
    // protects direct callers as well as events without making UI ranges into
    // universal constraints on the mathematical/artistic coefficients.
    if (!isValidVolumeConfig(config)) {
        return false;
    }

    const bool baseDimensionChanged = config.baseDimension != config_.baseDimension;
    const auto dimension = baseDimensionChanged
        ? std::clamp(config.baseDimension, volumetric_gpu_abi::kQuarterGridDimension,
                     volumetric_gpu_abi::kFullGridDimension)
        : activeDim_;
    const bool domainChanged = dimension != activeDim_ || config.worldRadius != config_.worldRadius;
    config_ = config;
    // Ordinary controls edit a copy of config(). Only an explicit base edit
    // selects a new extent; otherwise the staged governor owns activeDim_.
    activeDim_ = dimension;
    if (domainChanged) {
        replaceDomainHistory();
    } else {
        updateDescriptor();
    }
    return true;
}

void SculptureVolumetricMemory::applyResolutionScale(StageResolutionScale scale) {
    if (!isValidStageResolutionScale(scale)) {
        return;
    }
    std::uint32_t dimension = activeDim_;
    switch (scale) {
    case StageResolutionScale::Full:
        dimension = volumetric_gpu_abi::kFullGridDimension; // 128
        break;
    case StageResolutionScale::Half:
        dimension = volumetric_gpu_abi::kHalfGridDimension; // 64
        break;
    case StageResolutionScale::Quarter:
    case StageResolutionScale::Eighth:
        dimension = volumetric_gpu_abi::kQuarterGridDimension; // 32
        break;
    }
    // A custom/restored extent can differ even when the selected tier is equal.
    if (scale == currentScale_ && dimension == activeDim_) {
        return;
    }
    const bool domainChanged = dimension != activeDim_;
    currentScale_ = scale;
    activeDim_ = dimension;
    if (domainChanged) {
        replaceDomainHistory();
    } else {
        updateDescriptor();
    }
}

void SculptureVolumetricMemory::replaceDomainHistory() {
    // The caller has committed the new domain. Old strides/world coordinates
    // cannot identify its feedback, so publish vacuum rather than reinterpret or
    // read back the predecessor. Storage is preallocated at the maximum extent.
    const auto count = activeVoxelCount();
    std::fill_n(volumeA_.begin(), count, volumetric_gpu_abi::VolumetricVoxelGpu{});
    std::fill_n(volumeB_.begin(), count, volumetric_gpu_abi::VolumetricVoxelGpu{});
    hostBuffersDirty_ = false;
    macroOccupancy_.fill(0ULL);
    macroOccupancyRatio_ = 0.0f;
    telemetry_.macroOccupancyRatio = 0.0f;
    telemetry_.meanDensity = 0.0f;
    telemetry_.peakEnergy = 0.0f;
    contentRevision_ = nextVolumeContentRevision();
    updateDescriptor();

    // Preserve accepted clocks and executor preference. Upload uses the existing
    // in-order queue; failure leaves the new host field available for CPU use.
    if (gpuActive_ && !syncGpuFromHost()) {
        deactivateGpu("CPU-Oracle (Domain Upload Failed)");
    }
}

void SculptureVolumetricMemory::updateDescriptor() {
    descriptor_.identity[0] = volumetric_gpu_abi::kAbiVersion;
    descriptor_.identity[1] = volumetric_gpu_abi::kDescriptorBytes;
    descriptor_.identity[2] = activeDim_;
    descriptor_.identity[3] = activeDim_;

    descriptor_.extentCadence[0] = activeDim_;
    descriptor_.extentCadence[1] = static_cast<std::uint32_t>(currentScale_);
    descriptor_.extentCadence[2] = 1U;

    std::uint32_t flags = volumetric_gpu_abi::VolumetricFlag_None;
    if (config_.enableFeedback)    flags |= volumetric_gpu_abi::VolumetricFlag_EnableFeedback;
    if (config_.enableTorusWrap)   flags |= volumetric_gpu_abi::VolumetricFlag_EnableTorusWrap;
    if (config_.enableWcaSeed)     flags |= volumetric_gpu_abi::VolumetricFlag_EnableWcaSeed;
    if (config_.enableHyper4DWave) flags |= volumetric_gpu_abi::VolumetricFlag_EnableHyper4DWave;
    descriptor_.extentCadence[3] = flags;

    descriptor_.worldMinimum[0] = -config_.worldRadius;
    descriptor_.worldMinimum[1] = -config_.worldRadius;
    descriptor_.worldMinimum[2] = -config_.worldRadius;
    descriptor_.worldMinimum[3] = config_.decayFactor;

    descriptor_.worldMaximum[0] = config_.worldRadius;
    descriptor_.worldMaximum[1] = config_.worldRadius;
    descriptor_.worldMaximum[2] = config_.worldRadius;
    descriptor_.worldMaximum[3] = config_.seedAlpha;

    telemetry_.activeDimension = activeDim_;
    telemetry_.activeVoxelCount = activeVoxelCount();
    telemetry_.bufferBytes = activeVoxelCount() * sizeof(volumetric_gpu_abi::VolumetricVoxelGpu);
    telemetry_.activeScaleTier = currentScale_;
}

void SculptureVolumetricMemory::reset() {
    std::fill(volumeA_.begin(), volumeA_.end(), volumetric_gpu_abi::VolumetricVoxelGpu{});
    std::fill(volumeB_.begin(), volumeB_.end(), volumetric_gpu_abi::VolumetricVoxelGpu{});
    macroOccupancy_.fill(0ULL);
    macroOccupancyRatio_ = 0.0f;
    telemetry_.stepCount = 0U;
    telemetry_.meanDensity = 0.0f;
    telemetry_.peakEnergy = 0.0f;
    telemetry_.macroOccupancyRatio = 0.0f;
    readBufferA_ = true;
    hostBuffersDirty_ = false;
    contentRevision_ = nextVolumeContentRevision();

    if (gpuInitialized_ && !syncGpuFromHost()) {
        deactivateGpu("CPU-Oracle (Reset Upload Failed)");
    }
}

std::string SculptureVolumetricMemory::resolveKernelSource(const std::string& preferredPath) const {
    const std::vector<std::string> candidatePaths = {
        preferredPath,
        "MODULES/3dEpiSculpT/kernels/SculptureVolumetricFeedback3D.cl",
        "kernels/SculptureVolumetricFeedback3D.cl",
        "../MODULES/3dEpiSculpT/kernels/SculptureVolumetricFeedback3D.cl",
        "../../MODULES/3dEpiSculpT/kernels/SculptureVolumetricFeedback3D.cl",
        "3dEpiSculpTMorph/kernels/SculptureVolumetricFeedback3D.cl"
    };

    for (const auto& path : candidatePaths) {
        if (path.empty()) continue;
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            std::ostringstream ss;
            ss << file.rdbuf();
            return ss.str();
        }
    }
    // Fallback to embedded OpenCL kernel string
    return std::string(kEmbeddedVolumetricFeedbackKernel);
}

bool SculptureVolumetricMemory::initGpu(
    cl_context context,
    cl_device_id device,
    const std::string& kernelSourceOrPath
) {
#if !EPI_SCULPT_HAS_OPENCL
    (void)context;
    (void)device;
    (void)kernelSourceOrPath;
    telemetry_.backendName = "CPU-Oracle";
    telemetry_.gpuAccelerated = false;
    return false;
#else
    if (context == nullptr || device == nullptr) {
        return false;
    }

    releaseGpu();

    context_ = context;
    device_ = device;

    cl_int res = CL_SUCCESS;
    const cl_queue_properties qProps[] = {
        CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0
    };
    queue_ = clCreateCommandQueueWithProperties(context_, device_, qProps, &res);
    if (res != CL_SUCCESS || queue_ == nullptr) {
        // Fallback for OpenCL 1.2 / 2.0 implementations
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
        queue_ = clCreateCommandQueue(context_, device_, CL_QUEUE_PROFILING_ENABLE, &res);
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
        if (res != CL_SUCCESS || queue_ == nullptr) {
            releaseGpu();
            return false;
        }
    }

    // Allocate ping-pong buffers for maximum full resolution (128^3 = 64 MiB each)
    const std::size_t maxVoxelBytes = static_cast<std::size_t>(volumetric_gpu_abi::kFullGridDimension)
        * volumetric_gpu_abi::kFullGridDimension
        * volumetric_gpu_abi::kFullGridDimension
        * sizeof(volumetric_gpu_abi::VolumetricVoxelGpu);

    volumeGpuA_ = clCreateBuffer(context_, CL_MEM_READ_WRITE, maxVoxelBytes, nullptr, &res);
    if (res != CL_SUCCESS || volumeGpuA_ == nullptr) {
        releaseGpu();
        return false;
    }

    volumeGpuB_ = clCreateBuffer(context_, CL_MEM_READ_WRITE, maxVoxelBytes, nullptr, &res);
    if (res != CL_SUCCESS || volumeGpuB_ == nullptr) {
        releaseGpu();
        return false;
    }

    auxVolumeGpu_ = clCreateBuffer(context_, CL_MEM_READ_WRITE, maxVoxelBytes, nullptr, &res);
    if (res != CL_SUCCESS || auxVolumeGpu_ == nullptr) {
        releaseGpu();
        return false;
    }

    descriptorBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(volumetric_gpu_abi::VolumetricFieldDescriptorGpu), &descriptor_, &res);
    if (res != CL_SUCCESS || descriptorBuffer_ == nullptr) {
        releaseGpu();
        return false;
    }

    // Compile OpenCL Kernel Program
    const std::string kernelSource = resolveKernelSource(kernelSourceOrPath);
    const char* srcPtr = kernelSource.c_str();
    const size_t srcLen = kernelSource.size();

    program_ = clCreateProgramWithSource(context_, 1U, &srcPtr, &srcLen, &res);
    if (res != CL_SUCCESS || program_ == nullptr) {
        releaseGpu();
        return false;
    }

    const char* buildOptions = "-cl-fast-relaxed-math -cl-mad-enable";
    res = clBuildProgram(program_, 1U, &device_, buildOptions, nullptr, nullptr);
    if (res != CL_SUCCESS) {
        size_t logSize = 0;
        clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
        std::string buildLog(logSize, '\0');
        clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, logSize, buildLog.data(), nullptr);
        std::cerr << "[SculptureVolumetricMemory] OpenCL Program Build Error:\n" << buildLog << std::endl;
        releaseGpu();
        return false;
    }

    kernel_ = clCreateKernel(program_, "evolve_volumetric_epimorphism_3d", &res);
    if (res != CL_SUCCESS || kernel_ == nullptr) {
        releaseGpu();
        return false;
    }

    auxKernel_ = clCreateKernel(program_, "evaluate_volumetric_seed_3d", &res);
    if (res != CL_SUCCESS || auxKernel_ == nullptr) {
        // Evolution samples this kernel's output. Missing seed production cannot
        // be treated as a usable GPU backend with an undefined auxiliary field.
        std::cerr << "[SculptureVolumetricMemory] Missing 'evaluate_volumetric_seed_3d' kernel." << std::endl;
        releaseGpu();
        return false;
    }

    // Preparation is distinct from readiness: syncGpuFromHost() deliberately
    // requires an initialized backend, so startup uses the resource-level path.
    // Only the read field needs seeding; evolution fully writes the alternate.
    gpuReadA_ = true;
    if (!uploadHostField()) {
        releaseGpu();
        return false;
    }

    gpuInitialized_ = true;
    gpuActive_ = gpuEnabled_;

    telemetry_.backendName = gpuActive_ ? "OpenCL-GPU" : "CPU-Oracle";
    telemetry_.gpuAccelerated = gpuActive_;
    return true;
#endif
}

void SculptureVolumetricMemory::releaseGpu() noexcept {
    // This existing path discards any device-only field and exposes the retained
    // host copy. Make that content change visible even when stepCount is equal.
    if (hostBuffersDirty_) {
        contentRevision_ = nextVolumeContentRevision();
    }
#if EPI_SCULPT_HAS_OPENCL
    if (queue_ != nullptr) {
        clFinish(queue_);
    }
#endif
    releaseDescriptorUploads();
#if EPI_SCULPT_HAS_OPENCL
    if (queue_ != nullptr) {
        clReleaseCommandQueue(queue_);
        queue_ = nullptr;
    }
    if (kernel_ != nullptr) {
        clReleaseKernel(kernel_);
        kernel_ = nullptr;
    }
    if (auxKernel_ != nullptr) {
        clReleaseKernel(auxKernel_);
        auxKernel_ = nullptr;
    }
    if (program_ != nullptr) {
        clReleaseProgram(program_);
        program_ = nullptr;
    }
    if (descriptorBuffer_ != nullptr) {
        clReleaseMemObject(descriptorBuffer_);
        descriptorBuffer_ = nullptr;
    }
    if (volumeGpuA_ != nullptr) {
        clReleaseMemObject(volumeGpuA_);
        volumeGpuA_ = nullptr;
    }
    if (volumeGpuB_ != nullptr) {
        clReleaseMemObject(volumeGpuB_);
        volumeGpuB_ = nullptr;
    }
    if (auxVolumeGpu_ != nullptr) {
        clReleaseMemObject(auxVolumeGpu_);
        auxVolumeGpu_ = nullptr;
    }
#endif
    context_ = nullptr;
    device_ = nullptr;
    gpuInitialized_ = false;
    gpuActive_ = false;
    hostBuffersDirty_ = false;
    telemetry_.backendName = "CPU-Oracle";
    telemetry_.gpuAccelerated = false;
}

void SculptureVolumetricMemory::deactivateGpu(const char* backendName) noexcept {
    gpuActive_ = false;
    telemetry_.backendName = backendName;
    telemetry_.gpuAccelerated = false;
}

void SculptureVolumetricMemory::setGpuEnabled(bool enabled) noexcept {
    if (gpuEnabled_ == enabled &&
        ((enabled && gpuActive_) || (!enabled && !hostBuffersDirty_))) {
        return;
    }
    gpuEnabled_ = enabled;
    if (gpuEnabled_ && gpuInitialized_) {
        // Failed/disabled readback may leave the newest field on the device.
        // Reuse it directly; uploading the retained host copy would rewind it.
        gpuActive_ = hostBuffersDirty_ || syncGpuFromHost();
        telemetry_.backendName = gpuActive_ ? "OpenCL-GPU" : "CPU-Oracle (Upload Failed)";
        telemetry_.gpuAccelerated = gpuActive_;
    } else {
        ensureHostBufferSynced();
        deactivateGpu(hostBuffersDirty_ ? "CPU-Oracle (Readback Unavailable)" : "CPU-Oracle");
    }
}

void SculptureVolumetricMemory::syncHostFromGpu() const {
#if EPI_SCULPT_HAS_OPENCL
    if (!gpuInitialized_ || queue_ == nullptr || !hostBuffersDirty_) {
        return;
    }

    cl_mem activeGpu = gpuReadA_ ? volumeGpuA_ : volumeGpuB_;
    auto& targetHost = readBufferA_ ? volumeA_ : volumeB_;
    if (activeGpu == nullptr || targetHost.size() < activeVoxelCount()) {
        return;
    }
    const std::size_t activeBytes = activeVoxelCount() * sizeof(volumetric_gpu_abi::VolumetricVoxelGpu);

    cl_int res = clEnqueueReadBuffer(
        queue_,
        activeGpu,
        CL_TRUE, // Blocking read
        0,
        activeBytes,
        targetHost.data(),
        0, nullptr, nullptr
    );

    if (res == CL_SUCCESS) {
        hostBuffersDirty_ = false;
        recomputeMacroOccupancy();
    }
#endif
}

bool SculptureVolumetricMemory::syncGpuFromHost() {
    if (!gpuInitialized_) {
        return false;
    }
    return uploadHostField();
}

bool SculptureVolumetricMemory::uploadHostField() {
#if EPI_SCULPT_HAS_OPENCL
    if (context_ == nullptr || queue_ == nullptr || hostBuffersDirty_) {
        return false;
    }

    const auto& sourceHost = readBufferA_ ? volumeA_ : volumeB_;
    cl_mem activeGpu = gpuReadA_ ? volumeGpuA_ : volumeGpuB_;
    if (activeGpu == nullptr || sourceHost.size() < activeVoxelCount()) {
        return false;
    }
    const std::size_t activeBytes = activeVoxelCount() * sizeof(volumetric_gpu_abi::VolumetricVoxelGpu);

    const cl_int result = clEnqueueWriteBuffer(
        queue_,
        activeGpu,
        CL_TRUE, // Blocking write
        0,
        activeBytes,
        sourceHost.data(),
        0, nullptr, nullptr
    );
    return result == CL_SUCCESS;
#else
    return false;
#endif
}

void SculptureVolumetricMemory::uploadDescriptor() {
#if EPI_SCULPT_HAS_OPENCL
    auto& slot = descriptorUploads_[nextDescriptorUpload_];
    if (slot.completion != nullptr) {
        // The event qualifies this host snapshot, not field acceptance. A failed
        // wait leaves the event and bytes intact for recovery or cold teardown.
        const cl_int waited = clWaitForEvents(1, &slot.completion);
        if (waited != CL_SUCCESS) {
            throw std::runtime_error("descriptor upload wait failed: " + std::to_string(waited));
        }
        clReleaseEvent(slot.completion);
        slot.completion = nullptr;
    }

    slot.descriptor = descriptor_;
    cl_event completion = nullptr;
    const cl_int result = clEnqueueWriteBuffer(queue_, descriptorBuffer_, CL_FALSE,
        0, sizeof(slot.descriptor), &slot.descriptor, 0, nullptr, &completion);
    if (result != CL_SUCCESS) {
        throw std::runtime_error("clEnqueueWriteBuffer(descriptor) failed: " + std::to_string(result));
    }
    // Retain immediately: later argument/dispatch failures do not cancel this
    // accepted transfer or shorten its borrow of the immutable host bytes.
    slot.completion = completion;
    nextDescriptorUpload_ = (nextDescriptorUpload_ + 1) % descriptorUploads_.size();
#endif
}

void SculptureVolumetricMemory::releaseDescriptorUploads() noexcept {
    for (auto& slot : descriptorUploads_) {
#if EPI_SCULPT_HAS_OPENCL
        if (slot.completion != nullptr) clReleaseEvent(slot.completion);
#endif
        slot.completion = nullptr;
    }
    nextDescriptorUpload_ = 0;
}

void SculptureVolumetricMemory::step(
    const SculptureProgramParams& params,
    float simTime,
    float deltaSeconds
) {
    if (isGpuActive()) {
        try {
            stepGpu(params, simTime, deltaSeconds);
            return;
        } catch (...) {
            // Keep the last accepted device field. CPU recovery below acquires
            // that field, or rejects the step if readback is unavailable.
            deactivateGpu("CPU-Oracle (Fallback)");
        }
    }
    stepCpuOracle(params, simTime, deltaSeconds);
}

void SculptureVolumetricMemory::stepGpu(
    const SculptureProgramParams& params,
    float simTime,
    float deltaSeconds
) {
#if !EPI_SCULPT_HAS_OPENCL
    stepCpuOracle(params, simTime, deltaSeconds);
#else
    const auto t0 = std::chrono::steady_clock::now();

    descriptor_.transformParameters[0] = simTime;
    descriptor_.transformParameters[1] = deltaSeconds;
    descriptor_.transformParameters[2] = params.latentW;
    descriptor_.transformParameters[3] = params.rotorAlpha;

    // Determine auxiliary buffer resolution scale
    std::uint32_t auxDim = volumetric_gpu_abi::kQuarterGridDimension;
    switch (params.auxiliaryResolutionScale) {
    case StageResolutionScale::Full: auxDim = volumetric_gpu_abi::kFullGridDimension; break;
    case StageResolutionScale::Half: auxDim = volumetric_gpu_abi::kHalfGridDimension; break;
    case StageResolutionScale::Quarter:
    case StageResolutionScale::Eighth: auxDim = volumetric_gpu_abi::kQuarterGridDimension; break;
    }
    descriptor_.extentCadence[2] = auxDim;

    uploadDescriptor();
    cl_int res = CL_SUCCESS;

    cl_mem prevVol = gpuReadA_ ? volumeGpuA_ : volumeGpuB_;
    cl_mem nextVol = gpuReadA_ ? volumeGpuB_ : volumeGpuA_;

    const cl_uint innerId = static_cast<cl_uint>(params.inner);

    const float audioBass   = (config_.enableAcousticCoupling ? params.audioSpectrum[0] : 0.0f) * config_.acousticGain;
    const float audioMid    = (config_.enableAcousticCoupling ? params.audioSpectrum[1] : 0.0f) * config_.acousticGain;
    const float audioTreble = (config_.enableAcousticCoupling ? params.audioSpectrum[2] : 0.0f) * config_.acousticGain;
    const float audioFlux   = (config_.enableAcousticCoupling ? params.audioSpectrum[3] : 0.0f) * config_.acousticGain;
    const cl_float4 audioSpectrum = {{ audioBass, audioMid, audioTreble, audioFlux }};

    if (auxKernel_) {
        res = clSetKernelArg(auxKernel_, 0U, sizeof(cl_mem), &auxVolumeGpu_);
        res |= clSetKernelArg(auxKernel_, 1U, sizeof(cl_mem), &descriptorBuffer_);
        res |= clSetKernelArg(auxKernel_, 2U, sizeof(cl_uint), &innerId);
        res |= clSetKernelArg(auxKernel_, 3U, sizeof(cl_float4), &audioSpectrum);
        
        const std::size_t auxWorkSize[3] = { auxDim, auxDim, auxDim };
        res |= clEnqueueNDRangeKernel(queue_, auxKernel_, 3U, nullptr, auxWorkSize, nullptr, 0, nullptr, nullptr);
        if (res != CL_SUCCESS) {
            throw std::runtime_error("clEnqueueNDRangeKernel(auxKernel_) failed: " + std::to_string(res));
        }
    }

    res = clSetKernelArg(kernel_, 0U, sizeof(cl_mem), &prevVol);
    res |= clSetKernelArg(kernel_, 1U, sizeof(cl_mem), &nextVol);
    res |= clSetKernelArg(kernel_, 2U, sizeof(cl_mem), &auxVolumeGpu_);
    res |= clSetKernelArg(kernel_, 3U, sizeof(cl_mem), &descriptorBuffer_);
    res |= clSetKernelArg(kernel_, 4U, sizeof(cl_uint), &innerId);
    res |= clSetKernelArg(kernel_, 5U, sizeof(cl_float4), &audioSpectrum);
    if (res != CL_SUCCESS) {
        throw std::runtime_error("clSetKernelArg failed: " + std::to_string(res));
    }

    const std::size_t globalWorkSize[3] = {
        static_cast<std::size_t>(activeDim_),
        static_cast<std::size_t>(activeDim_),
        static_cast<std::size_t>(activeDim_)
    };

    // Calculate optimal 3D local workgroup size (dividing activeDim_)
    std::size_t localWorkSize[3] = { 8U, 8U, 4U };
    if (activeDim_ % 8U != 0U || activeDim_ % 4U != 0U) {
        localWorkSize[0] = 4U;
        localWorkSize[1] = 4U;
        localWorkSize[2] = 2U;
    }

    cl_event kernelEvent = nullptr;
    res = clEnqueueNDRangeKernel(
        queue_,
        kernel_,
        3U,
        nullptr,
        globalWorkSize,
        localWorkSize,
        0, nullptr, &kernelEvent
    );
    if (res != CL_SUCCESS) {
        // Retry with driver-chosen local workgroup size
        res = clEnqueueNDRangeKernel(queue_, kernel_, 3U, nullptr, globalWorkSize, nullptr, 0, nullptr, &kernelEvent);
        if (res != CL_SUCCESS) {
            throw std::runtime_error("clEnqueueNDRangeKernel failed: " + std::to_string(res));
        }
    }

    clFlush(queue_);
    if (kernelEvent != nullptr) {
        clReleaseEvent(kernelEvent);
    }

    // Ping-pong buffer swap on GPU
    gpuReadA_ = !gpuReadA_;
    swapBuffers(); // Keep host logical read/write index aligned
    hostBuffersDirty_ = true;
    contentRevision_ = nextVolumeContentRevision();

    const auto t1 = std::chrono::steady_clock::now();
    telemetry_.lastStepTimeMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
    telemetry_.stepCount++;
    telemetry_.simulationTime = simTime;
    telemetry_.backendName = "OpenCL-GPU";
    telemetry_.gpuAccelerated = true;
#endif
}

void SculptureVolumetricMemory::stepCpuOracle(
    const SculptureProgramParams& params,
    float simTime,
    float deltaSeconds
) {
    // A backend switch changes the executor, not the recursive history. Acquire
    // that history before beginning CPU evolution or changing its intent/counters.
    ensureHostBufferSynced();
    if (!hasCurrentHostBuffer()) {
        throw std::runtime_error("Volumetric CPU evolution requires a current host field");
    }
    const auto t0 = std::chrono::steady_clock::now();

    const auto& readBuf = readBufferA_ ? volumeA_ : volumeB_;
    auto& writeBuf = readBufferA_ ? volumeB_ : volumeA_;

    descriptor_.transformParameters[0] = simTime;
    descriptor_.transformParameters[1] = deltaSeconds;
    descriptor_.transformParameters[2] = params.latentW;
    descriptor_.transformParameters[3] = params.rotorAlpha;

    const std::uint32_t dim = activeDim_;
    const float invDim = 1.0f / static_cast<float>(dim);
    const float decay = config_.decayFactor;
    const float seedAlphaParam = config_.seedAlpha;
    const float time = simTime;
    const float latentW = params.latentW;
    const float rotorAlpha = params.rotorAlpha;

    const float cosA = std::cos(rotorAlpha + time * 0.15f);
    const float sinA = std::sin(rotorAlpha + time * 0.15f);

    const float audioBass = (config_.enableAcousticCoupling ? params.audioSpectrum[0] : 0.0f) * config_.acousticGain;
    const float audioMid = (config_.enableAcousticCoupling ? params.audioSpectrum[1] : 0.0f) * config_.acousticGain;
    const float audioTreble = (config_.enableAcousticCoupling ? params.audioSpectrum[2] : 0.0f) * config_.acousticGain;
    const float audioFlux = (config_.enableAcousticCoupling ? params.audioSpectrum[3] : 0.0f) * config_.acousticGain;
    const bool hasAcousticEnergy = (audioBass > 1e-4f || audioMid > 1e-4f || audioFlux > 1e-4f);

    std::uint32_t auxDim = volumetric_gpu_abi::kQuarterGridDimension;
    switch (params.auxiliaryResolutionScale) {
    case StageResolutionScale::Full: auxDim = volumetric_gpu_abi::kFullGridDimension; break;
    case StageResolutionScale::Half: auxDim = volumetric_gpu_abi::kHalfGridDimension; break;
    case StageResolutionScale::Quarter:
    case StageResolutionScale::Eighth: auxDim = volumetric_gpu_abi::kQuarterGridDimension; break;
    }
    const std::size_t auxVoxels = static_cast<std::size_t>(auxDim) * auxDim * auxDim;
    if (auxVolumeCpu_.size() < auxVoxels) {
        auxVolumeCpu_.resize(auxVoxels);
    }
    
    // Pass 1: Evaluate Seed topologies on lower resolution buffer
    const float invAuxDim = 1.0f / static_cast<float>(auxDim);
    for (std::uint32_t gz = 0U; gz < auxDim; ++gz) {
        const float pzNorm = 2.0f * (static_cast<float>(gz) + 0.5f) * invAuxDim - 1.0f;
        for (std::uint32_t gy = 0U; gy < auxDim; ++gy) {
            const float pyNorm = 2.0f * (static_cast<float>(gy) + 0.5f) * invAuxDim - 1.0f;
            for (std::uint32_t gx = 0U; gx < auxDim; ++gx) {
                const float pxNorm = 2.0f * (static_cast<float>(gx) + 0.5f) * invAuxDim - 1.0f;
                const std::size_t outIdx = gx + auxDim * (gy + auxDim * gz);

                const float rNorm = std::sqrt(pxNorm * pxNorm + pyNorm * pyNorm + pzNorm * pzNorm);
                float hueShift = time * 0.08f + latentW * 0.15f;
                if (hasAcousticEnergy) {
                    hueShift += (audioFlux * 0.35f);
                }
                float hue = positiveFmod(hueShift + rNorm * 0.25f, 1.0f);

                float wSeed = 0.65f;
                float wxSeed = (std::fabs(pxNorm) + wSeed - 1.0f) / wSeed;
                float wySeed = (std::fabs(pyNorm) + wSeed - 1.0f) / wSeed;
                float wzSeed = (std::fabs(pzNorm) + wSeed - 1.0f) / wSeed;
                float wVal = std::clamp(std::max({wxSeed, wySeed, wzSeed}), 0.0f, 1.0f);
                if (hasAcousticEnergy) {
                    wVal = std::clamp(wVal + audioBass * 0.30f + audioFlux * 0.25f, 0.0f, 1.0f);
                }

                float sr = 0.0f, sg = 0.0f, sb = 0.0f;
                hsvToRgb(positiveFmod(hue + 0.33f, 1.0f), 1.0f, wVal, sr, sg, sb);
                float seedAlpha = config_.enableWcaSeed ? (wVal * seedAlphaParam) : 0.0f;

                auxVolumeCpu_[outIdx].colorDensity[0] = sr;
                auxVolumeCpu_[outIdx].colorDensity[1] = sg;
                auxVolumeCpu_[outIdx].colorDensity[2] = sb;
                auxVolumeCpu_[outIdx].colorDensity[3] = seedAlpha;
                auxVolumeCpu_[outIdx].velocityEnergy[0] = 0.0f;
                auxVolumeCpu_[outIdx].velocityEnergy[1] = 0.0f;
                auxVolumeCpu_[outIdx].velocityEnergy[2] = 0.0f;
                auxVolumeCpu_[outIdx].velocityEnergy[3] = 0.0f;
            }
        }
    }

    double totalDensity = 0.0;
    float maxEnergy = 0.0f;

    for (std::uint32_t gz = 0U; gz < dim; ++gz) {
        const float pzNorm = 2.0f * (static_cast<float>(gz) + 0.5f) * invDim - 1.0f;
        for (std::uint32_t gy = 0U; gy < dim; ++gy) {
            const float pyNorm = 2.0f * (static_cast<float>(gy) + 0.5f) * invDim - 1.0f;
            for (std::uint32_t gx = 0U; gx < dim; ++gx) {
                const float pxNorm = 2.0f * (static_cast<float>(gx) + 0.5f) * invDim - 1.0f;
                const std::size_t outIdx = gx + dim * (gy + dim * gz);

                // 1. SO(3) Rotor Rotation
                float rx = pxNorm * cosA - pyNorm * sinA;
                float ry = pxNorm * sinA + pyNorm * cosA;
                float rz = pzNorm;

                if (config_.enableHyper4DWave) {
                    rz += std::sin(latentW * 2.0f + rx * 1.5f) * 0.12f;
                }

                // 2. Cyclic Inner Map Lift
                std::complex<float> z_xy(rx, ry);
                std::complex<float> z_yz(ry, rz);
                std::complex<float> z_zx(rz, rx);

                auto w_xy = evaluateInner(z_xy, params.inner);
                auto w_yz = evaluateInner(z_yz, params.inner);
                auto w_zx = evaluateInner(z_zx, params.inner);

                float wx = 0.5f * (w_xy.real() + w_zx.imag());
                float wy = 0.5f * (w_yz.real() + w_xy.imag());
                float wz = 0.5f * (w_zx.real() + w_yz.imag());

                // 3. Domain Reduction
                float redX = wx, redY = wy, redZ = wz;
                if (config_.enableTorusWrap) {
                    if (params.reduction == ReductionMode::Torus) {
                        redX = wx - 2.0f * std::floor((wx + 1.0f) * 0.5f);
                        redY = wy - 2.0f * std::floor((wy + 1.0f) * 0.5f);
                        redZ = wz - 2.0f * std::floor((wz + 1.0f) * 0.5f);
                    } else {
                        const auto red = oracle::reduceDomain(epi::Vec3f{wx, wy, wz}, params.reduction, params.stageMask);
                        redX = red.x;
                        redY = red.y;
                        redZ = red.z;
                    }
                } else {
                    redX = std::clamp(wx, -1.0f, 1.0f);
                    redY = std::clamp(wy, -1.0f, 1.0f);
                    redZ = std::clamp(wz, -1.0f, 1.0f);
                }

                // 4. Trilinear Sample of Prior Generation
                volumetric_gpu_abi::VolumetricVoxelGpu historySample{};
                if (config_.enableFeedback) {
                    // Map [-1, 1]^3 to [0, dim-1]
                    float gxSample = (redX * 0.5f + 0.5f) * static_cast<float>(dim - 1U);
                    float gySample = (redY * 0.5f + 0.5f) * static_cast<float>(dim - 1U);
                    float gzSample = (redZ * 0.5f + 0.5f) * static_cast<float>(dim - 1U);

                    int i0 = static_cast<int>(std::floor(gxSample));
                    int j0 = static_cast<int>(std::floor(gySample));
                    int k0 = static_cast<int>(std::floor(gzSample));
                    float fx = gxSample - static_cast<float>(i0);
                    float fy = gySample - static_cast<float>(j0);
                    float fz = gzSample - static_cast<float>(k0);

                    auto clampCoord = [dim](int v) -> std::size_t {
                        return static_cast<std::size_t>(std::clamp(v, 0, static_cast<int>(dim) - 1));
                    };

                    auto readCorner = [&](int ix, int iy, int iz) -> const volumetric_gpu_abi::VolumetricVoxelGpu& {
                        return readBuf[clampCoord(ix) + dim * (clampCoord(iy) + dim * clampCoord(iz))];
                    };

                    const auto& c000 = readCorner(i0,     j0,     k0);
                    const auto& c100 = readCorner(i0 + 1, j0,     k0);
                    const auto& c010 = readCorner(i0,     j0 + 1, k0);
                    const auto& c110 = readCorner(i0 + 1, j0 + 1, k0);
                    const auto& c001 = readCorner(i0,     j0,     k0 + 1);
                    const auto& c101 = readCorner(i0 + 1, j0,     k0 + 1);
                    const auto& c011 = readCorner(i0,     j0 + 1, k0 + 1);
                    const auto& c111 = readCorner(i0 + 1, j0 + 1, k0 + 1);

                    for (int ch = 0; ch < 4; ++ch) {
                        float c00 = c000.colorDensity[ch] * (1.0f - fx) + c100.colorDensity[ch] * fx;
                        float c10 = c010.colorDensity[ch] * (1.0f - fx) + c110.colorDensity[ch] * fx;
                        float c01 = c001.colorDensity[ch] * (1.0f - fx) + c101.colorDensity[ch] * fx;
                        float c11 = c011.colorDensity[ch] * (1.0f - fx) + c111.colorDensity[ch] * fx;
                        float c0 = c00 * (1.0f - fy) + c10 * fy;
                        float c1 = c01 * (1.0f - fy) + c11 * fy;
                        historySample.colorDensity[ch] = c0 * (1.0f - fz) + c1 * fz;

                        float v00 = c000.velocityEnergy[ch] * (1.0f - fx) + c100.velocityEnergy[ch] * fx;
                        float v10 = c010.velocityEnergy[ch] * (1.0f - fx) + c110.velocityEnergy[ch] * fx;
                        float v01 = c001.velocityEnergy[ch] * (1.0f - fx) + c101.velocityEnergy[ch] * fx;
                        float v11 = c011.velocityEnergy[ch] * (1.0f - fx) + c111.velocityEnergy[ch] * fx;
                        float v0 = v00 * (1.0f - fy) + v10 * fy;
                        float v1 = v01 * (1.0f - fy) + v11 * fy;
                        historySample.velocityEnergy[ch] = v0 * (1.0f - fz) + v1 * fz;
                    }
                }

                // 5. Color and Energy Rotation with Acoustic Cymatic Coupling
                float dx = wx - rx, dy = wy - ry, dz = wz - rz;
                const float rNorm = std::sqrt(pxNorm * pxNorm + pyNorm * pyNorm + pzNorm * pzNorm);

                if (hasAcousticEnergy) {
                    const float cymaticPulse = std::cos(rNorm * (7.0f + audioMid * 6.0f) - time * 4.0f) * audioBass * 0.25f;
                    const float invR = 1.0f / (rNorm + 1e-3f);
                    dx += pxNorm * invR * cymaticPulse;
                    dy += pyNorm * invR * cymaticPulse;
                    dz += pzNorm * invR * cymaticPulse;
                }

                float orbitTension = std::sqrt(dx * dx + dy * dy + dz * dz);
                float accumulatedEnergy = historySample.velocityEnergy[3] * 0.92f + orbitTension * 0.08f;
                if (hasAcousticEnergy) {
                    accumulatedEnergy += (audioBass * 0.05f + audioFlux * 0.08f);
                }
                maxEnergy = std::max(maxEnergy, accumulatedEnergy);

                float hueShift = time * 0.08f + accumulatedEnergy * 0.45f + latentW * 0.15f;
                if (hasAcousticEnergy) {
                    hueShift += (audioFlux * 0.35f + audioTreble * 0.20f);
                }
                float hue = positiveFmod(hueShift + rNorm * 0.25f, 1.0f);
                float sat = std::clamp(0.75f + 0.25f * std::sin(time * 0.3f) + audioMid * 0.15f, 0.0f, 1.0f);
                float val = std::clamp(historySample.colorDensity[3] * 1.2f + audioBass * 0.15f, 0.0f, 1.0f);

                float er = 0.0f, eg = 0.0f, eb = 0.0f;
                hsvToRgb(hue, sat, val, er, eg, eb);
                er *= decay; eg *= decay; eb *= decay;

                // 6. Trilinear Sample of Seed Volume
                float gxSeedSample = (pxNorm * 0.5f + 0.5f) * static_cast<float>(auxDim - 1U);
                float gySeedSample = (pyNorm * 0.5f + 0.5f) * static_cast<float>(auxDim - 1U);
                float gzSeedSample = (pzNorm * 0.5f + 0.5f) * static_cast<float>(auxDim - 1U);

                int si0 = static_cast<int>(std::floor(gxSeedSample));
                int sj0 = static_cast<int>(std::floor(gySeedSample));
                int sk0 = static_cast<int>(std::floor(gzSeedSample));
                float sfx = gxSeedSample - static_cast<float>(si0);
                float sfy = gySeedSample - static_cast<float>(sj0);
                float sfz = gzSeedSample - static_cast<float>(sk0);

                auto clampAuxCoord = [auxDim](int v) -> std::size_t {
                    return static_cast<std::size_t>(std::clamp(v, 0, static_cast<int>(auxDim) - 1));
                };

                auto readSeedCorner = [&](int ix, int iy, int iz) -> const volumetric_gpu_abi::VolumetricVoxelGpu& {
                    return auxVolumeCpu_[clampAuxCoord(ix) + auxDim * (clampAuxCoord(iy) + auxDim * clampAuxCoord(iz))];
                };

                const auto& s000 = readSeedCorner(si0,     sj0,     sk0);
                const auto& s100 = readSeedCorner(si0 + 1, sj0,     sk0);
                const auto& s010 = readSeedCorner(si0,     sj0 + 1, sk0);
                const auto& s110 = readSeedCorner(si0 + 1, sj0 + 1, sk0);
                const auto& s001 = readSeedCorner(si0,     sj0,     sk0 + 1);
                const auto& s101 = readSeedCorner(si0 + 1, sj0,     sk0 + 1);
                const auto& s011 = readSeedCorner(si0,     sj0 + 1, sk0 + 1);
                const auto& s111 = readSeedCorner(si0 + 1, sj0 + 1, sk0 + 1);

                float sr = 0.0f, sg = 0.0f, sb = 0.0f, seedAlpha = 0.0f;
                auto interpSeed = [&](int ch) -> float {
                    float c00 = s000.colorDensity[ch] * (1.0f - sfx) + s100.colorDensity[ch] * sfx;
                    float c10 = s010.colorDensity[ch] * (1.0f - sfx) + s110.colorDensity[ch] * sfx;
                    float c01 = s001.colorDensity[ch] * (1.0f - sfx) + s101.colorDensity[ch] * sfx;
                    float c11 = s011.colorDensity[ch] * (1.0f - sfx) + s111.colorDensity[ch] * sfx;
                    float c0 = c00 * (1.0f - sfy) + c10 * sfy;
                    float c1 = c01 * (1.0f - sfy) + c11 * sfy;
                    return c0 * (1.0f - sfz) + c1 * sfz;
                };
                sr = interpSeed(0);
                sg = interpSeed(1);
                sb = interpSeed(2);
                seedAlpha = interpSeed(3);

                // 7. Blend Memory and Seed
                float finalR = er * (1.0f - seedAlpha) + sr * seedAlpha;
                float finalG = eg * (1.0f - seedAlpha) + sg * seedAlpha;
                float finalB = eb * (1.0f - seedAlpha) + sb * seedAlpha;
                float finalDensity = std::clamp(historySample.colorDensity[3] * decay + seedAlpha * 0.75f, 0.0f, 1.0f);

                totalDensity += finalDensity;

                volumetric_gpu_abi::VolumetricVoxelGpu outVoxel;
                outVoxel.colorDensity[0] = finalR;
                outVoxel.colorDensity[1] = finalG;
                outVoxel.colorDensity[2] = finalB;
                outVoxel.colorDensity[3] = finalDensity;

                outVoxel.velocityEnergy[0] = dx;
                outVoxel.velocityEnergy[1] = dy;
                outVoxel.velocityEnergy[2] = dz;
                outVoxel.velocityEnergy[3] = accumulatedEnergy;

                writeBuf[outIdx] = outVoxel;
            }
        }
    }

    swapBuffers();

    hostBuffersDirty_ = false;
    // The device still holds the predecessor. Explicit reactivation must upload
    // this new host field before GPU evolution is admitted again.
    deactivateGpu("CPU-Oracle");
    contentRevision_ = nextVolumeContentRevision();
    telemetry_.stepCount++;
    telemetry_.simulationTime = simTime;
    telemetry_.meanDensity = static_cast<float>(totalDensity / static_cast<double>(activeVoxelCount()));
    telemetry_.peakEnergy = maxEnergy;

    const auto t1 = std::chrono::steady_clock::now();
    telemetry_.lastStepTimeMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
    recomputeMacroOccupancy();
}

void SculptureVolumetricMemory::recomputeMacroOccupancy() const {
    macroOccupancy_.fill(0ULL);
    const auto& buf = readBufferA_ ? volumeA_ : volumeB_;
    const std::uint32_t dim = activeDim_;
    if (dim == 0U || buf.empty()) {
        macroOccupancyRatio_ = 0.0f;
        telemetry_.macroOccupancyRatio = 0.0f;
        return;
    }

    std::uint32_t activeBlocks = 0U;

    for (std::uint32_t gz = 0U; gz < dim; ++gz) {
        const std::uint32_t bz = std::min((gz * kMacroGridDim) / dim, kMacroGridDim - 1U);
        for (std::uint32_t gy = 0U; gy < dim; ++gy) {
            const std::uint32_t by = std::min((gy * kMacroGridDim) / dim, kMacroGridDim - 1U);
            for (std::uint32_t gx = 0U; gx < dim; ++gx) {
                const std::uint32_t bx = std::min((gx * kMacroGridDim) / dim, kMacroGridDim - 1U);
                const std::size_t idx = gx + dim * (gy + dim * gz);
                if (buf[idx].colorDensity[3] > 0.001f) {
                    const std::size_t bitIndex = bx + kMacroGridDim * (by + kMacroGridDim * bz);
                    const std::size_t wordIndex = bitIndex >> 6U;
                    const std::uint64_t bitMask = 1ULL << (bitIndex & 63U);
                    macroOccupancy_[wordIndex] |= bitMask;
                }
            }
        }
    }

    for (const auto word : macroOccupancy_) {
        activeBlocks += static_cast<std::uint32_t>(std::popcount(word));
    }

    macroOccupancyRatio_ = static_cast<float>(activeBlocks) / static_cast<float>(kMacroGridDim * kMacroGridDim * kMacroGridDim);
    telemetry_.macroOccupancyRatio = macroOccupancyRatio_;
}

bool SculptureVolumetricMemory::isMacroBlockOccupied(std::uint32_t bx, std::uint32_t by, std::uint32_t bz) const noexcept {
    if (bx >= kMacroGridDim || by >= kMacroGridDim || bz >= kMacroGridDim) {
        return false;
    }
    const std::size_t bitIndex = bx + kMacroGridDim * (by + kMacroGridDim * bz);
    return (macroOccupancy_[bitIndex >> 6U] & (1ULL << (bitIndex & 63U))) != 0ULL;
}

bool SculptureVolumetricMemory::isWorldPositionOccupied(float x, float y, float z) const noexcept {
    const float r = std::max(config_.worldRadius, 0.001f);
    if (std::fabs(x) > r || std::fabs(y) > r || std::fabs(z) > r) {
        return false;
    }
    const auto toBlock = [r](float coord) -> std::uint32_t {
        const float norm = coord / r * 0.5f + 0.5f;
        const int b = static_cast<int>(std::floor(norm * static_cast<float>(kMacroGridDim)));
        return static_cast<std::uint32_t>(std::clamp(b, 0, static_cast<int>(kMacroGridDim - 1U)));
    };
    return isMacroBlockOccupied(toBlock(x), toBlock(y), toBlock(z));
}

std::optional<SculptureVolumetricMemory::HostFieldView>
SculptureVolumetricMemory::tryAcquireHostField() const noexcept {
    ensureHostBufferSynced();
    const auto& buffer = readBufferA_ ? volumeA_ : volumeB_;
    if (hostBuffersDirty_ || activeDim_ <= 1U || buffer.size() < activeVoxelCount()) {
        return std::nullopt;
    }
    return HostFieldView(*this);
}

volumetric_gpu_abi::VolumetricVoxelGpu SculptureVolumetricMemory::HostFieldView::sampleWorld(
    float x, float y, float z) const noexcept {
    return memory_->sampleHostWorld(x, y, z);
}

volumetric_gpu_abi::VolumetricVoxelGpu SculptureVolumetricMemory::sampleWorld(float x, float y, float z) const noexcept {
    const auto field = tryAcquireHostField();
    return field ? field->sampleWorld(x, y, z) : volumetric_gpu_abi::VolumetricVoxelGpu{};
}

volumetric_gpu_abi::VolumetricVoxelGpu SculptureVolumetricMemory::sampleHostWorld(
    float x, float y, float z) const noexcept {
    const auto& buf = readBufferA_ ? volumeA_ : volumeB_;
    const std::uint32_t dim = activeDim_;
    if (dim <= 1U || buf.empty()) {
        return volumetric_gpu_abi::VolumetricVoxelGpu{};
    }
    const float r = std::max(config_.worldRadius, 0.001f);

    // Normalize from [-r, r]^3 to [0, dim-1]
    float u = std::clamp((x / r * 0.5f + 0.5f) * static_cast<float>(dim - 1U), 0.0f, static_cast<float>(dim - 1U));
    float v = std::clamp((y / r * 0.5f + 0.5f) * static_cast<float>(dim - 1U), 0.0f, static_cast<float>(dim - 1U));
    float w = std::clamp((z / r * 0.5f + 0.5f) * static_cast<float>(dim - 1U), 0.0f, static_cast<float>(dim - 1U));

    int i0 = static_cast<int>(u);
    int j0 = static_cast<int>(v);
    int k0 = static_cast<int>(w);
    int i1 = std::min(i0 + 1, static_cast<int>(dim - 1U));
    int j1 = std::min(j0 + 1, static_cast<int>(dim - 1U));
    int k1 = std::min(k0 + 1, static_cast<int>(dim - 1U));

    float fx = u - static_cast<float>(i0);
    float fy = v - static_cast<float>(j0);
    float fz = w - static_cast<float>(k0);

    auto getVoxel = [&](int ix, int iy, int iz) -> const volumetric_gpu_abi::VolumetricVoxelGpu& {
        return buf[static_cast<std::size_t>(ix) + dim * (static_cast<std::size_t>(iy) + dim * static_cast<std::size_t>(iz))];
    };

    const auto& c000 = getVoxel(i0, j0, k0);
    const auto& c100 = getVoxel(i1, j0, k0);
    const auto& c010 = getVoxel(i0, j1, k0);
    const auto& c110 = getVoxel(i1, j1, k0);
    const auto& c001 = getVoxel(i0, j0, k1);
    const auto& c101 = getVoxel(i1, j0, k1);
    const auto& c011 = getVoxel(i0, j1, k1);
    const auto& c111 = getVoxel(i1, j1, k1);

    volumetric_gpu_abi::VolumetricVoxelGpu result{};
    for (int ch = 0; ch < 4; ++ch) {
        float c00 = c000.colorDensity[ch] * (1.0f - fx) + c100.colorDensity[ch] * fx;
        float c10 = c010.colorDensity[ch] * (1.0f - fx) + c110.colorDensity[ch] * fx;
        float c01 = c001.colorDensity[ch] * (1.0f - fx) + c101.colorDensity[ch] * fx;
        float c11 = c011.colorDensity[ch] * (1.0f - fx) + c111.colorDensity[ch] * fx;
        float c0 = c00 * (1.0f - fy) + c10 * fy;
        float c1 = c01 * (1.0f - fy) + c11 * fy;
        result.colorDensity[ch] = c0 * (1.0f - fz) + c1 * fz;

        float v00 = c000.velocityEnergy[ch] * (1.0f - fx) + c100.velocityEnergy[ch] * fx;
        float v10 = c010.velocityEnergy[ch] * (1.0f - fx) + c110.velocityEnergy[ch] * fx;
        float v01 = c001.velocityEnergy[ch] * (1.0f - fx) + c101.velocityEnergy[ch] * fx;
        float v11 = c011.velocityEnergy[ch] * (1.0f - fx) + c111.velocityEnergy[ch] * fx;
        float v0 = v00 * (1.0f - fy) + v10 * fy;
        float v1 = v01 * (1.0f - fy) + v11 * fy;
        result.velocityEnergy[ch] = v0 * (1.0f - fz) + v1 * fz;
    }
    return result;
}

float SculptureVolumetricMemory::sampleDensity(float x, float y, float z) const noexcept {
    return sampleWorld(x, y, z).colorDensity[3];
}

// -----------------------------------------------------------------------------
// Binary Stream & File Serialization (.volmem sidecar)
// -----------------------------------------------------------------------------
#pragma pack(push, 1)
struct VolumetricMemoryFileHeader {
    std::uint32_t magic;         // 'VOLM' = 0x4D4C4F56
    std::uint32_t version;       // 1
    std::uint32_t headerSize;    // sizeof(VolumetricMemoryFileHeader) = 32
    std::uint32_t activeDim;     // 32, 64, or 128
    std::uint32_t scaleTier;     // StageResolutionScale wire value: 1, 2, 4, or 8
    float simTime;               // simulation time in seconds
    float meanDensity;           // mean density
    float peakEnergy;            // peak kinetic energy
};
#pragma pack(pop)
static_assert(sizeof(VolumetricMemoryFileHeader) == 32, "VolumetricMemoryFileHeader must be exactly 32 bytes");

constexpr std::uint32_t kVolmemMagic = 0x4D4C4F56U; // 'VOLM'
constexpr std::uint32_t kVolmemVersion = 1U;

bool SculptureVolumetricMemory::saveToStream(std::ostream& os) const {
    ensureHostBufferSynced();
    if (!hasCurrentHostBuffer()) {
        return false;
    }

    VolumetricMemoryFileHeader header{};
    header.magic = kVolmemMagic;
    header.version = kVolmemVersion;
    header.headerSize = static_cast<std::uint32_t>(sizeof(VolumetricMemoryFileHeader));
    header.activeDim = activeDim_;
    header.scaleTier = static_cast<std::uint32_t>(currentScale_);
    header.simTime = telemetry_.simulationTime;
    header.meanDensity = telemetry_.meanDensity;
    header.peakEnergy = telemetry_.peakEnergy;

    os.write(reinterpret_cast<const char*>(&header), sizeof(header));
    if (!os) return false;

    const auto& buf = readBufferA_ ? volumeA_ : volumeB_;
    const std::size_t voxelCount = activeVoxelCount();
    const std::size_t payloadBytes = voxelCount * sizeof(volumetric_gpu_abi::VolumetricVoxelGpu);

    if (buf.size() < voxelCount) {
        return false;
    }

    os.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(payloadBytes));
    return bool(os);
}

bool SculptureVolumetricMemory::loadFromStream(std::istream& is) {
    VolumetricMemoryFileHeader header{};
    is.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!is) return false;

    if (header.magic != kVolmemMagic || header.version != kVolmemVersion) {
        return false;
    }
    if (header.headerSize < sizeof(VolumetricMemoryFileHeader)) {
        return false;
    }
    if (header.activeDim < 4U || header.activeDim > volumetric_gpu_abi::kFullGridDimension) {
        return false;
    }
    const auto restoredScale = static_cast<StageResolutionScale>(header.scaleTier);
    switch (header.scaleTier) {
    case static_cast<std::uint32_t>(StageResolutionScale::Full):
    case static_cast<std::uint32_t>(StageResolutionScale::Half):
    case static_cast<std::uint32_t>(StageResolutionScale::Quarter):
    case static_cast<std::uint32_t>(StageResolutionScale::Eighth):
        break;
    default:
        return false;
    }
    if (!std::isfinite(header.simTime) || !std::isfinite(header.meanDensity)
        || !std::isfinite(header.peakEnergy)) {
        return false;
    }

    // Skip any extra header bytes if the format expands in the future
    if (header.headerSize > sizeof(VolumetricMemoryFileHeader)) {
        const std::size_t extra = header.headerSize - sizeof(VolumetricMemoryFileHeader);
        is.seekg(static_cast<std::streamoff>(extra), std::ios::cur);
        if (!is) return false;
    }

    // Decode privately before changing dimensions, buffers, telemetry or revision.
    const std::size_t voxelCount = static_cast<std::size_t>(header.activeDim)
        * header.activeDim * header.activeDim;
    std::vector<volumetric_gpu_abi::VolumetricVoxelGpu> decoded(voxelCount);
    const std::size_t payloadBytes = voxelCount * sizeof(volumetric_gpu_abi::VolumetricVoxelGpu);
    is.read(reinterpret_cast<char*>(decoded.data()), static_cast<std::streamsize>(payloadBytes));
    if (!is) return false;

    // Finite header metrics do not certify the per-cell recursive state. Validate
    // the private candidate before host takeover, derived occupancy or GPU upload.
    // Finite signed/artistic values retain their meaning; this is not a clamp.
    for (const auto& voxel : decoded) {
        for (float value : voxel.colorDensity) {
            if (!std::isfinite(value)) return false;
        }
        for (float value : voxel.velocityEnergy) {
            if (!std::isfinite(value)) return false;
        }
    }

    const std::size_t maxVoxels = static_cast<std::size_t>(volumetric_gpu_abi::kFullGridDimension)
        * volumetric_gpu_abi::kFullGridDimension * volumetric_gpu_abi::kFullGridDimension;
    // Any required allocation also precedes takeover. Existing active data remains
    // unchanged if growth throws; normal instances already have maximum storage.
    if (volumeA_.size() < maxVoxels) volumeA_.resize(maxVoxels);
    if (volumeB_.size() < maxVoxels) volumeB_.resize(maxVoxels);

    // Mirror restored voxel array into alternate ping-pong buffer to prevent single-frame flicker
    std::copy(decoded.begin(), decoded.end(), volumeA_.begin());
    std::copy(decoded.begin(), decoded.end(), volumeB_.begin());
    currentScale_ = restoredScale;
    activeDim_ = header.activeDim;
    updateDescriptor();

    // Reset ping-pong index so active read buffer is volumeA_
    readBufferA_ = true;
    // The decoded host candidate is now authoritative. A previous device-only
    // field must not be read back over it if the restore upload is unavailable.
    hostBuffersDirty_ = false;
    contentRevision_ = nextVolumeContentRevision();
    telemetry_.activeDimension = activeDim_;
    telemetry_.activeVoxelCount = static_cast<std::uint32_t>(voxelCount);
    telemetry_.activeScaleTier = currentScale_;
    telemetry_.simulationTime = header.simTime;
    telemetry_.meanDensity = header.meanDensity;
    telemetry_.peakEnergy = header.peakEnergy;
    if (telemetry_.stepCount == 0U && (header.meanDensity > 0.0f || header.simTime > 0.0f)) {
        telemetry_.stepCount = 1U;
    }

    // Recompute hierarchical coarse macro-occupancy
    recomputeMacroOccupancy();

    // If GPU is initialized and active, sync restored host buffer to GPU VRAM
    if (gpuInitialized_ && context_ != nullptr && queue_ != nullptr) {
        if (!syncGpuFromHost()) {
            deactivateGpu("CPU-Oracle (Restore Fallback)");
        }
    }

    return true;
}

bool SculptureVolumetricMemory::saveToFile(const std::string& filepath) const {
    try {
        return file_publication_detail::publishStreamFile(filepath,
            [this](std::ostream& stream) { return saveToStream(stream); });
    } catch (...) {
        return false;
    }
}

bool SculptureVolumetricMemory::loadFromFile(const std::string& filepath) {
    if (filepath.empty()) return false;
    try {
        std::filesystem::path p(filepath);
        if (!std::filesystem::exists(p)) return false;
        std::ifstream ifs(filepath, std::ios::binary);
        if (!ifs.is_open()) return false;
        return loadFromStream(ifs);
    } catch (...) {
        return false;
    }
}

} // namespace epi_sculpt
