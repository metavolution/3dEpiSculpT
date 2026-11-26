#include "SculptureWorldExternalTransport.h"

#include <mgfx/Engine.h>
#include <stdexcept>
#include <utility>

namespace epi_sculpt {

namespace {
using WorldRing = SculptureVulkanOpenClRing<SculptureWorldExternalTransport::kSlotCount>;
constexpr std::uint64_t kWorldRingBytes =
    sizeof(world_gpu_abi::WorldSnapshotGpu) * SculptureWorldExternalTransport::kSlotCount;

class WorldSourceRead final : public mgfx::NativeBufferReadLease {
public:
    WorldSourceRead(mgfx::Engine& engine, std::shared_ptr<WorldRing> ring,
                    const WorldPresentationTicket& ticket)
        : NativeBufferReadLease(engine, *ring->buffer(ticket.slotIndex), ring, kWorldRingBytes,
              // Allocation identity is process-local and attempt-qualified.
              // Retaining the ring prevents its address being recycled here.
              {static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(ring.get())),
               ticket.generation, ticket.generation, ticket.slotIndex}),
          ring_(std::move(ring)), ticket_(ticket) {}

private:
    [[nodiscard]] bool authorized() const noexcept override {
        const auto state = ring_->slotStatus(ticket_.slotIndex).state;
        return ticket_.status() == WorldPresentationHandoff::Status::Scheduled
            && (state == VulkanOpenClRingSlotState::Primed
                || state == VulkanOpenClRingSlotState::Released)
            && ring_->canScheduleVulkanRelease(ticket_.slotIndex, ticket_.generation);
    }

    std::shared_ptr<WorldRing> ring_;
    WorldPresentationTicket ticket_;
};
} // namespace

SculptureWorldExternalTransport::SculptureWorldExternalTransport(mgfx::vk::Context& context, const Config& config) {
    const std::uint32_t byteSize = sizeof(world_gpu_abi::WorldSnapshotGpu);
    ring_ = std::make_shared<SculptureVulkanOpenClRing<kSlotCount>>(
        context, byteSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
            | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    );
}

std::unique_ptr<WorldOpenClImportSlot>
SculptureWorldExternalTransport::exportOpenClImport(std::uint32_t slotIndex) const {
    if (slotIndex >= kSlotCount || ring_->slotStatus(slotIndex).state != VulkanOpenClRingSlotState::Allocated)
        return nullptr;
    // Allocate the envelope before exporting FDs, so envelope allocation failure
    // cannot orphan descriptors. Native handle import semantics are unchanged.
    auto result = std::make_unique<WorldOpenClImportSlot>();
    auto handles = ring_->exportOpenClImport(slotIndex);
    if (!handles) return nullptr;
    result->handles = *handles;
    result->bootstrap = bootstrap_;
    result->slotIndex = slotIndex;
    return result;
}

bool SculptureWorldExternalTransport::primeVulkanReleaseSemaphores(mgfx::Engine& engine) const {
    // Binary semaphores begin unsignaled. Without an initial release, OpenCL
    // waits to fill an empty slot while Vulkan waits for OpenCL's first ready
    // signal: a circular first-frame dependency.
    bool queuedAny = false;
    for (std::uint32_t slot = 0; slot < kSlotCount; ++slot) {
        if (primeScheduled_[slot]) {
            queuedAny = true;
            continue;
        }
        if (ring_->slotStatus(slot).state != VulkanOpenClRingSlotState::Imported) continue;

        auto& frameLoop = engine.frameLoop();
        if (!frameLoop.canEnqueueSubmission({
                .externalSignals = 1U,
                .commitCallbacks = 1U,
            })) {
            // FrameLoop capacity is shared by every interop user. Lack of room
            // is normal frame-local backpressure, not a broken native slot.
            break;
        }

        primeScheduled_[slot] = true;
        try {
            // Register settlement first so recording failures also quarantine
            // this transaction. Logical priming follows native queue acceptance.
            const bool commitQueued = frameLoop.enqueueSubmitCommit([this, slot](bool success) noexcept {
                primeScheduled_[slot] = false;
                if (!success || !ring_->markPrimed(slot) || !bootstrap_->markSubmitted(slot)) {
                    quarantine(slot, VulkanOpenClRingFault::SubmissionFailed);
                }
            });
            if (!commitQueued) {
                throw std::logic_error(
                    "world bootstrap commit capacity changed after preflight");
            }
            if (!frameLoop.enqueueExternalSignal(
                    vulkanReleaseSemaphore(slot), VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT)) {
                throw std::logic_error(
                    "world bootstrap signal capacity changed after preflight");
            }
        } catch (...) {
            primeScheduled_[slot] = false;
            quarantine(slot, VulkanOpenClRingFault::SubmissionFailed);
            throw;
        }
        queuedAny = true;
    }
    return queuedAny;
}

