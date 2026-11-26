// -----------------------------------------------------------------------------
// Educational Annotation: 3D VOLUMETRIC RECURSIVE MEMORY COORDINATOR
// -----------------------------------------------------------------------------
// Fulfills Phase 3 of the 3D Higher-Dimensional Procedural World Vision:
// Camera-Independent Recursive Memory Core (docu/3D-HIGHER-DIMENSIONAL-WORLD-VISION.md).
//
// In canonical 2D epimorphic feedback, every frame is an iterative complex-plane
// feedback loop: z -> Transform -> Reduce -> Sample(prev) -> Color -> Seed -> Blend.
// This coordinator elevates that fundamental computation into 3-dimensional
// Euclidean world space, maintaining a camera-independent persistent memory
// field storing color, density, and flow velocity/orbit energy.
//
// Architectural Pillars:
// 1. Double-Buffered Ping-Pong Memory: Dual 3D voxel fields (volumeA / volumeB)
//    prevent race conditions during simultaneous sampling and writing.
// 2. High-Performance OpenCL GPU Execution: Evolving 262k (64^3) to 2.09M (128^3)
//    voxels per frame via parallel 3D NDRange compute workgroups.
// 3. Headless CPU Oracle: Explicit reference/recovery implementation, without
//    claiming full numerical parity with canonical OpenCL production.
// 4. Hierarchical 3D Macro-Block Occupancy: An 8x8x8 coarse bitmask tracking
//    active density cells, enabling O(1) empty-space vacancy leaping in raymarchers.
// 5. Staged Compute Governor Integration: Dynamic resolution tiering (Full: 128^3,
//    Half: 64^3, Quarter: 32^3) responding to target frame budgets.
// -----------------------------------------------------------------------------

#pragma once

#include "SculptureVolumetricFeedbackGpuAbi.h"
#include "SculptureProgramSchema.h"
#include "SculptureStagedComputeGovernor.h"

#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <array>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <iosfwd>
#include <optional>

#ifndef EPI_SCULPT_HAS_OPENCL
#if defined(__has_include)
#if __has_include(<CL/cl.h>)
#define EPI_SCULPT_HAS_OPENCL 1
#else
#define EPI_SCULPT_HAS_OPENCL 0
#endif
#else
#define EPI_SCULPT_HAS_OPENCL 0
#endif
#endif

#if EPI_SCULPT_HAS_OPENCL
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#elif !defined(CL_TARGET_OPENCL_VERSION) && !defined(_CL_H)
// Portable opaque aliases for environments without OpenCL headers
using cl_context = struct _cl_context*;
using cl_device_id = struct _cl_device_id*;
using cl_command_queue = struct _cl_command_queue*;
using cl_program = struct _cl_program*;
using cl_kernel = struct _cl_kernel*;
using cl_mem = struct _cl_mem*;
using cl_event = struct _cl_event*;
#endif

namespace epi_sculpt {

struct VolumetricMemoryConfig {
    std::uint32_t baseDimension = volumetric_gpu_abi::kHalfGridDimension; // 64^3 default (balanced interactive)
    float decayFactor = 0.94f;          // Frame-to-frame feedback memory persistence
    float seedAlpha = 0.28f;            // Boundary procedural seed injection weight
    bool enableFeedback = true;         // Continuous temporal history accumulation
    bool enableTorusWrap = true;        // 3D domain reduction via toroidal boundary wrap
    bool enableWcaSeed = true;          // Procedural WCA boundary impulse injection
    bool enableHyper4DWave = true;      // 4D spacetime foliation coupling
    bool enableAcousticCoupling = true; // Cymatics & acoustic standing wave injection
    float acousticGain = 1.0f;          // Acoustic modulation sensitivity
    float worldRadius = 2.5f;           // Bounding half-extent [-worldRadius, worldRadius]^3
};

struct VolumetricMemoryTelemetry {
    std::uint32_t activeDimension = 64U;
    std::size_t activeVoxelCount = 262144U;
    std::size_t bufferBytes = 8388608U;
    std::uint64_t stepCount = 0U;
    float simulationTime = 0.0f;
    float meanDensity = 0.0f;
    float peakEnergy = 0.0f;
    float lastStepTimeMs = 0.0f;
    float macroOccupancyRatio = 0.0f;
    StageResolutionScale activeScaleTier = StageResolutionScale::Half;
    const char* backendName = "CPU-Oracle";
    bool gpuAccelerated = false;
};

class SculptureVolumetricMemory {
public:
    static constexpr std::uint32_t kMacroGridDim = 8U;
    static constexpr std::size_t kMacroWordCount = 8U; // 8 * 64 bits = 512 macro-blocks (8^3)

