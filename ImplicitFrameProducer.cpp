#ifndef EPI_SCULPT_HAS_OPENCL
#define EPI_SCULPT_HAS_OPENCL 1
#endif

#include "ImplicitFrameProducer.h"

#include <span>
#include "ImplicitDeliveryRing.h"
#include "SculptureProgramBinaryCache.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <unistd.h>

#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <CL/cl_ext.h>

namespace epi_sculpt {
namespace {

void requireOpenCl(cl_int result, const char* operation) {
    if (result != CL_SUCCESS) {
        throw std::runtime_error(
            std::string(operation) + " failed with OpenCL error "
            + std::to_string(result));
    }
}

std::string readTextFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open OpenCL source: " + path);
    }
    std::ostringstream stream;
    stream << input.rdbuf();
    return stream.str();
}

void releaseEvent(cl_event& event) noexcept {
    if (event != nullptr) {
        clReleaseEvent(event);
        event = nullptr;
    }
}

// Cold-path capability query. Distinguish a valid empty list from an unsupported
// query; a platform function pointer alone says nothing about this device.
template<class T>
std::optional<std::vector<T>> deviceInfoList(cl_device_id device, cl_device_info key) {
    std::size_t bytes = 0U;
    if (clGetDeviceInfo(device, key, 0, nullptr, &bytes) != CL_SUCCESS
        || bytes % sizeof(T) != 0U || bytes > 65536U) return std::nullopt;
    std::vector<T> values(bytes / sizeof(T));
    if (bytes != 0U) {
        std::size_t written = 0U;
        if (clGetDeviceInfo(device, key, bytes, values.data(), &written) != CL_SUCCESS
            || written != bytes) return std::nullopt;
    }
    return values;
}

std::string implicitImageImportRejection(cl_device_id device) {
    const auto extensions = deviceInfoList<char>(device, CL_DEVICE_EXTENSIONS);
    if (!extensions || extensions->empty() || extensions->back() != '\0') {
        return "cannot query OpenCL device extensions";
    }
    const std::string names = " " + std::string(extensions->data()) + " ";
    for (const char* required : {"cl_khr_external_memory", "cl_khr_external_memory_opaque_fd",
            "cl_khr_semaphore", "cl_khr_external_semaphore", "cl_khr_external_semaphore_opaque_fd"}) {
        if (names.find(" " + std::string(required) + " ") == std::string::npos) {
            return "OpenCL device missing " + std::string(required);
        }
    }
#if defined(CL_DEVICE_EXTERNAL_MEMORY_IMPORT_ASSUME_LINEAR_IMAGES_HANDLE_TYPES_KHR)
    const auto memory = deviceInfoList<cl_external_memory_handle_type_khr>(
        device, CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR);
    const auto semaphores = deviceInfoList<cl_external_semaphore_handle_type_khr>(
        device, CL_DEVICE_SEMAPHORE_IMPORT_HANDLE_TYPES_KHR);
    const auto types = deviceInfoList<cl_semaphore_type_khr>(device, CL_DEVICE_SEMAPHORE_TYPES_KHR);
    if (!memory || std::find(memory->begin(), memory->end(),
            CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR) == memory->end()) {
        return "OpenCL device cannot import opaque-FD memory";
    }
    if (!semaphores || std::find(semaphores->begin(), semaphores->end(),
            CL_SEMAPHORE_HANDLE_OPAQUE_FD_KHR) == semaphores->end()
        || !types || std::find(types->begin(), types->end(),
            CL_SEMAPHORE_TYPE_BINARY_KHR) == types->end()) {
        return "OpenCL device cannot import opaque-FD binary semaphores";
    }
    const auto linear = deviceInfoList<cl_external_memory_handle_type_khr>(
        device, CL_DEVICE_EXTERNAL_MEMORY_IMPORT_ASSUME_LINEAR_IMAGES_HANDLE_TYPES_KHR);
    if (!linear) return "OpenCL image-layout interpretation query unavailable";
    if (std::find(linear->begin(), linear->end(), CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR)
        != linear->end()) {
        return "OpenCL assumes linear opaque-FD images; Vulkan outputs use optimal tiling";
    }
    // Absence from the linear list means implementation-defined layout, not
    // proof of Vulkan-optimal compatibility. The focused demo remains staged.
    return {};
#else
    return "OpenCL headers lack the external image-layout interpretation query";
#endif
}

[[nodiscard]] bool deviceMatchesUuid(
    cl_device_id device,
    const std::array<std::uint8_t, 16>& requiredUuid) noexcept {
#if defined(CL_DEVICE_UUID_KHR)
    std::array<cl_uchar, 16> candidate{};
    const cl_int result = clGetDeviceInfo(
        device, CL_DEVICE_UUID_KHR, candidate.size(), candidate.data(), nullptr);
    return result == CL_SUCCESS
        && std::equal(candidate.begin(), candidate.end(), requiredUuid.begin());
#else
    (void)device;
    (void)requiredUuid;
    return false;
#endif
}

} // namespace

struct ImplicitFrameProducer::Impl {
    struct Slot {
        ImplicitFrameRequest request{};
        OutputBuffers output{};
        cl_kernel kernel = nullptr;
        cl_mem paramsBuffer = nullptr;
        cl_mem frameBuffer = nullptr;
        cl_mem colorBuffer = nullptr;
        cl_mem depthBuffer = nullptr;
        cl_mem normalBuffer = nullptr;
        cl_mem velocityBuffer = nullptr;
        cl_event firstEvent = nullptr;
        cl_event kernelEvent = nullptr;
        cl_event completionEvent = nullptr;
        cl_semaphore_khr waitSemaphore = nullptr;
        cl_semaphore_khr signalSemaphore = nullptr;
        ImplicitExternalColorSlot externalColor{};
        bool outputBindingsDirty = false;
    };

    cl_device_id device = nullptr; // Borrowed root device, owned by its platform.
    cl_context context = nullptr;
    cl_command_queue queue = nullptr;
    cl_program program = nullptr;
    std::array<Slot, ImplicitFrameProducer::kSlotCount> slots{};
    ImplicitDeliveryRing<ImplicitFrameProducer::kSlotCount> ring{};
    ImplicitQualityPolicy qualityPolicy{};
    ImplicitCamera camera{};
    ImplicitProducerTelemetry telemetry{};
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    bool hasRequestedGeneration = false;
    bool failed = false;
    bool includeDepth = false;
    bool includeNormal = false;
    bool includeVelocity = false;