mgfx::NativeBufferReadView SculptureWorldExternalTransport::acquirePresentationRead(
    mgfx::Engine& engine, const WorldPresentationTicket& ticket) const {
    if (ticket.slotIndex >= kSlotCount || ticket.generation == 0U
        || ticket.status() != WorldPresentationHandoff::Status::Ready) return {};
    const auto* source = ring_->buffer(ticket.slotIndex);
    if (!source || !source->belongsTo(engine.context())
        || !engine.frameLoop().canEnqueueSubmission({
            .externalWaits = 1U, .externalSignals = 1U, .commitCallbacks = 1U,
        })) return {};

    // Allocate the selection and retain all ring buffers/semaphores before the
    // ticket changes state. Neither allocation failure nor retention pressure
    // may leave an abandoned ready wait or release signal in the frame.
    auto lease = std::make_shared<WorldSourceRead>(engine, ring_, ticket);
    if (!engine.retainFramePayloadResources(ring_, kWorldRingBytes)) return {};
    if (!enqueuePresentationHandoff(engine, ticket)) return {};

    const auto read = lease->view(engine).subview(0U, sizeof(world_gpu_abi::WorldSnapshotGpu));
    if (!read.validFor(engine)) {
        // The handoff is already queued. Abort recording so FrameLoop rejects
        // its transaction rather than releasing a source that was never read.
        throw std::logic_error("world source read lost authorization after handoff");
    }
    return read;
}

bool SculptureWorldExternalTransport::enqueuePresentationHandoff(
    mgfx::Engine& engine, const WorldPresentationTicket& ticket) const {
    const auto slot = ticket.slotIndex;
    if (slot >= kSlotCount || ticket.generation == 0U) return false;
    const auto state = ring_->slotStatus(slot).state;
    if (state == VulkanOpenClRingSlotState::Quarantined) {
        if (ticket.schedule()) static_cast<void>(ticket.settle(false));
        return false;
    }
    if ((state != VulkanOpenClRingSlotState::Primed && state != VulkanOpenClRingSlotState::Released)
        || !ring_->canScheduleVulkanRelease(slot, ticket.generation)) return false;

    auto& frameLoop = engine.frameLoop();
    if (!frameLoop.canEnqueueSubmission({
            .externalWaits = 1U,
            .externalSignals = 1U,
            .commitCallbacks = 1U,
        })) {
        // Do not move the ticket to Scheduled until every native record can be
        // represented. The same immutable delivery can be retried next frame.
        return false;
    }
    if (!ticket.schedule()) return false;

    try {
        const bool commitQueued = frameLoop.enqueueSubmitCommit([this, ticket](bool success) noexcept {
            // Check identity before touching either ledger: duplicate or late
            // callbacks must not release a newer generation occupying this slot.
            if (ticket.status() != WorldPresentationHandoff::Status::Scheduled) return;
            const bool released = success
                && ring_->markVulkanReleaseSubmitted(ticket.slotIndex, ticket.generation);
            if (!released) quarantine(ticket.slotIndex, VulkanOpenClRingFault::SubmissionFailed);
            static_cast<void>(ticket.settle(released));
        });
        if (!commitQueued
            || !frameLoop.enqueueExternalWait(
                openClReadySemaphore(slot),
                VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT)
            || !frameLoop.enqueueExternalSignal(
                vulkanReleaseSemaphore(slot), VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT)) {
            throw std::logic_error(
                "world presentation handoff capacity changed after preflight");
        }
    } catch (...) {
        static_cast<void>(ticket.settle(false));
        quarantine(slot, VulkanOpenClRingFault::SubmissionFailed);
        throw;
    }
    return true;
}

} // namespace epi_sculpt
