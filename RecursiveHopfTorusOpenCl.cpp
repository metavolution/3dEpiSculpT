#include "RecursiveHopfTorusOpenCl.h"
#include "ExperimentExternalMemoryImport.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <tuple>
#include <vector>

#if defined(EPI_SCULPT_HAS_OPENCL) && EPI_SCULPT_HAS_OPENCL
#include <CL/cl.h>
#include <CL/cl_ext.h>
#endif

namespace epi_sculpt::recursive_hopf {

namespace {

std::filesystem::path resolveDefaultKernelPath(const std::filesystem::path& explicitPath) {
    if (!explicitPath.empty() && std::filesystem::exists(explicitPath)) {
        return explicitPath;
    }
#ifdef MGFX_EPI_RESOURCE_ROOT
    {
        for (const auto& sub : {
                 std::filesystem::path("MODULES/3dEpiSculpT/kernels/RecursiveHopfTorus.cl"),
                 std::filesystem::path("mGFX.production/MODULES/3dEpiSculpT/kernels/RecursiveHopfTorus.cl"),
                 std::filesystem::path("3dEpiSculpTMorph/kernels/RecursiveHopfTorus.cl")}) {
            const std::filesystem::path p = std::filesystem::path(MGFX_EPI_RESOURCE_ROOT) / sub;
            if (std::filesystem::exists(p)) return p;
        }
    }
#endif
    const std::filesystem::path candidates[] = {
        "MODULES/3dEpiSculpT/kernels/RecursiveHopfTorus.cl",
        "../MODULES/3dEpiSculpT/kernels/RecursiveHopfTorus.cl",
        "../../MODULES/3dEpiSculpT/kernels/RecursiveHopfTorus.cl",
        "3dEpiSculpTMorph/kernels/RecursiveHopfTorus.cl",
    };
    for (const auto& c : candidates) {
        if (std::filesystem::exists(c)) return c;
    }
    return explicitPath;
}

#if defined(EPI_SCULPT_HAS_OPENCL) && EPI_SCULPT_HAS_OPENCL
struct HostWriteLifetime {
    HostWriteLifetime() = default;
    HostWriteLifetime(const HostWriteLifetime&) = delete;
    HostWriteLifetime& operator=(const HostWriteLifetime&) = delete;
    cl_event event = nullptr;
    ~HostWriteLifetime() {
        if (event) {
            // The normal blocking result read has already completed this write.
            // On an earlier enqueue/argument failure, keep its host source alive
            // until DMA no longer references the caller's stack bytes.
            static_cast<void>(clWaitForEvents(1U, &event));
            static_cast<void>(clReleaseEvent(event));
        }
    }
};

// Capability queries are cold-path and bounded; entry points alone do not
// prove that this device supports the exported POSIX handle types.
template<class T>
std::optional<std::vector<T>> externalDeviceInfo(cl_device_id device, cl_device_info key) {
    std::size_t bytes = 0U;
    if (clGetDeviceInfo(device, key, 0U, nullptr, &bytes) != CL_SUCCESS
        || bytes > 65536U || bytes % sizeof(T)) return std::nullopt;
    std::vector<T> values(bytes / sizeof(T));
    std::size_t actual = 0U;
    if (bytes && (clGetDeviceInfo(device, key, bytes, values.data(), &actual) != CL_SUCCESS
            || actual != bytes)) return std::nullopt;
    return values;
}
#endif

} // namespace

struct RecursiveHopfOpenClProducer::Impl {
    Config config{};
    std::uint32_t nextGeneration = 1U;
    std::string deviceName = "CPU Reference Oracle";
    std::optional<DeviceUuid> deviceUuid{};
    std::string selectionDiagnostic = "OpenCL is unavailable in this build";
    bool ready = false;

#if defined(EPI_SCULPT_HAS_OPENCL) && EPI_SCULPT_HAS_OPENCL
    cl_platform_id platform = nullptr;
    cl_device_id device = nullptr;
    cl_context context = nullptr;
    cl_command_queue queue = nullptr;
    cl_program program = nullptr;
    cl_kernel grammarKernel = nullptr;
    cl_kernel expandKernel = nullptr;
    cl_mem recipeBuffer = nullptr;
    cl_mem grammarBuffer = nullptr;
    cl_mem instanceBuffer = nullptr;
    cl_mem instanceCountBuffer = nullptr;
    std::size_t instanceBufferCapacity = 0U;

