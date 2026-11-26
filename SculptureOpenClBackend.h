#pragma once
#include "SculptureFrameProducer.h"
#include "SculptureFractalCatalog.h"
#include "SculptureWorldGpuAbi.h"
#include "SculptureWorldOpenClDispatcher.h"
#include "SculptureGridOpenClDispatcher.h"
#include <filesystem>
#include "SculptureTemporalHistory.h"
#include "SculptureProgramBinaryCache.h"
#include "SculptureKernelAssembler.h"
#include "SculptureKernelSourceWatch.h"
#include "SculptureProgramVariantKey.h"
#include "SculptureKernelSpecializer.h"
#include "SculptureCompactBuild.h"
#include "SculptureCompactSourceBundle.h"

#include <chrono>
#include <algorithm>
#include <cstring>
#include <exception>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

#ifndef EPI_SCULPT_HAS_OPENCL
#define EPI_SCULPT_HAS_OPENCL 0
#endif

#if EPI_SCULPT_HAS_OPENCL
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <CL/cl_ext.h>
#endif

namespace epi_sculpt {

#if EPI_SCULPT_HAS_OPENCL
namespace {

[[nodiscard]] std::string readTextFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open OpenCL source: " + path);
    }
    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

void checkOpenCl(cl_int result, const char* operation) {
    if (result != CL_SUCCESS) {
        throw std::runtime_error(
            std::string(operation) + " failed with OpenCL error "
            + std::to_string(result));
    }
}

} // namespace

// -----------------------------------------------------------------------------
// Educational Annotation: UNIVERSAL OPENCL DRIVER
// -----------------------------------------------------------------------------
// A generalized OpenCL device context manager that provides a fault-tolerant
// unified GPU driver connection. Both the Instancing (3D) and Raymarching (2D)
// pipelines share this foundation, avoiding context duplication and VRAM waste.
// -----------------------------------------------------------------------------
struct SelectedDevice {
    cl_platform_id platform = nullptr;
    cl_device_id device = nullptr;
};

class OpenClDriver {
public:
    struct Config {
        bool preferOpenCl = true;
        std::array<uint8_t, 16> requiredDeviceUuid = {};
        bool matchDeviceUuid = false;
        bool vulkanExternalMemoryFd = false;
        bool vulkanExternalSemaphoreFd = false;
    };

    static SelectedDevice selectDevice(const Config& config) {
        cl_uint platformCount = 0U;
        checkOpenCl(clGetPlatformIDs(0U, nullptr, &platformCount), "clGetPlatformIDs(count)");
        if (platformCount == 0U) throw std::runtime_error("no OpenCL platform is available");

        std::vector<cl_platform_id> platforms(platformCount);
        checkOpenCl(clGetPlatformIDs(platformCount, platforms.data(), nullptr), "clGetPlatformIDs(list)");

        for (const cl_device_type type : {CL_DEVICE_TYPE_GPU, CL_DEVICE_TYPE_CPU}) {
            for (const cl_platform_id platform : platforms) {
                cl_uint deviceCount = 0U;
                const cl_int countResult = clGetDeviceIDs(platform, type, 0U, nullptr, &deviceCount);
                if (countResult == CL_DEVICE_NOT_FOUND || deviceCount == 0U) continue;
                checkOpenCl(countResult, "clGetDeviceIDs(count)");
                std::vector<cl_device_id> devices(deviceCount);
                checkOpenCl(clGetDeviceIDs(platform, type, deviceCount, devices.data(), nullptr), "clGetDeviceIDs(list)");
                            
                for (cl_device_id device : devices) {
                    if (config.matchDeviceUuid) {
                        cl_uchar uuid[16] = {};
                        size_t retSize = 0;
                        cl_int uuidResult = clGetDeviceInfo(device, CL_DEVICE_UUID_KHR, sizeof(uuid), uuid, &retSize);
                        if (uuidResult == CL_SUCCESS && retSize == 16) {
                            if (std::memcmp(uuid, config.requiredDeviceUuid.data(), 16) == 0) return {platform, device};
                        }
                        continue;
                    }
                    return {platform, device};
                }
            }
        }
        throw std::runtime_error("no OpenCL GPU or CPU device is available matching criteria");
    }
};

namespace {

[[nodiscard]] SelectedDevice selectDevice(const SculptureFrameProducer::Config& config) {
    cl_uint platformCount = 0U;
    checkOpenCl(clGetPlatformIDs(0U, nullptr, &platformCount),
                "clGetPlatformIDs(count)");
    if (platformCount == 0U) {
        throw std::runtime_error("no OpenCL platform is available");
    }

    std::vector<cl_platform_id> platforms(platformCount);
    checkOpenCl(clGetPlatformIDs(platformCount, platforms.data(), nullptr),
                "clGetPlatformIDs(list)");

    for (const cl_device_type type : {CL_DEVICE_TYPE_GPU, CL_DEVICE_TYPE_CPU}) {
        for (const cl_platform_id platform : platforms) {
            cl_uint deviceCount = 0U;
            const cl_int countResult = clGetDeviceIDs(
                platform, type, 0U, nullptr, &deviceCount);
            if (countResult == CL_DEVICE_NOT_FOUND || deviceCount == 0U) {
                continue;
            }
            checkOpenCl(countResult, "clGetDeviceIDs(count)");
            std::vector<cl_device_id> devices(deviceCount);
            checkOpenCl(clGetDeviceIDs(
                            platform, type, deviceCount, devices.data(), nullptr),
                        "clGetDeviceIDs(list)");
                        
            for (cl_device_id device : devices) {
                if (config.matchDeviceUuid) {
                    cl_uchar uuid[16] = {};
                    size_t retSize = 0;
                    cl_int uuidResult = clGetDeviceInfo(device, CL_DEVICE_UUID_KHR, sizeof(uuid), uuid, &retSize);
                    if (uuidResult == CL_SUCCESS && retSize == 16) {
                        if (std::memcmp(uuid, config.requiredDeviceUuid.data(), 16) == 0) {
                            return {platform, device};
                        }
                    }
                    continue;
                }
                return {platform, device};
            }
        }
    }
    throw std::runtime_error("no OpenCL GPU or CPU device is available matching criteria");
}

[[nodiscard]] std::string deviceName(cl_device_id device) {
    std::size_t bytes = 0U;
    checkOpenCl(clGetDeviceInfo(
                    device, CL_DEVICE_NAME, 0U, nullptr, &bytes),
                "clGetDeviceInfo(name size)");
    std::string name(bytes, '\0');
    checkOpenCl(clGetDeviceInfo(
                    device, CL_DEVICE_NAME, bytes, name.data(), nullptr),
                "clGetDeviceInfo(name)");
    while (!name.empty() && name.back() == '\0') {
        name.pop_back();
    }
    return name;
}


} // namespace
#endif

class SculptureFrameProducer::OpenClState {
public:
#if EPI_SCULPT_HAS_OPENCL
    struct SubmitResult {
        gpu_abi::PackedSculptureSnapshot* completed = nullptr;
        gpu_abi::SculptureAnalyticsGpu* analytics = nullptr;
        std::shared_ptr<const std::vector<float>> spatialLease;
        std::uint32_t spatialGeneration = 0U;
        float completionMilliseconds = 0.0f;
        std::uint32_t pendingSlots = 0U;
        std::uint32_t temporalSourceGeneration = 0U;
        bool submitted = false;
        bool temporalHistoryUsed = false;
    };

    // The dedicated world lane shares the selected device and context, while
    // its dispatcher owns a separate queue and host-visible ring. Both queues
    // must retire before their common presentation resources can be released.

    explicit OpenClState(const SculptureFrameProducer::Config& config) {
        try {
            initialize(config);
        } catch (...) {
            release();
            throw;
        }
    }

    ~OpenClState() {
        release();
    }

    // Stop future compiler publication without waiting on the OpenCL driver.
    // The backend and all native resources remain owned by the caller.
    void quarantineBackgroundWork() noexcept {
        stopWatcher_.store(true, std::memory_order_relaxed);
        stopCompiler_.store(true, std::memory_order_release);
        if (binaryPrewarmer_) binaryPrewarmer_->requestStop();
        compilerRequestCv_.notify_all();
    }

    // Called after quarantine, outside dispatchLock: a publication already
    // holding this mutex must finish before the fault handler returns.
    void settleCompilerPublication() noexcept {
        std::lock_guard<std::mutex> lock(compileMutex_);
    }

