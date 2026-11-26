#pragma once

#include "SculptureExternalMemoryImport.h"
#include <mgfx/vk/Context.h>
#include <mgfx/vk/Buffer.h>
#include <mgfx/vk/Semaphore.h>
#include <vulkan/vulkan.h>
#include <cstdint>
#include <memory>
#include <vector>
#include <stdexcept>

#include <unistd.h> // For close()

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: ZERO-COPY EXTERNAL MEMORY RING
// -----------------------------------------------------------------------------
// A universal, templated Vulkan-OpenCL zero-copy ring buffer. 
// Abstracts the tedious boilerplate of allocating POSIX file descriptors for 
// cross-API memory and binary semaphores.
// -----------------------------------------------------------------------------

// Ownership state is deliberately compact and observational. It tells the
// host whether a slot is safe to schedule without creating a second data path.
enum class VulkanOpenClRingSlotState : std::uint8_t {
    Allocated,
    Imported,
    Primed,
    Released,
    Quarantined,
};

enum class VulkanOpenClRingFault : std::uint8_t {
    None,
    ExportFailed,
    ImportFailed,
    SubmissionFailed,
};

struct VulkanOpenClRingSlotStatus {
    VulkanOpenClRingSlotState state = VulkanOpenClRingSlotState::Allocated;
    VulkanOpenClRingFault fault = VulkanOpenClRingFault::None;
    std::uint64_t lastReleasedGeneration = 0U;
};

template<std::size_t SlotCount = kSculptureExternalSlotCount>
class SculptureVulkanOpenClRing {
public:
    SculptureVulkanOpenClRing(mgfx::vk::Context& context, std::uint32_t byteSize, VkBufferUsageFlags usageFlags) {
        slots_.resize(SlotCount);
        for (std::size_t i = 0; i < SlotCount; ++i) {
            slots_[i].buffer = std::make_unique<mgfx::vk::Buffer>(
                context,
                byteSize,
                usageFlags,
                mgfx::vk::BufferLocation::DeviceLocal,
                true // exportFd
            );
            
            if (!(slots_[i].vulkanReleaseSemaphore = std::make_unique<mgfx::vk::Semaphore>(context, true))) {
                throw std::runtime_error("Failed to create external binary release semaphore for Ring");
            }
            if (!(slots_[i].openClReady = std::make_unique<mgfx::vk::Semaphore>(context, true))) {
                throw std::runtime_error("Failed to create external binary ready semaphore for Ring");
            }
            
            slots_[i].byteSize = byteSize;
        }
    }
    
    ~SculptureVulkanOpenClRing() = default;
    SculptureVulkanOpenClRing(const SculptureVulkanOpenClRing&) = delete;
    SculptureVulkanOpenClRing& operator=(const SculptureVulkanOpenClRing&) = delete;

    [[nodiscard]] VkSemaphore vulkanReleaseSemaphore(std::uint32_t slotIndex) const noexcept {
        return slots_[slotIndex].vulkanReleaseSemaphore->handle();
    }
    
    [[nodiscard]] VkSemaphore openClReadySemaphore(std::uint32_t slotIndex) const noexcept {
        return slots_[slotIndex].openClReady->handle();
    }
    
    [[nodiscard]] VkBuffer bufferHandle(std::uint32_t slotIndex) const noexcept {
        return slots_[slotIndex].buffer->handle();
    }
    [[nodiscard]] mgfx::vk::Buffer* buffer(std::uint32_t slotIndex) const noexcept {
        return slots_[slotIndex].buffer.get();
    }
    
    [[nodiscard]] bool canScheduleVulkanRelease(std::uint32_t slotIndex, std::uint64_t generation) const noexcept {
        return slotIndex < SlotCount
            && slots_[slotIndex].status.state != VulkanOpenClRingSlotState::Quarantined
            && generation > slots_[slotIndex].status.lastReleasedGeneration;
    }
    
    [[nodiscard]] bool markVulkanReleaseSubmitted(std::uint32_t slotIndex, std::uint64_t generation) noexcept {
        if (!canScheduleVulkanRelease(slotIndex, generation)) return false;
        slots_[slotIndex].status.lastReleasedGeneration = generation;
        slots_[slotIndex].status.state = VulkanOpenClRingSlotState::Released;
        return true;
    }

    [[nodiscard]] bool markOpenClImported(std::uint32_t slotIndex) noexcept {
        if (slotIndex >= SlotCount
            || slots_[slotIndex].status.state == VulkanOpenClRingSlotState::Quarantined) {
            return false;
        }
        slots_[slotIndex].status.state = VulkanOpenClRingSlotState::Imported;
        slots_[slotIndex].status.fault = VulkanOpenClRingFault::None;
        return true;
    }

    [[nodiscard]] bool markPrimed(std::uint32_t slotIndex) noexcept {
        if (slotIndex >= SlotCount
            || slots_[slotIndex].status.state != VulkanOpenClRingSlotState::Imported) {
            return false;
        }
        slots_[slotIndex].status.state = VulkanOpenClRingSlotState::Primed;
        return true;
    }

    void quarantine(std::uint32_t slotIndex, VulkanOpenClRingFault fault) noexcept {
        if (slotIndex >= SlotCount) return;
        slots_[slotIndex].status.state = VulkanOpenClRingSlotState::Quarantined;
        slots_[slotIndex].status.fault = fault;
    }

    [[nodiscard]] VulkanOpenClRingSlotStatus slotStatus(std::uint32_t slotIndex) const noexcept {
        return slotIndex < SlotCount ? slots_[slotIndex].status : VulkanOpenClRingSlotStatus{
            .state = VulkanOpenClRingSlotState::Quarantined,
            .fault = VulkanOpenClRingFault::ExportFailed,
        };
    }
    
    [[nodiscard]] std::unique_ptr<SculptureOpenClImportSlot> exportOpenClImport(std::uint32_t slotIndex) const noexcept {
        auto importSlot = std::make_unique<SculptureOpenClImportSlot>();
        importSlot->memoryFd = slots_[slotIndex].buffer->exportFd();
        importSlot->releaseSemaphoreFd = slots_[slotIndex].vulkanReleaseSemaphore->exportFd();
        importSlot->openClReadySemaphoreFd = slots_[slotIndex].openClReady->exportFd();
        importSlot->byteSize = slots_[slotIndex].byteSize;
        
        if (importSlot->memoryFd < 0 || importSlot->releaseSemaphoreFd < 0 || importSlot->openClReadySemaphoreFd < 0) {
            if (importSlot->memoryFd >= 0) close(importSlot->memoryFd);
            if (importSlot->releaseSemaphoreFd >= 0) close(importSlot->releaseSemaphoreFd);
            if (importSlot->openClReadySemaphoreFd >= 0) close(importSlot->openClReadySemaphoreFd);
            return nullptr;
        }
        
        return importSlot;
    }

private:
    struct Slot {
        std::unique_ptr<mgfx::vk::Buffer> buffer;
        std::unique_ptr<mgfx::vk::Semaphore> vulkanReleaseSemaphore;
        std::unique_ptr<mgfx::vk::Semaphore> openClReady;
        std::uint32_t byteSize = 0;
        VulkanOpenClRingSlotStatus status{};
    };
    std::vector<Slot> slots_;
};

} // namespace epi_sculpt