    // Borrowed observation, issued only after checked acquisition. Sampling and
    // occupancy reads never synchronize. Keep the owner alive and unchanged
    // during use; this is not an owned snapshot or a lock against mutation.
    class HostFieldView {
    public:
        [[nodiscard]] volumetric_gpu_abi::VolumetricVoxelGpu sampleWorld(
            float x, float y, float z) const noexcept;
        [[nodiscard]] float sampleDensity(float x, float y, float z) const noexcept {
            return sampleWorld(x, y, z).colorDensity[3];
        }
        [[nodiscard]] bool isWorldPositionOccupied(float x, float y, float z) const noexcept {
            return memory_->isWorldPositionOccupied(x, y, z);
        }
        [[nodiscard]] std::uint64_t contentRevision() const noexcept {
            return memory_->contentRevision();
        }

    private:
        friend class SculptureVolumetricMemory;
        explicit HostFieldView(const SculptureVolumetricMemory& memory) noexcept : memory_(&memory) {}
        const SculptureVolumetricMemory* memory_;
    };

    explicit SculptureVolumetricMemory(const VolumetricMemoryConfig& config = VolumetricMemoryConfig{});
    ~SculptureVolumetricMemory();

    // Transfer the complete host/device field and auxiliary resources. Moving
    // settles source GPU work because queued uploads may borrow descriptor_.
    // Source is resource-empty/inactive, suitable for destruction or reassignment.
    // Both objects receive fresh content revisions; self-move is a no-op.
    SculptureVolumetricMemory(SculptureVolumetricMemory&&) noexcept;
    SculptureVolumetricMemory& operator=(SculptureVolumetricMemory&&) noexcept;
    SculptureVolumetricMemory(const SculptureVolumetricMemory&) = delete;
    SculptureVolumetricMemory& operator=(const SculptureVolumetricMemory&) = delete;

    // Reject the entire candidate if any scalar is non-finite or worldRadius
    // is nonpositive, before changing configuration, fields or native resources.
    // Finite artistic coefficients are not clamped to event/UI control ranges.
    // Invalid construction retains coherent member-default configuration.
    [[nodiscard]] bool trySetConfig(const VolumetricMemoryConfig& config);

    // Compatibility wrappers ignore rejected candidates. Non-domain control
    // edits preserve the governed extent, tier and field. Unchanged baseDimension
    // leaves the governed extent alone.
    // An explicit baseDimension edit selects its clamped extent. A changed
    // effective extent or worldRadius starts empty history under a fresh content
    // revision, preserving accepted step/time counters. Failed device upload
    // leaves that host field authoritative and GPU execution inactive;
    // trySetConfig reports config admission, not device readiness.
    void setConfig(const VolumetricMemoryConfig& config);
    void configure(const VolumetricMemoryConfig& config) { setConfig(config); }
    [[nodiscard]] const VolumetricMemoryConfig& config() const noexcept { return config_; }

    // Unknown enum values are ignored before any field/resource mutation. Valid
    // requests reassert their physical extent, even after a same-tier custom
    // dimension edit. A changed extent uses the same empty-history policy above.
    void applyResolutionScale(StageResolutionScale scale);
    // Selected tier; activeDimension() remains the actual extent when an explicit
    // custom base dimension or checkpoint has overridden the tier's usual grid.
    [[nodiscard]] StageResolutionScale currentResolutionScale() const noexcept { return currentScale_; }

    // Resets all voxel density, color, and flow velocity to vacuum state
    void reset();

    // -------------------------------------------------------------------------
    // OpenCL GPU Device Lifecycle & Acceleration
    // -------------------------------------------------------------------------
    // Initializes GPU compute resources on the specified OpenCL context and device.
    // If kernelSourceOrPath is empty, automatically locates kernels/SculptureVolumetricFeedback3D.cl
    // or falls back to the embedded OpenCL C kernel source string.
    // Success requires both seed/evolution kernels and a completed starting-field
    // upload. Readiness publishes only then; activation respects setGpuEnabled().
    bool initGpu(cl_context context, cl_device_id device, const std::string& kernelSourceOrPath = "");
    void releaseGpu() noexcept;
    [[nodiscard]] bool isGpuActive() const noexcept { return gpuActive_ && gpuEnabled_; }
    [[nodiscard]] bool isGpuInitialized() const noexcept { return gpuInitialized_; }
    [[nodiscard]] bool isGpuEnabled() const noexcept { return gpuEnabled_; }
    // Stores the requested preference. Activation requires a current device
    // field; repeated enable requests retry a previously failed upload.
    void setGpuEnabled(bool enabled) noexcept;