    bool zeroCopyEnabled = false;
    clCreateSemaphoreWithPropertiesKHR_fn pfn_clCreateSemaphoreWithPropertiesKHR = nullptr;
    clEnqueueWaitSemaphoresKHR_fn pfn_clEnqueueWaitSemaphoresKHR = nullptr;
    clEnqueueSignalSemaphoresKHR_fn pfn_clEnqueueSignalSemaphoresKHR = nullptr;
    clReleaseSemaphoreKHR_fn pfn_clReleaseSemaphoreKHR = nullptr;
    clEnqueueAcquireExternalMemObjectsKHR_fn acquireExternalMemory = nullptr;
    clEnqueueReleaseExternalMemObjectsKHR_fn releaseExternalMemory = nullptr;

    explicit Impl(const Config& config)
        : qualityPolicy(config.quality),
          includeDepth(config.includeDepth),
          includeNormal(config.includeNormal),
          includeVelocity(config.includeVelocity) {
        telemetry.maxRaySteps = qualityPolicy.raySteps();
    }

    ~Impl() {
        if (queue != nullptr) clFinish(queue);
        for (auto& slot : slots) {
            releaseEvent(slot.firstEvent);
            releaseEvent(slot.kernelEvent);
            if (pfn_clReleaseSemaphoreKHR) {
                if (slot.waitSemaphore) pfn_clReleaseSemaphoreKHR(slot.waitSemaphore);
                if (slot.signalSemaphore) pfn_clReleaseSemaphoreKHR(slot.signalSemaphore);
            }
            releaseEvent(slot.completionEvent);
            if (slot.paramsBuffer) clReleaseMemObject(slot.paramsBuffer);
            if (slot.frameBuffer) clReleaseMemObject(slot.frameBuffer);
            if (slot.colorBuffer) clReleaseMemObject(slot.colorBuffer);
            if (slot.depthBuffer) clReleaseMemObject(slot.depthBuffer);
            if (slot.normalBuffer) clReleaseMemObject(slot.normalBuffer);
            if (slot.velocityBuffer) clReleaseMemObject(slot.velocityBuffer);
            if (slot.kernel) clReleaseKernel(slot.kernel);
        }
        if (program) clReleaseProgram(program);
        if (queue) clReleaseCommandQueue(queue);
        if (context) clReleaseContext(context);
    }

    [[nodiscard]] bool fail(
        cl_int result, const char* operation, std::string& diagnostic) {
        if (result == CL_SUCCESS) return false;
        failed = true;
        if (zeroCopyEnabled) {
            telemetry.interop.transitionToHostStaged(SculptureInteropEvent::DispatchFailed);
        }
        diagnostic = std::string(operation) + " failed with OpenCL error "
            + std::to_string(result);
        return true;
    }

    [[nodiscard]] bool poll(std::string& diagnostic) {
        for (std::size_t index = 0U; index < slots.size(); ++index) {
            if (ring.slot(index).state != ImplicitSlotState::Producing) continue;
            auto& slot = slots[index];
            cl_int executionStatus = CL_QUEUED;
            if (fail(clGetEventInfo(
                         slot.completionEvent,
                         CL_EVENT_COMMAND_EXECUTION_STATUS,
                         sizeof(executionStatus), &executionStatus, nullptr),
                     "clGetEventInfo(implicit completion)", diagnostic)) {
                return false;
            }
            if (executionStatus < 0) {
                failed = true;
                diagnostic = "implicit OpenCL delivery completed with error "
                    + std::to_string(executionStatus);
                return false;
            }
            if (executionStatus != CL_COMPLETE) continue;

            cl_ulong beginNanoseconds = 0U;
            cl_ulong endNanoseconds = 0U;
            const cl_int beginResult = clGetEventProfilingInfo(
                slot.firstEvent, CL_PROFILING_COMMAND_START,
                sizeof(beginNanoseconds), &beginNanoseconds, nullptr);
            const cl_int endResult = clGetEventProfilingInfo(
                slot.completionEvent, CL_PROFILING_COMMAND_END,
                sizeof(endNanoseconds), &endNanoseconds, nullptr);
            if (beginResult == CL_SUCCESS && endResult == CL_SUCCESS
                && endNanoseconds >= beginNanoseconds) {
                telemetry.openClCompletionMilliseconds = static_cast<float>(
                    endNanoseconds - beginNanoseconds) * 1.0e-6f;
            }

            cl_ulong kernelBeginNanoseconds = 0U;
            cl_ulong kernelEndNanoseconds = 0U;
            const cl_int kernelBeginResult = clGetEventProfilingInfo(
                slot.kernelEvent, CL_PROFILING_COMMAND_START,
                sizeof(kernelBeginNanoseconds), &kernelBeginNanoseconds, nullptr);
            const cl_int kernelEndResult = clGetEventProfilingInfo(
                slot.kernelEvent, CL_PROFILING_COMMAND_END,
                sizeof(kernelEndNanoseconds), &kernelEndNanoseconds, nullptr);
            if (kernelBeginResult == CL_SUCCESS && kernelEndResult == CL_SUCCESS
                && kernelEndNanoseconds >= kernelBeginNanoseconds) {
                telemetry.kernelMilliseconds = static_cast<float>(
                    kernelEndNanoseconds - kernelBeginNanoseconds) * 1.0e-6f;
                if (qualityPolicy.observeKernelMilliseconds(
                        telemetry.kernelMilliseconds)) {
                    ++telemetry.qualityAdjustments;
                }
                telemetry.maxRaySteps = qualityPolicy.raySteps();
                telemetry.resolutionScale = qualityPolicy.resolutionScale();
            }

            releaseEvent(slot.firstEvent);
            releaseEvent(slot.kernelEvent);
            // Imported external semaphores are persistent slot resources. A
            // completed frame releases ownership through the semaphore signal,
            // not by destroying the OpenCL semaphore object after one use.
            releaseEvent(slot.completionEvent);
            if (!ring.markReady(index)) {
                failed = true;
                diagnostic = "implicit delivery ownership transition failed";
                return false;
            }
        }
        telemetry.pendingSlots = ring.pendingCount();
        return true;
    }

    [[nodiscard]] bool deliverNewest() noexcept {
        const auto delivered = ring.deliverNewestReady(!zeroCopyEnabled);
        if (!delivered) return false;
        telemetry.deliveredGeneration = ring.deliveredGeneration();
        telemetry.delivery = ImplicitFrameDelivery::Fresh;
        return true;
    }
};