    struct ExternalSlot {
        cl_mem memory = nullptr;
        cl_semaphore_khr release = nullptr;
        cl_semaphore_khr ready = nullptr;
        bool usable = false;
    };
    std::array<ExternalSlot, kExperimentExternalSlotCount> external{};
    std::uint64_t externalAllocation = 0U;
    std::uint32_t lastGrammarGeneration = 0U;
    gpu_abi::RecursiveHopfGrammarGpu lastValidatedGrammar{};
    bool externalProbed = false;
    bool externalCapable = false;
    clCreateSemaphoreWithPropertiesKHR_fn createSemaphore = nullptr;
    clReleaseSemaphoreKHR_fn releaseSemaphore = nullptr;
    clEnqueueWaitSemaphoresKHR_fn waitSemaphores = nullptr;
    clEnqueueSignalSemaphoresKHR_fn signalSemaphores = nullptr;
    clEnqueueAcquireExternalMemObjectsKHR_fn acquireMemory = nullptr;
    clEnqueueReleaseExternalMemObjectsKHR_fn releaseMemory = nullptr;

    void clearExternal() noexcept {
        // Every enqueued wait follows an accepted Vulkan release. There is no
        // speculative wait on a frame that has not reached the graphics queue.
        if (queue) static_cast<void>(clFinish(queue));
        for (auto& slot : external) {
            if (slot.memory) clReleaseMemObject(slot.memory);
            if (slot.release && releaseSemaphore) releaseSemaphore(slot.release);
            if (slot.ready && releaseSemaphore) releaseSemaphore(slot.ready);
            slot = {};
        }
        externalAllocation = 0U;
    }

    bool qualifyExternal() {
        if (externalProbed) return externalCapable;
        externalProbed = true;
        if (!ready || !config.requiredDeviceUuid || deviceUuid != config.requiredDeviceUuid)
            return false;
        const auto ext = externalDeviceInfo<char>(device, CL_DEVICE_EXTENSIONS);
        if (!ext || ext->empty() || ext->back() != '\0') return false;
        const std::string names = " " + std::string(ext->data()) + " ";
        for (const char* required : {"cl_khr_external_memory", "cl_khr_external_memory_opaque_fd",
                "cl_khr_semaphore", "cl_khr_external_semaphore", "cl_khr_external_semaphore_opaque_fd"})
            if (names.find(" " + std::string(required) + " ") == std::string::npos) return false;
        const auto memories = externalDeviceInfo<cl_external_memory_handle_type_khr>(
            device, CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR);
        const auto semaphores = externalDeviceInfo<cl_external_semaphore_handle_type_khr>(
            device, CL_DEVICE_SEMAPHORE_IMPORT_HANDLE_TYPES_KHR);
        const auto types = externalDeviceInfo<cl_semaphore_type_khr>(device, CL_DEVICE_SEMAPHORE_TYPES_KHR);
        if (!memories || !semaphores || !types
            || std::find(memories->begin(), memories->end(), CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR) == memories->end()
            || std::find(semaphores->begin(), semaphores->end(), CL_SEMAPHORE_HANDLE_OPAQUE_FD_KHR) == semaphores->end()
            || std::find(types->begin(), types->end(), CL_SEMAPHORE_TYPE_BINARY_KHR) == types->end()) return false;
#define HOPF_EXTERNAL_ENTRY(member, type, name) \
        member = reinterpret_cast<type>(clGetExtensionFunctionAddressForPlatform(platform, name))
        HOPF_EXTERNAL_ENTRY(createSemaphore, clCreateSemaphoreWithPropertiesKHR_fn, "clCreateSemaphoreWithPropertiesKHR");
        HOPF_EXTERNAL_ENTRY(releaseSemaphore, clReleaseSemaphoreKHR_fn, "clReleaseSemaphoreKHR");
        HOPF_EXTERNAL_ENTRY(waitSemaphores, clEnqueueWaitSemaphoresKHR_fn, "clEnqueueWaitSemaphoresKHR");
        HOPF_EXTERNAL_ENTRY(signalSemaphores, clEnqueueSignalSemaphoresKHR_fn, "clEnqueueSignalSemaphoresKHR");
        HOPF_EXTERNAL_ENTRY(acquireMemory, clEnqueueAcquireExternalMemObjectsKHR_fn, "clEnqueueAcquireExternalMemObjectsKHR");
        HOPF_EXTERNAL_ENTRY(releaseMemory, clEnqueueReleaseExternalMemObjectsKHR_fn, "clEnqueueReleaseExternalMemObjectsKHR");
#undef HOPF_EXTERNAL_ENTRY
        externalCapable = createSemaphore && releaseSemaphore && waitSemaphores
            && signalSemaphores && acquireMemory && releaseMemory;
        return externalCapable;
    }

