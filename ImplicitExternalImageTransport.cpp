#include "ImplicitExternalImageTransport.h"

#include "mgfx/vk/Context.h"
#include "mgfx/Engine.h"
#include "vk/RenderGraph.h"

#include <limits>
#include <stdexcept>

namespace epi_sculpt {
namespace {

// RenderGraph does not express external queue-family ownership. Keep the
// image handoff barriers together at this explicit transport boundary.
VkImageMemoryBarrier2 imageBarrier(VkImage image,
    VkImageLayout oldLayout, VkImageLayout newLayout,
    VkPipelineStageFlags2 sourceStage, VkAccessFlags2 sourceAccess,
    VkPipelineStageFlags2 destinationStage, VkAccessFlags2 destinationAccess,
    std::uint32_t sourceFamily = VK_QUEUE_FAMILY_IGNORED,
    std::uint32_t destinationFamily = VK_QUEUE_FAMILY_IGNORED) {
    VkImageMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    barrier.srcStageMask = sourceStage;
    barrier.srcAccessMask = sourceAccess;
    barrier.dstStageMask = destinationStage;
    barrier.dstAccessMask = destinationAccess;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = sourceFamily;
    barrier.dstQueueFamilyIndex = destinationFamily;
    barrier.image = image;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0U, 1U, 0U, 1U};
    return barrier;
}

template<std::size_t N>
void emitBarriers(VkCommandBuffer cmd, const std::array<VkImageMemoryBarrier2, N>& barriers) {
    VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency.imageMemoryBarrierCount = static_cast<std::uint32_t>(N);
    dependency.pImageMemoryBarriers = barriers.data();
    vkCmdPipelineBarrier2(cmd, &dependency);
}

} // namespace

ImplicitExternalImageTransport::ImplicitExternalImageTransport(
    mgfx::vk::Context& context, const Config& config)
    : config_(config) {
    const std::size_t bytes = implicitRgba32ColorBytes(config_.width, config_.height);
    if (bytes == 0U || config_.allocationGeneration == 0U) {
        throw std::invalid_argument(
            "external implicit transport requires a non-zero extent and allocation generation");
    }
    if (!context.device().supportsExternalMemoryFd()
        || !context.device().supportsExternalSemaphoreFd()) {
        throw std::runtime_error(
            "Vulkan device cannot export implicit images and semaphores");
    }
    if (bytes > std::numeric_limits<VkDeviceSize>::max()) {
        throw std::overflow_error("external implicit color image exceeds VkDeviceSize");
    }

    mgfx::vk::Image::CreateInfo colorInfo{};
    colorInfo.width = config_.width;
    colorInfo.height = config_.height;
    colorInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    colorInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    colorInfo.exportFd = true;

    mgfx::vk::Image::CreateInfo depthInfo = colorInfo;
    depthInfo.format = VK_FORMAT_R32_SFLOAT;

    mgfx::vk::Image::CreateInfo normalInfo = colorInfo;
    normalInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT; // RGBA32 for now

    mgfx::vk::Image::CreateInfo velocityInfo = colorInfo;
    velocityInfo.format = VK_FORMAT_R32G32_SFLOAT;

    for (Slot& slot : slots_) {
        slot.image = std::make_unique<mgfx::vk::Image>(context, colorInfo);
        slot.depthImage = std::make_unique<mgfx::vk::Image>(context, depthInfo);
        slot.normalImage = std::make_unique<mgfx::vk::Image>(context, normalInfo);
        slot.velocityImage = std::make_unique<mgfx::vk::Image>(context, velocityInfo);
        slot.vulkanRelease = std::make_unique<mgfx::vk::Semaphore>(context, true);
        slot.openClReady = std::make_unique<mgfx::vk::Semaphore>(context, true);
    }
}

ImplicitExternalImageTransport::~ImplicitExternalImageTransport() = default;

bool ImplicitExternalImageTransport::enqueueInitialRelease(
    mgfx::Engine& engine, std::uint32_t slotIndex) {
    if (!canScheduleVulkanRelease(slotIndex)) return false;
    const Slot& slot = slots_[slotIndex];
    if (slot.initialReleaseSubmitted) return true;
    const std::array images{slot.image->handle(), slot.depthImage->handle(),
        slot.normalImage->handle(), slot.velocityImage->handle()};
    const auto graphics = engine.context().device().queueFamilies().graphics;
    engine.deferGraphPass([images, graphics](mgfx::vk::RenderGraph& graph) {
        graph.addPass("Implicit.InitialExternalRelease").execute([images, graphics](VkCommandBuffer cmd) {
            std::array<VkImageMemoryBarrier2, 4> barriers{};
            for (std::size_t i = 0; i < images.size(); ++i) {
                barriers[i] = imageBarrier(images[i], VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, 0,
                    VK_PIPELINE_STAGE_2_NONE, 0, graphics, VK_QUEUE_FAMILY_EXTERNAL);
            }
            emitBarriers(cmd, barriers);
        });
    });
    return true;
}

