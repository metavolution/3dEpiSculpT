#include <thread>
#include <chrono>
#include "SculptureGridOpenClDispatcher.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include "SculptureGridVertex.h"

namespace epi_sculpt {

SculptureGridOpenClDispatcher::SculptureGridOpenClDispatcher(cl_context context, cl_device_id device, cl_command_queue commandQueue)
    : context_(context), device_(device), commandQueue_(commandQueue) {
    
    // Shared device parameter buffers are safe only on an in-order queue:
    // upload A → kernel A → upload B → kernel B. Reject other queue contracts.
    cl_command_queue_properties properties = 0;
    if (!context_ || !device_ || !commandQueue_
        || clGetCommandQueueInfo(commandQueue_, CL_QUEUE_PROPERTIES, sizeof(properties),
                                 &properties, nullptr) != CL_SUCCESS
        || (properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE))
        throw std::invalid_argument("Grid dispatcher requires an in-order OpenCL queue");

    cl_int result = CL_SUCCESS;
    paramBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY,
        sizeof(gpu_abi::SculptureProgramParamsGpu), nullptr, &result);
    if (result != CL_SUCCESS || !paramBuffer_) {
        if (paramBuffer_) clReleaseMemObject(paramBuffer_);
        throw std::runtime_error("Grid parameter-buffer allocation failed");
    }
    frameBuffer_ = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY,
        sizeof(gpu_abi::SculptureFrameGpu), nullptr, &result);
    if (result != CL_SUCCESS || !frameBuffer_) {
        if (frameBuffer_) clReleaseMemObject(frameBuffer_);
        clReleaseMemObject(paramBuffer_);
        throw std::runtime_error("Grid frame-buffer allocation failed");
    }
}

SculptureGridOpenClDispatcher::~SculptureGridOpenClDispatcher() {
    hotSwap_.reset();
    // Teardown may block; frame submission never waits for upload completion.
    // Keep host packets alive until every accepted transfer has terminated.
    for (auto& slot : slots_) {
        for (auto& event : slot.uploads) {
            if (!event) continue;
            clWaitForEvents(1, &event);
            clReleaseEvent(event);
            event = nullptr;
        }
        if (slot.buffer) clReleaseMemObject(slot.buffer);
    }
    if (paramBuffer_) clReleaseMemObject(paramBuffer_);
    if (frameBuffer_) clReleaseMemObject(frameBuffer_);
}

void SculptureGridOpenClDispatcher::initializeKernel(const char* sourceCode) {
    if (!hotSwap_) {
        hotSwap_ = std::make_unique<SculptureOpenClHotSwap>(context_, device_, "compute_grid_displacement");
    }
    hotSwap_->requestAsyncCompile(sourceCode);
    while (hotSwap_->isCompiling()) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
}

void SculptureGridOpenClDispatcher::recompileKernelAsync(const char* source, const char* buildOptions) {
    if (hotSwap_) hotSwap_->requestAsyncCompile(source, buildOptions);
}

bool SculptureGridOpenClDispatcher::importExternalOutput(std::uint32_t slotIndex, const SculptureOpenClImportSlot& importSlot) {
    if (slotIndex >= slots_.size()) return false;
    auto& slot = slots_[slotIndex];
    if (slot.buffer || importSlot.memoryFd < 0 || importSlot.byteSize == 0) return false;

    const cl_mem_properties properties[] = {
        CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR, (cl_mem_properties)importSlot.memoryFd, 0
    };
    cl_int result = CL_SUCCESS;
    cl_mem imported = clCreateBufferWithProperties(context_, properties, CL_MEM_WRITE_ONLY,
                                                   importSlot.byteSize, nullptr, &result);
    if (result != CL_SUCCESS || !imported) {
        if (imported) clReleaseMemObject(imported);
        return false;
    }
    slot.buffer = imported;
    slot.byteSize = importSlot.byteSize;
    // External-memory acquire/release semaphores remain a separate contract.
    return true;
}

bool SculptureGridOpenClDispatcher::retireUploads(Slot& slot) noexcept {
    for (auto event : slot.uploads) {
        if (!event) continue;
        cl_int status = CL_QUEUED;
        if (clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(status),
                           &status, nullptr) != CL_SUCCESS || status > CL_COMPLETE)
            return false;
    }
    // Negative execution status is also terminal: the host bytes are no longer
    // borrowed. Both transfers must be terminal before repacking either field.
    for (auto& event : slot.uploads) {
        if (event) clReleaseEvent(event);
        event = nullptr;
    }
    return true;
}

bool SculptureGridOpenClDispatcher::submitGridTransform(
    const SculptureProgramParams& params,
    float time, float morph, std::uint32_t gridResolution, std::uint32_t slotIndex) {
    if (slotIndex >= slots_.size() || gridResolution < 2 || !commandQueue_)
        return false;
    auto& slot = slots_[slotIndex];
    const auto vertices = std::uint64_t{gridResolution} * gridResolution;
    if (!slot.buffer || vertices > std::numeric_limits<cl_uint>::max()
        || vertices > slot.byteSize / sizeof(GridVertex) || !retireUploads(slot))
        return false;
    cl_kernel kernel = hotSwap_ ? hotSwap_->acquireActiveKernel() : nullptr;
    if (!kernel) return false;

    // Bind before queueing host transfers, so rejected arguments leave no work.
    if (clSetKernelArg(kernel, 0, sizeof(cl_mem), &paramBuffer_) != CL_SUCCESS
        || clSetKernelArg(kernel, 1, sizeof(cl_mem), &frameBuffer_) != CL_SUCCESS
        || clSetKernelArg(kernel, 2, sizeof(cl_uint), &gridResolution) != CL_SUCCESS
        || clSetKernelArg(kernel, 3, sizeof(cl_mem), &slot.buffer) != CL_SUCCESS)
        return false;

    slot.params = gpu_abi::packProgramParams(params);
    slot.frame = gpu_abi::packFrame(time, morph, 0.016f, 0);
    cl_int result = clEnqueueWriteBuffer(commandQueue_, paramBuffer_, CL_FALSE, 0,
        sizeof(slot.params), &slot.params, 0, nullptr, &slot.uploads[0]);
    if (result == CL_SUCCESS)
        result = clEnqueueWriteBuffer(commandQueue_, frameBuffer_, CL_FALSE, 0,
            sizeof(slot.frame), &slot.frame, 0, nullptr, &slot.uploads[1]);
    if (result == CL_SUCCESS) {
        const size_t globalWorkSize = static_cast<size_t>(vertices);
        result = clEnqueueNDRangeKernel(commandQueue_, kernel, 1, nullptr,
            &globalWorkSize, nullptr, 2, slot.uploads.data(), nullptr);
    }
    // Even a partial submission retains its upload events and stable packet.
    // Flush accepted work so later nonblocking retirement can make progress.
    const cl_int flushed = clFlush(commandQueue_);
    return result == CL_SUCCESS && flushed == CL_SUCCESS;
}

} // namespace epi_sculpt