    ~Impl() {
        releaseOpenCl();
    }

    void releaseOpenCl() noexcept {
        clearExternal();
        if (instanceCountBuffer) { clReleaseMemObject(instanceCountBuffer); instanceCountBuffer = nullptr; }
        if (instanceBuffer) { clReleaseMemObject(instanceBuffer); instanceBuffer = nullptr; }
        instanceBufferCapacity = 0U;
        if (grammarBuffer) { clReleaseMemObject(grammarBuffer); grammarBuffer = nullptr; }
        if (recipeBuffer) { clReleaseMemObject(recipeBuffer); recipeBuffer = nullptr; }
        if (expandKernel) { clReleaseKernel(expandKernel); expandKernel = nullptr; }
        if (grammarKernel) { clReleaseKernel(grammarKernel); grammarKernel = nullptr; }
        if (program) { clReleaseProgram(program); program = nullptr; }
        if (queue) { clReleaseCommandQueue(queue); queue = nullptr; }
        if (context) { clReleaseContext(context); context = nullptr; }
        ready = false;
    }

    void tryInitializeOpenCl() {
        selectionDiagnostic = "OpenCL initialization did not complete";
        if (!config.preferOpenCl) {
            selectionDiagnostic = "OpenCL is disabled by configuration";
            return;
        }
        if (config.requiredDeviceUuid && std::none_of(config.requiredDeviceUuid->begin(),
                config.requiredDeviceUuid->end(), [](auto byte) { return byte != 0U; })) {
            selectionDiagnostic = "A zero UUID cannot qualify an OpenCL device";
            return;
        }
        const auto kernelPath = resolveDefaultKernelPath(config.kernelSourcePath);
        if (kernelPath.empty() || !std::filesystem::exists(kernelPath)) return;

        std::ifstream file(kernelPath);
        if (!file.is_open()) return;
        const std::string source(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
        if (source.empty()) return;

        cl_uint numPlatforms = 0U;
        if (clGetPlatformIDs(0U, nullptr, &numPlatforms) != CL_SUCCESS || numPlatforms == 0U) return;
        std::vector<cl_platform_id> platforms(numPlatforms);
        if (clGetPlatformIDs(numPlatforms, platforms.data(), nullptr) != CL_SUCCESS) return;

        // Search every device on every platform. Looking only at device zero
        // misses a matching display GPU and can accidentally prefer a CPU ICD
        // on the first platform over a GPU on a later platform.
        for (const cl_device_type type : std::array<cl_device_type, 2>{CL_DEVICE_TYPE_GPU, CL_DEVICE_TYPE_ALL}) {
            for (cl_platform_id plat : platforms) {
                cl_uint numDevices = 0U;
                if (clGetDeviceIDs(plat, type, 0U, nullptr, &numDevices) != CL_SUCCESS
                    || numDevices == 0U) continue;
                std::vector<cl_device_id> candidates(numDevices);
                if (clGetDeviceIDs(plat, type, numDevices, candidates.data(), nullptr) != CL_SUCCESS)
                    continue;
                for (auto candidate : candidates) {
                    std::optional<DeviceUuid> candidateUuid{};
#if defined(CL_DEVICE_UUID_KHR)
                    DeviceUuid uuid{};
                    std::size_t uuidBytes = 0U;
                    if (clGetDeviceInfo(candidate, CL_DEVICE_UUID_KHR, uuid.size(),
                            uuid.data(), &uuidBytes) == CL_SUCCESS && uuidBytes == uuid.size()
                        && std::any_of(uuid.begin(), uuid.end(), [](auto byte) { return byte != 0U; }))
                        candidateUuid = uuid;
#endif
                    if (config.requiredDeviceUuid && candidateUuid != config.requiredDeviceUuid)
                        continue;
                    device = candidate;
                    platform = plat;
                    deviceUuid = candidateUuid;
                    break;
                }
                if (device) break;
            }
            if (device) break;
        }
        if (device == nullptr) {
            selectionDiagnostic = config.requiredDeviceUuid
                ? "No OpenCL device reports the required UUID"
                : "No OpenCL device is available";
            return;
        }

        cl_int err = CL_SUCCESS;
        context = clCreateContext(nullptr, 1U, &device, nullptr, nullptr, &err);
        if (err != CL_SUCCESS || context == nullptr) { releaseOpenCl(); return; }

        queue = clCreateCommandQueueWithProperties(context, device, nullptr, &err);
        if (err != CL_SUCCESS || queue == nullptr) { releaseOpenCl(); return; }

        const char* srcPtr = source.c_str();
        const std::size_t srcLen = source.size();
        program = clCreateProgramWithSource(context, 1U, &srcPtr, &srcLen, &err);
        if (err != CL_SUCCESS || program == nullptr) { releaseOpenCl(); return; }

        const std::string includeOpt =
            "-cl-std=CL1.2 -I \"" + kernelPath.parent_path().string() + "\"";
        err = clBuildProgram(program, 1U, &device, includeOpt.c_str(), nullptr, nullptr);
        if (err != CL_SUCCESS) { releaseOpenCl(); return; }

        grammarKernel = clCreateKernel(program, "evaluate_recursive_hopf_grammar", &err);
        if (err != CL_SUCCESS || grammarKernel == nullptr) { releaseOpenCl(); return; }

        expandKernel = clCreateKernel(program, "expand_recursive_hopf_instances", &err);
        if (err != CL_SUCCESS || expandKernel == nullptr) { releaseOpenCl(); return; }

        recipeBuffer = clCreateBuffer(
            context, CL_MEM_READ_ONLY, sizeof(gpu_abi::RecursiveHopfRecipeGpu), nullptr, &err);
        if (err != CL_SUCCESS || recipeBuffer == nullptr) { releaseOpenCl(); return; }

        grammarBuffer = clCreateBuffer(
            context, CL_MEM_READ_WRITE, sizeof(gpu_abi::RecursiveHopfGrammarGpu), nullptr, &err);
        if (err != CL_SUCCESS || grammarBuffer == nullptr) { releaseOpenCl(); return; }

        instanceCountBuffer = clCreateBuffer(
            context, CL_MEM_READ_WRITE, sizeof(std::uint32_t), nullptr, &err);
        if (err != CL_SUCCESS || instanceCountBuffer == nullptr) { releaseOpenCl(); return; }

        char nameBuf[256]{};
        if (clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(nameBuf) - 1U, nameBuf, nullptr) == CL_SUCCESS) {
            deviceName = std::string("OpenCL (") + nameBuf + ")";
        } else {
            deviceName = "OpenCL Device";
        }
        ready = true;
        selectionDiagnostic = config.requiredDeviceUuid
            ? "Required Vulkan/OpenCL device UUID verified"
            : "OpenCL selected without a required device UUID";
    }