bool ImplicitExternalImageTransport::enqueuePresentationCopy(
    mgfx::Engine& engine, const ImplicitExternalFrameView& frame,
    const PresentationImages& destinations, bool destinationInitialized) {
    const PresentationImages sources{imageFor(frame), depthImageFor(frame),
        normalImageFor(frame), velocityImageFor(frame)};
    std::array<VkImage, 4> sourceHandles{}, destinationHandles{};
    // Validate the entire image set before declaring any commands. A partial
    // G-buffer copy would combine unrelated generations in one material.
    for (std::size_t i = 0; i < sources.size(); ++i) {
        const auto* source = sources[i];
        const auto* destination = destinations[i];
        if (!source || !destination || source->handle() == destination->handle()
            || source->format() != destination->format()
            || source->width() != destination->width()
            || source->height() != destination->height()
            || source->mipLevels() != 1U || destination->mipLevels() != 1U
            || source->depth() != 1U || destination->depth() != 1U) return false;
        sourceHandles[i] = source->handle();
        destinationHandles[i] = destination->handle();
    }
    for (std::size_t i = 0; i < destinationHandles.size(); ++i) {
        for (std::size_t j = 0; j < sourceHandles.size(); ++j) {
            if (destinationHandles[i] == sourceHandles[j]
                || (i != j && destinationHandles[i] == destinationHandles[j])) return false;
        }
    }
    const auto graphics = engine.context().device().queueFamilies().graphics;
    const VkExtent3D extent{frame.width, frame.height, 1U};
    engine.deferGraphPass([sourceHandles, destinationHandles, graphics, extent,
                          destinationInitialized](mgfx::vk::RenderGraph& graph) {
        graph.addPass("Implicit.RetainPresentationImages").execute(
            [sourceHandles, destinationHandles, graphics, extent, destinationInitialized](VkCommandBuffer cmd) {
                std::array<VkImageMemoryBarrier2, 8> before{}, after{};
                for (std::size_t i = 0; i < sourceHandles.size(); ++i) {
                    // External images remain GENERAL across both API boundaries.
                    // The submit waits on OpenCL-ready before this acquire/copy.
                    before[i] = imageBarrier(sourceHandles[i], VK_IMAGE_LAYOUT_GENERAL,
                        VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_2_NONE, 0,
                        VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
                        VK_QUEUE_FAMILY_EXTERNAL, graphics);
                    before[i + 4] = imageBarrier(destinationHandles[i],
                        destinationInitialized ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                               : VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        destinationInitialized ? VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
                                               : VK_PIPELINE_STAGE_2_NONE,
                        destinationInitialized ? VK_ACCESS_2_SHADER_SAMPLED_READ_BIT : 0,
                        VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
                    after[i] = imageBarrier(sourceHandles[i], VK_IMAGE_LAYOUT_GENERAL,
                        VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_2_COPY_BIT,
                        VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_NONE, 0,
                        graphics, VK_QUEUE_FAMILY_EXTERNAL);
                    after[i + 4] = imageBarrier(destinationHandles[i],
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
                }
                emitBarriers(cmd, before);
                VkImageCopy region{};
                region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0U, 0U, 1U};
                region.dstSubresource = region.srcSubresource;
                region.extent = extent;
                for (std::size_t i = 0; i < sourceHandles.size(); ++i) {
                    vkCmdCopyImage(cmd, sourceHandles[i], VK_IMAGE_LAYOUT_GENERAL,
                        destinationHandles[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &region);
                }
                emitBarriers(cmd, after);
            });
    });
    return true;
}

std::optional<ImplicitFrameProducer::ExternalImageImports>
ImplicitExternalImageTransport::exportOpenClImports() const noexcept {
    ImplicitFrameProducer::ExternalImageImports result;
    for (std::uint32_t index = 0U; index < kSlotCount; ++index) {
        const Slot* slot = slotAt(index);
        if (slot == nullptr || !slot->image || !slot->depthImage
            || !slot->normalImage || !slot->velocityImage
            || !slot->vulkanRelease || !slot->openClReady) {
            return std::nullopt;
        }
        auto& imported = result.slots_[index];
        imported.slotIndex = index;
        imported.allocationGeneration = config_.allocationGeneration;
        imported.width = config_.width;
        imported.height = config_.height;
        imported.format = ImplicitExternalColorFormat::Rgba32Float;
        imported.colorAllocationSizeBytes = static_cast<std::size_t>(slot->image->allocationSize());
        imported.depthSizeBytes = static_cast<std::size_t>(slot->depthImage->allocationSize());
        imported.normalSizeBytes = static_cast<std::size_t>(slot->normalImage->allocationSize());
        imported.velocitySizeBytes = static_cast<std::size_t>(slot->velocityImage->allocationSize());
        imported.colorFd = slot->image->exportFd();
        imported.depthFd = slot->depthImage->exportFd();
        imported.normalFd = slot->normalImage->exportFd();
        imported.velocityFd = slot->velocityImage->exportFd();
        imported.vulkanReleaseSemaphoreFd = slot->vulkanRelease->exportFd();
        imported.openClReadySemaphoreFd = slot->openClReady->exportFd();
        if (!imported.contract().hasImportHandles()) {
            return std::nullopt;
        }
    }
    return result;
}

bool ImplicitExternalImageTransport::markVulkanReleaseSubmitted(
    std::uint32_t slotIndex, std::uint32_t generation) noexcept {
    if (!validSlot(slotIndex) || generation == 0U) return false;
    std::uint64_t epoch = 0U;
    const bool success = ownership_.releaseFromVulkan(slotIndex, &epoch)
        && ownership_.acquireForOpenCl(slotIndex, {generation, epoch});
    if (success) slots_[slotIndex].initialReleaseSubmitted = true;
    if (!success) ++rejectedTransitionCount_;
    return success;
}

bool ImplicitExternalImageTransport::markOpenClWriteComplete(
    const ImplicitExternalFrameView& frame) noexcept {
    SculptureExternalSlotTicket ticket{};
    const bool success = matchesFrame(frame, &ticket)
        && ownership_.releaseFromOpenCl(frame.slotIndex, ticket);
    if (!success) ++rejectedTransitionCount_;
    return success;
}

bool ImplicitExternalImageTransport::acquireForVulkanRead(
    const ImplicitExternalFrameView& frame) noexcept {
    SculptureExternalSlotTicket ticket{};
    const bool success = matchesFrame(frame, &ticket)
        && ownership_.acquireForVulkan(frame.slotIndex, ticket);
    if (!success) ++rejectedTransitionCount_;
    return success;
}

bool ImplicitExternalImageTransport::releaseVulkanReadSubmitted(
    const ImplicitExternalFrameView& frame) noexcept {
    SculptureExternalSlotTicket ticket{};
    const bool success = matchesFrame(frame, &ticket)
        && ownership_.releaseFromVulkanRead(frame.slotIndex, ticket);
    if (!success) ++rejectedTransitionCount_;
    return success;
}

bool ImplicitExternalImageTransport::quarantine(
    const ImplicitExternalFrameView& frame) noexcept {
    SculptureExternalSlotTicket ticket{};
    const bool quarantined = matchesFrame(frame, &ticket)
        && ownership_.quarantine(frame.slotIndex, ticket);
    if (quarantined) ++quarantineCount_;
    return quarantined;
}

bool ImplicitExternalImageTransport::quarantineTicket(
    std::uint32_t slotIndex, std::uint32_t generation) noexcept {
    SculptureExternalSlotSnapshot snapshot{};
    if (!validSlot(slotIndex) || generation == 0U
        || !ownership_.inspect(slotIndex, &snapshot)
        || snapshot.ticket.generation != generation) {
        return false;
    }
    const bool quarantined = ownership_.quarantine(slotIndex, snapshot.ticket);
    if (quarantined) ++quarantineCount_;
    return quarantined;
}

ImplicitExternalImageTransport::Diagnostics
ImplicitExternalImageTransport::diagnostics() const noexcept {
    Diagnostics d{};
    d.allocationGeneration = config_.allocationGeneration;
    d.quarantineCount = quarantineCount_;
    d.rejectedTransitionCount = rejectedTransitionCount_;
    for (std::uint32_t i = 0U; i < kSlotCount; ++i) {
        static_cast<void>(ownership_.inspect(i, &d.slots[i]));
    }
    return d;
}

const mgfx::vk::Image* ImplicitExternalImageTransport::imageFor(
    const ImplicitExternalFrameView& frame) const noexcept {
    SculptureExternalSlotTicket ticket{};
    SculptureExternalSlotSnapshot snapshot{};
    if (!matchesFrame(frame, &ticket)
        || !ownership_.inspect(frame.slotIndex, &snapshot)
        || snapshot.state != SculptureExternalSlotState::VulkanReading
        || snapshot.ticket.generation != ticket.generation
        || snapshot.ticket.epoch != ticket.epoch) {
        return nullptr;
    }
    const Slot* slot = slotAt(frame.slotIndex);
    return slot != nullptr ? slot->image.get() : nullptr;
}

const mgfx::vk::Image* ImplicitExternalImageTransport::depthImageFor(
    const ImplicitExternalFrameView& frame) const noexcept {
    SculptureExternalSlotTicket ticket{};
    SculptureExternalSlotSnapshot snapshot{};
    if (!matchesFrame(frame, &ticket)
        || !ownership_.inspect(frame.slotIndex, &snapshot)
        || snapshot.state != SculptureExternalSlotState::VulkanReading
        || snapshot.ticket.generation != ticket.generation
        || snapshot.ticket.epoch != ticket.epoch) {
        return nullptr;
    }
    const Slot* slot = slotAt(frame.slotIndex);
    return slot != nullptr ? slot->depthImage.get() : nullptr;
}

const mgfx::vk::Image* ImplicitExternalImageTransport::normalImageFor(
    const ImplicitExternalFrameView& frame) const noexcept {
    SculptureExternalSlotTicket ticket{};
    SculptureExternalSlotSnapshot snapshot{};
    if (!matchesFrame(frame, &ticket)
        || !ownership_.inspect(frame.slotIndex, &snapshot)
        || snapshot.state != SculptureExternalSlotState::VulkanReading
        || snapshot.ticket.generation != ticket.generation
        || snapshot.ticket.epoch != ticket.epoch) {
        return nullptr;
    }
    const Slot* slot = slotAt(frame.slotIndex);
    return slot != nullptr ? slot->normalImage.get() : nullptr;
}

const mgfx::vk::Image* ImplicitExternalImageTransport::velocityImageFor(
    const ImplicitExternalFrameView& frame) const noexcept {
    SculptureExternalSlotTicket ticket{};
    SculptureExternalSlotSnapshot snapshot{};
    if (!matchesFrame(frame, &ticket)
        || !ownership_.inspect(frame.slotIndex, &snapshot)
        || snapshot.state != SculptureExternalSlotState::VulkanReading
        || snapshot.ticket.generation != ticket.generation
        || snapshot.ticket.epoch != ticket.epoch) {
        return nullptr;
    }
    const Slot* slot = slotAt(frame.slotIndex);
    return slot != nullptr ? slot->velocityImage.get() : nullptr;
}

VkSemaphore ImplicitExternalImageTransport::vulkanReleaseSemaphore(
    std::uint32_t slotIndex) const noexcept {
    const Slot* slot = slotAt(slotIndex);
    return slot != nullptr && slot->vulkanRelease
        ? slot->vulkanRelease->handle() : VK_NULL_HANDLE;
}

VkSemaphore ImplicitExternalImageTransport::openClReadySemaphore(
    std::uint32_t slotIndex) const noexcept {
    const Slot* slot = slotAt(slotIndex);
    return slot != nullptr && slot->openClReady
        ? slot->openClReady->handle() : VK_NULL_HANDLE;
}

bool ImplicitExternalImageTransport::canScheduleVulkanRelease(
    std::uint32_t slotIndex) const noexcept {
    return validSlot(slotIndex)
        && ownership_.state(slotIndex) == SculptureExternalSlotState::VulkanOwned;
}

bool ImplicitExternalImageTransport::matchesFrame(
    const ImplicitExternalFrameView& frame,
    SculptureExternalSlotTicket* outTicket) const noexcept {
    if (!frame.valid() || !validSlot(frame.slotIndex)
        || frame.allocationGeneration != config_.allocationGeneration
        || frame.width != config_.width || frame.height != config_.height
        || frame.format != ImplicitExternalColorFormat::Rgba32Float) {
        return false;
    }
    SculptureExternalSlotSnapshot snapshot{};
    if (!ownership_.inspect(frame.slotIndex, &snapshot)
        || snapshot.ticket.generation != frame.generation) {
        return false;
    }
    if (outTicket != nullptr) *outTicket = snapshot.ticket;
    return true;
}

bool ImplicitExternalImageTransport::validSlot(std::uint32_t slotIndex) noexcept {
    return slotIndex < kSlotCount;
}

const ImplicitExternalImageTransport::Slot*
ImplicitExternalImageTransport::slotAt(std::uint32_t slotIndex) const noexcept {
    return validSlot(slotIndex) ? &slots_[slotIndex] : nullptr;
}

} // namespace epi_sculpt
