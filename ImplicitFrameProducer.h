#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "ImplicitFrameRequest.h"
#include "ImplicitQualityPolicy.h"
#include "ImplicitSubmissionResult.h"
#include "ImplicitExternalFrameContract.h"
#include "SculptureInteropCapability.h"

// OpenCL-based implicit field raymarcher producer.
namespace epi_sculpt {

enum class ImplicitFrameDelivery : std::uint8_t {
    NoFrame,
    Fresh,
    Reused,
};

struct ImplicitProducerTelemetry {
    // Counts actual flushed requests, excluding healthy backpressure drops.
    std::uint64_t submittedRequests = 0U;
    float openClCompletionMilliseconds = 0.0f;
    float kernelMilliseconds = 0.0f;
    std::uint32_t requestedGeneration = 0U;
    std::uint32_t deliveredGeneration = 0U;
    std::uint32_t pendingSlots = 0U;
    std::uint32_t staleFrameReuses = 0U;
    std::uint32_t droppedSubmissions = 0U;
    std::uint32_t maxRaySteps = 128U;
    StageResolutionScale resolutionScale = StageResolutionScale::Full;
    std::uint32_t qualityAdjustments = 0U;
    std::uint64_t stagedColorBytes = 0U;
    std::uint64_t stagedAuxiliaryBytes = 0U;
    ImplicitFrameDelivery delivery = ImplicitFrameDelivery::NoFrame;
    SculptureInteropHealth interop{};
};

// Immutable producer-to-presentation view. Linear HDR color is mandatory;
// depth is optional because the current fullscreen compositor does not consume
// it. When requested, both originate from the same completed slot/generation.



class ImplicitFrameProducer {
public:
    static constexpr std::size_t kSlotCount = 3U;

    struct OutputBuffers {
        std::vector<float> color;    // RGBA float
        std::vector<float> depth;    // float
        std::vector<float> normal;   // RGBA float
        std::vector<float> velocity; // RG float
    };

    struct Config {
        std::string kernelPath;
        std::uint32_t width = 1920;
        std::uint32_t height = 1080;
        ImplicitQualityConfig quality{};
        // Live Vulkan presentation supplies its physical-device UUID. The
        // implicit OpenCL renderer must use that same GPU; a second GPU would
        // make the host-staged image transfer a hidden performance cliff.
        std::array<std::uint8_t, 16> requiredDeviceUuid{};
        bool matchDeviceUuid = false;
        // Auxiliaries are intentionally opt-in. The live fullscreen
        // compositor currently samples only HDR color, so allocating,
        // ray-writing, and reading depth/normal/velocity would be invisible
        // GPU and PCIe work.
        bool includeDepth = false;
        bool includeNormal = false;
        bool includeVelocity = false;
        // Alternate optical fields reuse the same immutable 18-argument image
        // delivery contract; they do not need another OpenCL/Vulkan runtime.
        std::string kernelEntryPoint = "raymarch_sculpture_chamber";
    };


    struct ZeroCopyBuffer {
        std::uint32_t slotIndex = ImplicitExternalColorSlot::kInvalidSlot;
        std::uint64_t allocationGeneration = 0U;
        std::uint32_t width = 0U;
        std::uint32_t height = 0U;
        ImplicitExternalColorFormat format = ImplicitExternalColorFormat::Rgba32Float;
        int colorFd = -1;
        int depthFd = -1;
        int normalFd = -1;
        int velocityFd = -1;
        // Allocation bounds, not a description of optimal image tiling.
        std::size_t colorAllocationSizeBytes = 0U;
        std::size_t depthSizeBytes = 0U;
        std::size_t normalSizeBytes = 0U;
        std::size_t velocitySizeBytes = 0U;
        int vulkanReleaseSemaphoreFd = -1;
        int openClReadySemaphoreFd = -1;

        [[nodiscard]] ImplicitExternalColorSlot contract() const noexcept {
            return ImplicitExternalColorSlot{
                .slotIndex = slotIndex,
                .allocationGeneration = allocationGeneration,
                .width = width,
                .height = height,
                .format = format,
                .colorAllocationSizeBytes = colorAllocationSizeBytes,
                .depthSizeBytes = depthSizeBytes,
                .normalSizeBytes = normalSizeBytes,
                .velocitySizeBytes = velocitySizeBytes,
                .colorFd = colorFd,
                .depthFd = depthFd,
                .normalFd = normalFd,
                .velocityFd = velocityFd,
                .vulkanReleaseSemaphoreFd = vulkanReleaseSemaphoreFd,
                .openClReadySemaphoreFd = openClReadySemaphoreFd
            };
        }
    };

