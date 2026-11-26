#pragma once

#include "SculptureWorldGpuAbi.h"
#include "SculptureExternalMemoryImport.h"
#include "SculptureVulkanOpenClRing.h"
#include "SculptureWorldPresentationTicket.h"
#include <mgfx/NativeBufferReadView.h>
#include <mgfx/vk/Context.h>
#include <array>
#include <cstdint>
#include <memory>

namespace mgfx { class Engine; }

namespace epi_sculpt {

class SculptureWorldExternalTransport {
public:
    static constexpr std::uint32_t kSlotCount = kSculptureExternalSlotCount;

    struct Config {
        std::uint32_t maxInstanceCount = world_gpu_abi::kMaxInstanceCount;
    };

    SculptureWorldExternalTransport(mgfx::vk::Context& context, const Config& config);
    ~SculptureWorldExternalTransport() = default;

    SculptureWorldExternalTransport(const SculptureWorldExternalTransport&) = delete;
    SculptureWorldExternalTransport& operator=(const SculptureWorldExternalTransport&) = delete;

    [[nodiscard]] VkSemaphore vulkanReleaseSemaphore(std::uint32_t slotIndex) const noexcept { return ring_->vulkanReleaseSemaphore(slotIndex); }
    [[nodiscard]] VkSemaphore openClReadySemaphore(std::uint32_t slotIndex) const noexcept { return ring_->openClReadySemaphore(slotIndex); }
    [[nodiscard]] VkBuffer bufferHandle(std::uint32_t slotIndex) const noexcept { return ring_->bufferHandle(slotIndex); }
    
    [[nodiscard]] bool canScheduleVulkanRelease(std::uint32_t slotIndex, std::uint64_t generation) const noexcept { return ring_->canScheduleVulkanRelease(slotIndex, generation); }
    [[nodiscard]] bool markVulkanReleaseSubmitted(std::uint32_t slotIndex, std::uint64_t generation) noexcept { return ring_->markVulkanReleaseSubmitted(slotIndex, generation); }
    [[nodiscard]] bool markOpenClImported(std::uint32_t slotIndex) noexcept { return ring_->markOpenClImported(slotIndex); }
    void quarantine(std::uint32_t slotIndex, VulkanOpenClRingFault fault) const noexcept {
        bootstrap_->fail(slotIndex);
        ring_->quarantine(slotIndex, fault);
    }
    [[nodiscard]] VulkanOpenClRingSlotStatus slotStatus(std::uint32_t slotIndex) const noexcept { return ring_->slotStatus(slotIndex); }
    // One usable slot is sufficient for asynchronous world production. The
    // control plane falls back only after every imported slot is quarantined.
    [[nodiscard]] bool operational() const noexcept {
        for (std::uint32_t slot = 0U; slot < kSlotCount; ++slot) {
            const auto state = ring_->slotStatus(slot).state;
            if (state == VulkanOpenClRingSlotState::Imported
                || state == VulkanOpenClRingSlotState::Primed
                || state == VulkanOpenClRingSlotState::Released) {
                return true;
            }
        }
        return false;
    }
    
    [[nodiscard]] std::unique_ptr<WorldOpenClImportSlot> exportOpenClImport(std::uint32_t slotIndex) const;

    // Queue exactly one initial Vulkan release while a frame is recording.
    // True means this bootstrap frame must not also wait for OpenCL-ready data.
    [[nodiscard]] bool primeVulkanReleaseSemaphores(mgfx::Engine& engine) const;

    // Registers one ready-wait/release-signal transaction while recording.
    // The frame callback settles the ticket only after native queue submission.
    // This transport must outlive the active frame, as required by FrameLoop.
    [[nodiscard]] bool enqueuePresentationHandoff(
        mgfx::Engine& engine, const WorldPresentationTicket& ticket) const;

    // Schedule the existing handoff exactly once and select its retained source
    // range for this recording. Capacity pressure returns empty before scheduling;
    // the native ready wait/release signal still owns cross-API synchronization.
    // This transport still outlives active-frame callbacks; the selected native
    // ring is additionally retained through the Engine's frame retirement.
    [[nodiscard]] mgfx::NativeBufferReadView acquirePresentationRead(
        mgfx::Engine& engine, const WorldPresentationTicket& ticket) const;

private:
    std::shared_ptr<SculptureVulkanOpenClRing<kSlotCount>> ring_;
    std::shared_ptr<WorldBootstrapHandoff> bootstrap_ = std::make_shared<WorldBootstrapHandoff>();
    static_assert(kSlotCount == WorldBootstrapHandoff::kSlotCount);
    mutable std::array<bool, kSlotCount> primeScheduled_{};
};

} // namespace epi_sculpt