    [[nodiscard]] SubmitResult submitAndTakeLatest(
        const SculptureProgramParams& params,
        float time,
        float morph,
        float deltaSeconds,
        std::uint32_t generation,
        SculptureExternalSlotOwnership<>* externalOwnership) {
        // Kernel publication is brief, but it must not race argument binding
        // or queue submission. Compilation itself happens outside this lock.
        std::unique_lock<std::mutex> dispatchLock(compileMutex_);
        struct FreezeOnFailure {
            OpenClState& owner;
            int exceptions = std::uncaught_exceptions();
            ~FreezeOnFailure() noexcept {
                if (std::uncaught_exceptions() > exceptions)
                    owner.quarantineBackgroundWork();
            }
        } freezeOnFailure{*this};
        std::uint32_t paramsInnerA = static_cast<std::uint32_t>(params.inner);
        std::uint32_t paramsInnerB = static_cast<std::uint32_t>(params.innerB);
        std::uint32_t paramsSeedA = static_cast<std::uint32_t>(params.seedType);
        std::uint32_t paramsSeedB = static_cast<std::uint32_t>(params.seedTypeB);
        std::uint32_t paramsReduction = static_cast<std::uint32_t>(params.reduction);
        std::uint32_t paramsColorEffect = static_cast<std::uint32_t>(params.colorEffect);
        
        const auto matchesLiveVariant = [&] {
            return paramsInnerA == active_innerA_ && paramsInnerB == active_innerB_
                && paramsSeedA == active_seedA_ && paramsSeedB == active_seedB_
                && paramsReduction == active_reduction_ && paramsColorEffect == active_colorEffect_;
        };
        if (!matchesLiveVariant() && !isCompiling()) {
            // Request admission takes compileMutex_ itself. Do not recursively
            // lock it, and let an in-flight build finish before retrying the
            // current variant; otherwise each frame supersedes the same build.
            dispatchLock.unlock();
            triggerAsyncRecompile(params);
            dispatchLock.lock();
        }

        SubmitResult result = collectCompleted(externalOwnership);

        // Component snippets are baked into the live program. Continue to
        // publish completed work, but do not accept a new generation under
        // a different mathematical program while its variant is compiling.
        if (!matchesLiveVariant()) {
            result.pendingSlots = pendingSlotCount();
            return result;
        }
        
        // Temporal evolution uses OpenCL-private history. Delivery slots may
        // already belong to Vulkan and are never borrowed as compute inputs.
        cl_mem prevBuffer = nullptr;
        cl_mem prevSpatial = nullptr;
        // Image arguments require an actual image object even when this is
        // the first generation. The nullable history buffer gates all image
        // reads in epi_evaluate_fluid_diffusion; binding the image does not
        // make uninitialized history available to that generation.
        cl_mem prevSpatialImage = temporalSpatialImage_;
        if (temporalHistoryValid_
            && canUseTemporalHistory(params, temporalHistoryFamily_)) {
            if (canUseSourceSampleHistory(params, temporalHistoryFamily_, temporalSourceScale_))
                prevBuffer = temporalSourceBuffers_[temporalSourceIndex_];
            if (temporalSpatialHistoryValid_)
                prevSpatial = temporalSpatialBuffer_;
            prevSpatialImage = temporalSpatialImage_;
            result.temporalHistoryUsed = prevBuffer != nullptr || prevSpatial != nullptr;
            if (result.temporalHistoryUsed)
                result.temporalSourceGeneration = temporalHistoryGeneration_;
        }

        for (std::size_t slotIndex = 0U; slotIndex < slots_.size(); ++slotIndex) {
            auto& slot = slots_[slotIndex];
            if (slot.state == SlotState::Free) {
                SculptureExternalSlotTicket ticket{};
                const bool externalSlot = usesExternalMemory(slot);
                const bool producesSpatial = params.outputMode == 1U || params.outputModeB == 1U;
                std::shared_ptr<std::vector<float>> hostSpatial;
                if (producesSpatial && !externalSlot) {
                    hostSpatial = acquireHostSpatial();
                    if (!hostSpatial) break;
                }
                if (externalSlot) {
                    std::uint64_t epoch = 0U;
                    if (externalOwnership == nullptr
                        || !externalOwnership->releaseFromVulkan(slotIndex, &epoch)
                        || !externalOwnership->acquireForOpenCl(
                            slotIndex, {generation, epoch})) {
                        continue;
                    }
                    ticket = {generation, epoch};
                }
                slot.hostSpatial = std::move(hostSpatial);
                slot.spatialProduced = producesSpatial;
                enqueue(slot, slotIndex, params, time, morph, deltaSeconds,
                        generation, prevBuffer, prevSpatial, prevSpatialImage, externalOwnership, ticket);
                result.submitted = true;
                break;
            }
        }
        result.pendingSlots = pendingSlotCount();
        return result;
    }

    // Observe completed native work without submitting a new scientific step.
    [[nodiscard]] SubmitResult pollAndTakeLatest(
        SculptureExternalSlotOwnership<>* externalOwnership) {
        std::lock_guard<std::mutex> lock(compileMutex_);
        try {
            auto result = collectCompleted(externalOwnership);
            result.pendingSlots = pendingSlotCount();
            return result;
        } catch (...) {
            quarantineBackgroundWork();
            throw;
        }
    }

    [[nodiscard]] WorldSubmitResult submitWorldAndTakeLatest(
        const SculptureProgramParams& params,
        const WorldRecipe& recipe,
        float time,
        float morph,
        float deltaSeconds,
        std::uint32_t generation) {
        if (worldDispatcher_) {
            return worldDispatcher_->submitWorldAndTakeLatest(params, recipe, time, morph, deltaSeconds, generation);
        }
        return {};
    }

    void acknowledgeWorldDelivery(
        const world_gpu_abi::WorldSnapshotGpu* snapshot) noexcept {
        if (worldDispatcher_) {
            worldDispatcher_->acknowledgeWorldDelivery(snapshot);
        }
    }

    [[nodiscard]] const std::string& name() const noexcept {
        return deviceName_;
    }

    [[nodiscard]] const std::string& worldDiagnostic() const noexcept {
        return worldDiagnostic_;
    }

    [[nodiscard]] bool worldKernelReady() const noexcept {
        return worldDispatcher_ != nullptr;
    }

    bool prewarmWorldKernel(WorldGeneratorKind generator) noexcept {
        return worldDispatcher_ && worldDispatcher_->prewarmWorldKernel(generator);
    }

    [[nodiscard]] bool isWorldKernelCompiled(WorldGeneratorKind generator) const noexcept {
        return worldDispatcher_ && worldDispatcher_->isWorldKernelCompiled(generator);
    }

    [[nodiscard]] std::size_t compiledWorldKernelCount() const noexcept {
        return worldDispatcher_ ? worldDispatcher_->compiledWorldKernelCount() : 0U;
    }

    
    bool importZeroCopyBuffers(std::span<const SculptureFrameProducer::ZeroCopyBuffer> buffers) {
        if (buffers.size() != kSlotCount) return false;
        // Slot metadata describes the transport that produced its completion.
        // Even after clFinish, Producing slots remain so until pollSlots runs;
        // changing their bindings would reinterpret staged data as external.
        // Leave those results for the ordinary submission/delivery path and
        // decline this optional transport rather than waiting or discarding.
        std::lock_guard<std::mutex> dispatchLock(compileMutex_);
        if (std::any_of(slots_.begin(), slots_.end(),
                        [](const Slot& slot) { return slot.state != SlotState::Free; })) {
            return false;
        }
        for (std::size_t i = 0; i < kSlotCount; ++i) {
            slots_[i].hostMapped = buffers[i].hostMapped;
            slots_[i].zeroCopyFd = buffers[i].exportFd;
            slots_[i].spatialFd = buffers[i].spatialFd;
            slots_[i].vulkanReleaseSemaphoreFd = buffers[i].vulkanReleaseSemaphoreFd;
            slots_[i].openClReleaseSemaphoreFd = buffers[i].openClReleaseSemaphoreFd;
        }
        if (!importExternalSemaphores() || !rebindExternalOutputBuffers()) {
            // An incomplete import must never leave a mapped Vulkan slot
            // discoverable through takeReadyForDelivery(). The local OpenCL
            // buffers remain the one coherent HostStaged fallback.
            if (!disableExternalBindings()) {
                throw std::runtime_error(
                    "could not restore local OpenCL buffers after external import failure");
            }
            return false;
        }
        // Private history has a transport-independent sample layout; changing
        // the delivery buffers does not create a mathematical reset.
        return true;
    }

    [[nodiscard]] std::uint32_t slotIndexOf(
        const gpu_abi::PackedSculptureSnapshot* snapshot) const noexcept {
        for (std::size_t i = 0; i < slots_.size(); ++i) {
            const auto* candidate = slots_[i].hostMapped
                ? reinterpret_cast<const gpu_abi::PackedSculptureSnapshot*>(slots_[i].hostMapped)
                : &slots_[i].snapshot;
            if (candidate == snapshot) return static_cast<std::uint32_t>(i);
        }
        return SculptureSnapshotDelivery::kNoSlot;
    }

    // Vulkan has queued its release semaphore before this is called.  Only
    // then may the OpenCL scheduler reuse a completed external slot.
    [[nodiscard]] bool recycleExternalSlot(
        std::uint32_t slotIndex,
        SculptureExternalSlotTicket ticket) noexcept {
        if (slotIndex >= slots_.size()) return false;
        auto& slot = slots_[slotIndex];
        const auto* snapshot = slot.hostMapped
            ? reinterpret_cast<const gpu_abi::PackedSculptureSnapshot*>(slot.hostMapped)
            : &slot.snapshot;
        if (!usesExternalMemory(slot) || slot.state != SlotState::Ready
            || snapshot->frame.generation != ticket.generation) {
            return false;
        }

        slot.state = SlotState::Free;
        return true;
    }

    bool rebindExternalOutputBuffers() {
        if (!context_) return false;
        bool allBound = true;
        
        for (auto& slot : slots_) {
            if (slot.zeroCopyFd < 0) continue;
            
            if (slot.outputBuffer) {
                clReleaseMemObject(slot.outputBuffer);
                slot.outputBuffer = nullptr;
            }

            cl_mem_properties properties[] = {
                CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR,
                (cl_mem_properties)slot.zeroCopyFd,
                0
            };
            
            cl_int result;
            slot.outputBuffer = clCreateBufferWithProperties(
                context_,
                properties,
                CL_MEM_WRITE_ONLY,
                sizeof(gpu_abi::PackedSculptureSnapshot),
                nullptr,
                &result);
                
            if (result != CL_SUCCESS) {
                std::fprintf(stderr, "[OpenCL] Failed to create external buffer from FD: %d\n", result);
                slot.outputBuffer = clCreateBuffer(
                    context_, CL_MEM_WRITE_ONLY,
                    sizeof(gpu_abi::PackedSculptureSnapshot),
                    nullptr, &result);
                checkOpenCl(result, "clCreateBuffer(output fallback)");
                slot.zeroCopyFd = -1;
                allBound = false;
            }
            
            checkOpenCl(clSetKernelArg(
                slot.kernel, 3U,
                sizeof(slot.outputBuffer), &slot.outputBuffer),
                "clSetKernelArg(output zero-copy)");
                
            if (slot.spatialFd >= 0) {
                if (slot.spatialBuffer) {
                    clReleaseMemObject(slot.spatialBuffer);
                    slot.spatialBuffer = nullptr;
                }
                cl_mem_properties spatialProps[] = {
                    CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR,
                    (cl_mem_properties)slot.spatialFd,
                    0
                };
                slot.spatialBuffer = clCreateBufferWithProperties(
                    context_, spatialProps, CL_MEM_READ_WRITE,
                    sizeof(cl_float4) * gpu_abi::kVolumeVoxelCount,
                    nullptr, &result);
                if (result != CL_SUCCESS) {
                    std::fprintf(stderr, "[OpenCL] Failed to create external spatial buffer from FD: %d\n", result);
                    slot.spatialBuffer = clCreateBuffer(
                        context_, CL_MEM_READ_WRITE,
                        sizeof(cl_float4) * gpu_abi::kVolumeVoxelCount,
                        nullptr, &result);
                    checkOpenCl(result, "clCreateBuffer(spatial fallback)");
                    slot.spatialFd = -1;
                    allBound = false;
                }
                
                checkOpenCl(clSetKernelArg(slot.macroKernel, 5U, sizeof(cl_mem), &slot.spatialBuffer), "clSetKernelArg(macro spatial_buffer zero-copy)");
                checkOpenCl(clSetKernelArg(slot.kernel, 7U, sizeof(cl_mem), &slot.spatialBuffer), "clSetKernelArg(spatial_buffer zero-copy)");
            }
                
            const cl_uint samplesOffset = slot.zeroCopyFd >= 0
                ? static_cast<cl_uint>(offsetof(gpu_abi::PackedSculptureSnapshot, samples)) : 0U;
            checkOpenCl(clSetKernelArg(
                slot.kernel, 5U,
                sizeof(samplesOffset), &samplesOffset),
                "clSetKernelArg(samples_offset zero-copy)");

        }
        return allBound;
    }

