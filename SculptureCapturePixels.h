#pragma once
#include "CapturePixelLayout.h"
#include "mgfx/vk/CaptureLifecycle.h"
#include <stdexcept>
#include <vector>

namespace epi_sculpt {

// Sequence recording copies the borrowed pixels synchronously and performs its
// own BGRA-to-RGBA conversion on the worker. Reject unknown layouts before it
// reads the buffer; CaptureResult itself does not expose a buffer length.
template<class Record>
bool withSequenceCapturePixels(const mgfx::vk::CaptureResult& image, Record&& record) {
    if (!image.pixels || !image.width || !image.height ||
        !capturePixelLayoutError(image.width, image.height,
                                 std::numeric_limits<std::size_t>::max()).empty())
        return false;
    switch (image.format) {
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SRGB:
        return record(image, true);
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SRGB:
        return record(image, false);
    default:
        return false;
    }
}

// Preserve the readback format at a synchronous recorder boundary. The image
// remains borrowed from FrameLoop and must not escape the callback.
template<class Write>
void withRecorderCapturePixels(const mgfx::vk::CaptureResult& image, Write&& write) {
    if (!image.pixels || !image.width || !image.height)
        throw std::invalid_argument("Capture has no pixels");
    const auto byteCount = std::uint64_t{image.width} * image.height * 4;
    if (byteCount > std::numeric_limits<std::size_t>::max()
        || !capturePixelLayoutError(image.width, image.height, static_cast<std::size_t>(byteCount)).empty())
        throw std::invalid_argument("Capture dimensions exceed recorder limits");
    switch (image.format) {
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SRGB:
        write(image, CapturePixelOrder::Bgra);
        return;
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SRGB:
        write(image, CapturePixelOrder::Rgba);
        return;
    default:
        throw std::invalid_argument("Unsupported capture pixel format");
    }
}

// Compatibility adapter for consumers whose interface still requires BGRA.
template<class Write>
void withBgraCapturePixels(const mgfx::vk::CaptureResult& image, Write&& write) {
    withRecorderCapturePixels(image, [&](const auto& pixels, CapturePixelOrder order) {
        if (order == CapturePixelOrder::Bgra) {
            write(pixels);
            return;
        }
        const auto bytes = static_cast<std::size_t>(pixels.width) * pixels.height * 4U;
        std::vector<std::uint8_t> bgra(bytes);
        for (std::size_t i = 0; i < bytes; i += 4) {
            bgra[i] = pixels.pixels[i + 2];
            bgra[i + 1] = pixels.pixels[i + 1];
            bgra[i + 2] = pixels.pixels[i];
            bgra[i + 3] = pixels.pixels[i + 3];
        }
        auto converted = pixels;
        converted.pixels = bgra.data();
        converted.format = pixels.format == VK_FORMAT_R8G8B8A8_SRGB
            ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_B8G8R8A8_UNORM;
        write(converted);
    });
}
} // namespace epi_sculpt
