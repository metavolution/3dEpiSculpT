// Camera-independent sculpture generation. OpenCL is authoritative; CPU is
// available only when explicitly selected for debug/validation or recovery.

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <span>
#include <vector>
#include <atomic>
#include <array>

#include "SculptureInteropCapability.h"
#include "SculptureExternalSlotOwnership.h"
#include "SculptureGpuAbi.h"
#include "SculptureStagingRing.h"
#include "SculptureTemporalHistory.h"
#include "SculptureWorldProducer.h"
#include "SculptureExternalMemoryImport.h"
#include "SculptureVolumetricFeedbackGpuAbi.h"

namespace epi_sculpt {

class SculpturePresetLibrary;
class SculptureVolumetricMemory;

// Describes the frame the renderer actually received, rather than merely the
// backend requested for this iteration.  This is the safe input for HUDs and
// future adaptive quality policy while asynchronous OpenCL work is in flight.
enum class SculptureFrameDelivery : std::uint8_t {
    None,
    CpuGenerated,
    OpenClFresh,
    OpenClReused,
};

[[nodiscard]] constexpr std::string_view name(
    SculptureFrameDelivery delivery) noexcept {
    switch (delivery) {
    case SculptureFrameDelivery::None: return "none";
    case SculptureFrameDelivery::CpuGenerated: return "cpu";
    case SculptureFrameDelivery::OpenClFresh: return "opencl-fresh";
    case SculptureFrameDelivery::OpenClReused: return "opencl-reused";
    }
    return "unknown";
}

enum class SculptureSnapshotStorage : std::uint8_t { HostStaged, ExternalMemory };

// Immutable producer-to-renderer handoff. Slot identity is meaningful only
// for ExternalMemory; HostStaged snapshots are copied into a bridge-owned slot.
struct SculptureSnapshotDelivery {
    static constexpr std::uint32_t kNoSlot = 0xffffffffU;
    const gpu_abi::PackedSculptureSnapshot* snapshot = nullptr;
    const gpu_abi::SculptureAnalyticsGpu* analytics = nullptr;
    std::uint32_t generation = 0U;
    std::uint32_t slotIndex = kNoSlot;
    std::uint64_t ownershipEpoch = 0U;
    SculptureSnapshotStorage storage = SculptureSnapshotStorage::HostStaged;
    // The field lease is immutable; the snapshot remains a borrowed view.
    std::shared_ptr<const std::vector<float>> spatialLease;
    std::uint32_t spatialGeneration = 0U;

    [[nodiscard]] bool hasHostSpatial() const noexcept {
        return storage == SculptureSnapshotStorage::HostStaged && matchesSnapshot()
            && spatialGeneration == generation && spatialLease
            && spatialLease->size() == 4U * gpu_abi::kVolumeVoxelCount;
    }
    [[nodiscard]] bool hasExternalSpatial() const noexcept {
        return usesExternalMemory() && matchesSnapshot() && spatialGeneration == generation;
    }
    [[nodiscard]] std::span<const float> hostSpatial() const noexcept {
        return hasHostSpatial() ? std::span<const float>(*spatialLease) : std::span<const float>{};
    }

    [[nodiscard]] constexpr bool usesExternalMemory() const noexcept {
        return storage == SculptureSnapshotStorage::ExternalMemory
            && slotIndex != kNoSlot && ownershipEpoch != 0U;
    }

    [[nodiscard]] constexpr bool matchesSnapshot() const noexcept {
        return generation != 0U && snapshot != nullptr
            && generation == snapshot->frame.generation;
    }
};

// CPU snapshots are a fallback/auxiliary representation, not aliases of an
// asynchronously delivered OpenCL slot. Their generation must therefore be
// carried explicitly before an application can reuse one beside a delivery.
struct SculptureCpuSnapshotDelivery {
    const SculptureSnapshot* snapshot = nullptr;
    std::uint32_t generation = 0U;
    bool valid = false;