    void waitIdle() {
        if (worldDispatcher_) worldDispatcher_->waitIdle();
        checkOpenCl(clFinish(queue_), "clFinish(sculpture producer)");
    }

    [[nodiscard]] bool supportsExternalMemoryFd() const {
        std::size_t extensionBytes = 0U;
        if (clGetDeviceInfo(device_, CL_DEVICE_EXTENSIONS, 0U, nullptr,
                            &extensionBytes) != CL_SUCCESS
            || extensionBytes == 0U) {
            return false;
        }
        std::string extensions(extensionBytes, '\0');
        if (clGetDeviceInfo(device_, CL_DEVICE_EXTENSIONS, extensionBytes,
                            extensions.data(), nullptr) != CL_SUCCESS) {
            return false;
        }
        return extensions.find("cl_khr_external_memory") != std::string::npos;
    }

    [[nodiscard]] bool supportsExternalMemorySynchronization() const noexcept {
        return createSemaphore_ != nullptr && waitSemaphores_ != nullptr
            && signalSemaphores_ != nullptr && releaseSemaphore_ != nullptr;
    }

    [[nodiscard]] bool deviceUuidMatches(
        const std::array<std::uint8_t, 16>& required) const noexcept {
        cl_uchar uuid[16]{};
        std::size_t returned = 0U;
        return clGetDeviceInfo(device_, CL_DEVICE_UUID_KHR, sizeof(uuid), uuid, &returned)
                == CL_SUCCESS
            && returned == sizeof(uuid)
            && std::memcmp(uuid, required.data(), sizeof(uuid)) == 0;
    }

    // The caller retires both OpenCL and graphics work before this device
    // boundary. External completions still in the ring refer to the old
    // allocation; discard their delivery records before clearing its mapping.
    // Accepted simulation and private temporal history are not rewound.
    [[nodiscard]] bool releaseExternalPresentationBindings() noexcept {
        std::lock_guard<std::mutex> dispatchLock(compileMutex_);
        for (auto& slot : slots_) {
            if (!usesExternalMemory(slot)) continue;
            if (slot.state == SlotState::Producing) {
                // waitIdle has already completed the queue, but pollSlots may
                // not have released the events or changed the state to Ready.
                if (slot.firstEvent) clReleaseEvent(slot.firstEvent);
                if (slot.readEvent) clReleaseEvent(slot.readEvent);
                slot.firstEvent = nullptr;
                slot.readEvent = nullptr;
            }
            slot.state = SlotState::Free;
        }
        return disableExternalBindings();
    }

private:
    enum class SlotState : std::uint8_t {
        Free,
        Producing,
        Ready,
    };

    struct Slot {
        gpu_abi::SculptureProgramParamsGpu params{};
        gpu_abi::PackedSculptureSnapshot snapshot{};
        gpu_abi::SculptureAnalyticsGpu analytics{};
        cl_kernel kernel = nullptr;
        cl_kernel macroKernel = nullptr;
        cl_mem macroBlockBuffer = nullptr;
        cl_mem programBuffer = nullptr;
        cl_mem frameBuffer = nullptr;
        cl_mem outputBuffer = nullptr;
        cl_mem analyticsBuffer = nullptr;
        cl_mem spatialBuffer = nullptr;
        std::shared_ptr<std::vector<float>> hostSpatial;
        bool spatialProduced = false;
        cl_event firstEvent = nullptr;
        cl_event readEvent = nullptr;
        SlotState state = SlotState::Free;
        SculptureTemporalFamily temporalFamily = SculptureTemporalFamily::Surface;
        void* hostMapped = nullptr;
        int zeroCopyFd = -1;
        int spatialFd = -1;
        int vulkanReleaseSemaphoreFd = -1;
        int openClReleaseSemaphoreFd = -1;
        cl_semaphore_khr vulkanReleaseSemaphore = nullptr;
        cl_semaphore_khr openClReleaseSemaphore = nullptr;
        bool waitForVulkanRelease = false;
    };

    [[nodiscard]] std::shared_ptr<std::vector<float>> acquireHostSpatial() noexcept {
        for (auto& field : hostSpatialPool_) {
            if (!field) {
                try {
                    field = std::make_shared<std::vector<float>>(4U * gpu_abi::kVolumeVoxelCount);
                } catch (...) {
                    return {};
                }
            }
            if (field.use_count() == 1) return field;
        }
        return {};
    }

    void initializeWorldKernel(const std::string& path) {
        if (path.empty()) return;
        worldDispatcher_ = std::make_unique<SculptureWorldOpenClDispatcher>(context_, device_);
        worldDispatcher_->initializeKernel(path);
    }

    void releaseWorldResources() noexcept {
        worldDispatcher_.reset();
    }

    bool importExternalSemaphores() {
        if (!supportsExternalMemorySynchronization()) return false;
        for (auto& slot : slots_) {
            const cl_semaphore_properties_khr fromVulkan[] = {
                CL_SEMAPHORE_TYPE_KHR, CL_SEMAPHORE_TYPE_BINARY_KHR,
                CL_SEMAPHORE_HANDLE_OPAQUE_FD_KHR,
                static_cast<cl_semaphore_properties_khr>(slot.vulkanReleaseSemaphoreFd), 0};
            const cl_semaphore_properties_khr fromOpenCl[] = {
                CL_SEMAPHORE_TYPE_KHR, CL_SEMAPHORE_TYPE_BINARY_KHR,
                CL_SEMAPHORE_HANDLE_OPAQUE_FD_KHR,
                static_cast<cl_semaphore_properties_khr>(slot.openClReleaseSemaphoreFd), 0};
            cl_int result = CL_SUCCESS;
            slot.vulkanReleaseSemaphore = createSemaphore_(context_, fromVulkan, &result);
            if (result != CL_SUCCESS || !slot.vulkanReleaseSemaphore) return false;
            slot.openClReleaseSemaphore = createSemaphore_(context_, fromOpenCl, &result);
            if (result != CL_SUCCESS || !slot.openClReleaseSemaphore) return false;
        }
        return true;
    }

    bool disableExternalBindings() noexcept {
        for (auto& slot : slots_) {
            if (slot.vulkanReleaseSemaphore != nullptr) {
                releaseSemaphore_(slot.vulkanReleaseSemaphore);
                slot.vulkanReleaseSemaphore = nullptr;
            }
            if (slot.openClReleaseSemaphore != nullptr) {
                releaseSemaphore_(slot.openClReleaseSemaphore);
                slot.openClReleaseSemaphore = nullptr;
            }
            slot.hostMapped = nullptr;
            slot.zeroCopyFd = -1;
            slot.spatialFd = -1;
            slot.vulkanReleaseSemaphoreFd = -1;
            slot.openClReleaseSemaphoreFd = -1;
            slot.waitForVulkanRelease = false;
        }
        // A partial import may have succeeded for an earlier slot. Rebuild
        // every output pair locally so HostStaged never mixes ownership modes.
        for (auto& slot : slots_) {
            if (slot.outputBuffer != nullptr) clReleaseMemObject(slot.outputBuffer);
            if (slot.spatialBuffer != nullptr) clReleaseMemObject(slot.spatialBuffer);
            cl_int result = CL_SUCCESS;
            slot.outputBuffer = clCreateBuffer(
                context_, CL_MEM_WRITE_ONLY,
                sizeof(gpu_abi::SculptureSampleGpu) * kSampleCount,
                nullptr, &result);
            if (result != CL_SUCCESS || slot.outputBuffer == nullptr) return false;
            slot.spatialBuffer = clCreateBuffer(
                context_, CL_MEM_READ_WRITE,
                sizeof(cl_float4) * gpu_abi::kVolumeVoxelCount,
                nullptr, &result);
            if (result != CL_SUCCESS || slot.spatialBuffer == nullptr) return false;
            const cl_uint samplesOffset = 0U;
            if (clSetKernelArg(slot.kernel, 3U, sizeof(cl_mem), &slot.outputBuffer)
                    != CL_SUCCESS
                || clSetKernelArg(slot.kernel, 5U, sizeof(samplesOffset),
                                  &samplesOffset) != CL_SUCCESS
                || clSetKernelArg(slot.kernel, 7U, sizeof(cl_mem), &slot.spatialBuffer)
                    != CL_SUCCESS
                || clSetKernelArg(slot.macroKernel, 5U, sizeof(cl_mem),
                                  &slot.spatialBuffer) != CL_SUCCESS) {
                return false;
            }
        }
        return true;
    }

    void bindKernelArgs(Slot& slot) {
        // One internal signature gate also covers hot-reloaded candidates and
        // external imports. The compact publication ABI itself is unchanged.
        cl_uint argumentCount = 0U;
        checkOpenCl(clGetKernelInfo(slot.kernel, CL_KERNEL_NUM_ARGS,
                        sizeof(argumentCount), &argumentCount, nullptr),
                    "clGetKernelInfo(source history argument count)");
        if (argumentCount != 9U)
            throw std::runtime_error("generate_sculpture ABI mismatch: expected 9 arguments, got "
                                     + std::to_string(argumentCount));
        checkOpenCl(clSetKernelArg(slot.macroKernel, 0U, sizeof(cl_mem), &slot.programBuffer), "clSetKernelArg(macro program)");
        checkOpenCl(clSetKernelArg(slot.macroKernel, 1U, sizeof(cl_mem), &slot.frameBuffer), "clSetKernelArg(macro frame)");
        checkOpenCl(clSetKernelArg(slot.macroKernel, 2U, sizeof(cl_mem), &slot.macroBlockBuffer), "clSetKernelArg(macro active_blocks)");
        checkOpenCl(clSetKernelArg(slot.macroKernel, 5U, sizeof(cl_mem), &slot.spatialBuffer), "clSetKernelArg(macro spatial_buffer)");
        
        checkOpenCl(clSetKernelArg(slot.kernel, 4U, sizeof(cl_mem), &slot.macroBlockBuffer), "clSetKernelArg(active_blocks)");
        checkOpenCl(clSetKernelArg(slot.kernel, 7U, sizeof(cl_mem), &slot.spatialBuffer), "clSetKernelArg(spatial_buffer)");
        checkOpenCl(clSetKernelArg(slot.kernel, 0U, sizeof(cl_mem), &slot.programBuffer), "clSetKernelArg(program)");
        checkOpenCl(clSetKernelArg(slot.kernel, 1U, sizeof(cl_mem), &slot.frameBuffer), "clSetKernelArg(frame)");
        checkOpenCl(clSetKernelArg(slot.kernel, 2U, sizeof(cl_mem), &slot.analyticsBuffer), "clSetKernelArg(counter)");
        
        checkOpenCl(clSetKernelArg(slot.kernel, 3U, sizeof(cl_mem), &slot.outputBuffer),
                    "clSetKernelArg(output)");
        const cl_uint samplesOffset = slot.zeroCopyFd >= 0
            ? static_cast<cl_uint>(offsetof(gpu_abi::PackedSculptureSnapshot, samples)) : 0U;
        checkOpenCl(clSetKernelArg(slot.kernel, 5U, sizeof(samplesOffset), &samplesOffset),
                    "clSetKernelArg(samples_offset)");
    }