    bool ensureInstanceBuffer(std::size_t requiredInstances) noexcept {
        if (requiredInstances == 0U) return false;
        if (instanceBuffer != nullptr && instanceBufferCapacity >= requiredInstances) {
            return true;
        }
        if (instanceBuffer != nullptr) {
            clReleaseMemObject(instanceBuffer);
            instanceBuffer = nullptr;
            instanceBufferCapacity = 0U;
        }
        cl_int err = CL_SUCCESS;
        instanceBuffer = clCreateBuffer(
            context,
            CL_MEM_WRITE_ONLY,
            requiredInstances * sizeof(gpu_abi::RecursiveHopfInstanceGpu),
            nullptr,
            &err);
        if (err != CL_SUCCESS || instanceBuffer == nullptr) {
            instanceBuffer = nullptr;
            return false;
        }
        instanceBufferCapacity = requiredInstances;
        return true;
    }

    bool dispatchOpenCl(
        const gpu_abi::RecursiveHopfRecipeGpu& packedRecipe,
        float domainTime,
        std::uint32_t generation,
        gpu_abi::RecursiveHopfGrammarGpu& outGrammar) noexcept {
        lastGrammarGeneration = 0U;
        if (!ready) return false;
        HostWriteLifetime recipeWrite;
        if (clEnqueueWriteBuffer(
                queue, recipeBuffer, CL_FALSE, 0U,
                sizeof(packedRecipe), &packedRecipe, 0U, nullptr, &recipeWrite.event) != CL_SUCCESS) {
            return false;
        }
        if (clSetKernelArg(grammarKernel, 0U, sizeof(cl_mem), &recipeBuffer) != CL_SUCCESS) return false;
        if (clSetKernelArg(grammarKernel, 1U, sizeof(float), &domainTime) != CL_SUCCESS) return false;
        if (clSetKernelArg(grammarKernel, 2U, sizeof(std::uint32_t), &generation) != CL_SUCCESS) return false;
        if (clSetKernelArg(grammarKernel, 3U, sizeof(cl_mem), &grammarBuffer) != CL_SUCCESS) return false;

        const std::size_t workItems = std::max<std::size_t>(
            1U, 2U * packedRecipe.fibersPerFamily * packedRecipe.fibersPerFamily);
        if (clEnqueueNDRangeKernel(
                queue, grammarKernel, 1U, nullptr, &workItems, nullptr, 0U, nullptr, nullptr) != CL_SUCCESS) {
            return false;
        }
        if (clEnqueueReadBuffer(
                queue, grammarBuffer, CL_TRUE, 0U,
                sizeof(outGrammar), &outGrammar, 0U, nullptr, nullptr) != CL_SUCCESS) {
            return false;
        }
        if (!gpu_abi::isValid(outGrammar) || outGrammar.header.generation != generation) return false;
        lastGrammarGeneration = generation;
        lastValidatedGrammar = outGrammar;
        return true;
    }