    [[nodiscard]] constexpr bool matches(std::uint32_t expectedGeneration) const noexcept {
        return valid && snapshot != nullptr && generation == expectedGeneration;
    }
};

// Identity of the compact OpenCL program currently allowed to submit work.
// This is an observation value: it does not claim that an already-delivered
// asynchronous snapshot was produced by the latest live program.
struct SculptureKernelBuildIdentity {
    std::uint64_t revision = 0U;
    std::uint64_t sourceFingerprint = 0U;
    bool valid = false;
};

struct SculptureProducerTelemetry {
    float cpuCallMilliseconds = 0.0f;
    float openClCompletionMilliseconds = 0.0f;
    std::uint32_t requestedGeneration = 0U;
    std::uint32_t deliveredGeneration = 0U;
    std::uint32_t pendingSlots = 0U;
    std::uint32_t pendingCompilations = 0U;
    std::size_t programCacheSize = 0U;
    std::size_t programCacheHits = 0U;
    std::size_t programCacheMisses = 0U;
    std::uint32_t staleFrameReuses = 0U;
    std::uint32_t droppedSubmissions = 0U;
    std::uint32_t temporalSourceGeneration = 0U;
    bool outputFromOpenCl = false;
    bool submissionAccepted = false;
    // Sticky for this producer lifetime: a dispatch exception may have
    // queued work without returning a reliable acceptance/completion fact.
    bool submissionIndeterminate = false;
    bool temporalHistoryUsed = false;
    SculptureFrameDelivery delivery = SculptureFrameDelivery::CpuGenerated;
    SculptureInteropHealth interop{};
};

// Dedicated worlds do not share compact analytics, so their scheduling health
// is reported separately and consumed only by application quality policy.
struct SculptureWorldProducerTelemetry {
    float openClCompletionMilliseconds = 0.0f;
    float openClKernelMilliseconds = WorldGenerationMetrics::kUnavailableMilliseconds;
    float openClReadbackMilliseconds = WorldGenerationMetrics::kUnavailableMilliseconds;
    std::uint32_t openClReadbackBytes = 0U;
    std::uint32_t activeInstanceCount = 0U;
    std::uint32_t requestedGeneration = 0U;
    std::uint32_t deliveredGeneration = 0U;
    std::uint32_t pendingSlots = 0U;
    std::uint32_t pendingCompilations = 0U;
    std::uint32_t droppedSubmissions = 0U;
    bool outputFromOpenCl = false;
    bool submissionAccepted = false;
};

class SculptureFrameProducer {
public:
    static constexpr std::size_t kZeroCopySlotCount = 3;

    struct ZeroCopyBuffer {
        int exportFd = -1;
        std::size_t sizeBytes = 0;
        void* hostMapped = nullptr;
        int spatialFd = -1;
        std::size_t spatialSizeBytes = 0;
        // Binary semaphore pair: Vulkan signals `vulkanReleaseSemaphoreFd`
        // after reading; OpenCL signals `openClReleaseSemaphoreFd` after writing.
        int vulkanReleaseSemaphoreFd = -1;
        int openClReleaseSemaphoreFd = -1;
    };

    // Pure ABI gate: every slot must describe the same complete external
    // memory contract before OpenCL is allowed to replace the staged buffers.
    [[nodiscard]] static bool hasCompleteZeroCopyBufferSet(
        std::span<const ZeroCopyBuffer> buffers) noexcept {
        if (buffers.size() != kZeroCopySlotCount) return false;

        for (const ZeroCopyBuffer& buffer : buffers) {
            if (buffer.exportFd < 0 || buffer.hostMapped == nullptr
                || buffer.sizeBytes != sizeof(gpu_abi::PackedSculptureSnapshot)
                || buffer.spatialFd < 0
                || buffer.spatialSizeBytes
                    != sizeof(float) * 4 * gpu_abi::kVolumeVoxelCount
                || buffer.vulkanReleaseSemaphoreFd < 0
                || buffer.openClReleaseSemaphoreFd < 0) {
                return false;
            }
        }
        return true;
    }
    
