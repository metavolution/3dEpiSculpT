#include "SculptureOpenClContext.h"
#include <cstdio>
#include <iostream>

namespace epi_sculpt {

SculptureOpenClContext::SculptureOpenClContext(cl_context context, cl_device_id device, const cl_queue_properties* queueProperties)
    : context_(context), device_(device) {
    
    cl_int result = CL_SUCCESS;
    if (queueProperties) {
        queue_ = clCreateCommandQueueWithProperties(context_, device_, queueProperties, &result);
    } else {
        queue_ = clCreateCommandQueue(context_, device_, 0, &result);
    }
    check(result, "clCreateCommandQueue");
    
    cl_platform_id platform = nullptr;
    if (clGetDeviceInfo(device_, CL_DEVICE_PLATFORM, sizeof(platform), &platform, nullptr) == CL_SUCCESS && platform != nullptr) {
        createSemaphore_ = reinterpret_cast<clCreateSemaphoreWithPropertiesKHR_fn>(
            clGetExtensionFunctionAddressForPlatform(platform, "clCreateSemaphoreWithPropertiesKHR"));
        waitSemaphores_ = reinterpret_cast<clEnqueueWaitSemaphoresKHR_fn>(
            clGetExtensionFunctionAddressForPlatform(platform, "clEnqueueWaitSemaphoresKHR"));
        signalSemaphores_ = reinterpret_cast<clEnqueueSignalSemaphoresKHR_fn>(
            clGetExtensionFunctionAddressForPlatform(platform, "clEnqueueSignalSemaphoresKHR"));
        releaseSemaphore_ = reinterpret_cast<clReleaseSemaphoreKHR_fn>(
            clGetExtensionFunctionAddressForPlatform(platform, "clReleaseSemaphoreKHR"));
            
        if (!createSemaphore_ || !waitSemaphores_ || !signalSemaphores_ || !releaseSemaphore_) {
            std::cerr << "[SculptureOpenClContext] Warning: Vulkan External Semaphore extensions are missing." << std::endl;
        }
    }
}

SculptureOpenClContext::~SculptureOpenClContext() {
    if (queue_) {
        clReleaseCommandQueue(queue_);
    }
}

void SculptureOpenClContext::check(cl_int result, const char* message) const {
    if (result != CL_SUCCESS) {
        char buf[256];
        std::snprintf(buf, sizeof(buf), "OpenCL error %d: %s", result, message);
        throw std::runtime_error(buf);
    }
}

} // namespace epi_sculpt