    static constexpr std::size_t kSlotCount = SculptureFrameProducer::kZeroCopySlotCount;
    static constexpr std::size_t kSourceHistoryBytes =
        sizeof(SculptureSourceSample) * kSculptureSourceHistoryCapacity;
    static_assert(kSculptureSourceHistoryCapacity == gpu_abi::kVolumeVoxelCount);
    static_assert(kSourceHistoryBytes == 4'000'000U);

    [[nodiscard]] static bool usesExternalMemory(const Slot& slot) noexcept {
        return slot.zeroCopyFd >= 0 && slot.outputBuffer != nullptr
            && slot.vulkanReleaseSemaphore != nullptr
            && slot.openClReleaseSemaphore != nullptr;
    }

    void completeDelivery(Slot& slot) {
        checkOpenCl(clEnqueueReadBuffer(
                        queue_, slot.analyticsBuffer, CL_FALSE, 0U,
                        sizeof(gpu_abi::SculptureAnalyticsGpu), &slot.analytics,
                        0U, nullptr, nullptr),
                    "clEnqueueReadBuffer(analytics)");

        if (!usesExternalMemory(slot)) {
            // Host-staged data is copied only after the completion marker.
            // Read 1D parameter beads (samples)
            checkOpenCl(clEnqueueReadBuffer(
                            queue_, slot.outputBuffer, CL_FALSE, 0U,
                            sizeof(slot.snapshot.samples), slot.snapshot.samples.data(),
                            0U, nullptr, nullptr),
                        "clEnqueueReadBuffer(samples)");
                        
            if (slot.spatialProduced && slot.hostSpatial) {
                checkOpenCl(clEnqueueReadBuffer(
                    queue_, slot.spatialBuffer, CL_FALSE, 0U,
                    sizeof(cl_float4) * gpu_abi::kVolumeVoxelCount,
                    slot.hostSpatial->data(), 0U, nullptr, nullptr),
                    "clEnqueueReadBuffer(spatial)");
            }
            checkOpenCl(clEnqueueMarkerWithWaitList(queue_, 0U, nullptr, &slot.readEvent), "clEnqueueMarker");
                        
            return;
        }

        // OpenCL owns the full external ABI record until this signal. The
        // Vulkan queue will wait on it before any render-side consumption.
        checkOpenCl(clEnqueueCopyBuffer(
                        queue_, slot.analyticsBuffer, slot.outputBuffer,
                        0U,
                        offsetof(gpu_abi::PackedSculptureSnapshot,
                                 activeInstanceCount),
                        sizeof(gpu_abi::SculptureAnalyticsGpu), 0U, nullptr, nullptr),
                    "clEnqueueCopyBuffer(external active instance count)");
        checkOpenCl(signalSemaphores_(queue_, 1U, &slot.openClReleaseSemaphore,
                                      nullptr, 0U, nullptr, &slot.readEvent),
                    "clEnqueueSignalSemaphoresKHR(opencl release)");
        slot.waitForVulkanRelease = true;
    }

    void captureTemporalHistory(const Slot& slot) {
        // Position/velocity history is written by source index directly into
        // the private candidate. Only spatial history needs a copy here.
        if (slot.spatialProduced) {
            checkOpenCl(clEnqueueCopyBuffer(
                            queue_, slot.spatialBuffer, temporalSpatialBuffer_,
                            0U, 0U, sizeof(cl_float4) * gpu_abi::kVolumeVoxelCount,
                            0U, nullptr, nullptr),
                        "clEnqueueCopyBuffer(temporal spatial)");
            const size_t origin[3] = {0, 0, 0};
            const size_t region[3] = {50, 50, 50}; // EPI_SCULPT_VOLUME_GRID_SIZE
            checkOpenCl(clEnqueueCopyBufferToImage(
                queue_, slot.spatialBuffer, temporalSpatialImage_,
                0, origin, region,
                0, nullptr, nullptr), "clEnqueueCopyBufferToImage");
        }
    }

    [[nodiscard]] static bool generationNewer(
        std::uint32_t candidate, std::uint32_t reference) noexcept {
        // Compact generations do not wrap within a producer session.
        return candidate > reference;
    }

    void enqueue(Slot& slot,
                 std::size_t slotIndex,
                 const SculptureProgramParams& params,
                 float time,
                 float morph,
                 float deltaSeconds,
                 std::uint32_t generation,
                 cl_mem prevOutputBuffer,
                 cl_mem prevSpatialBuffer,
                 cl_mem prevSpatialImage,
                 SculptureExternalSlotOwnership<>* externalOwnership,
                 SculptureExternalSlotTicket externalTicket) {
        if (usesExternalMemory(slot) && slot.waitForVulkanRelease) {
            checkOpenCl(waitSemaphores_(queue_, 1U, &slot.vulkanReleaseSemaphore,
                                        nullptr, 0U, nullptr, nullptr),
                        "clEnqueueWaitSemaphoresKHR(vulkan release)");
        }
        slot.params = gpu_abi::packProgramParams(params);
        slot.temporalFamily = temporalFamily(params);
        slot.snapshot = {};
        slot.snapshot.frame = gpu_abi::packFrame(
            time, morph, deltaSeconds, generation);
        slot.snapshot.activeInstanceCount = 0U;
        std::uint32_t effStageMask = slot.params.stageMask;
        if (params.outputMode == 1 || params.outputModeB == 1) effStageMask |= 32768U;
        slot.snapshot.reserved0 = (effStageMask << 8) | (slot.params.colorEffectId & 0xFF);
        slot.snapshot.reserved1 = 0U; // Will receive sumOrbitEnergy
        slot.snapshot.reserved2 = 0U; // Will receive sumMagnitude

        // The two private source buffers are ordered on this same queue. Clear
        // only the candidate: absent/culled sources must not inherit old values.
        const std::size_t nextSourceIndex = 1U - temporalSourceIndex_;
        const cl_mem nextSourceHistory = temporalSourceBuffers_[nextSourceIndex];
        const cl_uint zeroHistory = 0U;
        checkOpenCl(clEnqueueFillBuffer(queue_, nextSourceHistory,
                        &zeroHistory, sizeof(zeroHistory), 0U, kSourceHistoryBytes,
                        0U, nullptr, nullptr),
                    "clEnqueueFillBuffer(source history candidate)");

        checkOpenCl(clEnqueueWriteBuffer(
                        queue_, slot.programBuffer, CL_FALSE, 0U,
                        sizeof(slot.params), &slot.params,
                        0U, nullptr, &slot.firstEvent),
                    "clEnqueueWriteBuffer(program)");
        checkOpenCl(clEnqueueWriteBuffer(
                        queue_, slot.frameBuffer, CL_FALSE, 0U,
                        sizeof(slot.snapshot.frame), &slot.snapshot.frame,
                        0U, nullptr, nullptr),
                    "clEnqueueWriteBuffer(frame)");
        checkOpenCl(clEnqueueWriteBuffer(
                        queue_, slot.analyticsBuffer, CL_FALSE, 0U,
                        sizeof(gpu_abi::SculptureAnalyticsGpu), &slot.snapshot.activeInstanceCount,
                        0U, nullptr, nullptr),
                    "clEnqueueWriteBuffer(counter)");
        if (usesExternalMemory(slot)) {
            checkOpenCl(clEnqueueWriteBuffer(
                            queue_, slot.outputBuffer, CL_FALSE, 0U,
                            offsetof(gpu_abi::PackedSculptureSnapshot, samples),
                            &slot.snapshot, 0U, nullptr, nullptr),
                        "clEnqueueWriteBuffer(external snapshot header)");
        }

        if (params.outputMode == 1U || params.outputModeB == 1U) {
            const cl_uint samplesOffset = slot.zeroCopyFd >= 0
                ? static_cast<cl_uint>(offsetof(gpu_abi::PackedSculptureSnapshot, samples)) : 0U;
            checkOpenCl(clSetKernelArg(slot.macroKernel, 3U, sizeof(cl_uint), &samplesOffset), "clSetKernelArg(macro offset)");
            checkOpenCl(clSetKernelArg(
                            slot.macroKernel, 4U, sizeof(cl_mem),
                            &prevOutputBuffer),
                        "clSetKernelArg(macro prev)");
            checkOpenCl(clSetKernelArg(
                            slot.macroKernel, 6U, sizeof(cl_mem),
                            &prevSpatialBuffer),
                        "clSetKernelArg(macro prev_spatial)");
            checkOpenCl(clSetKernelArg(
                            slot.macroKernel, 7U, sizeof(cl_mem),
                            &prevSpatialImage),
                        "clSetKernelArg(macro prev_spatial_image)");
            
            const cl_uchar zero = 0;
            checkOpenCl(clEnqueueFillBuffer(queue_, slot.macroBlockBuffer, &zero, sizeof(zero), 0, gpu_abi::kVolumeMacroBlockCount, 0, nullptr, nullptr), "clEnqueueFillBuffer(macroBlock)");
            
            const std::size_t macroGlobalSize = gpu_abi::kVolumeVoxelCount;
            // enqueue() is called only by submitAndTakeLatest(), which owns
            // compileMutex_ for the complete bind/submit transaction.
            checkOpenCl(clEnqueueNDRangeKernel(
                            queue_, slot.macroKernel, 1U, nullptr, &macroGlobalSize, nullptr,
                            0U, nullptr, nullptr),
                        "clEnqueueNDRangeKernel(mark_macro_blocks)");
        }

        
        checkOpenCl(clSetKernelArg(
        
                        slot.kernel, 6U, sizeof(cl_mem), &prevOutputBuffer),
                    "clSetKernelArg(prev_output)");
        checkOpenCl(clSetKernelArg(slot.kernel, 8U, sizeof(cl_mem), &nextSourceHistory),
                    "clSetKernelArg(next source history)");

        std::size_t globalSize = (slot.params.outputMode == 1 && slot.params.outputModeB == 1) ? gpu_abi::kVolumeVoxelCount : kSampleCount;
        if (slot.params.auxiliaryResolutionScale > 1U) {
            const std::size_t scale = slot.params.auxiliaryResolutionScale;
            if (slot.params.outputMode == 1 && slot.params.outputModeB == 1) {
                globalSize = gpu_abi::kVolumeVoxelCount / (scale * scale * scale);
            } else {
                globalSize = kSampleCount / (scale * scale);
            }
        }
        checkOpenCl(clEnqueueNDRangeKernel(
                        queue_, slot.kernel, 1U, nullptr, &globalSize, nullptr,
                        0U, nullptr, nullptr),
                    "clEnqueueNDRangeKernel(generate_sculpture)");

                    
        
        // The private source candidate and spatial copies precede delivery.
        // No source-history read aliases the compacted/external output.
        captureTemporalHistory(slot);
        completeDelivery(slot);
        if (usesExternalMemory(slot)
            && (externalOwnership == nullptr
                || !externalOwnership->releaseFromOpenCl(slotIndex, externalTicket))) {
            throw std::runtime_error("external OpenCL delivery ownership transition failed");
        }
        
        slot.state = SlotState::Producing;
        checkOpenCl(clFlush(queue_), "clFlush(sculpture producer)");
        // Promote only after the full native request succeeds. Partial enqueue
        // failures retain the existing quarantine rule and cannot authorize a
        // later generation to consume an unproven candidate.
        temporalSourceIndex_ = nextSourceIndex;
        temporalSpatialHistoryValid_ = slot.spatialProduced;
        temporalHistoryFamily_ = slot.temporalFamily;
        temporalSourceScale_ = params.auxiliaryResolutionScale;
        temporalHistoryGeneration_ = generation;
        temporalHistoryValid_ = true;
    }

