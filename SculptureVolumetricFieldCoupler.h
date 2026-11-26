// -----------------------------------------------------------------------------
// Educational Annotation: 3D VOLUMETRIC FIELD COUPLER & PHYSICAL MEMORY ADVECTION
// -----------------------------------------------------------------------------
// Fulfills Phase 3 & Phase 4 of the 3D Higher-Dimensional Procedural World Vision:
// Unified Volumetric Memory & Procedural Instance Coupling (docu/3D-HIGHER-DIMENSIONAL-WORLD-VISION.md).
//
// In canonical 2D epimorphic feedback, spatial coordinates iteratively sample
// and write back to a recursive framebuffer. In the 3D world domain,
// SculptureVolumetricMemory maintains a persistent camera-independent 3D voxel
// field storing color, density, and flow velocity/orbit energy.
//
// SculptureVolumetricFieldCoupler serves as the bidirectional dynamical bridge
// between this continuous 3D field and discrete procedural world instances
// (WorldInstanceGpu) as well as continuous relief manifolds (EpimorphicReliefData):
//
// 1. Flow Advection: Transports procedural instances along the voxel velocity field:
//    p' = p + deltaSeconds * advectionRate * u(p).
//
// 2. Density-Gradient Buoyancy & Pressure: Evaluates central-difference spatial
//    gradients grad(rho) = (d rho/dx, d rho/dy, d rho/dz) and applies buoyant or
//    repulsion forces driving instances toward or away from dense filament walls.
//
// 3. Fluid Vorticity & Eddy Rotation: Computes the 3D velocity curl omega = curl(u)
//    and applies incremental quaternion spin to instances matching local fluid vorticity.
//
// 4. Energy-to-Radiance & PBR Modulation: Modulates instance emissive radiance
//    by accumulated orbit energy E and adjusts PBR surface roughness based on
//    local flow kinetic energy K = 0.5 * |u|^2.
//
// 5. Hierarchical Macro-Vacancy Culling: Leverages the 8x8x8 coarse macro-occupancy
//    bitmask to cull or shrink instances located in deep vacuum, saving GPU draw bandwidth.
//
// 6. Volumetric Slicing for Relief Manifolds: Extracts 2D scalar fields from arbitrary
//    cross-sections of the 3D volume, allowing EpimorphicReliefGenerator to sculpt
//    topological surfaces that directly embody the persistent 3D memory.
// -----------------------------------------------------------------------------

#pragma once

#include "SculptureVolumetricMemory.h"
#include "SculptureWorldGpuAbi.h"
#include "SculptureProgramSchema.h"

#include <vector>
#include <span>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <array>

namespace epi_sculpt {

struct VolumetricCouplerConfig {
    bool enableCoupling = true;             // Master toggle for volumetric field coupling
    bool enableAdvection = true;            // Transport instances along voxel flow velocity
    float advectionRate = 0.85f;            // Scaling coefficient for velocity advection
    bool enableDensityBuoyancy = true;      // Apply force along density gradient
    float buoyancyStrength = 0.35f;         // Density gradient force multiplier
    bool enableVorticitySpin = true;        // Rotate instances around fluid vorticity curl(u)
    float vorticitySpinRate = 1.25f;        // Angular spin velocity multiplier
    bool enableEnergyModulation = true;     // Modulate emissive radiance by orbit energy
    float energyEmissionGain = 1.20f;       // Sensitivity of emissive tint to energy
    bool enableColorCoupling = true;        // Blend instance color with voxel field color
    float colorCouplingWeight = 0.40f;      // Voxel color blend weight [0..1]
    bool enablePbrTurbulence = true;        // Modulate PBR roughness from flow kinetic energy
    float roughnessGain = 0.35f;            // Roughness modulation gain
    bool enableVacancyCulling = false;      // Cull instances falling into vacuum macro-blocks
    float vacancyDensityThreshold = 0.005f; // Density threshold below which instance is culled
    bool clampToWorldBounds = true;         // Enforce world bounds [-radius, radius]^3
    bool enableToroidalWrap = false;        // Toroidal periodic wrapping vs bounding box clamp
    float worldBoundingRadius = 2.5f;       // Half-extent of volumetric simulation world
    float gradientProbeEpsilon = 0.035f;    // Finite difference step size for gradient/curl
};

struct VolumetricCouplerTelemetry {
    std::size_t totalInstancesProcessed = 0U;
    std::size_t culledVacantInstances = 0U;
    float meanSampledDensity = 0.0f;
    float peakSampledVelocity = 0.0f;
    float meanVorticityMagnitude = 0.0f;
    float meanEnergyModulation = 0.0f;
    float couplingExecutionTimeMs = 0.0f;
    bool active = false;
    bool hostFieldUnavailable = false; // Last requested CPU batch rejected before instance mutation.
};

class SculptureVolumetricFieldCoupler {
public:
    explicit SculptureVolumetricFieldCoupler(const VolumetricCouplerConfig& config = VolumetricCouplerConfig{});
    ~SculptureVolumetricFieldCoupler() = default;