    // Owns all exported FDs until individually consumed by successful OpenCL
    // imports. Moving the batch transfers ownership; dropping it closes unused
    // exports, including those left after partial failure. No GPU retirement.
    class ExternalImageImports {
    public:
        ExternalImageImports() = default;
        ~ExternalImageImports();
        ExternalImageImports(ExternalImageImports&& other) noexcept;
        ExternalImageImports& operator=(ExternalImageImports&& other) noexcept;
        ExternalImageImports(const ExternalImageImports&) = delete;
        ExternalImageImports& operator=(const ExternalImageImports&) = delete;

        [[nodiscard]] std::span<const ZeroCopyBuffer> slots() const noexcept { return slots_; }

    private:
        friend class ImplicitFrameProducer;
        friend class ImplicitExternalImageTransport;
        void reset() noexcept;
        std::array<ZeroCopyBuffer, kSlotCount> slots_{};
    };

    // Validates the complete producer-side half of the four-image ABI.
    // The presentation adapter separately proves Vulkan allocation/export and
    // submit-time semaphore ownership before it consumes an external frame.
    [[nodiscard]] static bool hasCompleteZeroCopyBufferSet(
        std::span<const ZeroCopyBuffer> buffers,
        std::uint32_t expectedWidth,
        std::uint32_t expectedHeight) noexcept;

    explicit ImplicitFrameProducer(const Config& config);
    ~ImplicitFrameProducer();

    // Initialization-only, before any request. Owns the batch on every outcome.
    // The legacy method name does not imply a buffer kernel or copy-free display.
    [[nodiscard]] bool importZeroCopyBuffers(ExternalImageImports imports);

    // Disable copy/move for simplicity
    ImplicitFrameProducer(const ImplicitFrameProducer&) = delete;
    ImplicitFrameProducer& operator=(const ImplicitFrameProducer&) = delete;

    // Compatibility health APIs; new admission-sensitive callers use submit().
    bool update(const SculptureProgramParams& params,
                float time,
                float morph,
                float deltaSeconds,
                std::uint32_t generation);
    // The external transport chooses and releases this exact slot through its
    // Vulkan/OpenCL ownership transaction. Normal staged users keep update().
    bool updateExternal(const SculptureProgramParams& params,
                        float time,
                        float morph,
                        float deltaSeconds,
                        std::uint32_t generation,
                        std::uint32_t slotIndex);
    [[nodiscard]] ImplicitSubmissionResult submit(const SculptureProgramParams& params,
        float time, float morph, float deltaSeconds, std::uint32_t generation);
    [[nodiscard]] ImplicitSubmissionResult submitExternal(const SculptureProgramParams& params,
        float time, float morph, float deltaSeconds, std::uint32_t generation,
        std::uint32_t slotIndex);
    // Prepared requests carry their own camera, extent and generation. No
    // mutable camera state is consulted after scheduling a deferred submission.
    [[nodiscard]] ImplicitSubmissionResult submitRequest(const ImplicitFrameRequest& request);
    [[nodiscard]] ImplicitSubmissionResult submitExternalRequest(
        const ImplicitFrameRequest& request, std::uint32_t slotIndex);
    [[nodiscard]] std::optional<std::uint32_t>
    nextExternalSlotIndex() const noexcept;
    // Explicit offline/capture barrier. The interactive update path never waits.
    bool waitForGeneration(std::uint32_t generation);
    // Poll even when all slots are busy or the simulation clock is paused.
    // Completion must not depend on admitting another production request.
    bool pollCompleted();
    void setCamera(const ImplicitCamera& camera);
    void setCamera(float px, float py, float pz,
                   float tx, float ty, float tz,
                   float ux, float uy, float uz,
                   float fovYRadians = 1.0f,
                   float eyeSeparation = 0.0f);
    uint32_t width() const;
    uint32_t height() const;
    [[nodiscard]] const OutputBuffers& getOutput() const;
    // Available only after a zero-copy slot has completed. It deliberately
    // carries no descriptor or synchronization ownership; use it to hand the
    // completed identity to the Vulkan/OpenCL transport adapter.
    [[nodiscard]] ImplicitExternalFrameView externalFrameView() const noexcept;
    // Call only after Vulkan has submitted the final copy/read and external
    // ownership release for this exact frame. The next production admission
    // signals Vulkan-release once. This frees the slot; it submits no GPU work.
    [[nodiscard]] bool releaseExternalFrameAfterVulkanSubmit(
        const ImplicitExternalFrameView& frame) noexcept;
    const ImplicitProducerTelemetry& telemetry() const noexcept;
    // Copy metadata before a later poll can recycle the delivered slot.
    [[nodiscard]] std::optional<ImplicitFrameRequest> deliveredRequest() const noexcept;
    [[nodiscard]] const ImplicitQualityPolicy& qualityPolicy() const noexcept;
    bool hasValidOutput() const noexcept;
    const std::string& diagnostic() const { return diagnostic_; }

private:
    ImplicitSubmissionResult updateIntoSlot(const ImplicitFrameRequest& request,
        std::optional<std::size_t> requestedSlot);
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::string diagnostic_;
};

} // namespace epi_sculpt