    // Optional external-memory handoff. It is accepted only after the producer
    // has verified compatible devices and a real synchronization protocol.
    bool importZeroCopyBuffers(std::span<const ZeroCopyBuffer> buffers);
    // Release imports only after the OpenCL queue and all graphics uses of
    // the exported buffers are retired. This is a device-lifetime boundary,
    // not a normal frame path.
    void clearZeroCopyBuffers() noexcept;
    bool importWorldExternalOutput(std::uint32_t slotIndex, const WorldOpenClImportSlot& importSlot);
    [[nodiscard]] bool clearWorldExternalOutputs() noexcept;
    bool initializeVolumetricMemoryGpu(SculptureVolumetricMemory& volMem, const std::string& kernelSourceOrPath = "") noexcept;
    void bindWorldVolumetricCoupling(
        const SculptureVolumetricMemory& volMem,
        const volumetric_gpu_abi::VolumetricCouplerConfigGpu& couplerConfig) noexcept;
    void clearWorldVolumetricCoupling() noexcept;
    [[nodiscard]] bool isWorldVolumetricCouplingBound() const noexcept;
    bool importGridExternalOutput(std::uint32_t slotIndex, const SculptureOpenClImportSlot& importSlot);
    bool submitGridTransform(const SculptureProgramParams& params, float time, float morph, std::uint32_t gridResolution, std::uint32_t slotIndex);


    struct Config {
        std::string kernelPath;
        // Optional dedicated-world kernel.  It shares the compact producer's
        // already selected device, context, and queue; it never opens a
        // second OpenCL connection just to render a different topology.
        std::string worldKernelPath;
        std::string gridKernelPath;
        // Set false only for an explicit CPU debug/validation run. This is
        // distinct from automatic recovery after an OpenCL failure.
        bool preferOpenCl = true;
        // OpenCL is the sole mathematical producer in production. A failed
        // dispatch reuses the last coherent OpenCL delivery when available;
        // it never starts an unrequested parallel CPU computation lane.
        bool enableCpuFailureRecovery = false;
        std::array<uint8_t, 16> requiredDeviceUuid = {};
        bool matchDeviceUuid = false;
        // Vulkan owns this capability. OpenCL import support is queried from
        // the selected device before an external handoff can be eligible.
        bool vulkanExternalMemoryFd = false;
        bool vulkanExternalSemaphoreFd = false;
    };

    SculptureFrameProducer();
    explicit SculptureFrameProducer(Config config);
    // Fail closed before destroying a retained indeterminate native session.
    // There is currently no recovery/retirement proof API for that state.
    ~SculptureFrameProducer();

    SculptureFrameProducer(const SculptureFrameProducer&) = delete;
    SculptureFrameProducer& operator=(const SculptureFrameProducer&) = delete;

    // Generation zero is reserved for an absent delivery. Accepted compact
    // generations increase without wrap for the producer's lifetime. Zero or
    // an already-accepted generation is rejected before production; an
    // unaccepted generation may be retried. After UINT32_MAX, callers must
    // start a new producer/identity session to produce another generation.
    // An indeterminate OpenCL failure blocks subsequent production instead of
    // replaying possibly queued mathematics on the CPU.
    [[nodiscard]] const gpu_abi::PackedSculptureSnapshot* produce(
        const SculptureProgramParams& params,
        float time,
        float morph,
        float deltaSeconds,
        std::uint32_t generation);

    // Observe asynchronous completions without admitting a new mathematical
    // generation. No CPU evaluation, queue wait, or cadence mutation occurs.
    [[nodiscard]] const gpu_abi::PackedSculptureSnapshot* pollCompleted();

    // Dedicated procedural-world lane.  It is intentionally separate from
    // the compact sample ABI: callers opt in only for recipes that require
    // full transforms and primitive/material selection.
    [[nodiscard]] WorldSnapshotDelivery produceWorld(
        const SculptureProgramParams& params,
        const WorldRecipe& recipe,
        float time,
        float morph,
        float deltaSeconds,
        std::uint32_t generation) noexcept;
    [[nodiscard]] WorldSnapshotDelivery worldDelivery() const noexcept;

