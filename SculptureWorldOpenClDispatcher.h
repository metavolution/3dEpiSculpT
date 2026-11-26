#pragma once

#include "SculptureProgramSchema.h"
#include "SculptureOpenClContext.h"
#include "SculptureWorldRecipe.h"
#include "SculptureWorldGpuAbi.h"
#include "SculptureVolumetricFeedbackGpuAbi.h"
#include "SculptureExternalMemoryImport.h"
#include "SculptureWorldPerformance.h"
#include "SculptureWorldPresentationTicket.h"
#include "SculptureGpuAbi.h"
#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <array>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <future>
#include <exception>
#include <memory>
#include <utility>
#include <mutex>
#include <algorithm>

namespace epi_sculpt {

struct WorldSubmitResult {
    bool submitted = false;
    float completionMilliseconds = 0.0f;
    std::uint32_t pendingSlots = 0U;
    std::uint32_t pendingCompilations = 0U;
    const world_gpu_abi::WorldSnapshotGpu* completed = nullptr;
    const WorldGenerationMetrics* completedMetrics = nullptr;
    WorldPresentationTicket completedTicket{};
};

class SculptureWorldOpenClDispatcher {
public:
    SculptureWorldOpenClDispatcher(cl_context context, cl_device_id device);
    ~SculptureWorldOpenClDispatcher();

    // Prevent copies
    SculptureWorldOpenClDispatcher(const SculptureWorldOpenClDispatcher&) = delete;
    SculptureWorldOpenClDispatcher& operator=(const SculptureWorldOpenClDispatcher&) = delete;

    // One source lifetime per dispatcher. Recreate it to reload/retry kernels.
    void initializeKernel(const std::string& path);
    
    // Optional setup-time external memory import for zero-copy Vulkan handoff.
    // When no external memory is imported for a slot, the dispatcher transparently
    // falls back to a device-local cl_mem buffer with asynchronous host staging.
    [[nodiscard]] bool importExternalOutput(std::uint32_t slotIndex, const WorldOpenClImportSlot& importSlot);
    // Clears imports only after checked queue retirement. Setup rollback and
    // device teardown share this path; false retains any unreleased handles.
    [[nodiscard]] bool clearExternalOutputs() noexcept;

    // Binds persistent 3D volumetric memory buffers for in-place GPU instance coupling
    // (`couple_world_volumetric_field`) chained on the in-order OpenCL queue prior to
    // Vulkan semaphore signaling.
    void bindVolumetricCoupling(
        cl_mem volumetricVoxelBuffer,
        cl_mem volumetricDescriptorBuffer,
        const volumetric_gpu_abi::VolumetricCouplerConfigGpu& couplerConfig
    ) noexcept;
    void clearVolumetricCoupling() noexcept;
    [[nodiscard]] bool isVolumetricCouplingBound() const noexcept {
        return volumetricVoxelBuffer_ != nullptr && volumetricDescriptorBuffer_ != nullptr;
    }

    // Drains this dispatcher's independent queue, not the compact producer's.
    // Call between frames, with native transport/device owners still alive.
    void waitIdle();
    
    WorldSubmitResult submitWorldAndTakeLatest(
        const SculptureProgramParams& params,
        const WorldRecipe& recipe,
        float time,
        float morph,
        float deltaSeconds,
        std::uint32_t generation);

    void acknowledgeWorldDelivery(const world_gpu_abi::WorldSnapshotGpu* snapshot) noexcept;

    std::uint32_t pendingSlotCount() const noexcept;
    std::uint32_t pendingCompileCount() const noexcept;

    // Proactively triggers asynchronous background compilation / disk binary cache
    // loading for a dedicated world generator kernel. Returns true if the kernel is
    // already compiled and ready for immediate dispatch.
    bool prewarmWorldKernel(WorldGeneratorKind generator) noexcept;
    [[nodiscard]] bool isWorldKernelCompiled(WorldGeneratorKind generator) const noexcept;
    [[nodiscard]] std::size_t compiledWorldKernelCount() const noexcept;

private:
    void checkOpenCl(cl_int result, const char* context) const;
    std::string readTextFile(const std::string& path) const;

