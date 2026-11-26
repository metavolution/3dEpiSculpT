#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace epi_sculpt {

// Renderer-neutral metadata for color plus depth/normal/velocity image slots.
// Raw FD fields are borrowed import-time descriptions, never owners. The
// producer's move-only ExternalImageImports owns their lifecycle; persistent
// delivery metadata keeps these fields invalid after OpenCL consumes the FDs.
enum class ImplicitExternalColorFormat : std::uint8_t {
    Rgba32Float,
};

struct ImplicitExternalColorSlot {
    static constexpr std::uint32_t kInvalidSlot =
        std::numeric_limits<std::uint32_t>::max();

    std::uint32_t slotIndex = kInvalidSlot;
    std::uint64_t allocationGeneration = 0U;
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    ImplicitExternalColorFormat format = ImplicitExternalColorFormat::Rgba32Float;
    std::size_t colorAllocationSizeBytes = 0U;
    std::size_t depthSizeBytes = 0U;
    std::size_t normalSizeBytes = 0U;
    std::size_t velocitySizeBytes = 0U;
    int colorFd = -1;
    int depthFd = -1;
    int normalFd = -1;
    int velocityFd = -1;
    // Vulkan signals this before OpenCL may overwrite the slot. OpenCL signals
    // the second semaphore after the kernel has completed its write.
    int vulkanReleaseSemaphoreFd = -1;
    int openClReadySemaphoreFd = -1;

    [[nodiscard]] constexpr bool hasImportHandles() const noexcept {
        return colorFd >= 0 && depthFd >= 0 && normalFd >= 0 && velocityFd >= 0
            && vulkanReleaseSemaphoreFd >= 0 && openClReadySemaphoreFd >= 0;
    }
};

// A completed external color frame has no host payload. It is intentionally
// not a Vulkan descriptor: the application transport must first submit the
// OpenCL-ready semaphore wait and acquire the matching ownership ticket.
struct ImplicitExternalFrameView {
    std::uint32_t slotIndex = ImplicitExternalColorSlot::kInvalidSlot;
    std::uint64_t allocationGeneration = 0U;
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    std::uint32_t generation = 0U;
    ImplicitExternalColorFormat format = ImplicitExternalColorFormat::Rgba32Float;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return slotIndex != ImplicitExternalColorSlot::kInvalidSlot
            && allocationGeneration > 0U && width > 0U && height > 0U
            && generation > 0U;
    }

    [[nodiscard]] constexpr bool newerThan(std::uint32_t reference) const noexcept {
        const std::uint32_t distance = generation - reference;
        return distance != 0U && distance < (std::uint32_t{1} << 31U);
    }
};

[[nodiscard]] constexpr std::size_t implicitRgba32ColorBytes(
    std::uint32_t width, std::uint32_t height) noexcept {
    const std::uint64_t pixels = static_cast<std::uint64_t>(width) * height;
    constexpr std::size_t kBytesPerPixel = 4U * sizeof(float);
    return pixels <= std::numeric_limits<std::size_t>::max() / kBytesPerPixel
        ? static_cast<std::size_t>(pixels) * kBytesPerPixel : 0U;
}

} // namespace epi_sculpt