    // -------------------------------------------------------------------------
    // Unified Temporal Feedback Evolution
    // -------------------------------------------------------------------------
    // Advances temporal evolution by one discrete simulation step.
    // Dispatches OpenCL when active; CPU recovery requires the current host field.
    // If required readback fails, throws instead of evolving stale history.
    void step(const SculptureProgramParams& params, float simTime, float deltaSeconds);

    // Explicit CPU reference/recovery. Acquires current history before evolution;
    // a committed CPU step deactivates GPU execution until explicitly re-enabled.
    void stepCpuOracle(const SculptureProgramParams& params, float simTime, float deltaSeconds);

    // Host/Device Bidirectional Buffer Synchronization
    void syncHostFromGpu() const;
    // Returns whether a current host field was uploaded. A stale host copy cannot
    // overwrite newer device feedback; transfer alone changes no content revision.
    bool syncGpuFromHost();
    void ensureHostBufferSynced() const {
        if (hostBuffersDirty_) {
            syncHostFromGpu();
        }
    }

    // At most one readback attempt; no view if history remains device-only or
    // active storage is unavailable. Hold the view only during an observation
    // phase in which the owner cannot mutate (including from another thread).
    [[nodiscard]] std::optional<HostFieldView> tryAcquireHostField() const noexcept;

    // Compatibility sampling returns zero when acquisition is unavailable.
    // Consumers needing to distinguish failure from vacuum must acquire a view.
    // Continuous 3D spatial field sampling (Hardware-equivalent Trilinear Interpolation)
    [[nodiscard]] volumetric_gpu_abi::VolumetricVoxelGpu sampleWorld(float x, float y, float z) const noexcept;
    [[nodiscard]] float sampleDensity(float x, float y, float z) const noexcept;

    // -------------------------------------------------------------------------
    // Hierarchical 3D Macro-Block Occupancy (Vacancy Leaping)
    // -------------------------------------------------------------------------
    // Returns true if the coarse macro-block (8x8x8 partition of world domain)
    // contains non-negligible volumetric density. Enables O(1) ray skipping.
    [[nodiscard]] bool isMacroBlockOccupied(std::uint32_t bx, std::uint32_t by, std::uint32_t bz) const noexcept;
    [[nodiscard]] bool isWorldPositionOccupied(float x, float y, float z) const noexcept;
    [[nodiscard]] float macroOccupancyRatio() const noexcept { return macroOccupancyRatio_; }
    [[nodiscard]] const std::array<std::uint64_t, kMacroWordCount>& macroOccupancyBits() const noexcept { return macroOccupancy_; }

    // Buffer and Telemetry accessors
    [[nodiscard]] const std::vector<volumetric_gpu_abi::VolumetricVoxelGpu>& activeReadBuffer() const noexcept {
        ensureHostBufferSynced();
        return readBufferA_ ? volumeA_ : volumeB_;
    }

    [[nodiscard]] const volumetric_gpu_abi::VolumetricFieldDescriptorGpu& descriptor() const noexcept {
        return descriptor_;
    }

    [[nodiscard]] const VolumetricMemoryTelemetry& telemetry() const noexcept { return telemetry_; }
    // Process-local identity of field contents/domain, independent of resettable
    // simulation counters. Inspection performs no synchronization or field scan.
    [[nodiscard]] std::uint64_t contentRevision() const noexcept { return contentRevision_; }
    [[nodiscard]] bool hasCurrentHostBuffer() const noexcept { return !hostBuffersDirty_; }
    [[nodiscard]] std::uint32_t activeDimension() const noexcept { return activeDim_; }
    [[nodiscard]] std::size_t activeVoxelCount() const noexcept { return static_cast<std::size_t>(activeDim_) * activeDim_ * activeDim_; }

    [[nodiscard]] cl_mem activeGpuReadBuffer() const noexcept {
        return gpuReadA_ ? volumeGpuA_ : volumeGpuB_;
    }
    [[nodiscard]] cl_mem descriptorGpuBuffer() const noexcept { return descriptorBuffer_; }
    [[nodiscard]] cl_context openClContext() const noexcept { return context_; }
    [[nodiscard]] cl_command_queue openClQueue() const noexcept { return queue_; }