ImplicitFrameProducer::ImplicitFrameProducer(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {
    ImplicitFrameRequest initialRequest{};
    std::string validationError;
    if (!makeImplicitFrameRequest(
            SculptureProgramParams{}, 0.0f, 0.0f, 0.0f, 0U,
            config.width, config.height, impl_->camera,
            initialRequest, validationError)) {
        throw std::invalid_argument(validationError);
    }

    impl_->width = config.width;
    impl_->height = config.height;

    cl_uint platformCount = 0U;
    requireOpenCl(clGetPlatformIDs(0U, nullptr, &platformCount),
                  "clGetPlatformIDs(count)");
    if (platformCount == 0U) {
        throw std::runtime_error("no OpenCL platform is available");
    }
    std::vector<cl_platform_id> platforms(platformCount);
    requireOpenCl(clGetPlatformIDs(
                      platformCount, platforms.data(), nullptr),
                  "clGetPlatformIDs(list)");

    cl_device_id device = nullptr;
    cl_platform_id activePlatform = nullptr;
    for (const auto platform : platforms) {
        cl_uint deviceCount = 0U;
        const cl_int countResult = clGetDeviceIDs(
            platform, CL_DEVICE_TYPE_GPU, 0U, nullptr, &deviceCount);
        if (countResult == CL_DEVICE_NOT_FOUND) continue;
        requireOpenCl(countResult, "clGetDeviceIDs(count)");
        if (deviceCount == 0U) continue;
        std::vector<cl_device_id> devices(deviceCount);
        requireOpenCl(clGetDeviceIDs(
                          platform, CL_DEVICE_TYPE_GPU, deviceCount,
                          devices.data(), nullptr),
                      "clGetDeviceIDs(list)");
        for (const cl_device_id candidate : devices) {
            if (!config.matchDeviceUuid
                || deviceMatchesUuid(candidate, config.requiredDeviceUuid)) {
                device = candidate;
                activePlatform = platform;
                break;
            }
        }
        if (device != nullptr) break;
    }
    if (device == nullptr) {
        throw std::runtime_error(config.matchDeviceUuid
            ? "no OpenCL GPU matches the active Vulkan device UUID"
            : "no OpenCL GPU device is available");
    }
    impl_->device = device;

    impl_->pfn_clCreateSemaphoreWithPropertiesKHR = (clCreateSemaphoreWithPropertiesKHR_fn)clGetExtensionFunctionAddressForPlatform(activePlatform, "clCreateSemaphoreWithPropertiesKHR");
    impl_->pfn_clEnqueueWaitSemaphoresKHR = (clEnqueueWaitSemaphoresKHR_fn)clGetExtensionFunctionAddressForPlatform(activePlatform, "clEnqueueWaitSemaphoresKHR");
    impl_->pfn_clEnqueueSignalSemaphoresKHR = (clEnqueueSignalSemaphoresKHR_fn)clGetExtensionFunctionAddressForPlatform(activePlatform, "clEnqueueSignalSemaphoresKHR");
    impl_->pfn_clReleaseSemaphoreKHR = (clReleaseSemaphoreKHR_fn)clGetExtensionFunctionAddressForPlatform(activePlatform, "clReleaseSemaphoreKHR");
    impl_->acquireExternalMemory = reinterpret_cast<clEnqueueAcquireExternalMemObjectsKHR_fn>(
        clGetExtensionFunctionAddressForPlatform(activePlatform, "clEnqueueAcquireExternalMemObjectsKHR"));
    impl_->releaseExternalMemory = reinterpret_cast<clEnqueueReleaseExternalMemObjectsKHR_fn>(
        clGetExtensionFunctionAddressForPlatform(activePlatform, "clEnqueueReleaseExternalMemObjectsKHR"));

    cl_int result = CL_SUCCESS;
    impl_->context = clCreateContext(
        nullptr, 1U, &device, nullptr, nullptr, &result);
    requireOpenCl(result, "clCreateContext");
    const cl_queue_properties queueProperties[] = {
        CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};
    impl_->queue = clCreateCommandQueueWithProperties(
        impl_->context, device, queueProperties, &result);
    requireOpenCl(result, "clCreateCommandQueueWithProperties");

    const std::string source = readTextFile(config.kernelPath);
    const std::string directory = config.kernelPath.substr(
        0U, config.kernelPath.find_last_of("/\\"));
    const std::string buildOptions = "-I " + directory + " -cl-fast-relaxed-math";

    impl_->program = SculptureProgramBinaryCache::buildOrLoadProgram(
        impl_->context, device, source, buildOptions, config.kernelEntryPoint,
        SculptureProgramBinaryCache::kDefaultCacheDir, "", &result);
    if (result != CL_SUCCESS || impl_->program == nullptr) {
        size_t logSize = 0U;
        std::string logMsg = "unknown OpenCL compiler error";
        if (impl_->program) {
            clGetProgramBuildInfo(
                impl_->program, device, CL_PROGRAM_BUILD_LOG, 0U, nullptr, &logSize);
            std::vector<char> log(logSize, '\0');
            clGetProgramBuildInfo(
                impl_->program, device, CL_PROGRAM_BUILD_LOG,
                log.size(), log.data(), nullptr);
            logMsg = std::string(log.data());
        }
        throw std::runtime_error(
            "clBuildProgram(implicit) failed with OpenCL error "
            + std::to_string(result) + ": " + logMsg);
    }

    const size_t pixelCount = static_cast<size_t>(config.width) * config.height;
    const size_t colorBytes = pixelCount * 4U * sizeof(float);
    const size_t depthBytes = pixelCount * sizeof(float);
    const size_t normalBytes = pixelCount * 4U * sizeof(float);
    const size_t velocityBytes = pixelCount * 2U * sizeof(float);
    impl_->telemetry.stagedColorBytes = colorBytes;
    impl_->telemetry.stagedAuxiliaryBytes =
        (impl_->includeDepth ? depthBytes : 0U)
        + (impl_->includeNormal ? normalBytes : 0U)
        + (impl_->includeVelocity ? velocityBytes : 0U);

    for (auto& slot : impl_->slots) {
        slot.output.color.resize(pixelCount * 4U);
        if (impl_->includeDepth) slot.output.depth.resize(pixelCount);
        if (impl_->includeNormal) slot.output.normal.resize(pixelCount * 4U);
        if (impl_->includeVelocity) slot.output.velocity.resize(pixelCount * 2U);
        slot.kernel = clCreateKernel(
            impl_->program, config.kernelEntryPoint.c_str(), &result);
        requireOpenCl(result, "clCreateKernel(implicit field)");
        cl_uint argumentCount = 0U;
        requireOpenCl(clGetKernelInfo(slot.kernel, CL_KERNEL_NUM_ARGS,
                      sizeof(argumentCount), &argumentCount, nullptr),
                      "clGetKernelInfo(implicit field arguments)");
        if (argumentCount != 18U) {
            throw std::runtime_error("implicit field must implement the 18-argument image ABI");
        }
        slot.paramsBuffer = clCreateBuffer(
            impl_->context, CL_MEM_READ_ONLY,
            sizeof(gpu_abi::SculptureProgramParamsGpu), nullptr, &result);
        requireOpenCl(result, "clCreateBuffer(implicit program)");
        slot.frameBuffer = clCreateBuffer(
            impl_->context, CL_MEM_READ_ONLY,
            sizeof(gpu_abi::SculptureFrameGpu), nullptr, &result);
        requireOpenCl(result, "clCreateBuffer(implicit frame)");
        cl_image_format format = {};
        format.image_channel_order = CL_RGBA;
        format.image_channel_data_type = CL_FLOAT;
        cl_image_desc desc = {};
        desc.image_type = CL_MEM_OBJECT_IMAGE2D;
        desc.image_width = config.width;
        desc.image_height = config.height;
        desc.image_row_pitch = 0;
        
        slot.colorBuffer = clCreateImage(
            impl_->context, CL_MEM_WRITE_ONLY, &format, &desc, nullptr, &result);
        requireOpenCl(result, "clCreateImage(implicit color)");
        cl_image_format depthFormat = {};
        depthFormat.image_channel_order = CL_R;
        depthFormat.image_channel_data_type = CL_FLOAT;
        slot.depthBuffer = clCreateImage(
            impl_->context, CL_MEM_WRITE_ONLY, &depthFormat, &desc, nullptr, &result);
        requireOpenCl(result, "clCreateImage(implicit depth)");

        cl_image_format normalFormat = {};
        normalFormat.image_channel_order = CL_RGBA;
        normalFormat.image_channel_data_type = CL_FLOAT;
        slot.normalBuffer = clCreateImage(
            impl_->context, CL_MEM_WRITE_ONLY, &normalFormat, &desc, nullptr, &result);
        requireOpenCl(result, "clCreateImage(implicit normal)");

        cl_image_format velocityFormat = {};
        velocityFormat.image_channel_order = CL_RG;
        velocityFormat.image_channel_data_type = CL_FLOAT;
        slot.velocityBuffer = clCreateImage(
            impl_->context, CL_MEM_WRITE_ONLY, &velocityFormat, &desc, nullptr, &result);
        requireOpenCl(result, "clCreateImage(implicit velocity)");

        requireOpenCl(clSetKernelArg(
                          slot.kernel, 0U, sizeof(cl_mem), &slot.paramsBuffer),
                      "clSetKernelArg(0)");
        requireOpenCl(clSetKernelArg(
                          slot.kernel, 1U, sizeof(cl_mem), &slot.frameBuffer),
                      "clSetKernelArg(1)");
        requireOpenCl(clSetKernelArg(
                          slot.kernel, 2U, sizeof(cl_mem), &slot.colorBuffer),
                      "clSetKernelArg(2)");
        requireOpenCl(clSetKernelArg(
                          slot.kernel, 3U, sizeof(cl_mem), &slot.depthBuffer),
                      "clSetKernelArg(3)");
        requireOpenCl(clSetKernelArg(
                          slot.kernel, 4U, sizeof(cl_mem), &slot.normalBuffer),
                      "clSetKernelArg(4)");
        requireOpenCl(clSetKernelArg(
                          slot.kernel, 5U, sizeof(cl_mem), &slot.velocityBuffer),
                      "clSetKernelArg(5)");
        requireOpenCl(clSetKernelArg(
                          slot.kernel, 6U, sizeof(cl_uint), &impl_->width),
                      "clSetKernelArg(6)");
        requireOpenCl(clSetKernelArg(
                          slot.kernel, 7U, sizeof(cl_uint), &impl_->height),
                      "clSetKernelArg(7)");
    }
}

ImplicitFrameProducer::~ImplicitFrameProducer() = default;

ImplicitFrameProducer::ExternalImageImports::~ExternalImageImports() { reset(); }

ImplicitFrameProducer::ExternalImageImports::ExternalImageImports(
    ExternalImageImports&& other) noexcept : slots_(other.slots_) {
    other.slots_ = {};
}

ImplicitFrameProducer::ExternalImageImports&
ImplicitFrameProducer::ExternalImageImports::operator=(ExternalImageImports&& other) noexcept {
    if (this != &other) {
        reset();
        slots_ = other.slots_;
        other.slots_ = {};
    }
    return *this;
}

void ImplicitFrameProducer::ExternalImageImports::reset() noexcept {
    const auto closeOwned = [](int& fd) {
        // Linux close must not be retried after EINTR: the descriptor number
        // may already have been recycled by another thread.
        if (fd >= 0) ::close(std::exchange(fd, -1));
    };
    for (auto& slot : slots_) {
        closeOwned(slot.colorFd);
        closeOwned(slot.depthFd);
        closeOwned(slot.normalFd);
        closeOwned(slot.velocityFd);
        closeOwned(slot.vulkanReleaseSemaphoreFd);
        closeOwned(slot.openClReadySemaphoreFd);
    }
}

bool ImplicitFrameProducer::update(const SculptureProgramParams& params,
    float time, float morph, float deltaSeconds, std::uint32_t generation) {
    return implicitSubmissionHealthy(submit(params, time, morph, deltaSeconds, generation));
}

bool ImplicitFrameProducer::updateExternal(const SculptureProgramParams& params,
    float time, float morph, float deltaSeconds, std::uint32_t generation,
    std::uint32_t slotIndex) {
    return implicitSubmissionHealthy(submitExternal(params, time, morph, deltaSeconds, generation, slotIndex));
}

ImplicitSubmissionResult ImplicitFrameProducer::submit(const SculptureProgramParams& params,
    float time, float morph, float deltaSeconds, std::uint32_t generation) {
    ImplicitFrameRequest request{};
    if (!makeImplicitFrameRequest(params, time, morph, deltaSeconds, generation,
            impl_->width, impl_->height, impl_->camera, request, diagnostic_))
        return ImplicitSubmissionResult::Failed;
    return submitRequest(request);
}

ImplicitSubmissionResult ImplicitFrameProducer::submitExternal(const SculptureProgramParams& params,
    float time, float morph, float deltaSeconds, std::uint32_t generation,
    std::uint32_t slotIndex) {
    ImplicitFrameRequest request{};
    if (!makeImplicitFrameRequest(params, time, morph, deltaSeconds, generation,
            impl_->width, impl_->height, impl_->camera, request, diagnostic_))
        return ImplicitSubmissionResult::Failed;
    return submitExternalRequest(request, slotIndex);
}

ImplicitSubmissionResult ImplicitFrameProducer::submitRequest(const ImplicitFrameRequest& request) {
    if (impl_->zeroCopyEnabled) {
        diagnostic_ = "external implicit transport requires an explicit released slot";
        return ImplicitSubmissionResult::Failed;
    }
    return updateIntoSlot(request, std::nullopt);
}

ImplicitSubmissionResult ImplicitFrameProducer::submitExternalRequest(
    const ImplicitFrameRequest& request, std::uint32_t slotIndex) {
    if (!impl_->zeroCopyEnabled || slotIndex >= kSlotCount) {
        diagnostic_ = "external implicit update requires an imported color slot";
        return ImplicitSubmissionResult::Failed;
    }
    return updateIntoSlot(request, slotIndex);
}

std::optional<std::uint32_t>
ImplicitFrameProducer::nextExternalSlotIndex() const noexcept {
    if (!impl_->zeroCopyEnabled) return std::nullopt;
    const auto slot = impl_->ring.nextFreeIndex();
    return slot ? std::optional<std::uint32_t>(static_cast<std::uint32_t>(*slot))
                : std::nullopt;
}

bool ImplicitFrameProducer::pollCompleted() {
    if (impl_->failed) return false;

    impl_->telemetry.delivery = ImplicitFrameDelivery::NoFrame;
    if (!impl_->poll(diagnostic_)) return false;
    const bool deliveredFresh = impl_->deliverNewest();
    if (!deliveredFresh && impl_->ring.deliveredIndex()) {
        impl_->telemetry.delivery = ImplicitFrameDelivery::Reused;
        ++impl_->telemetry.staleFrameReuses;
    }
    return true;
}

ImplicitSubmissionResult ImplicitFrameProducer::updateIntoSlot(
    const ImplicitFrameRequest& request, std::optional<std::size_t> requestedSlot) {
    if (!validateImplicitFrameRequest(request, diagnostic_))
        return ImplicitSubmissionResult::Failed;
    if (request.width != impl_->width || request.height != impl_->height) {
        diagnostic_ = "implicit request extent differs from producer allocation";
        return ImplicitSubmissionResult::Failed;
    }
    if (!pollCompleted()) return ImplicitSubmissionResult::Failed;
    const auto generation = request.frame.generation;

    if (impl_->hasRequestedGeneration
        && !ImplicitDeliveryRing<kSlotCount>::generationNewer(
            generation, impl_->telemetry.requestedGeneration)) {
        diagnostic_ = "ignored non-new implicit generation "
            + std::to_string(generation);
        return ImplicitSubmissionResult::Superseded;
    }
    std::optional<std::size_t> slotIndex;
    if (requestedSlot) {
        if (impl_->ring.acquireSpecificForProduction(*requestedSlot, generation)) {
            slotIndex = requestedSlot;
        }
    } else {
        slotIndex = impl_->ring.acquireForProduction(generation);
    }
    if (!slotIndex) {
        ++impl_->telemetry.droppedSubmissions;
        impl_->telemetry.pendingSlots = impl_->ring.pendingCount();
        diagnostic_ = requestedSlot
            ? "implicit external submission dropped; requested slot is busy"
            : "implicit submission dropped; all staging slots are busy";
        // Backpressure is not a device failure. Resetting this ledger would
        // allow a new request to overwrite storage still borrowed by the GPU.
        // Keep polling completions; transport recovery owns explicit teardown.
        return ImplicitSubmissionResult::Backpressure;
    }

    // Generation denotes an admitted mathematical production step. Do not
    // advance the producer's monotonic generation ledger merely because an
    // attempt reached the producer; a full ring is backpressure, not history.
    impl_->hasRequestedGeneration = true;
    impl_->telemetry.requestedGeneration = generation;

    auto& slot = impl_->slots[*slotIndex];
    slot.request = request;
    const auto check = [this](cl_int result, const char* operation) {
        return !impl_->fail(result, operation, diagnostic_);
    };
    const auto setArg = [&slot, &check](cl_uint index, size_t size,
                                        const void* value) {
        const std::string operation = "clSetKernelArg(" + std::to_string(index) + ")";
        return check(clSetKernelArg(slot.kernel, index, size, value),
                     operation.c_str());
    };

    // Import publishes a complete resource set without mutating kernel state.
    // Rebind once, before any commands borrow it, while its export owner is alive.
    if (slot.outputBindingsDirty) {
        if (!setArg(2U, sizeof(cl_mem), &slot.colorBuffer)
            || !setArg(3U, sizeof(cl_mem), &slot.depthBuffer)
            || !setArg(4U, sizeof(cl_mem), &slot.normalBuffer)
            || !setArg(5U, sizeof(cl_mem), &slot.velocityBuffer)) return ImplicitSubmissionResult::Failed;
        slot.outputBindingsDirty = false;
    }

    constexpr float keyLightDirection[4] = {1.0f, 1.0f, 1.0f, 0.0f};
    constexpr float keyLightRadiance[4] = {1.0f, 1.0f, 1.0f, 0.0f};
    const cl_uint maxSteps = impl_->qualityPolicy.raySteps();
    constexpr float hitEpsilon = 0.001f;
    constexpr float maxDistance = kImplicitMaximumDistance;
    if (!setArg(8U, sizeof(float) * 4U, request.cameraPosition.data())
        || !setArg(9U, sizeof(float) * 4U, request.cameraTarget.data())
        || !setArg(10U, sizeof(float) * 4U, request.cameraUp.data())
        || !setArg(11U, sizeof(request.fovYRadians), &request.fovYRadians)
        || !setArg(12U, sizeof(keyLightDirection), keyLightDirection)
        || !setArg(13U, sizeof(keyLightRadiance), keyLightRadiance)
        || !setArg(14U, sizeof(maxSteps), &maxSteps)
        || !setArg(15U, sizeof(hitEpsilon), &hitEpsilon)
        || !setArg(16U, sizeof(maxDistance), &maxDistance)
        || !setArg(17U, sizeof(request.eyeSeparation),
                   &request.eyeSeparation)) {
        return ImplicitSubmissionResult::Failed;
    }

    if (!check(clEnqueueWriteBuffer(
                   impl_->queue, slot.paramsBuffer, CL_FALSE, 0U,
                   sizeof(request.params), &slot.request.params,
                   0U, nullptr, &slot.firstEvent),
               "clEnqueueWriteBuffer(implicit program)")) return ImplicitSubmissionResult::Failed;
    if (!check(clEnqueueWriteBuffer(
                   impl_->queue, slot.frameBuffer, CL_FALSE, 0U,
                   sizeof(request.frame), &slot.request.frame,
                   0U, nullptr, nullptr),
               "clEnqueueWriteBuffer(implicit frame)")) return ImplicitSubmissionResult::Failed;

    if (impl_->zeroCopyEnabled && slot.waitSemaphore) {
        if (!check(impl_->pfn_clEnqueueWaitSemaphoresKHR(
            impl_->queue, 1, &slot.waitSemaphore, nullptr, 0, nullptr, nullptr),
            "clEnqueueWaitSemaphoresKHR")) return ImplicitSubmissionResult::Failed;
    }

    const std::array externalImages{slot.colorBuffer, slot.depthBuffer,
        slot.normalBuffer, slot.velocityBuffer};
    if (impl_->zeroCopyEnabled
        && !check(impl_->acquireExternalMemory(impl_->queue,
            static_cast<cl_uint>(externalImages.size()), externalImages.data(), 0, nullptr, nullptr),
            "clEnqueueAcquireExternalMemObjectsKHR(implicit)")) return ImplicitSubmissionResult::Failed;

    const size_t globalWorkSize[2] = {request.width, request.height};
    if (!check(clEnqueueNDRangeKernel(
                   impl_->queue, slot.kernel, 2U, nullptr, globalWorkSize,
                   nullptr, 0U, nullptr, &slot.kernelEvent),
               "clEnqueueNDRangeKernel(implicit field)")) return ImplicitSubmissionResult::Failed;

    if (impl_->zeroCopyEnabled && slot.signalSemaphore) {
        // In-order queue: the signal must follow release of every output image,
        // not merely the kernel event. Vulkan then acquires and copies the set.
        if (!check(impl_->releaseExternalMemory(impl_->queue,
            static_cast<cl_uint>(externalImages.size()), externalImages.data(), 0, nullptr, nullptr),
            "clEnqueueReleaseExternalMemObjectsKHR(implicit)")) return ImplicitSubmissionResult::Failed;
        if (!check(impl_->pfn_clEnqueueSignalSemaphoresKHR(
            impl_->queue, 1, &slot.signalSemaphore, nullptr, 0, nullptr,
            &slot.completionEvent),
            "clEnqueueSignalSemaphoresKHR")) return ImplicitSubmissionResult::Failed;
    }

    if (!impl_->zeroCopyEnabled) {
        const size_t origin[3] = {0, 0, 0};
        const size_t region[3] = {request.width, request.height, 1};
        const bool hasDepth = !slot.output.depth.empty();
        const bool hasNormal = !slot.output.normal.empty();
        const bool hasVelocity = !slot.output.velocity.empty();

        cl_event* colorEvent = (!hasDepth && !hasNormal && !hasVelocity) ? &slot.completionEvent : nullptr;
        cl_event* depthEvent = (hasDepth && !hasNormal && !hasVelocity) ? &slot.completionEvent : nullptr;
        cl_event* normalEvent = (hasNormal && !hasVelocity) ? &slot.completionEvent : nullptr;
        cl_event* velocityEvent = hasVelocity ? &slot.completionEvent : nullptr;

        if (!check(clEnqueueReadImage(
                       impl_->queue, slot.colorBuffer, CL_FALSE, origin, region,
                       0U, 0U, slot.output.color.data(), 0U, nullptr,
                       colorEvent),
                   "clEnqueueReadImage(implicit color)")) return ImplicitSubmissionResult::Failed;
        if (hasDepth
            && !check(clEnqueueReadImage(
                          impl_->queue, slot.depthBuffer, CL_FALSE, origin, region,
                          0U, 0U, slot.output.depth.data(), 0U, nullptr,
                          depthEvent),
                      "clEnqueueReadImage(implicit depth)")) return ImplicitSubmissionResult::Failed;
        if (hasNormal
            && !check(clEnqueueReadImage(
                          impl_->queue, slot.normalBuffer, CL_FALSE, origin, region,
                          0U, 0U, slot.output.normal.data(), 0U, nullptr,
                          normalEvent),
                      "clEnqueueReadImage(implicit normal)")) return ImplicitSubmissionResult::Failed;
        if (hasVelocity
            && !check(clEnqueueReadImage(
                          impl_->queue, slot.velocityBuffer, CL_FALSE, origin, region,
                          0U, 0U, slot.output.velocity.data(), 0U, nullptr,
                          velocityEvent),
                      "clEnqueueReadImage(implicit velocity)")) return ImplicitSubmissionResult::Failed;
    } else if (slot.completionEvent == nullptr) {
        // The signal command's event is the completion fence. A marker that
        // waits only for the kernel could publish the slot before Vulkan's
        // matching OpenCL-ready semaphore is actually signalled.
        if (!check(clEnqueueMarkerWithWaitList(
                       impl_->queue, 1, &slot.kernelEvent,
                       &slot.completionEvent),
                   "clEnqueueMarkerWithWaitList(external implicit)")) {
            return ImplicitSubmissionResult::Failed;
        }
    }
    if (!check(clFlush(impl_->queue), "clFlush(implicit producer)")) return ImplicitSubmissionResult::Failed;

    ++impl_->telemetry.submittedRequests;
    impl_->telemetry.pendingSlots = impl_->ring.pendingCount();
    diagnostic_ = "submitted implicit generation " + std::to_string(generation);
    return ImplicitSubmissionResult::Submitted;
}

bool ImplicitFrameProducer::waitForGeneration(std::uint32_t generation) {
    if (impl_->failed) return false;
    if (impl_->fail(clFinish(impl_->queue), "clFinish(implicit offline wait)",
                    diagnostic_)) {
        return false;
    }
    if (!impl_->poll(diagnostic_)) return false;
    (void)impl_->deliverNewest();
    const std::uint32_t delivered = impl_->ring.deliveredGeneration();
    if (delivered != generation
        && !ImplicitDeliveryRing<kSlotCount>::generationNewer(
            delivered, generation)) {
        diagnostic_ = "implicit generation " + std::to_string(generation)
            + " did not complete";
        return false;
    }
    diagnostic_ = "completed implicit generation " + std::to_string(delivered);
    return true;
}

void ImplicitFrameProducer::setCamera(const ImplicitCamera& camera) {
    impl_->camera = camera;
}

void ImplicitFrameProducer::setCamera(
    float px, float py, float pz,
    float tx, float ty, float tz,
    float ux, float uy, float uz,
    float fovYRadians,
    float eyeSeparation) {
    impl_->camera.position = {px, py, pz};
    impl_->camera.target = {tx, ty, tz};
    impl_->camera.up = {ux, uy, uz};
    impl_->camera.fovYRadians = fovYRadians;
    impl_->camera.eyeSeparation = eyeSeparation;
}

uint32_t ImplicitFrameProducer::width() const { return impl_->width; }
std::optional<ImplicitFrameRequest> ImplicitFrameProducer::deliveredRequest() const noexcept {
    const auto delivered = impl_->ring.deliveredIndex();
    if (!delivered) return std::nullopt;
    return impl_->slots[*delivered].request;
}
uint32_t ImplicitFrameProducer::height() const { return impl_->height; }

const ImplicitFrameProducer::OutputBuffers&
ImplicitFrameProducer::getOutput() const {
    static const OutputBuffers empty{};
    const auto delivered = impl_->ring.deliveredIndex();
    return delivered ? impl_->slots[*delivered].output : empty;
}

ImplicitExternalFrameView ImplicitFrameProducer::externalFrameView() const noexcept {
    if (!impl_->zeroCopyEnabled) return {};
    const auto delivered = impl_->ring.deliveredIndex();
    if (!delivered) return {};
    const auto& slot = impl_->slots[*delivered];
    return {
        .slotIndex = slot.externalColor.slotIndex,
        .allocationGeneration = slot.externalColor.allocationGeneration,
        .width = impl_->width,
        .height = impl_->height,
        .generation = impl_->ring.deliveredGeneration(),
        .format = slot.externalColor.format,
    };
}

bool ImplicitFrameProducer::releaseExternalFrameAfterVulkanSubmit(
    const ImplicitExternalFrameView& frame) noexcept {
    if (!impl_->zeroCopyEnabled || !frame.valid()) return false;
    const auto delivered = impl_->ring.deliveredIndex();
    if (!delivered) return false;
    const auto& slot = impl_->slots[*delivered];
    if (frame.slotIndex != slot.externalColor.slotIndex
        || frame.allocationGeneration != slot.externalColor.allocationGeneration
        || frame.width != impl_->width || frame.height != impl_->height
        || frame.format != slot.externalColor.format) {
        return false;
    }
    return impl_->ring.releaseDelivered(frame.generation);
}

const ImplicitProducerTelemetry&
ImplicitFrameProducer::telemetry() const noexcept {
    return impl_->telemetry;
}

const ImplicitQualityPolicy&
ImplicitFrameProducer::qualityPolicy() const noexcept {
    return impl_->qualityPolicy;
}

bool ImplicitFrameProducer::hasValidOutput() const noexcept {
    return impl_->ring.deliveredIndex().has_value();
}

bool ImplicitFrameProducer::hasCompleteZeroCopyBufferSet(
    std::span<const ZeroCopyBuffer> buffers,
    std::uint32_t expectedWidth,
    std::uint32_t expectedHeight) noexcept {
    if (buffers.size() != kSlotCount || expectedWidth == 0U || expectedHeight == 0U) {
        return false;
    }
    const std::size_t expectedBytes = implicitRgba32ColorBytes(expectedWidth, expectedHeight);
    if (expectedBytes == 0U) return false;
    const std::size_t expectedDepth = expectedBytes / 4U;
    const std::size_t expectedNormal = expectedBytes;
    const std::size_t expectedVelocity = expectedBytes / 2U;

    std::array<bool, kSlotCount> seen{};
    std::uint64_t allocationGeneration = 0U;
    for (const ZeroCopyBuffer& buffer : buffers) {
        const ImplicitExternalColorSlot slot = buffer.contract();
        if (slot.slotIndex >= kSlotCount || seen[slot.slotIndex]
            || !slot.hasImportHandles() || slot.allocationGeneration == 0U
            || slot.width != expectedWidth || slot.height != expectedHeight
            || slot.format != ImplicitExternalColorFormat::Rgba32Float
            || slot.colorAllocationSizeBytes < expectedBytes
            || slot.depthSizeBytes < expectedDepth
            || slot.normalSizeBytes < expectedNormal
            || slot.velocitySizeBytes < expectedVelocity) {
            return false;
        }
        if (allocationGeneration == 0U) allocationGeneration = slot.allocationGeneration;
        if (slot.allocationGeneration != allocationGeneration) return false;
        seen[slot.slotIndex] = true;
    }
    return true;
}

bool ImplicitFrameProducer::importZeroCopyBuffers(ExternalImageImports imports) {
    auto& buffers = imports.slots_;
    if (impl_->hasRequestedGeneration || impl_->zeroCopyEnabled || impl_->failed) {
        // Rejection does not change an already-active producer's transport state.
        diagnostic_ = "external implicit images require a fresh, idle producer";
        return false;
    }
    if (!hasCompleteZeroCopyBufferSet(buffers, impl_->width, impl_->height)) {
        impl_->telemetry.interop.transitionToHostStaged(SculptureInteropEvent::IncompleteSlotContract);
        diagnostic_ = "invalid external implicit color-slot contract";
        return false;
    }
    if (!impl_->pfn_clCreateSemaphoreWithPropertiesKHR
        || !impl_->pfn_clEnqueueWaitSemaphoresKHR
        || !impl_->pfn_clEnqueueSignalSemaphoresKHR
        || !impl_->pfn_clReleaseSemaphoreKHR
        || !impl_->acquireExternalMemory || !impl_->releaseExternalMemory) {
        impl_->telemetry.interop.noteHostStaged(SculptureInteropEvent::CapabilityHostStaged);
        diagnostic_ = "OpenCL external memory/semaphore entry points missing for implicit transport";
        return false;
    }
    if (auto rejection = implicitImageImportRejection(impl_->device); !rejection.empty()) {
        impl_->telemetry.interop.noteHostStaged(SculptureInteropEvent::CapabilityHostStaged);
        diagnostic_ = std::move(rejection);
        return false;
    }

    struct ImportedSlot {
        cl_mem color = nullptr;
        cl_mem depth = nullptr;
        cl_mem normal = nullptr;
        cl_mem velocity = nullptr;
        cl_semaphore_khr wait = nullptr;
        cl_semaphore_khr signal = nullptr;
    };
    struct ImportTransaction {
        std::array<ImportedSlot, kSlotCount> slots{};
        clReleaseSemaphoreKHR_fn releaseSemaphore = nullptr;

        ~ImportTransaction() {
            for (auto& slot : slots) {
                if (slot.color) clReleaseMemObject(slot.color);
                if (slot.depth) clReleaseMemObject(slot.depth);
                if (slot.normal) clReleaseMemObject(slot.normal);
                if (slot.velocity) clReleaseMemObject(slot.velocity);
                if (slot.wait) releaseSemaphore(slot.wait);
                if (slot.signal) releaseSemaphore(slot.signal);
            }
        }
    };
    ImportTransaction transaction{{}, impl_->pfn_clReleaseSemaphoreKHR};
    auto& imported = transaction.slots;
    const auto rejectImport = [this](const char* reason) {
        impl_->telemetry.interop.noteHostStaged(SculptureInteropEvent::ImportFailed);
        diagnostic_ = reason;
        return false;
    };

    // Only successful imports consume an FD. The batch closes all remaining
    // exports on every exit; imported objects own the successfully consumed ones.
    std::string successDiagnostic = "external implicit images imported; driver-defined layout; awaiting presentation";
    for (ZeroCopyBuffer& buf : buffers) {
        const std::size_t i = buf.slotIndex;
        cl_mem_properties colorProps[] = {
            (cl_mem_properties)CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR,
            (cl_mem_properties)buf.colorFd,
            0
        };
        cl_int err;
        
        cl_image_format format = {};
        format.image_channel_order = CL_RGBA;
        format.image_channel_data_type = CL_FLOAT;
        
        cl_image_desc desc = {};
        desc.image_type = CL_MEM_OBJECT_IMAGE2D;
        desc.image_width = buf.width;
        desc.image_height = buf.height;
        desc.image_row_pitch = 0; // Handled by implementation for imported FD
        
        imported[i].color = clCreateImageWithProperties(
            impl_->context, colorProps, CL_MEM_WRITE_ONLY, &format, &desc, nullptr, &err);
        if (err != CL_SUCCESS || !imported[i].color) {
            return rejectImport("failed to import external implicit color image FD");
        }
        buf.colorFd = -1;
            
        cl_mem_properties depthProps[] = {
            (cl_mem_properties)CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR,
            (cl_mem_properties)buf.depthFd, 0
        };
        cl_image_format depthFormat = {};
        depthFormat.image_channel_order = CL_R;
        depthFormat.image_channel_data_type = CL_FLOAT;
        imported[i].depth = clCreateImageWithProperties(
            impl_->context, depthProps, CL_MEM_WRITE_ONLY, &depthFormat, &desc, nullptr, &err);
        if (err != CL_SUCCESS || !imported[i].depth) {
            return rejectImport("failed to import external implicit depth image FD");
        }
        buf.depthFd = -1;

        cl_mem_properties normalProps[] = {
            (cl_mem_properties)CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR,
            (cl_mem_properties)buf.normalFd, 0
        };
        imported[i].normal = clCreateImageWithProperties(
            impl_->context, normalProps, CL_MEM_WRITE_ONLY, &format, &desc, nullptr, &err);
        if (err != CL_SUCCESS || !imported[i].normal) {
            return rejectImport("failed to import external implicit normal image FD");
        }
        buf.normalFd = -1;

        cl_mem_properties velocityProps[] = {
            (cl_mem_properties)CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR,
            (cl_mem_properties)buf.velocityFd, 0
        };
        cl_image_format velocityFormat = {};
        velocityFormat.image_channel_order = CL_RG;
        velocityFormat.image_channel_data_type = CL_FLOAT;
        imported[i].velocity = clCreateImageWithProperties(
            impl_->context, velocityProps, CL_MEM_WRITE_ONLY, &velocityFormat, &desc, nullptr, &err);
            
        if (err != CL_SUCCESS || !imported[i].velocity) {
            return rejectImport("failed to import external implicit velocity image FD");
        }
        buf.velocityFd = -1;

        cl_semaphore_properties_khr waitProps[] = {
            CL_SEMAPHORE_TYPE_KHR, CL_SEMAPHORE_TYPE_BINARY_KHR,
            CL_SEMAPHORE_HANDLE_OPAQUE_FD_KHR, (cl_semaphore_properties_khr)buf.vulkanReleaseSemaphoreFd,
            0
        };
        imported[i].wait = impl_->pfn_clCreateSemaphoreWithPropertiesKHR(
            impl_->context, waitProps, &err);
        if (err != CL_SUCCESS || !imported[i].wait) {
            return rejectImport("failed to import Vulkan-release semaphore FD");
        }
        buf.vulkanReleaseSemaphoreFd = -1;
        
        cl_semaphore_properties_khr signalProps[] = {
            CL_SEMAPHORE_TYPE_KHR, CL_SEMAPHORE_TYPE_BINARY_KHR,
            CL_SEMAPHORE_HANDLE_OPAQUE_FD_KHR, (cl_semaphore_properties_khr)buf.openClReadySemaphoreFd,
            0
        };
        imported[i].signal = impl_->pfn_clCreateSemaphoreWithPropertiesKHR(
            impl_->context, signalProps, &err);
        if (err != CL_SUCCESS || !imported[i].signal) {
            return rejectImport("failed to import OpenCL-ready semaphore FD");
        }
        buf.openClReadySemaphoreFd = -1;
    }

    // Commit only after every external object exists. Kernel rebinding belongs
    // to the next dispatch, so it cannot leave this import partially published.
    for (const ZeroCopyBuffer& buf : buffers) {
        auto& slot = impl_->slots[buf.slotIndex];
        if (slot.colorBuffer) clReleaseMemObject(slot.colorBuffer);
        if (slot.depthBuffer) clReleaseMemObject(slot.depthBuffer);
        if (slot.normalBuffer) clReleaseMemObject(slot.normalBuffer);
        if (slot.velocityBuffer) clReleaseMemObject(slot.velocityBuffer);
        if (impl_->pfn_clReleaseSemaphoreKHR && slot.waitSemaphore) {
            impl_->pfn_clReleaseSemaphoreKHR(slot.waitSemaphore);
        }
        if (impl_->pfn_clReleaseSemaphoreKHR && slot.signalSemaphore) {
            impl_->pfn_clReleaseSemaphoreKHR(slot.signalSemaphore);
        }
        slot.colorBuffer = imported[buf.slotIndex].color;
        slot.depthBuffer = imported[buf.slotIndex].depth;
        slot.normalBuffer = imported[buf.slotIndex].normal;
        slot.velocityBuffer = imported[buf.slotIndex].velocity;
        slot.waitSemaphore = imported[buf.slotIndex].wait;
        slot.signalSemaphore = imported[buf.slotIndex].signal;
        slot.externalColor = buf.contract();
        imported[buf.slotIndex] = {};
        
        slot.outputBindingsDirty = true;
        // Explicitly imported slots have no host readback. Reclaim the
        // preallocated fallback payload so enabling GPU transport cannot hide
        // a second three-frame HDR allocation on the CPU.
        std::vector<float>{}.swap(slot.output.color);
        std::vector<float>{}.swap(slot.output.depth);
        std::vector<float>{}.swap(slot.output.normal);
        std::vector<float>{}.swap(slot.output.velocity);
    }
    impl_->zeroCopyEnabled = true;
    impl_->failed = false;
    impl_->telemetry.stagedColorBytes = 0U;
    impl_->telemetry.stagedAuxiliaryBytes = 0U;
    impl_->telemetry.interop.markActivated();
    diagnostic_ = std::move(successDiagnostic);
    return true;
}

} // namespace epi_sculpt