    enum class SlotState : std::uint8_t {
        Free = 0,
        Producing = 1,
        Ready = 2,
        Quarantined = 3,
        AwaitingPresentation = 4
    };

    struct WorldSlot {
        gpu_abi::SculptureProgramParamsGpu params{};
        gpu_abi::SculptureFrameGpu frame{};
        world_gpu_abi::WorldRecipeGpu recipe{};
        volumetric_gpu_abi::VolumetricCouplerConfigGpu couplerConfig{};
        world_gpu_abi::WorldSnapshotGpu snapshot{};
        WorldGenerationMetrics metrics{};
        cl_kernel kernel = nullptr;
        cl_kernel couplerKernel = nullptr;
        cl_mem programBuffer = nullptr;
        cl_mem frameBuffer = nullptr;
        cl_mem recipeBuffer = nullptr;
        cl_mem couplerConfigBuffer = nullptr;
        cl_mem outputBuffer = nullptr;
        cl_event firstEvent = nullptr;
        cl_event kernelEvent = nullptr;
        cl_event readEvent = nullptr;
        cl_semaphore_khr vulkanReleaseSemaphore = nullptr;
        cl_semaphore_khr openClReadySemaphore = nullptr;
        bool outputBufferExternal = false;
        std::shared_ptr<const WorldBootstrapHandoff> bootstrap;
        std::uint32_t externalSlotIndex = WorldGenerationMetrics::kNoExternalSlot;
        std::uint32_t lastGeneration = 0U;
        SlotState state = SlotState::Free;
    };

    void pollWorldSlots();
    static void releaseSlotEvents(WorldSlot& slot) noexcept;
    WorldSlot* takeReadyWorldForDelivery() noexcept;
    void enqueueWorld(WorldSlot& slot,
                      const SculptureProgramParams& params,
                      const WorldRecipe& recipe,
                      float time,
                      float morph,
                      float deltaSeconds,
                      std::uint32_t generation);
    void bindWorldKernelArgs(WorldSlot& slot);
    struct CompiledWorldKernel {
        cl_program program = nullptr;
        cl_kernel kernel = nullptr;
        cl_kernel couplerKernel = nullptr;
        CompiledWorldKernel() = default;
        ~CompiledWorldKernel();
        CompiledWorldKernel(const CompiledWorldKernel&) = delete;
        CompiledWorldKernel& operator=(const CompiledWorldKernel&) = delete;
    };
    const CompiledWorldKernel* getOrCompileWorldKernel(std::uint32_t generatorId);

    struct WorldKernelEntry {
        std::future<std::unique_ptr<CompiledWorldKernel>> pending;
        std::unique_ptr<CompiledWorldKernel> compiled;
        // Failure is terminal for this source lifetime; explicit recreation
        // retries. A bad component must not launch a compiler every frame.
        std::exception_ptr failure;
    };
    static std::unique_ptr<CompiledWorldKernel> compileWorldKernel(
        cl_context context, cl_device_id device, const std::string& path,
        const std::string& fallbackSource, std::uint32_t generatorId);

    std::unique_ptr<SculptureOpenClContext> openCl_;
    
    std::string worldKernelSourcePath_;
    std::string worldKernelSourceCode_;
    // One entry owns compilation, its result, or its persistent failure.
    std::unordered_map<std::uint32_t, WorldKernelEntry> worldKernelCache_;

    cl_mem volumetricVoxelBuffer_ = nullptr;
    cl_mem volumetricDescriptorBuffer_ = nullptr;
    volumetric_gpu_abi::VolumetricCouplerConfigGpu volumetricCouplerConfig_{};

    static constexpr std::size_t kSlotCount = kSculptureExternalSlotCount;
    std::array<WorldSlot, kSlotCount> worldSlots_{};
    std::shared_ptr<WorldPresentationHandoff> presentationHandoff_ =
        std::make_shared<WorldPresentationHandoff>();
    static_assert(kSlotCount == WorldPresentationHandoff::kSlotCount);
    mutable std::mutex compileMutex_;
    float worldLastCompletionMilliseconds_ = 0.0f;
};

} // namespace epi_sculpt