    SculptureVolumetricFieldCoupler(const SculptureVolumetricFieldCoupler&) = default;
    SculptureVolumetricFieldCoupler& operator=(const SculptureVolumetricFieldCoupler&) = default;
    SculptureVolumetricFieldCoupler(SculptureVolumetricFieldCoupler&&) noexcept = default;
    SculptureVolumetricFieldCoupler& operator=(SculptureVolumetricFieldCoupler&&) noexcept = default;

    // Configuration & Telemetry
    void setConfig(const VolumetricCouplerConfig& config) noexcept { config_ = config; }
    void configure(const VolumetricCouplerConfig& config) noexcept { config_ = config; }
    [[nodiscard]] const VolumetricCouplerConfig& config() const noexcept { return config_; }
    [[nodiscard]] VolumetricCouplerConfig& config() noexcept { return config_; }
    [[nodiscard]] const VolumetricCouplerTelemetry& telemetry() const noexcept { return telemetry_; }

    void resetTelemetry() noexcept { telemetry_ = VolumetricCouplerTelemetry{}; }

    [[nodiscard]] volumetric_gpu_abi::VolumetricCouplerConfigGpu packGpuConfig(
        float deltaSeconds,
        float simTime
    ) const noexcept {
        using namespace volumetric_gpu_abi;
        std::uint32_t flags = CouplerFlag_None;
        if (config_.enableCoupling)        flags |= CouplerFlag_EnableCoupling;
        if (config_.enableAdvection)       flags |= CouplerFlag_EnableAdvection;
        if (config_.enableDensityBuoyancy) flags |= CouplerFlag_EnableDensityBuoyancy;
        if (config_.enableVorticitySpin)   flags |= CouplerFlag_EnableVorticitySpin;
        if (config_.enableEnergyModulation) flags |= CouplerFlag_EnableEnergyMod;
        if (config_.enableColorCoupling)   flags |= CouplerFlag_EnableColorCoupling;
        if (config_.enablePbrTurbulence)   flags |= CouplerFlag_EnablePbrTurbulence;
        if (config_.enableVacancyCulling)  flags |= CouplerFlag_EnableVacancyCulling;
        if (config_.clampToWorldBounds)    flags |= CouplerFlag_ClampToWorldBounds;
        if (config_.enableToroidalWrap)    flags |= CouplerFlag_EnableToroidalWrap;

        return VolumetricCouplerConfigGpu{
            .identityAndFlags = {kAbiVersion, kCouplerConfigBytes, flags, 0U},
            .dynamicsParams = {
                config_.advectionRate,
                config_.buoyancyStrength,
                config_.vorticitySpinRate,
                config_.energyEmissionGain
            },
            .materialParams = {
                config_.colorCouplingWeight,
                config_.roughnessGain,
                config_.vacancyDensityThreshold,
                config_.worldBoundingRadius
            },
            .timingAndProbe = {
                deltaSeconds,
                simTime,
                config_.gradientProbeEpsilon,
                0.0f
            }
        };
    }

