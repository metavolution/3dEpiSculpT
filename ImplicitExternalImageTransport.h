#pragma once

#include "ImplicitExternalFrameContract.h"
#include "ImplicitFrameProducer.h"
#include "SculptureExternalSlotOwnership.h"

#include "mgfx/vk/Image.h"
#include "mgfx/vk/Semaphore.h"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>

namespace mgfx::vk { class Context; }
namespace mgfx { class Engine; }

namespace epi_sculpt {

// Owns the imported OpenCL output images and their handoff, not the retained
// display image. A completed slot is copied into presenter-owned images before
// it becomes reusable; no material descriptor may retain a slot image.
class ImplicitExternalImageTransport {
public:
    static constexpr std::size_t kSlotCount = ImplicitFrameProducer::kSlotCount;

    struct Diagnostics {
        std::uint64_t allocationGeneration = 0U;
        std::array<SculptureExternalSlotSnapshot, kSlotCount> slots{};
        std::uint64_t quarantineCount = 0U;
        std::uint64_t rejectedTransitionCount = 0U;

        [[nodiscard]] std::uint32_t unavailableSlotCount() const noexcept {
            std::uint32_t count = 0U;
            for (const auto& s : slots) {
                if (s.state == SculptureExternalSlotState::Unavailable) ++count;
            }
            return count;
        }

        [[nodiscard]] std::uint32_t inFlightSlotCount() const noexcept {
            std::uint32_t count = 0U;
            for (const auto& s : slots) {
                if (s.state != SculptureExternalSlotState::VulkanOwned
                    && s.state != SculptureExternalSlotState::Unavailable) {
                    ++count;
                }
            }
            return count;
        }
    };

    struct Config {
        std::uint32_t width = 0U;
        std::uint32_t height = 0U;
        std::uint64_t allocationGeneration = 1U;
    };

    explicit ImplicitExternalImageTransport(
        mgfx::vk::Context& context, const Config& config);
    ~ImplicitExternalImageTransport();
    ImplicitExternalImageTransport(const ImplicitExternalImageTransport&) = delete;
    ImplicitExternalImageTransport& operator=(const ImplicitExternalImageTransport&) = delete;

    [[nodiscard]] const Config& config() const noexcept { return config_; }
    [[nodiscard]] std::optional<ImplicitFrameProducer::ExternalImageImports>
    exportOpenClImports() const noexcept;

    // State transitions commit only after the corresponding Vulkan/OpenCL
    // submission succeeds. The producer may queue its OpenCL wait only after
    // markVulkanReleaseSubmitted reports success.
    [[nodiscard]] bool markVulkanReleaseSubmitted(
        std::uint32_t slotIndex, std::uint32_t generation) noexcept;
    [[nodiscard]] bool markOpenClWriteComplete(
        const ImplicitExternalFrameView& frame) noexcept;
    [[nodiscard]] bool acquireForVulkanRead(
        const ImplicitExternalFrameView& frame) noexcept;
    [[nodiscard]] bool releaseVulkanReadSubmitted(
        const ImplicitExternalFrameView& frame) noexcept;
    [[nodiscard]] bool quarantine(const ImplicitExternalFrameView& frame) noexcept;
    [[nodiscard]] bool quarantineTicket(std::uint32_t slotIndex, std::uint32_t generation) noexcept;

    [[nodiscard]] const mgfx::vk::Image* imageFor(
        const ImplicitExternalFrameView& frame) const noexcept;
    [[nodiscard]] const mgfx::vk::Image* depthImageFor(
        const ImplicitExternalFrameView& frame) const noexcept;
    [[nodiscard]] const mgfx::vk::Image* normalImageFor(
        const ImplicitExternalFrameView& frame) const noexcept;
    [[nodiscard]] const mgfx::vk::Image* velocityImageFor(
        const ImplicitExternalFrameView& frame) const noexcept;
    [[nodiscard]] VkSemaphore vulkanReleaseSemaphore(
        std::uint32_t slotIndex) const noexcept;
    [[nodiscard]] VkSemaphore openClReadySemaphore(
        std::uint32_t slotIndex) const noexcept;
    [[nodiscard]] bool canScheduleVulkanRelease(std::uint32_t slotIndex) const noexcept;

    // Called during Drawable collection; deferred past Engine's graph reset.
    // The caller submits the release signal and commits ownership afterward.
    [[nodiscard]] bool enqueueInitialRelease(mgfx::Engine& engine, std::uint32_t slotIndex);

    // Color, depth, normal, velocity, in that order. Destinations must be
    // presenter-owned, single-mip transfer-destination images of matching shape.
    using PresentationImages = std::array<const mgfx::vk::Image*, 4>;
    [[nodiscard]] bool enqueuePresentationCopy(mgfx::Engine& engine,
        const ImplicitExternalFrameView& frame,
        const PresentationImages& destinations, bool destinationInitialized);

    [[nodiscard]] Diagnostics diagnostics() const noexcept;

private:
    struct Slot {
        std::unique_ptr<mgfx::vk::Image> image; // color
        std::unique_ptr<mgfx::vk::Image> depthImage;
        std::unique_ptr<mgfx::vk::Image> normalImage;
        std::unique_ptr<mgfx::vk::Image> velocityImage;
        std::unique_ptr<mgfx::vk::Semaphore> vulkanRelease;
        std::unique_ptr<mgfx::vk::Semaphore> openClReady;
        bool initialReleaseSubmitted = false;
    };

    [[nodiscard]] bool matchesFrame(const ImplicitExternalFrameView& frame,
                                    SculptureExternalSlotTicket* outTicket = nullptr) const noexcept;
    [[nodiscard]] static bool validSlot(std::uint32_t slotIndex) noexcept;
    [[nodiscard]] const Slot* slotAt(std::uint32_t slotIndex) const noexcept;

    Config config_{};
    std::array<Slot, kSlotCount> slots_{};
    SculptureExternalSlotOwnership<kSlotCount> ownership_{};
    std::uint64_t quarantineCount_ = 0U;
    std::uint64_t rejectedTransitionCount_ = 0U;
};

} // namespace epi_sculpt