    // A faulted backend remains owned for lifetime safety but is unavailable.
    [[nodiscard]] bool usingOpenCl() const noexcept;
    [[nodiscard]] bool worldKernelReady() const noexcept;
    bool prewarmWorldKernel(WorldGeneratorKind generator) noexcept;
    [[nodiscard]] bool isWorldKernelCompiled(WorldGeneratorKind generator) const noexcept;
    [[nodiscard]] std::size_t compiledWorldKernelCount() const noexcept;
    [[nodiscard]] std::string_view backendName() const noexcept;
    [[nodiscard]] std::string_view diagnostic() const noexcept;
    [[nodiscard]] std::string_view worldDiagnostic() const noexcept;
    [[nodiscard]] float lastFrameMilliseconds() const noexcept;
    [[nodiscard]] const SculptureProducerTelemetry& telemetry() const noexcept;
    [[nodiscard]] const SculptureWorldProducerTelemetry& worldTelemetry() const noexcept;
    // Re-evaluate zero-copy eligibility after the Vulkan presentation device
    // changes. This never rebuilds or resets the OpenCL mathematical producer;
    // it only decides whether a future native import may be attempted.
    [[nodiscard]] bool requalifyExternalMemoryInterop(
        const std::array<std::uint8_t, 16>& vulkanDeviceUuid,
        bool vulkanExternalMemoryFd,
        bool vulkanExternalSemaphoreFd) noexcept;
    [[nodiscard]] bool externalMemoryInteropReady() const noexcept;
    [[nodiscard]] SculptureInteropState interopState() const noexcept;
    [[nodiscard]] const SculptureSnapshotDelivery& delivery() const noexcept;
    
    // Legacy tooling can inspect an explicitly requested CPU validation frame.
    // Production routes keep this empty because OpenCL remains authoritative.
    [[nodiscard]] SculptureCpuSnapshotDelivery cpuSnapshotDelivery() const noexcept {
        return {lastCpuSnapshot_.get(), lastCpuSnapshotGeneration_, lastCpuSnapshotValid_};
    }
    // Returns a generation-validated private copy of the latest delivery, or
    // empty if its bytes are unavailable. CPU recovery is only a CPU fallback.
    // Renderers use this only before queue commitment when an external slot
    // cannot be acquired; it never aliases Vulkan/OpenCL shared storage.
    [[nodiscard]] SculptureSnapshotDelivery safeStagedDelivery() const noexcept;
    [[nodiscard]] bool acquireExternalDelivery(
        const SculptureSnapshotDelivery& delivery) noexcept;
    [[nodiscard]] bool releaseExternalDelivery(
        const SculptureSnapshotDelivery& delivery) noexcept;
    void cancelExternalDelivery(
        const SculptureSnapshotDelivery& delivery) noexcept;
    [[nodiscard]] bool isKernelCompilationActive() const noexcept;
    [[nodiscard]] SculptureKernelBuildIdentity kernelBuildIdentity() const noexcept;
    [[nodiscard]] std::size_t programCacheSize() const noexcept;
    [[nodiscard]] std::size_t programCacheHits() const noexcept;
    [[nodiscard]] std::size_t programCacheMisses() const noexcept;
    void startPrewarming(const SculpturePresetLibrary& library) noexcept;
    void stopPrewarming() noexcept;
    [[nodiscard]] bool isPrewarming() const noexcept;
    void triggerAsyncRecompile(const SculptureProgramParams& params) noexcept;

    // Hot-reloads OpenCL compute shaders dynamically from disk with zero downtime.
    bool reloadKernels() noexcept;

    // Explicit synchronization for shutdown, capture, and deterministic tools.
    // Normal rendering does not call this in its frame loop.
    // True only when all owned OpenCL producer queues have drained, or when
    // no OpenCL backend exists and no dispatch outcome is indeterminate.
    // Used before retiring shared native resources.
    [[nodiscard]] bool waitIdle() noexcept;

private:
    class OpenClState;

