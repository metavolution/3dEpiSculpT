#pragma once

#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <string>
#include <stdexcept>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: OPENCL DEVICE & EXTENSION MANAGER
// -----------------------------------------------------------------------------
// This dynamic component encapsulates an OpenCL context and its associated
// command queue. It automatically resolves the KHR external semaphore extensions
// required for Zero-Copy Vulkan interop, ensuring that any Dispatcher subclass
// simply works without repeating platform queries.
// -----------------------------------------------------------------------------
class SculptureOpenClContext {
public:
    SculptureOpenClContext(cl_context context, cl_device_id device, const cl_queue_properties* queueProperties = nullptr);
    ~SculptureOpenClContext();

    SculptureOpenClContext(const SculptureOpenClContext&) = delete;
    SculptureOpenClContext& operator=(const SculptureOpenClContext&) = delete;

    [[nodiscard]] cl_context context() const noexcept { return context_; }
    [[nodiscard]] cl_device_id device() const noexcept { return device_; }
    [[nodiscard]] cl_command_queue queue() const noexcept { return queue_; }

    [[nodiscard]] clCreateSemaphoreWithPropertiesKHR_fn createSemaphore() const noexcept { return createSemaphore_; }
    [[nodiscard]] clEnqueueWaitSemaphoresKHR_fn waitSemaphores() const noexcept { return waitSemaphores_; }
    [[nodiscard]] clEnqueueSignalSemaphoresKHR_fn signalSemaphores() const noexcept { return signalSemaphores_; }
    [[nodiscard]] clReleaseSemaphoreKHR_fn releaseSemaphore() const noexcept { return releaseSemaphore_; }
    
    void check(cl_int result, const char* message) const;

private:
    cl_context context_ = nullptr;
    cl_device_id device_ = nullptr;
    cl_command_queue queue_ = nullptr;
    
    clCreateSemaphoreWithPropertiesKHR_fn createSemaphore_ = nullptr;
    clEnqueueWaitSemaphoresKHR_fn waitSemaphores_ = nullptr;
    clEnqueueSignalSemaphoresKHR_fn signalSemaphores_ = nullptr;
    clReleaseSemaphoreKHR_fn releaseSemaphore_ = nullptr;
};

} // namespace epi_sculpt