    // -------------------------------------------------------------------------
    // Binary Stream & File Serialization (.volmem sidecar)
    // -------------------------------------------------------------------------
    // Serializes and deserializes the entire 3D volumetric field state including
    // resolution tier, active dimensions, simulation metrics, and dense voxel array.
    // Fails before writing a header if current device contents cannot be acquired.
    bool saveToStream(std::ostream& os) const;
    // Reject non-finite voxel payloads before changing the active field.
    bool loadFromStream(std::istream& is);
    // Publish a complete sibling by rename after checked write/sync/close.
    // Failure preserves the previous destination; this commits only one file.
    bool saveToFile(const std::string& filepath) const;
    bool loadFromFile(const std::string& filepath);

private:
    // Stop execution without releasing device resources or discarding which copy
    // is current. Recovery/readback and explicit field replacement own that choice.
    void deactivateGpu(const char* backendName) noexcept;
    // Resource-level upload shared by startup preparation and initialized sync.
    // This does not publish GPU lifecycle readiness or activation.
    bool uploadHostField();
    // Borrow immutable staging until the nonblocking descriptor write completes.
    // Only reuse of an occupied slot waits; no field readback or queue-wide idle.
    void uploadDescriptor();
    // Cold move/teardown callers settle the owning queue before releasing events.
    void releaseDescriptorUploads() noexcept;
    // After a domain edit: publish vacuum/identity/descriptor, then synchronize
    // an active device or deactivate it. Does not rewind accepted step/time.
    void replaceDomainHistory();
    // No acquisition or mutation: called only with an acquired host observation.
    [[nodiscard]] volumetric_gpu_abi::VolumetricVoxelGpu sampleHostWorld(
        float x, float y, float z) const noexcept;
    void updateDescriptor();
    void swapBuffers() noexcept { readBufferA_ = !readBufferA_; }
    void stepGpu(const SculptureProgramParams& params, float simTime, float deltaSeconds);
    void recomputeMacroOccupancy() const;
    std::string resolveKernelSource(const std::string& preferredPath) const;

    VolumetricMemoryConfig config_{};
    StageResolutionScale currentScale_ = StageResolutionScale::Half;
    std::uint32_t activeDim_ = volumetric_gpu_abi::kHalfGridDimension;
    bool readBufferA_ = true;
    std::uint64_t contentRevision_ = 0U;

    // Double-buffered ping-pong CPU voxel storage
    mutable std::vector<volumetric_gpu_abi::VolumetricVoxelGpu> volumeA_;
    mutable std::vector<volumetric_gpu_abi::VolumetricVoxelGpu> volumeB_;

    volumetric_gpu_abi::VolumetricFieldDescriptorGpu descriptor_{};
    struct DescriptorUploadSlot {
        volumetric_gpu_abi::VolumetricFieldDescriptorGpu descriptor{};
        cl_event completion = nullptr;
    };
    std::array<DescriptorUploadSlot, 2> descriptorUploads_{};
    std::size_t nextDescriptorUpload_ = 0;
    mutable VolumetricMemoryTelemetry telemetry_{};

    // Hierarchical coarse macro-occupancy bitmask (8x8x8 = 512 bits = 64 bytes)
    mutable std::array<std::uint64_t, kMacroWordCount> macroOccupancy_{};
    mutable float macroOccupancyRatio_ = 0.0f;

    // OpenCL GPU Acceleration State
    cl_context context_ = nullptr;
    cl_device_id device_ = nullptr;
    cl_command_queue queue_ = nullptr;
    cl_program program_ = nullptr;
    cl_kernel kernel_ = nullptr;
    cl_kernel auxKernel_ = nullptr;
    cl_mem volumeGpuA_ = nullptr;
    cl_mem volumeGpuB_ = nullptr;
    cl_mem auxVolumeGpu_ = nullptr;
    cl_mem descriptorBuffer_ = nullptr;
    std::uint32_t auxDim_ = 32U;
    mutable std::vector<volumetric_gpu_abi::VolumetricVoxelGpu> auxVolumeCpu_;

    bool gpuReadA_ = true;
    bool gpuActive_ = false;
    bool gpuInitialized_ = false;
    bool gpuEnabled_ = true;
    // True: newer feedback exists only on the device. False: host is current;
    // device readiness after a host edit is separately gated by gpuActive_.
    mutable bool hostBuffersDirty_ = false;
};

} // namespace epi_sculpt