    // Caller holds compileMutex_; selection and slot release are shared by
    // submission and poll-only paths so each delivery follows the same rules.
    [[nodiscard]] SubmitResult collectCompleted(
        SculptureExternalSlotOwnership<>* externalOwnership) {
        pollSlots();

        SubmitResult result{};
        Slot* readySlot = takeReadyForDelivery(externalOwnership);
        if (readySlot != nullptr) {
            result.completed = usesExternalMemory(*readySlot)
                ? reinterpret_cast<gpu_abi::PackedSculptureSnapshot*>(readySlot->hostMapped)
                : &lastSnapshot_;
            result.analytics = usesExternalMemory(*readySlot)
                ? &readySlot->analytics
                : &lastAnalytics_;
            if (readySlot->spatialProduced) {
                if (usesExternalMemory(*readySlot)) {
                    if (readySlot->spatialFd >= 0 && readySlot->spatialBuffer)
                        result.spatialGeneration = readySlot->snapshot.frame.generation;
                } else if (readySlot->hostSpatial) {
                    result.spatialLease = readySlot->hostSpatial;
                    result.spatialGeneration = readySlot->snapshot.frame.generation;
                }
            }
            if (!usesExternalMemory(*readySlot)) readySlot->hostSpatial.reset();
        }
        result.completionMilliseconds = lastCompletionMilliseconds_;

        return result;
    }

    void pollSlots() {
        for (auto& slot : slots_) {
            if (slot.state != SlotState::Producing) {
                continue;
            }
            cl_int executionStatus = CL_QUEUED;
            checkOpenCl(clGetEventInfo(
                            slot.readEvent, CL_EVENT_COMMAND_EXECUTION_STATUS,
                            sizeof(executionStatus), &executionStatus, nullptr),
                        "clGetEventInfo(read status)");
            if (executionStatus < 0) {
                throw std::runtime_error(
                    "OpenCL sculpture read completed with error "
                    + std::to_string(executionStatus));
            }
            if (executionStatus != CL_COMPLETE) {
                continue;
            }

            if (!usesExternalMemory(slot)) {
                // The in-order marker follows the nonblocking analytics read.
                // Copy its completed count into the matching staged snapshot.
                slot.snapshot.activeInstanceCount = slot.analytics.activeInstanceCount;
            }

            cl_ulong beginNanoseconds = 0U;
            cl_ulong endNanoseconds = 0U;
            const cl_int beginResult = clGetEventProfilingInfo(
                slot.firstEvent, CL_PROFILING_COMMAND_START,
                sizeof(beginNanoseconds), &beginNanoseconds, nullptr);
            const cl_int endResult = clGetEventProfilingInfo(
                slot.readEvent, CL_PROFILING_COMMAND_END,
                sizeof(endNanoseconds), &endNanoseconds, nullptr);
            if (beginResult == CL_SUCCESS && endResult == CL_SUCCESS
                && endNanoseconds >= beginNanoseconds) {
                lastCompletionMilliseconds_ = static_cast<float>(
                    endNanoseconds - beginNanoseconds) * 1.0e-6f;
            }

            clReleaseEvent(slot.firstEvent);
            clReleaseEvent(slot.readEvent);
            slot.firstEvent = nullptr;
            slot.readEvent = nullptr;
            slot.state = SlotState::Ready;
        }
    }

    [[nodiscard]] Slot* takeReadyForDelivery(
        SculptureExternalSlotOwnership<>* externalOwnership) {
        Slot* selected = nullptr;
        const bool externalDelivery = std::any_of(
            slots_.begin(), slots_.end(),
            [](const Slot& slot) { return usesExternalMemory(slot); });

        for (std::size_t slotIndex = 0U; slotIndex < slots_.size(); ++slotIndex) {
            auto& slot = slots_[slotIndex];
            if (slot.state != SlotState::Ready) continue;
            if (usesExternalMemory(slot) != externalDelivery) continue;

            gpu_abi::PackedSculptureSnapshot* ptr = slot.hostMapped
                ? reinterpret_cast<gpu_abi::PackedSculptureSnapshot*>(slot.hostMapped)
                : &slot.snapshot;

            if (externalDelivery) {
                SculptureExternalSlotTicket ticket{};
                if (externalOwnership == nullptr
                    || !externalOwnership->readyTicket(slotIndex, &ticket)
                    || ticket.generation != ptr->frame.generation) {
                    continue;
                }
            }

            gpu_abi::PackedSculptureSnapshot* selectedPtr = selected
                ? (selected->hostMapped
                    ? reinterpret_cast<gpu_abi::PackedSculptureSnapshot*>(selected->hostMapped)
                    : &selected->snapshot)
                : nullptr;

            // Host-staged buffers are private and may discard obsolete work,
            // so select the newest. External releases are synchronization
            // obligations: deliver the oldest first so every signalled slot
            // completes its Vulkan round-trip before it can be reused.
            const bool shouldSelect = selected == nullptr
                || (externalDelivery
                    ? generationNewer(selectedPtr->frame.generation,
                                      ptr->frame.generation)
                    : generationNewer(ptr->frame.generation,
                                      selectedPtr->frame.generation));
            if (shouldSelect) {
                selected = &slot;
            }
        }

        if (selected == nullptr) return nullptr;

        if (!externalDelivery) {
            lastSnapshot_ = selected->snapshot;
            lastAnalytics_ = selected->analytics;
        }

        for (auto& slot : slots_) {
            if (!externalDelivery && slot.state == SlotState::Ready
                && &slot != selected) {
                slot.state = SlotState::Free;
                slot.hostSpatial.reset();
                slot.spatialProduced = false;
            }
        }

        // A staged result was copied into producer-owned storage and its slot
        // can be reused immediately. An external result still owns a native
        // release ticket; keep the slot Ready until recycleExternalSlot sees
        // the matching Vulkan handoff. Freeing it here makes that handoff fail
        // and can strand every external slot in quarantine.
        if (!externalDelivery) selected->state = SlotState::Free;
        return selected;
    }

    [[nodiscard]] std::uint32_t pendingSlotCount() const noexcept {
        std::uint32_t count = 0U;
        for (const auto& slot : slots_) {
            count += slot.state == SlotState::Producing ? 1U : 0U;
        }
        return count;
    }

public:
    bool triggerHotReload() {
        std::array<std::uint32_t, 6> active{};
        {
            std::lock_guard<std::mutex> lock(compileMutex_);
            active = {active_innerA_, active_innerB_, active_seedA_, active_seedB_, active_reduction_, active_colorEffect_};
        }
        return triggerAsyncRecompileRaw(
            active[0], active[1], active[2], active[3], active[4], active[5]);
    }

    [[nodiscard]] bool isCompiling() const noexcept {
        return isCompiling_.load();
    }

    [[nodiscard]] SculptureKernelBuildIdentity kernelBuildIdentity() const noexcept {
        std::lock_guard<std::mutex> lock(compileMutex_);
        return activeKernelBuild_;
    }

    void triggerAsyncRecompile(const SculptureProgramParams& params) {
        triggerAsyncRecompileRaw(
            static_cast<std::uint32_t>(params.inner),
            static_cast<std::uint32_t>(params.innerB),
            static_cast<std::uint32_t>(params.seedType),
            static_cast<std::uint32_t>(params.seedTypeB),
            static_cast<std::uint32_t>(params.reduction),
            static_cast<std::uint32_t>(params.colorEffect)
        );
    }
    
    struct CompileRequest {
        std::uint32_t innerA = 0U;
        std::uint32_t innerB = 0U;
        std::uint32_t seedA = 0U;
        std::uint32_t seedB = 0U;
        std::uint32_t reduction = 0U;
        std::uint32_t colorEffect = 0U;
        // This is a control-plane publication revision, not a GPU generation.
        // A completed request may publish only while it is still the newest
        // accepted request, preventing an obsolete source build from going live.
        std::uint64_t revision = 0U;
        std::string source;
        std::string prefix;
        std::string buildOptions;
        std::uint64_t sourceFingerprint = 0U;
    };