    bool dispatchExpandedOpenCl(
        const gpu_abi::RecursiveHopfRecipeGpu& packedRecipe,
        float domainTime,
        std::uint32_t generation,
        std::uint32_t effectiveMaxDepth,
        std::size_t totalCandidateNodes,
        float minNodeScale,
        std::uint32_t maxInstances,
        gpu_abi::RecursiveHopfGrammarGpu& outGrammar,
        std::vector<gpu_abi::RecursiveHopfInstanceGpu>& outInstances) noexcept {
        if (!ready || expandKernel == nullptr || maxInstances == 0U || totalCandidateNodes == 0U) {
            return false;
        }
        if (!ensureInstanceBuffer(maxInstances)) {
            return false;
        }
        if (!dispatchOpenCl(packedRecipe, domainTime, generation, outGrammar)) {
            return false;
        }

        const std::uint32_t zeroCount = 0U;
        HostWriteLifetime countWrite;
        if (clEnqueueWriteBuffer(
                queue, instanceCountBuffer, CL_FALSE, 0U,
                sizeof(zeroCount), &zeroCount, 0U, nullptr, &countWrite.event) != CL_SUCCESS) {
            return false;
        }

        if (clSetKernelArg(expandKernel, 0U, sizeof(cl_mem), &grammarBuffer) != CL_SUCCESS) return false;
        if (clSetKernelArg(expandKernel, 1U, sizeof(std::uint32_t), &effectiveMaxDepth) != CL_SUCCESS) return false;
        if (clSetKernelArg(expandKernel, 2U, sizeof(float), &minNodeScale) != CL_SUCCESS) return false;
        if (clSetKernelArg(expandKernel, 3U, sizeof(std::uint32_t), &maxInstances) != CL_SUCCESS) return false;
        if (clSetKernelArg(expandKernel, 4U, sizeof(cl_mem), &instanceBuffer) != CL_SUCCESS) return false;
        if (clSetKernelArg(expandKernel, 5U, sizeof(cl_mem), &instanceCountBuffer) != CL_SUCCESS) return false;

        const std::size_t workItems = totalCandidateNodes;
        if (clEnqueueNDRangeKernel(
                queue, expandKernel, 1U, nullptr, &workItems, nullptr, 0U, nullptr, nullptr) != CL_SUCCESS) {
            return false;
        }

        std::uint32_t emittedCount = 0U;
        if (clEnqueueReadBuffer(
                queue, instanceCountBuffer, CL_TRUE, 0U,
                sizeof(emittedCount), &emittedCount, 0U, nullptr, nullptr) != CL_SUCCESS) {
            return false;
        }

        const std::uint32_t readCount = std::min(emittedCount, maxInstances);
        outInstances.resize(readCount);
        if (readCount > 0U) {
            if (clEnqueueReadBuffer(
                    queue,
                    instanceBuffer,
                    CL_TRUE,
                    0U,
                    readCount * sizeof(gpu_abi::RecursiveHopfInstanceGpu),
                    outInstances.data(),
                    0U,
                    nullptr,
                    nullptr) != CL_SUCCESS) {
                return false;
            }
            std::sort(
                outInstances.begin(),
                outInstances.end(),
                [](const gpu_abi::RecursiveHopfInstanceGpu& a,
                   const gpu_abi::RecursiveHopfInstanceGpu& b) noexcept {
                    return std::tie(a.depth, a.nodeIdLow, a.chirality, a.fiberIndex)
                        < std::tie(b.depth, b.nodeIdLow, b.chirality, b.fiberIndex);
                });
        }
        return true;
    }
#endif
};

RecursiveHopfOpenClProducer::RecursiveHopfOpenClProducer()
    : RecursiveHopfOpenClProducer(Config{}) {}

RecursiveHopfOpenClProducer::RecursiveHopfOpenClProducer(const Config& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
#if defined(EPI_SCULPT_HAS_OPENCL) && EPI_SCULPT_HAS_OPENCL
    try {
        impl_->tryInitializeOpenCl();
    } catch (const std::exception& error) {
        impl_->releaseOpenCl();
        impl_->selectionDiagnostic = std::string("OpenCL initialization failed: ") + error.what();
    }
#endif
}

RecursiveHopfOpenClProducer::~RecursiveHopfOpenClProducer() = default;

bool RecursiveHopfOpenClProducer::openClAvailable() const noexcept {
    return impl_ && impl_->ready;
}

std::string RecursiveHopfOpenClProducer::deviceDescription() const {
    return impl_ && impl_->ready ? impl_->deviceName : "CPU Reference Oracle";
}

std::optional<RecursiveHopfOpenClProducer::DeviceUuid>
RecursiveHopfOpenClProducer::selectedDeviceUuid() const noexcept {
    return impl_ && impl_->ready ? impl_->deviceUuid : std::nullopt;
}

bool RecursiveHopfOpenClProducer::requiredDeviceUuidMatched() const noexcept {
    return impl_ && impl_->ready && impl_->config.requiredDeviceUuid
        && impl_->deviceUuid == impl_->config.requiredDeviceUuid;
}

std::string RecursiveHopfOpenClProducer::deviceSelectionDiagnostic() const {
    return impl_ ? impl_->selectionDiagnostic : "Producer is uninitialized";
}

bool RecursiveHopfOpenClProducer::supportsExternalGrammar() const noexcept {
#if defined(EPI_SCULPT_HAS_OPENCL) && EPI_SCULPT_HAS_OPENCL
    try { return impl_ && impl_->qualifyExternal(); } catch (...) { return false; }
#else
    return false;
#endif
}

bool RecursiveHopfOpenClProducer::hasExternalGrammarImports() const noexcept {
#if defined(EPI_SCULPT_HAS_OPENCL) && EPI_SCULPT_HAS_OPENCL
    return impl_ && impl_->externalAllocation != 0U;
#else
    return false;
#endif
}

void RecursiveHopfOpenClProducer::clearExternalGrammarImports() noexcept {
#if defined(EPI_SCULPT_HAS_OPENCL) && EPI_SCULPT_HAS_OPENCL
    if (impl_) impl_->clearExternal();
#endif
}

bool RecursiveHopfOpenClProducer::importExternalGrammar(ExperimentOpenClImportSlot& value) {
#if defined(EPI_SCULPT_HAS_OPENCL) && EPI_SCULPT_HAS_OPENCL
    if (!supportsExternalGrammar() || !value.valid() || value.elementCount != 1U
        || value.elementStrideBytes != sizeof(gpu_abi::RecursiveHopfGrammarGpu)
        || value.allocationSize < sizeof(gpu_abi::RecursiveHopfGrammarGpu)
        || (impl_->externalAllocation && impl_->externalAllocation != value.allocationGeneration)) return false;
    auto& slot = impl_->external[value.slotIndex];
    if (slot.memory || slot.release || slot.ready) return false;
    // Partial candidates belong to this function until all three imports exist.
    // On success OpenCL consumes the FD; remaining descriptors stay caller-owned.
    cl_int error = CL_SUCCESS;
    const cl_mem_properties properties[] = {CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR,
        static_cast<cl_mem_properties>(value.memoryFd), 0};
    cl_mem memory = clCreateBufferWithProperties(impl_->context, properties, CL_MEM_READ_WRITE,
        value.allocationSize, nullptr, &error);
    if (error != CL_SUCCESS || !memory) { if (memory) clReleaseMemObject(memory); return false; }
    value.memoryFd = -1;
    const auto importSemaphore = [&](int& fd) {
        const cl_semaphore_properties_khr props[] = {CL_SEMAPHORE_TYPE_KHR, CL_SEMAPHORE_TYPE_BINARY_KHR,
            CL_SEMAPHORE_HANDLE_OPAQUE_FD_KHR, static_cast<cl_semaphore_properties_khr>(fd), 0};
        auto result = impl_->createSemaphore(impl_->context, props, &error);
        if (error == CL_SUCCESS && result) { fd = -1; return result; }
        if (result) impl_->releaseSemaphore(result);
        return static_cast<cl_semaphore_khr>(nullptr);
    };
    auto release = importSemaphore(value.vulkanReleaseSemaphoreFd);
    if (!release) { clReleaseMemObject(memory); return false; }
    auto ready = importSemaphore(value.openClReadySemaphoreFd);
    if (!ready) { impl_->releaseSemaphore(release); clReleaseMemObject(memory); return false; }
    slot = {memory, release, ready, true};
    impl_->externalAllocation = value.allocationGeneration;
    return true;
#else
    (void)value;
    return false;
#endif
}

RecursiveHopfOpenClProducer::ExternalGrammarCopyResult RecursiveHopfOpenClProducer::copyExternalGrammar(
    std::uint32_t index, const RecursiveHopfDelivery& source) noexcept {
#if defined(EPI_SCULPT_HAS_OPENCL) && EPI_SCULPT_HAS_OPENCL
    if (!impl_ || !impl_->ready || !impl_->externalCapable || index >= impl_->external.size()
        || !source.producedByOpenCl || source.generation == 0U
        || impl_->lastGrammarGeneration != source.generation
        || std::memcmp(&source.gpuGrammar, &impl_->lastValidatedGrammar, sizeof(source.gpuGrammar)) != 0)
        return ExternalGrammarCopyResult::Rejected;
    auto& slot = impl_->external[index];
    if (!slot.usable) return ExternalGrammarCopyResult::Rejected;
    // In-order queue: acquire only after the graphics release, copy canonical
    // bytes before any later production can overwrite them, release then signal.
    if (impl_->waitSemaphores(impl_->queue, 1U, &slot.release, nullptr, 0U, nullptr, nullptr) != CL_SUCCESS
        || impl_->acquireMemory(impl_->queue, 1U, &slot.memory, 0U, nullptr, nullptr) != CL_SUCCESS
        || clEnqueueCopyBuffer(impl_->queue, impl_->grammarBuffer, slot.memory, 0U, 0U,
            sizeof(gpu_abi::RecursiveHopfGrammarGpu), 0U, nullptr, nullptr) != CL_SUCCESS
        || impl_->releaseMemory(impl_->queue, 1U, &slot.memory, 0U, nullptr, nullptr) != CL_SUCCESS
        || impl_->signalSemaphores(impl_->queue, 1U, &slot.ready, nullptr, 0U, nullptr, nullptr) != CL_SUCCESS
        || clFlush(impl_->queue) != CL_SUCCESS) {
        slot.usable = false;
        return ExternalGrammarCopyResult::Failed;
    }
    return ExternalGrammarCopyResult::Queued;
#else
    (void)index; (void)source;
    return ExternalGrammarCopyResult::Rejected;
#endif
}

RecursiveHopfDelivery RecursiveHopfOpenClProducer::produce(
    const Recipe& recipe,
    double domainTime) {
    const auto t0 = std::chrono::steady_clock::now();
    const std::uint32_t gen = impl_->nextGeneration++;
    Grammar cpuGrammar = Grammar::prepare(recipe, domainTime);

    gpu_abi::RecursiveHopfGrammarGpu gpuGrammar{};
    bool usedOpenCl = false;

#if defined(EPI_SCULPT_HAS_OPENCL) && EPI_SCULPT_HAS_OPENCL
    if (impl_->ready) {
        const auto packedRecipe = gpu_abi::packRecipe(recipe);
        usedOpenCl = impl_->dispatchOpenCl(
            packedRecipe, static_cast<float>(domainTime), gen, gpuGrammar);
    }
#endif

    if (!usedOpenCl) {
        gpuGrammar = gpu_abi::packGrammar(cpuGrammar, gen);
    }

    const float ms = std::chrono::duration<float, std::milli>(
        std::chrono::steady_clock::now() - t0).count();

    return RecursiveHopfDelivery{
        .generation = gen,
        .producedByOpenCl = usedOpenCl,
        .evaluationMilliseconds = ms,
        .grammar = std::move(cpuGrammar),
        .gpuGrammar = gpuGrammar,
    };
}

RecursiveHopfExpandedDelivery RecursiveHopfOpenClProducer::produceExpandedInstances(
    const Recipe& recipe,
    double domainTime,
    const TraversalBudget& budget) {
    const auto t0 = std::chrono::steady_clock::now();
    const std::uint32_t gen = impl_->nextGeneration++;
    Grammar cpuGrammar = Grammar::prepare(recipe, domainTime);

    gpu_abi::RecursiveHopfGrammarGpu gpuGrammar{};
    std::vector<gpu_abi::RecursiveHopfInstanceGpu> gpuInstances{};
    bool usedOpenCl = false;

#if defined(EPI_SCULPT_HAS_OPENCL) && EPI_SCULPT_HAS_OPENCL
    if (impl_->ready && !budget.enableViewCulling && recipe.weaveAmplitude <= 0.0) {
        const std::uint32_t effDepth = std::min(recipe.logicalDepth, budget.maxDepth);
        const auto levelCounts = computeHierarchyCounts(recipe.fibersPerFamily, effDepth);
        const std::size_t maxNodes = std::min<std::size_t>(
            static_cast<std::size_t>(levelCounts.nodes), budget.maxNodes);
        const std::uint32_t maxInstances = static_cast<std::uint32_t>(std::min<std::size_t>(
            budget.maxFibers, maxNodes * 2U * recipe.fibersPerFamily));
        const auto packedRecipe = gpu_abi::packRecipe(recipe);
        usedOpenCl = impl_->dispatchExpandedOpenCl(
            packedRecipe,
            static_cast<float>(domainTime),
            gen,
            effDepth,
            maxNodes,
            static_cast<float>(budget.minWorldScale),
            maxInstances,
            gpuGrammar,
            gpuInstances);
    }
#endif

    if (!usedOpenCl) {
        gpuGrammar = gpu_abi::packGrammar(cpuGrammar, gen);
        const TraversalResult traversal = cpuGrammar.traverse(budget);
        gpuInstances.resize(traversal.fibers.size());
        gpu_abi::packExpandedInstances(cpuGrammar, traversal, gpuInstances);
    }

    const float ms = std::chrono::duration<float, std::milli>(
        std::chrono::steady_clock::now() - t0).count();

    return RecursiveHopfExpandedDelivery{
        .generation = gen,
        .producedByOpenCl = usedOpenCl,
        .evaluationMilliseconds = ms,
        .grammar = std::move(cpuGrammar),
        .gpuGrammar = gpuGrammar,
        .gpuInstances = std::move(gpuInstances),
    };
}

} // namespace epi_sculpt::recursive_hopf
