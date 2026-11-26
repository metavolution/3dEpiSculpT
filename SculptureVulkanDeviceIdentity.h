// Shared Vulkan -> OpenCL physical-device identity bridge.
//
// Vulkan exposes several UUIDs.  Only VkPhysicalDeviceIDProperties::deviceUUID
// identifies the physical adapter and can be compared with CL_DEVICE_UUID_KHR;
// pipelineCacheUUID identifies a driver cache format and is not interchangeable.

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <vulkan/vulkan.h>

#include <mgfx/vk/Context.h>

namespace epi_sculpt {

using VulkanDeviceUuid = mgfx::vk::PhysicalDeviceUuid;

[[nodiscard]] inline VulkanDeviceUuid queryVulkanDeviceUuid(
    VkPhysicalDevice physicalDevice) noexcept {
    return mgfx::vk::queryPhysicalDeviceUuid(physicalDevice);
}

// Human-readable provenance is emitted with the immutable UUID. The name is
// useful to an operator; the UUID is the stable identity used for comparison.
[[nodiscard]] inline std::string formatVulkanDeviceUuid(
    const VulkanDeviceUuid& uuid) {
    char encoded[VK_UUID_SIZE * 2U + 1U]{};
    for (std::size_t index = 0U; index < uuid.size(); ++index) {
        std::snprintf(encoded + index * 2U, 3U, "%02x",
                      static_cast<unsigned int>(uuid[index]));
    }
    return encoded;
}

[[nodiscard]] inline std::string describeVulkanPhysicalDevice(
    VkPhysicalDevice physicalDevice) {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    return std::string(properties.deviceName) + " [uuid="
        + formatVulkanDeviceUuid(queryVulkanDeviceUuid(physicalDevice)) + "]";
}

// Both explicit-mesh and implicit producers expose these two configuration
// fields.  Keeping the adapter query here prevents demos from accidentally
// matching OpenCL against a Vulkan driver or pipeline-cache identifier.
template <typename ProducerConfig>
inline void matchOpenClToVulkanDevice(
    ProducerConfig& config,
    VkPhysicalDevice physicalDevice) noexcept {
    config.requiredDeviceUuid = queryVulkanDeviceUuid(physicalDevice);
    config.matchDeviceUuid = true;
}

struct OpenClContextOwner {
    cl_context context = nullptr;

    OpenClContextOwner() noexcept = default;
    explicit OpenClContextOwner(cl_context c) noexcept : context(c) {}
    ~OpenClContextOwner() {
        if (context != nullptr) {
            clReleaseContext(context);
            context = nullptr;
        }
    }

    OpenClContextOwner(const OpenClContextOwner&) = delete;
    OpenClContextOwner& operator=(const OpenClContextOwner&) = delete;

    OpenClContextOwner(OpenClContextOwner&& other) noexcept : context(other.context) {
        other.context = nullptr;
    }

    OpenClContextOwner& operator=(OpenClContextOwner&& other) noexcept {
        if (this != &other) {
            if (context != nullptr) {
                clReleaseContext(context);
            }
            context = other.context;
            other.context = nullptr;
        }
        return *this;
    }
};

inline OpenClContextOwner makeMatchedOpenClContext(
    VkPhysicalDevice physicalDevice,
    cl_device_id& outDevice) {
    const auto requiredUuid=queryVulkanDeviceUuid(physicalDevice);
    const auto check=[](cl_int result,const char* operation) {
        if(result!=CL_SUCCESS)throw std::runtime_error(std::string(operation)+" failed with OpenCL error "+std::to_string(result));
    };
    cl_uint platformCount=0;
    check(clGetPlatformIDs(0,nullptr,&platformCount),"clGetPlatformIDs(count)");
    std::vector<cl_platform_id> platforms(platformCount);
    if(platformCount)check(clGetPlatformIDs(platformCount,platforms.data(),nullptr),"clGetPlatformIDs(list)");
    for(const cl_device_type type:{CL_DEVICE_TYPE_GPU,CL_DEVICE_TYPE_CPU}) {
        for(const auto platform:platforms) {
            cl_uint count=0;
            const auto result=clGetDeviceIDs(platform,type,0,nullptr,&count);
            if(result==CL_DEVICE_NOT_FOUND)continue;
            check(result,"clGetDeviceIDs(count)");
            if(!count)continue;
            std::vector<cl_device_id> devices(count);
            check(clGetDeviceIDs(platform,type,count,devices.data(),nullptr),"clGetDeviceIDs(list)");
            for(const auto device:devices) {
                cl_uchar uuid[VK_UUID_SIZE]{};
                std::size_t size=0;
                if(clGetDeviceInfo(device,CL_DEVICE_UUID_KHR,sizeof(uuid),uuid,&size)!=CL_SUCCESS ||
                    size!=requiredUuid.size() || std::memcmp(uuid,requiredUuid.data(),size)!=0)continue;
                const cl_context_properties properties[]={CL_CONTEXT_PLATFORM,
                    reinterpret_cast<cl_context_properties>(platform),0};
                cl_int error=CL_SUCCESS;
                const auto context=clCreateContext(properties,1,&device,nullptr,nullptr,&error);
                if(error!=CL_SUCCESS || !context)
                    throw std::runtime_error("failed to create Vulkan-matched OpenCL context (code "+std::to_string(error)+")");
                outDevice=device;
                return OpenClContextOwner(context);
            }
        }
    }
    throw std::runtime_error("no OpenCL GPU or CPU device matches the Vulkan physical-device UUID");
}

} // namespace epi_sculpt

