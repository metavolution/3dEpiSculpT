#pragma once

#include <cstdint>

namespace epi_sculpt {

// Host-side facts about one completed OpenCL world generation. This is kept
// outside the GPU ABI: timing is observational metadata, not kernel input.
struct WorldGenerationMetrics {
    static constexpr float kUnavailableMilliseconds = -1.0f;
    static constexpr std::uint32_t kNoExternalSlot = 0xFFFFFFFFU;

    std::uint32_t generation = 0U;
    // Identity of the Vulkan/OpenCL shared output buffer that completed this
    // generation. It is delivery metadata, never a kernel input or CPU copy.
    std::uint32_t externalSlotIndex = kNoExternalSlot;
    std::uint32_t activeInstanceCount = 0U;
    // External worlds read only the 64-byte frame/recipe header, not instances.
    std::uint32_t readbackBytes = 0U;
    float queueToDeliveryMilliseconds = kUnavailableMilliseconds;
    float kernelMilliseconds = kUnavailableMilliseconds;
    float readbackMilliseconds = kUnavailableMilliseconds;
    bool gpuVolumetricCoupled = false;

    [[nodiscard]] bool matches(std::uint32_t candidate) const noexcept {
        return generation != 0U && generation == candidate;
    }
};

// Presentation owns this companion record. It never mutates the immutable
// OpenCL delivery, so renderer work remains separately attributable.
struct WorldBridgeMetrics {
    static constexpr float kUnavailableMilliseconds = -1.0f;

    std::uint32_t generation = 0U;
    std::uint32_t sourceInstanceCount = 0U;
    std::uint32_t acceptedInstanceCount = 0U;
    std::uint32_t activeBatchCount = 0U;
    std::uint32_t droppedInstanceCount = 0U;
    float rebuildMilliseconds = kUnavailableMilliseconds;

    [[nodiscard]] bool matches(std::uint32_t candidate) const noexcept {
        return generation != 0U && generation == candidate;
    }
};

// Application-side presentation submission facts for one accepted generation.
// This is deliberately CPU wall time around the mGFX/Vulkan submission call;
// it does not wait for GPU completion or claim to be a GPU timestamp.
struct WorldPresentationMetrics {
    static constexpr float kUnavailableMilliseconds = -1.0f;

    std::uint32_t generation = 0U;
    float submitCpuMilliseconds = kUnavailableMilliseconds;

    [[nodiscard]] bool matches(std::uint32_t candidate) const noexcept {
        return generation != 0U && generation == candidate;
    }
};

} // namespace epi_sculpt
