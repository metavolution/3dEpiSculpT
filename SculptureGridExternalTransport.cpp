#include "SculptureGridVertex.h"
#include "SculptureGridExternalTransport.h"
#include <vector>

namespace epi_sculpt {

SculptureGridExternalTransport::SculptureGridExternalTransport(mgfx::vk::Context& context, const Config& config)
    : config_(config) {
    
    const std::uint32_t byteSize = config_.gridResolution * config_.gridResolution * sizeof(GridVertex);
    ring_ = std::make_unique<SculptureVulkanOpenClRing<kSlotCount>>(
        context, byteSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    );
    
    std::vector<std::uint32_t> indices;
    const std::uint32_t res = config_.gridResolution;
    indices.reserve((res - 1) * (res - 1) * 6);
    
    for (std::uint32_t y = 0; y < res - 1; ++y) {
        for (std::uint32_t x = 0; x < res - 1; ++x) {
            std::uint32_t i0 = y * res + x;
            std::uint32_t i1 = i0 + 1;
            std::uint32_t i2 = i0 + res;
            std::uint32_t i3 = i2 + 1;
            indices.push_back(i0); indices.push_back(i2); indices.push_back(i1);
            indices.push_back(i1); indices.push_back(i2); indices.push_back(i3);
        }
    }
    indexCount_ = indices.size();
    
    indexBuffer_ = std::make_unique<mgfx::vk::Buffer>(
        context, indices.size() * sizeof(std::uint32_t),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT, mgfx::vk::BufferLocation::HostVisible
    );
    indexBuffer_->write(indices.data(), indices.size() * sizeof(std::uint32_t));
}

} // namespace epi_sculpt