    [[nodiscard]] std::optional<CompileRequest> prepareCompileRequest(
        std::uint32_t innerA, std::uint32_t innerB, std::uint32_t seedA,
        std::uint32_t seedB, std::uint32_t reduction, std::uint32_t colorEffect) const noexcept {
        try {
        SculptureProgramParams params{};
        params.inner = static_cast<epi_sculpt::InnerMap>(innerA);
        params.innerB = static_cast<epi_sculpt::InnerMap>(innerB);
        params.seedType = static_cast<epi_sculpt::SeedType>(seedA);
        params.seedTypeB = static_cast<epi_sculpt::SeedType>(seedB);
        params.reduction = static_cast<epi_sculpt::ReductionMode>(reduction);
        params.colorEffect = static_cast<epi_sculpt::ColorEffectType>(colorEffect);
        const std::filesystem::path kernelFile(kernelPath_);
        const auto parent = kernelFile.parent_path();
        const auto bundle = SculptureCompactSourceBundle::prepare(kernelFile);
        if (!bundle) return std::nullopt;
        const auto build = SculptureCompactBuild::describe(params, parent, false);
        return CompileRequest{innerA, innerB, seedA, seedB, reduction, colorEffect,
            0U, bundle->source, build.prefix, build.options,
            SculptureProgramBinaryCache::computeSourceHash(
                bundle->source, build.options, build.prefix)};
        } catch (...) {
            return std::nullopt;
        }
    }

    bool triggerAsyncRecompileRaw(std::uint32_t req_innerA,
                                  std::uint32_t req_innerB,
                                  std::uint32_t req_seedA,
                                  std::uint32_t req_seedB,
                                  std::uint32_t req_reduction = 0U,
                                  std::uint32_t req_colorEffect = 0U) {
        // Preparation reads source files and builds a cache key. Avoid that
        // work once cancellation is visible, but retain the locked check below:
        // cancellation can still arrive while preparation is in progress.
        if (stopCompiler_.load(std::memory_order_acquire)) return false;
        auto prepared = prepareCompileRequest(
            req_innerA, req_innerB, req_seedA, req_seedB, req_reduction, req_colorEffect);
        if (!prepared) return false;
        {
            // Publication and request admission share the live-program mutex.
            // If compilation is already ready to publish, this makes the order
            // explicit: the prior program publishes before this request, or the
            // candidate sees this newer revision and is discarded.
            std::lock_guard<std::mutex> liveLock(compileMutex_);
            std::lock_guard<std::mutex> requestLock(compilerRequestMutex_);
            if (stopCompiler_.load(std::memory_order_acquire)) return false;
            // Latest request wins. One active build may finish, then the worker
            // performs at most one coalesced follow-up with current parameters.
            const std::uint64_t revision = latestCompileRequestRevision_.fetch_add(
                1U, std::memory_order_acq_rel) + 1U;
            prepared->revision = revision;
            pendingCompile_ = std::move(*prepared);
            isCompiling_.store(true, std::memory_order_release);
        }
        compilerRequestCv_.notify_one();
        return true;
    }

    void startPrewarming(const SculpturePresetLibrary& library) {
        if (!binaryPrewarmer_) {
            binaryPrewarmer_ = std::make_unique<SculptureProgramBinaryCache::Prewarmer>();
        }
        binaryPrewarmer_->start(context_, device_, library, kernelPath_);
    }

    void stopPrewarming() noexcept {
        if (binaryPrewarmer_) {
            binaryPrewarmer_->stop();
        }
    }

    [[nodiscard]] bool isPrewarming() const noexcept {
        return binaryPrewarmer_ && binaryPrewarmer_->isRunning();
    }

    using ProgramCacheKey = SculptureProgramVariantKey;
    using ProgramCacheKeyHash = SculptureProgramVariantKeyHash;

    struct CachedProgramRecord {
        cl_program program = nullptr;
        std::size_t hitCount = 0U;
        std::chrono::steady_clock::time_point lastUsed{};
    };

    // Keep recent preset/component revisions without retaining every program
    // produced during a long editing session. This bounds entries, not driver
    // bytes; the live program and candidate own separate OpenCL references.
    static constexpr std::size_t kMaxCachedPrograms = 32U;

    [[nodiscard]] cl_program retainCachedProgram(const ProgramCacheKey& key) noexcept {
        try {
            std::lock_guard<std::mutex> lock(programCacheMutex_);
            const auto it = programCache_.find(key);
            if (it == programCache_.end() || it->second.program == nullptr) return nullptr;
            // Never hand cleanup a borrowed cache handle if retain fails.
            if (clRetainProgram(it->second.program) != CL_SUCCESS) return nullptr;
            ++it->second.hitCount;
            it->second.lastUsed = std::chrono::steady_clock::now();
            programCacheHits_.fetch_add(1, std::memory_order_relaxed);
            return it->second.program;
        } catch (...) {
            return nullptr; // Optional cache failure falls through to compilation.
        }
    }

    void rememberProgram(const ProgramCacheKey& key, cl_program program) noexcept {
        if (program == nullptr) return;
        try {
            std::lock_guard<std::mutex> lock(programCacheMutex_);
            // Allocate/copy the key before retaining a native reference. If map
            // allocation fails, the caller still owns its valid compiled program.
            const auto [entry, inserted] = programCache_.try_emplace(key);
            if (!inserted) return;
            if (clRetainProgram(program) != CL_SUCCESS) {
                programCache_.erase(entry);
                return;
            }
            entry->second = CachedProgramRecord{
                program, 1U, std::chrono::steady_clock::now()};

            while (programCache_.size() > kMaxCachedPrograms) {
                const auto oldest = std::min_element(programCache_.begin(), programCache_.end(),
                    [](const auto& lhs, const auto& rhs) {
                        return lhs.second.lastUsed < rhs.second.lastUsed;
                    });
                // Release only the cache's reference. Existing kernels and the
                // live/candidate program remain owned by their original holders.
                if (oldest->second.program != nullptr) clReleaseProgram(oldest->second.program);
                programCache_.erase(oldest);
            }
        } catch (...) {
            // Persistence/reuse is optional; never discard a successful build
            // solely because this cache could not allocate its bookkeeping.
        }
    }

    void compileKernelSet(const CompileRequest& request) {
        cl_program newProgram = nullptr;
        std::array<cl_kernel, kZeroCopySlotCount> newGenerate{};
        std::array<cl_kernel, kZeroCopySlotCount> newMacro{};
        auto releaseNewSet = [&]() noexcept {
            for (cl_kernel kernel : newGenerate) {
                if (kernel != nullptr) clReleaseKernel(kernel);
            }
            for (cl_kernel kernel : newMacro) {
                if (kernel != nullptr) clReleaseKernel(kernel);
            }
            if (newProgram != nullptr) clReleaseProgram(newProgram);
        };

        try {
            const ProgramCacheKey cacheKey{
                request.innerA, request.innerB, request.seedA,
                request.seedB, request.reduction, request.colorEffect,
                request.prefix, request.source, request.buildOptions, request.sourceFingerprint
            };

            newProgram = retainCachedProgram(cacheKey);
            const bool fromCache = newProgram != nullptr;

            if (!fromCache) {
                programCacheMisses_.fetch_add(1, std::memory_order_relaxed);
                cl_int result = CL_SUCCESS;
                newProgram = SculptureProgramBinaryCache::buildOrLoadProgram(
                    context_, device_, request.source, request.buildOptions,
                    SculptureCompactBuild::kernelTag,
                    SculptureProgramBinaryCache::kDefaultCacheDir, request.prefix, &result, {});

                if (result != CL_SUCCESS || newProgram == nullptr) {
                    std::size_t logBytes = 0U;
                    if (newProgram) {
                        static_cast<void>(clGetProgramBuildInfo(
                            newProgram, device_, CL_PROGRAM_BUILD_LOG, 0U, nullptr, &logBytes));
                        std::vector<char> log(logBytes + 1U, '\0');
                        static_cast<void>(clGetProgramBuildInfo(
                            newProgram, device_, CL_PROGRAM_BUILD_LOG,
                            logBytes, log.data(), nullptr));
                        std::fprintf(stderr,
                            "[OpenCL Hot-Reload Error] Kernel build failed (err %d):\n%s\n",
                            result, log.data());
                    } else {
                        std::fprintf(stderr,
                            "[OpenCL Hot-Reload Error] Kernel creation failed (err %d)\n", result);
                    }
                    releaseNewSet();
                    return;
                }

                rememberProgram(cacheKey, newProgram);
            }

            cl_int result = CL_SUCCESS;
            for (std::size_t i = 0; i < kZeroCopySlotCount; ++i) {
                newGenerate[i] = clCreateKernel(
                    newProgram, "generate_sculpture", &result);
                checkOpenCl(result, "clCreateKernel(generate_sculpture hot reload)");
                newMacro[i] = clCreateKernel(
                    newProgram, "mark_macro_blocks", &result);
                checkOpenCl(result, "clCreateKernel(mark_macro_blocks hot reload)");
            }

            // Shutdown may begin while clBuildProgram is inside the driver.
            // In that case discard the candidate and let release() join us
            // before touching any live OpenCL resource.
            if (stopCompiler_.load(std::memory_order_acquire)) {
                releaseNewSet();
                return;
            }

            {
                std::lock_guard<std::mutex> lock(compileMutex_);
                if (stopCompiler_.load(std::memory_order_acquire)) {
                    releaseNewSet();
                    return;
                }
                if (request.revision != latestCompileRequestRevision_.load(
                        std::memory_order_acquire)) {
                    releaseNewSet();
                    std::fprintf(stderr,
                        "[OpenCL Hot-Reload] Discarded superseded kernel build revision %llu.\n",
                        static_cast<unsigned long long>(request.revision));
                    return;
                }
                // Bind every candidate before replacing any live handle. A
                // failed bind therefore leaves the complete prior set active.
                for (std::size_t i = 0; i < kZeroCopySlotCount; ++i) {
                    Slot candidate = slots_[i];
                    candidate.kernel = newGenerate[i];
                    candidate.macroKernel = newMacro[i];
                    bindKernelArgs(candidate);
                }
                for (std::size_t i = 0; i < kZeroCopySlotCount; ++i) {
                    if (slots_[i].kernel != nullptr) clReleaseKernel(slots_[i].kernel);
                    if (slots_[i].macroKernel != nullptr) clReleaseKernel(slots_[i].macroKernel);
                    slots_[i].kernel = newGenerate[i];
                    slots_[i].macroKernel = newMacro[i];
                    newGenerate[i] = nullptr;
                    newMacro[i] = nullptr;
                }
                if (program_ != nullptr) clReleaseProgram(program_);
                program_ = newProgram;
                newProgram = nullptr;
                active_innerA_ = request.innerA;
                active_innerB_ = request.innerB;
                active_seedA_ = request.seedA;
                active_seedB_ = request.seedB;
                active_reduction_ = request.reduction;
                active_colorEffect_ = request.colorEffect;
                activeKernelBuild_ = SculptureKernelBuildIdentity{
                    request.revision, request.sourceFingerprint, true};
            }
            std::fprintf(stderr,
                "[OpenCL Hot-Reload] Kernels compiled and live-swapped (cache size: %zu).\n",
                programCacheSize());
        } catch (const std::exception& error) {
            releaseNewSet();
            std::fprintf(stderr, "[OpenCL Hot-Reload Exception] %s\n", error.what());
        } catch (...) {
            releaseNewSet();
        }
    }