    // -------------------------------------------------------------------------
    // Procedural Instance Coupling
    // -------------------------------------------------------------------------
    // Evaluates volumetric memory at each instance's coordinate, advecting positions,
    // applying density-gradient buoyancy, spinning by vorticity curl, and modulating
    // color, emission, and PBR roughness. Acquires once before any culling or
    // mutation; unavailable history leaves instances unchanged and reports
    // telemetry.hostFieldUnavailable with active=false. Disabled/empty work
    // does not acquire history.
    void coupleInstances(
        const SculptureVolumetricMemory& memory,
        std::span<world_gpu_abi::WorldInstanceGpu> instances,
        float deltaSeconds,
        float simTime
    );

    // Convenience overload for an entire WorldSnapshotGpu
    void coupleSnapshot(
        const SculptureVolumetricMemory& memory,
        world_gpu_abi::WorldSnapshotGpu& snapshot,
        float deltaSeconds,
        float simTime
    );

    // -------------------------------------------------------------------------
    // Continuous Spatial Differential Operators
    // -------------------------------------------------------------------------
    // Computes central-difference numerical gradient of density field grad(rho)
    // Compatibility memory overloads return zero if acquisition fails. Checked
    // batch callers use the view overloads to distinguish unavailable history.
    [[nodiscard]] SculptureVec3 computeDensityGradient(
        const SculptureVolumetricMemory& memory,
        float x, float y, float z,
        float epsilon = 0.035f
    ) const noexcept;

    // Computes central-difference velocity curl (fluid vorticity vector omega = curl(u))
    [[nodiscard]] SculptureVec3 computeVelocityVorticity(
        const SculptureVolumetricMemory& memory,
        float x, float y, float z,
        float epsilon = 0.035f
    ) const noexcept;

    // Pure probes of one admitted observation; never trigger another readback.
    [[nodiscard]] SculptureVec3 computeDensityGradient(
        const SculptureVolumetricMemory::HostFieldView& field,
        float x, float y, float z,
        float epsilon = 0.035f
    ) const noexcept;

    [[nodiscard]] SculptureVec3 computeVelocityVorticity(
        const SculptureVolumetricMemory::HostFieldView& field,
        float x, float y, float z,
        float epsilon = 0.035f
    ) const noexcept;

    // -------------------------------------------------------------------------
    // Volumetric 2D Slice Extraction for Epimorphic Relief Manifold
    // -------------------------------------------------------------------------
    // Slices the 3D persistent volumetric memory along a given elevation plane
    // producing a 2D scalar field suitable for EpimorphicReliefGenerator.
    // Both slice APIs acquire once and return empty when history is unavailable.
    // A valid vacuum returns the usual nonempty, sized zero field. No result
    // allocation or sampling occurs after a failed acquisition.
    [[nodiscard]] std::vector<float> extractReliefSlice(
        const SculptureVolumetricMemory& memory,
        std::uint32_t width,
        std::uint32_t height,
        float sliceZ = 0.0f,
        float energyMix = 0.20f
    ) const;

    // Slices with arbitrary coordinate orientation (plane defined by normal and origin)
    [[nodiscard]] std::vector<float> extractOrientedSlice(
        const SculptureVolumetricMemory& memory,
        std::uint32_t width,
        std::uint32_t height,
        const SculptureVec3& origin,
        const SculptureVec3& uAxis,
        const SculptureVec3& vAxis,
        float halfExtent = 2.5f,
        float energyMix = 0.20f
    ) const;

private:
    VolumetricCouplerConfig config_{};
    mutable VolumetricCouplerTelemetry telemetry_{};
};

} // namespace epi_sculpt
