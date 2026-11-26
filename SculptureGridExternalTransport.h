#pragma once

#include "SculptureVulkanOpenClRing.h"
#include <mgfx/vk/Context.h>
#include <mgfx/vk/Buffer.h>
#include <cstdint>
#include <memory>

namespace epi_sculpt {

class SculptureGridExternalTransport {
public:
    static constexpr std::uint32_t kSlotCount = kSculptureExternalSlotCount;

    struct Config {
        std::uint32_t gridResolution = 256U;
    };

    SculptureGridExternalTransport(mgfx::vk::Context& context, const Config& config);
    ~SculptureGridExternalTransport() = default;

    SculptureGridExternalTransport(const SculptureGridExternalTransport&) = delete;
    SculptureGridExternalTransport& operator=(const SculptureGridExternalTransport&) = delete;

    [[nodiscard]] VkSemaphore vulkanReleaseSemaphore(std::uint32_t slotIndex) const noexcept { return ring_->vulkanReleaseSemaphore(slotIndex); }
    [[nodiscard]] VkSemaphore openClReadySemaphore(std::uint32_t slotIndex) const noexcept { return ring_->openClReadySemaphore(slotIndex); }
    [[nodiscard]] VkBuffer bufferHandle(std::uint32_t slotIndex) const noexcept { return ring_->bufferHandle(slotIndex); }
    [[nodiscard]] mgfx::vk::Buffer* buffer(std::uint32_t slotIndex) const noexcept { return ring_->buffer(slotIndex); }
    [[nodiscard]] std::unique_ptr<SculptureOpenClImportSlot> exportOpenClImport(std::uint32_t slotIndex) const noexcept { return ring_->exportOpenClImport(slotIndex); }
    
    [[nodiscard]] VkBuffer indexBufferHandle() const noexcept { return indexBuffer_->handle(); }
    [[nodiscard]] mgfx::vk::Buffer* indexBuffer() const noexcept { return indexBuffer_.get(); }
    [[nodiscard]] std::uint32_t indexCount() const noexcept { return indexCount_; }
    [[nodiscard]] std::uint32_t vertexCount() const noexcept { return config_.gridResolution * config_.gridResolution; }

private:
    std::unique_ptr<SculptureVulkanOpenClRing<kSlotCount>> ring_;
    std::unique_ptr<mgfx::vk::Buffer> indexBuffer_;
    std::uint32_t indexCount_ = 0;
    Config config_;
};

} // namespace epi_sculpt