    void compilerLoop() noexcept {
        for (;;) {
            CompileRequest request{};
            {
                std::unique_lock<std::mutex> lock(compilerRequestMutex_);
                compilerRequestCv_.wait(lock, [this] {
                    return stopCompiler_.load(std::memory_order_acquire)
                        || pendingCompile_.has_value();
                });
                if (stopCompiler_.load(std::memory_order_acquire)) {
                    pendingCompile_.reset();
                    isCompiling_.store(false, std::memory_order_release);
                    return;
                }
                request = std::move(*pendingCompile_);
                pendingCompile_.reset();
            }

            compileKernelSet(request);

            std::lock_guard<std::mutex> lock(compilerRequestMutex_);
            if (stopCompiler_.load(std::memory_order_acquire)) {
                pendingCompile_.reset();
                isCompiling_.store(false, std::memory_order_release);
                return;
            }
            if (!pendingCompile_.has_value()) {
                isCompiling_.store(false, std::memory_order_release);
            }
        }
    }

    void initialize(const SculptureFrameProducer::Config& config) {
        if (config.kernelPath.empty()) {
            throw std::runtime_error("OpenCL sculpture kernel path is empty");
        }

        const SelectedDevice selected = selectDevice(config);
        platform_ = selected.platform;
        device_ = selected.device;
        deviceName_ = deviceName(device_);
        kernelPath_ = config.kernelPath;
        sourceWatch_.initialize(SculptureKernelSourceWatch::scan(kernelPath_));

        cl_int result = CL_SUCCESS;
        const cl_context_properties contextProperties[] = {
            CL_CONTEXT_PLATFORM,
            reinterpret_cast<cl_context_properties>(selected.platform),
            0,
        };
        context_ = clCreateContext(
            contextProperties, 1U, &device_, nullptr, nullptr, &result);
        checkOpenCl(result, "clCreateContext");

        // Educational Annotation: Modern OpenCL 2.0+ Command Queue
        // We use clCreateCommandQueueWithProperties to avoid deprecation warnings 
        // and ensure the driver uses the most up-to-date execution pipeline.
        const cl_queue_properties queueProperties[] = {
            CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0
        };
        queue_ = clCreateCommandQueueWithProperties(
            context_, device_, queueProperties, &result);
        checkOpenCl(result, "clCreateCommandQueueWithProperties");

        createSemaphore_ = reinterpret_cast<clCreateSemaphoreWithPropertiesKHR_fn>(
            clGetExtensionFunctionAddressForPlatform(
                platform_, "clCreateSemaphoreWithPropertiesKHR"));
        waitSemaphores_ = reinterpret_cast<clEnqueueWaitSemaphoresKHR_fn>(
            clGetExtensionFunctionAddressForPlatform(
                platform_, "clEnqueueWaitSemaphoresKHR"));
        signalSemaphores_ = reinterpret_cast<clEnqueueSignalSemaphoresKHR_fn>(
            clGetExtensionFunctionAddressForPlatform(
                platform_, "clEnqueueSignalSemaphoresKHR"));
        releaseSemaphore_ = reinterpret_cast<clReleaseSemaphoreKHR_fn>(
            clGetExtensionFunctionAddressForPlatform(
                platform_, "clReleaseSemaphoreKHR"));

        // Educational Annotation: Modular OpenCL Compilation
        // The compact bundle freezes the allowed quoted-header closure before
        // OpenCL sees it. The compiler therefore receives explicit source
        // strings rather than reopening mutable project headers through -I.
        const std::filesystem::path kernelFile(config.kernelPath);
        const auto parent = kernelFile.parent_path();
        const auto bundle = SculptureCompactSourceBundle::prepare(kernelFile);
        if (!bundle) throw std::runtime_error("OpenCL compact source bundle preparation failed");
        const auto& source = bundle->source;
        const SculptureProgramParams initialParams{};
        const auto build = SculptureCompactBuild::describe(initialParams, parent, false);
        const auto& prefix = build.prefix;
        const auto& buildOptions = build.options;
        const auto sourceFingerprint = SculptureProgramBinaryCache::computeSourceHash(
            source, buildOptions, prefix);

        program_ = SculptureProgramBinaryCache::buildOrLoadProgram(
            context_, device_, source, buildOptions, SculptureCompactBuild::kernelTag,
            SculptureProgramBinaryCache::kDefaultCacheDir, prefix, &result, {});

        if (result != CL_SUCCESS || program_ == nullptr) {
            std::size_t logBytes = 0U;
            if (program_) {
                static_cast<void>(clGetProgramBuildInfo(
                    program_, device_, CL_PROGRAM_BUILD_LOG, 0U, nullptr, &logBytes));
                std::vector<char> log(logBytes + 1U, '\0');
                static_cast<void>(clGetProgramBuildInfo(
                    program_, device_, CL_PROGRAM_BUILD_LOG,
                    logBytes, log.data(), nullptr));
                throw std::runtime_error(
                    "OpenCL sculpture kernel build failed:\n" + std::string(log.data()));
            } else {
                throw std::runtime_error(
                    "OpenCL sculpture kernel creation failed with error code: " + std::to_string(result));
            }
        }
        activeKernelBuild_ = SculptureKernelBuildIdentity{
            0U, sourceFingerprint, true};
        active_innerA_ = static_cast<std::uint32_t>(initialParams.inner);
        active_innerB_ = static_cast<std::uint32_t>(initialParams.innerB);
        active_seedA_ = static_cast<std::uint32_t>(initialParams.seedType);
        active_seedB_ = static_cast<std::uint32_t>(initialParams.seedTypeB);
        active_reduction_ = static_cast<std::uint32_t>(initialParams.reduction);
        active_colorEffect_ = static_cast<std::uint32_t>(initialParams.colorEffect);

        // History is intentionally separate from the delivery ring. This
        // keeps temporal feedback under one API's ownership even when every
        // output slot is imported by Vulkan.
        for (auto& history : temporalSourceBuffers_) {
            history = clCreateBuffer(context_, CL_MEM_READ_WRITE,
                kSourceHistoryBytes, nullptr, &result);
            checkOpenCl(result, "clCreateBuffer(source history)");
        }
        temporalSpatialBuffer_ = clCreateBuffer(
            context_, CL_MEM_READ_WRITE,
            sizeof(cl_float4) * gpu_abi::kVolumeVoxelCount, nullptr, &result);
        checkOpenCl(result, "clCreateBuffer(temporal spatial)");

        cl_image_format format{};
        format.image_channel_order = CL_RGBA;
        format.image_channel_data_type = CL_FLOAT;
        cl_image_desc desc{};
        desc.image_type = CL_MEM_OBJECT_IMAGE3D;
        desc.image_width = 50; // EPI_SCULPT_VOLUME_GRID_SIZE
        desc.image_height = 50;
        desc.image_depth = 50;
        temporalSpatialImage_ = clCreateImage(context_, CL_MEM_READ_ONLY, &format, &desc, nullptr, &result);
        checkOpenCl(result, "clCreateImage(temporal spatial image)");

        for (auto& slot : slots_) {
            slot.kernel = clCreateKernel(
                program_, "generate_sculpture", &result);
            checkOpenCl(result, "clCreateKernel(generate_sculpture)");


            
            slot.macroKernel = clCreateKernel(
                program_, "mark_macro_blocks", &result);
            checkOpenCl(result, "clCreateKernel(mark_macro_blocks)");

            slot.macroBlockBuffer = clCreateBuffer(
                context_, CL_MEM_READ_WRITE,
                gpu_abi::kVolumeMacroBlockCount, nullptr, &result);
            checkOpenCl(result, "clCreateBuffer(macroBlock)");

            slot.programBuffer = clCreateBuffer(
                context_, CL_MEM_READ_ONLY,
                gpu_abi::kProgramParamsBytes, nullptr, &result);
            checkOpenCl(result, "clCreateBuffer(program)");
            slot.frameBuffer = clCreateBuffer(
                context_, CL_MEM_READ_ONLY,
                gpu_abi::kFrameBytes, nullptr, &result);
            checkOpenCl(result, "clCreateBuffer(frame)");
            slot.outputBuffer = clCreateBuffer(
                context_, CL_MEM_WRITE_ONLY,
                sizeof(gpu_abi::SculptureSampleGpu) * kSampleCount,
                nullptr, &result);
            checkOpenCl(result, "clCreateBuffer(output)");
            
            slot.spatialBuffer = clCreateBuffer(
                context_, CL_MEM_READ_WRITE,
                sizeof(cl_float4) * gpu_abi::kVolumeVoxelCount,
                nullptr, &result);
            checkOpenCl(result, "clCreateBuffer(spatial)");
            float zeroFloat = 0.0f;
            checkOpenCl(clEnqueueFillBuffer(queue_, slot.spatialBuffer, &zeroFloat, sizeof(zeroFloat), 0, sizeof(cl_float4) * gpu_abi::kVolumeVoxelCount, 0, nullptr, nullptr), "clEnqueueFillBuffer(spatial)");
            
            slot.analyticsBuffer = clCreateBuffer(
                context_, CL_MEM_READ_WRITE,
                sizeof(gpu_abi::SculptureAnalyticsGpu),
                nullptr, &result);
            checkOpenCl(result, "clCreateBuffer(counter)");

            bindKernelArgs(slot);
        }

        // The world lane is optional.  Its compilation failure must not take
        // down the proven compact path; a recipe simply remains unavailable
        // until its dedicated kernel can be loaded correctly.
        if (!config.worldKernelPath.empty()) {
            try {
                initializeWorldKernel(config.worldKernelPath);
                worldDiagnostic_ = "dedicated world kernel ready";
            } catch (const std::exception& error) {
                releaseWorldResources();
                worldDiagnostic_ = std::string("dedicated world kernel unavailable: ")
                    + error.what();
            } catch (...) {
                releaseWorldResources();
                worldDiagnostic_ = "dedicated world kernel unavailable: unknown error";
            }
        }

        compilerThread_ = std::thread([this]() { compilerLoop(); });
        watcherThread_ = std::thread([this]() {
            while (!stopWatcher_.load(std::memory_order_relaxed)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                if (stopWatcher_.load(std::memory_order_relaxed)) break;
                
                try {
                    sourceWatch_.poll(
                        [this] { return SculptureKernelSourceWatch::scan(kernelPath_); },
                        [this] {
                            std::array<std::uint32_t, 6> active{};
                            {
                                std::lock_guard<std::mutex> lock(compileMutex_);
                                active = {active_innerA_, active_innerB_, active_seedA_,
                                          active_seedB_, active_reduction_, active_colorEffect_};
                            }
                            if (active[0] == 0xFFFFFFFF) return false;
                            return triggerAsyncRecompileRaw(
                                active[0], active[1], active[2], active[3], active[4], active[5]);
                        });
                } catch(...) {}
            }
        });
    }