    // The only transition out of external-memory mode. Keeping it centralized
    // prevents status, selected backend, and diagnostics from disagreeing
    // after a recoverable failure.
    void enterExternalMemoryFallback(SculptureInteropEvent event,
                                     std::string diagnostic,
                                     bool preserveOpenClProducer) noexcept;
    void retainSafeObservation() noexcept;
    [[nodiscard]] const gpu_abi::PackedSculptureSnapshot* acceptOpenClCompletion(
        const gpu_abi::PackedSculptureSnapshot* completed,
        const gpu_abi::SculptureAnalyticsGpu* analytics,
        const std::shared_ptr<const std::vector<float>>& spatialLease,
        std::uint32_t spatialGeneration) noexcept;
    [[nodiscard]] bool markExternalSlotUnavailable(
        std::uint32_t slotIndex,
        SculptureExternalSlotTicket ticket) noexcept;

    [[nodiscard]] const gpu_abi::PackedSculptureSnapshot* produceCpu(
        const SculptureProgramParams& params,
        float time,
        float morph,
        float deltaSeconds,
        std::uint32_t generation) const;
    void publishDelivery(const gpu_abi::PackedSculptureSnapshot* snapshot,
                         const gpu_abi::SculptureAnalyticsGpu* analytics,
                         bool fromOpenCl,
                         std::shared_ptr<const std::vector<float>> spatialLease = {},
                         std::uint32_t spatialGeneration = 0U) noexcept;

    std::unique_ptr<OpenClState> openCl_;
    std::string backendName_ = "OpenCL pending";
    std::string diagnostic_;
    std::string worldRuntimeDiagnostic_;
    bool enableCpuFailureRecovery_ = false;
    mutable std::unique_ptr<gpu_abi::PackedSculptureSnapshot> lastSnapshot_ = std::make_unique<gpu_abi::PackedSculptureSnapshot>();
    mutable std::unique_ptr<SculptureSnapshot> lastCpuSnapshot_ = std::make_unique<SculptureSnapshot>();
    // Separate output from retained feedback; recycle storage after successful
    // CPU evaluation so a failed step cannot mutate the accepted history.
    mutable std::unique_ptr<SculptureSnapshot> candidateCpuSnapshot_;
    mutable std::uint32_t lastCpuSnapshotGeneration_ = 0U;
    mutable bool lastCpuSnapshotValid_ = false;
    mutable SculptureTemporalFamily lastCpuTemporalFamily_ = SculptureTemporalFamily::Surface;
    // Acceptance is distinct from delivery: queued work can advance this
    // watermark before a completed snapshot becomes available.
    std::uint32_t lastAcceptedGeneration_ = 0U;
    SculptureProducerTelemetry telemetry_{};
    bool hasLastSnapshot_ = false;
    bool lastSnapshotFromOpenCl_ = false;
    SculptureExternalSlotOwnership<> externalSlotOwnership_{};
    SculptureSnapshotDelivery delivery_{};
    std::shared_ptr<const std::vector<float>> lastHostSpatialLease_;
    std::uint32_t lastHostSpatialGeneration_ = 0U;
    mutable std::unique_ptr<SculptureStagingRing<gpu_abi::PackedSculptureSnapshot, 3>> hostStagingRing_ =
        std::make_unique<SculptureStagingRing<gpu_abi::PackedSculptureSnapshot, 3>>();
    const gpu_abi::PackedSculptureSnapshot* lastSnapshotPtr_ = nullptr;
    gpu_abi::SculptureAnalyticsGpu lastAnalytics_{};
    const gpu_abi::SculptureAnalyticsGpu* lastAnalyticsPtr_ = nullptr;
    SculptureWorldProducer worldProducer_{};
    SculptureWorldProducerTelemetry worldTelemetry_{};
};

} // namespace epi_sculpt
