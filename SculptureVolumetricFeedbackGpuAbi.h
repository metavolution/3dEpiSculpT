// -----------------------------------------------------------------------------
// Educational Annotation: 3D VOLUMETRIC EPIMORPHISM FEEDBACK GPU ABI
// -----------------------------------------------------------------------------
// Fulfills Phase 3 of the 3D Procedural World Vision (docu/3D-HIGHER-DIMENSIONAL-WORLD-VISION.md):
// "Camera-independent recursive memory: Add a 3D field storing energy/material/density
// rather than authoritative SDF geometry. Use trilinear sampling and ping-pong evolution
// analogous to the current 2D framebuffer."
//
// In canonical 2D epimorphism, every frame is an iterative complex-plane feedback
// loop: z -> Transform -> Reduce -> Sample(prev) -> Color -> Seed -> Blend.
// This ABI elevates that fundamental computation into 3-dimensional Euclidean
// and non-Euclidean space, maintaining a double-buffered 3D volumetric field
// that evolves persistently and camera-independently on the GPU via OpenCL.
//
// All records are strictly 16-byte aligned, standard-layout, and portable between
// C++ host code and OpenCL device kernels without compiler packing ambiguity.
// -----------------------------------------------------------------------------

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace epi_sculpt::volumetric_gpu_abi {

inline constexpr std::uint32_t kAbiVersion = 1U;
inline constexpr std::uint32_t kVoxelBytes = 32U;
inline constexpr std::uint32_t kDescriptorBytes = 80U;

// Default volumetric resolution tiers (staged compute governor compatible)
inline constexpr std::uint32_t kFullGridDimension = 128U;    // 128^3 = 2,097,152 voxels (64 MiB ping-pong)
inline constexpr std::uint32_t kHalfGridDimension = 64U;     // 64^3  = 262,144 voxels   (8 MiB ping-pong)
inline constexpr std::uint32_t kQuarterGridDimension = 32U;  // 32^3  = 32,768 voxels    (1 MiB ping-pong)

// Volumetric execution flags
enum VolumetricFlags : std::uint32_t {
    VolumetricFlag_None              = 0U,
    VolumetricFlag_EnableFeedback    = 1U << 0U, // If clear, skips previous frame trilinear sampling
    VolumetricFlag_EnableTorusWrap   = 1U << 1U, // 3D Torus boundary wrap vs mirror clamp
    VolumetricFlag_EnableWcaSeed     = 1U << 2U, // Inject procedural 3D WCA boundary seed
    VolumetricFlag_EnableHyper4DWave = 1U << 3U, // Modulate density with 4D spacetime foliation
    VolumetricFlag_EnableAdvection   = 1U << 4U  // Transport density along velocity vector field
};

// -----------------------------------------------------------------------------
// VolumetricVoxelGpu: 32-byte 3D memory cell
// -----------------------------------------------------------------------------
// Packed representation of state at coordinate (x, y, z):
// - colorDensity:    xyz = Linear RGB/OKLCH color coordinates, w = Density/Opacity [0..1]
// - velocityEnergy:  xyz = Internal advection/flow vector,     w = Accumulated orbit energy
// -----------------------------------------------------------------------------
struct alignas(16) VolumetricVoxelGpu {
    std::array<float, 4> colorDensity{0.0f, 0.0f, 0.0f, 0.0f};
    std::array<float, 4> velocityEnergy{0.0f, 0.0f, 0.0f, 0.0f};
};

static_assert(std::is_standard_layout_v<VolumetricVoxelGpu>, "VolumetricVoxelGpu must be standard layout");
static_assert(sizeof(VolumetricVoxelGpu) == kVoxelBytes, "VolumetricVoxelGpu must be exactly 32 bytes");
static_assert(alignof(VolumetricVoxelGpu) == 16U, "VolumetricVoxelGpu must be 16-byte aligned");

// -----------------------------------------------------------------------------
// VolumetricFieldDescriptorGpu: 80-byte uniform buffer block
// -----------------------------------------------------------------------------
// Completely specifies 3D field geometry, coordinate domain, temporal rates,
// and transformation parameters for OpenCL execution.
// -----------------------------------------------------------------------------
struct alignas(16) VolumetricFieldDescriptorGpu {
    // identity: x = version, y = byteSize, z = gridWidth, w = gridHeight
    std::array<std::uint32_t, 4> identity{kAbiVersion, kDescriptorBytes, kHalfGridDimension, kHalfGridDimension};

    // extentCadence: x = gridDepth, y = resolutionTier (0=Full, 1=Half, 2=Quarter), z = temporalCadence, w = flags
    std::array<std::uint32_t, 4> extentCadence{kHalfGridDimension, 1U, 1U, VolumetricFlag_EnableFeedback | VolumetricFlag_EnableTorusWrap | VolumetricFlag_EnableWcaSeed};

    // worldMinimum: xyz = spatial min bounding box, w = temporal feedback decay (e.g. 0.95f)
    std::array<float, 4> worldMinimum{-2.5f, -2.5f, -2.5f, 0.95f};

    // worldMaximum: xyz = spatial max bounding box, w = seed injection alpha (e.g. 0.25f)
    std::array<float, 4> worldMaximum{2.5f, 2.5f, 2.5f, 0.25f};

    // transformParameters: x = time, y = deltaSeconds, z = latentW, w = rotorAlpha
    std::array<float, 4> transformParameters{0.0f, 0.016667f, 0.0f, 0.0f};
};

static_assert(std::is_standard_layout_v<VolumetricFieldDescriptorGpu>, "VolumetricFieldDescriptorGpu must be standard layout");
static_assert(sizeof(VolumetricFieldDescriptorGpu) == kDescriptorBytes, "VolumetricFieldDescriptorGpu must be exactly 80 bytes");
static_assert(alignof(VolumetricFieldDescriptorGpu) == 16U, "VolumetricFieldDescriptorGpu must be 16-byte aligned");

[[nodiscard]] constexpr std::size_t computeVoxelCount(const VolumetricFieldDescriptorGpu& desc) noexcept {
    return static_cast<std::size_t>(desc.identity[2]) * desc.identity[3] * desc.extentCadence[0];
}

[[nodiscard]] constexpr std::size_t computeBufferBytes(const VolumetricFieldDescriptorGpu& desc) noexcept {
    return computeVoxelCount(desc) * sizeof(VolumetricVoxelGpu);
}

inline constexpr std::uint32_t kCouplerConfigBytes = 64U;

enum VolumetricCouplerFlags : std::uint32_t {
    CouplerFlag_None                  = 0U,
    CouplerFlag_EnableCoupling        = 1U << 0U,
    CouplerFlag_EnableAdvection       = 1U << 1U,
    CouplerFlag_EnableDensityBuoyancy = 1U << 2U,
    CouplerFlag_EnableVorticitySpin   = 1U << 3U,
    CouplerFlag_EnableEnergyMod       = 1U << 4U,
    CouplerFlag_EnableColorCoupling   = 1U << 5U,
    CouplerFlag_EnablePbrTurbulence   = 1U << 6U,
    CouplerFlag_EnableVacancyCulling  = 1U << 7U,
    CouplerFlag_ClampToWorldBounds    = 1U << 8U,
    CouplerFlag_EnableToroidalWrap    = 1U << 9U
};

// -----------------------------------------------------------------------------
// VolumetricCouplerConfigGpu: 64-byte uniform buffer block for GPU coupling
// -----------------------------------------------------------------------------
// Parameterizes the chained OpenCL kernel `couple_world_volumetric_field` that
// couples `WorldSnapshotGpu` instances in-place on the GPU with `VolumetricVoxelGpu`
// before signaling the Vulkan external semaphore.
// -----------------------------------------------------------------------------
struct alignas(16) VolumetricCouplerConfigGpu {
    // identityAndFlags: x = abiVersion, y = byteSize, z = VolumetricCouplerFlags, w = reserved
    std::array<std::uint32_t, 4> identityAndFlags{
        kAbiVersion,
        kCouplerConfigBytes,
        CouplerFlag_EnableCoupling | CouplerFlag_EnableAdvection | CouplerFlag_EnableDensityBuoyancy |
            CouplerFlag_EnableVorticitySpin | CouplerFlag_EnableEnergyMod | CouplerFlag_EnableColorCoupling |
            CouplerFlag_EnablePbrTurbulence | CouplerFlag_ClampToWorldBounds,
        0U
    };

    // dynamicsParams: x = advectionRate, y = buoyancyStrength, z = vorticitySpinRate, w = energyEmissionGain
    std::array<float, 4> dynamicsParams{0.85f, 0.35f, 1.25f, 1.20f};

    // materialParams: x = colorCouplingWeight, y = roughnessGain, z = vacancyDensityThreshold, w = worldBoundingRadius
    std::array<float, 4> materialParams{0.40f, 0.35f, 0.005f, 2.5f};

    // timingAndProbe: x = deltaSeconds, y = simTime, z = gradientProbeEpsilon, w = reserved
    std::array<float, 4> timingAndProbe{0.016667f, 0.0f, 0.035f, 0.0f};
};

static_assert(std::is_standard_layout_v<VolumetricCouplerConfigGpu>, "VolumetricCouplerConfigGpu must be standard layout");
static_assert(sizeof(VolumetricCouplerConfigGpu) == kCouplerConfigBytes, "VolumetricCouplerConfigGpu must be 64 bytes");
static_assert(alignof(VolumetricCouplerConfigGpu) == 16U, "VolumetricCouplerConfigGpu must be 16-byte aligned");

} // namespace epi_sculpt::volumetric_gpu_abi