    bool importGridExternalOutput(std::uint32_t slotIndex, const SculptureOpenClImportSlot& importSlot) {
        if (!gridDispatcher_) return false;
        return gridDispatcher_->importExternalOutput(slotIndex, importSlot);
    }

    bool submitGridTransform(const SculptureProgramParams& params, float time, float morph, std::uint32_t gridResolution, std::uint32_t slotIndex) {
        if (!gridDispatcher_) return false;
        return gridDispatcher_->submitGridTransform(params, time, morph, gridResolution, slotIndex);
    }

    void release() noexcept {
        stopWatcher_.store(true, std::memory_order_relaxed);
        if (watcherThread_.joinable()) {
            watcherThread_.join();
        }
        if (binaryPrewarmer_) {
            binaryPrewarmer_->stop();
        }
        stopCompiler_.store(true, std::memory_order_release);
        {
            std::lock_guard<std::mutex> lock(compilerRequestMutex_);
            pendingCompile_.reset();
        }
        compilerRequestCv_.notify_one();
        if (compilerThread_.joinable()) {
            compilerThread_.join();
        }
        isCompiling_.store(false, std::memory_order_release);
        if (queue_ != nullptr) {
            static_cast<void>(clFinish(queue_));
        }
        for (auto& slot : slots_) {
            if (slot.firstEvent != nullptr) clReleaseEvent(slot.firstEvent);
            if (slot.readEvent != nullptr) clReleaseEvent(slot.readEvent);
            if (slot.vulkanReleaseSemaphore != nullptr && releaseSemaphore_ != nullptr) {
                releaseSemaphore_(slot.vulkanReleaseSemaphore);
            }
            if (slot.openClReleaseSemaphore != nullptr && releaseSemaphore_ != nullptr) {
                releaseSemaphore_(slot.openClReleaseSemaphore);
            }
            if (slot.macroBlockBuffer != nullptr) clReleaseMemObject(slot.macroBlockBuffer);
            if (slot.macroKernel != nullptr) clReleaseKernel(slot.macroKernel);
            if (slot.outputBuffer != nullptr) clReleaseMemObject(slot.outputBuffer);
            if (slot.analyticsBuffer != nullptr) clReleaseMemObject(slot.analyticsBuffer);
            if (slot.spatialBuffer != nullptr) clReleaseMemObject(slot.spatialBuffer);
            if (slot.frameBuffer != nullptr) clReleaseMemObject(slot.frameBuffer);
            if (slot.programBuffer != nullptr) clReleaseMemObject(slot.programBuffer);
            if (slot.kernel != nullptr) clReleaseKernel(slot.kernel);
            // Slot now owns an in-flight host lease; destroy it normally.
            slot = Slot{};
        }
        releaseWorldResources();
        for (auto& history : temporalSourceBuffers_) {
            if (history != nullptr) clReleaseMemObject(history);
            history = nullptr;
        }
        if (temporalSpatialBuffer_ != nullptr) {
            clReleaseMemObject(temporalSpatialBuffer_);
        }
        if (temporalSpatialImage_ != nullptr) {
            clReleaseMemObject(temporalSpatialImage_);
        }
        {
                                std::lock_guard<std::mutex> cacheLock(programCacheMutex_);
            for (auto& [key, entry] : programCache_) {
                if (entry.program != nullptr) {
                    clReleaseProgram(entry.program);
                    entry.program = nullptr;
                }
            }
            programCache_.clear();
        }
        if (program_ != nullptr) clReleaseProgram(program_);
        if (queue_ != nullptr) clReleaseCommandQueue(queue_);
        if (context_ != nullptr) clReleaseContext(context_);
        program_ = nullptr;
        temporalSourceIndex_ = 0U;
        temporalSpatialBuffer_ = nullptr;
        temporalSpatialImage_ = nullptr;
        temporalHistoryValid_ = false;
        temporalSpatialHistoryValid_ = false;
        queue_ = nullptr;
        context_ = nullptr;
    }

    [[nodiscard]] std::size_t programCacheSize() const noexcept {
        std::lock_guard<std::mutex> lock(programCacheMutex_);
        return programCache_.size();
    }

    [[nodiscard]] std::size_t programCacheHits() const noexcept {
        return programCacheHits_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] std::size_t programCacheMisses() const noexcept {
        return programCacheMisses_.load(std::memory_order_relaxed);
    }

    cl_platform_id platform_ = nullptr;
    cl_device_id device_ = nullptr;
    cl_context context_ = nullptr;
    cl_command_queue queue_ = nullptr;
    cl_program program_ = nullptr;
    std::unique_ptr<SculptureWorldOpenClDispatcher> worldDispatcher_;
    std::unique_ptr<SculptureGridOpenClDispatcher> gridDispatcher_;
    // Two fixed 4MB private records; never exported or allocated per frame.
    std::array<cl_mem, 2> temporalSourceBuffers_{};
    std::size_t temporalSourceIndex_ = 0U;
    cl_mem temporalSpatialBuffer_ = nullptr;
    cl_mem temporalSpatialImage_ = nullptr;
    SculptureTemporalFamily temporalHistoryFamily_ = SculptureTemporalFamily::Surface;
    StageResolutionScale temporalSourceScale_ = StageResolutionScale::Full;
    std::uint32_t temporalHistoryGeneration_ = 0U;
    bool temporalHistoryValid_ = false;
    bool temporalSpatialHistoryValid_ = false;
    std::array<Slot, kSlotCount> slots_{};
    std::array<std::shared_ptr<std::vector<float>>, 4U> hostSpatialPool_{};
    std::string deviceName_;
    std::string worldDiagnostic_ = "dedicated world kernel not configured";
    std::string kernelPath_;
    std::string gridKernelPath_;
    std::uint32_t active_innerA_ = 0xFFFFFFFF;
    std::uint32_t active_innerB_ = 0xFFFFFFFF;
    std::uint32_t active_seedA_ = 0xFFFFFFFF;
    std::uint32_t active_seedB_ = 0xFFFFFFFF;
    std::uint32_t active_reduction_ = 0xFFFFFFFF;
    std::uint32_t active_colorEffect_ = 0xFFFFFFFF;
    std::atomic<bool> stopWatcher_{false};
    std::thread watcherThread_;
    std::atomic<bool> stopCompiler_{false};
    std::thread compilerThread_;
    std::mutex compilerRequestMutex_;
    std::condition_variable compilerRequestCv_;
    std::optional<CompileRequest> pendingCompile_;
    SculptureKernelSourceWatch sourceWatch_;
    std::atomic<bool> isCompiling_{false};
    mutable std::mutex compileMutex_;
    SculptureKernelBuildIdentity activeKernelBuild_{};
    std::atomic<std::uint64_t> latestCompileRequestRevision_{0U};
    mutable std::mutex programCacheMutex_;
    std::unordered_map<ProgramCacheKey, CachedProgramRecord, ProgramCacheKeyHash> programCache_;
    std::unique_ptr<SculptureProgramBinaryCache::Prewarmer> binaryPrewarmer_;
    std::atomic<std::size_t> programCacheHits_{0};
    std::atomic<std::size_t> programCacheMisses_{0};
    float lastCompletionMilliseconds_ = 0.0f;
    gpu_abi::PackedSculptureSnapshot lastSnapshot_{};
    gpu_abi::SculptureAnalyticsGpu lastAnalytics_{};
    clCreateSemaphoreWithPropertiesKHR_fn createSemaphore_ = nullptr;
    clEnqueueWaitSemaphoresKHR_fn waitSemaphores_ = nullptr;
    clEnqueueSignalSemaphoresKHR_fn signalSemaphores_ = nullptr;
    clReleaseSemaphoreKHR_fn releaseSemaphore_ = nullptr;
#else
    struct SubmitResult {};

    explicit OpenClState(const SculptureFrameProducer::Config&) {
        throw std::runtime_error("OpenCL support was not available at build time");
    }

    bool importZeroCopyBuffers(
        std::span<const SculptureFrameProducer::ZeroCopyBuffer>) noexcept {
        return false;
    }
    [[nodiscard]] bool releaseExternalPresentationBindings() noexcept {
        return true;
    }
    [[nodiscard]] bool supportsExternalMemoryFd() const noexcept { return false; }
    [[nodiscard]] bool supportsExternalMemorySynchronization() const noexcept { return false; }
    [[nodiscard]] bool deviceUuidMatches(
        const std::array<std::uint8_t, 16>&) const noexcept { return false; }
    [[nodiscard]] std::uint32_t slotIndexOf(
        const gpu_abi::PackedSculptureSnapshot*) const noexcept {
        return SculptureSnapshotDelivery::kNoSlot;
    }
    [[nodiscard]] std::string_view worldDiagnostic() const noexcept {
        return "OpenCL support unavailable";
    }
    [[nodiscard]] bool worldKernelReady() const noexcept { return false; }
    [[nodiscard]] WorldSubmitResult submitWorldAndTakeLatest(
        const SculptureProgramParams&, const WorldRecipe&, float, float,
        float, std::uint32_t) noexcept {
        return {};
    }
    void acknowledgeWorldDelivery(
        const world_gpu_abi::WorldSnapshotGpu*) noexcept {}
#endif
};
} // namespace epi_sculpt
