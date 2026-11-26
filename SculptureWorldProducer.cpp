#include "SculptureWorldProducer.h"

#include <limits>

namespace epi_sculpt {

namespace {

[[nodiscard]] bool generationStrictlyNewer(
    std::uint32_t candidate,
    std::uint32_t published) noexcept {
    return candidate != published
        && static_cast<std::int32_t>(candidate - published) > 0;
}

} // namespace

bool SculptureWorldProducer::publishOpenClCompletion(
    const world_gpu_abi::WorldSnapshotGpu& snapshot,
    const WorldGenerationMetrics& metrics,
    const WorldPresentationTicket& ticket) noexcept {
    
    const bool external = metrics.externalSlotIndex != WorldGenerationMetrics::kNoExternalSlot;
    constexpr std::size_t headerBytes = offsetof(world_gpu_abi::WorldSnapshotGpu, instances);
    if (!metrics.matches(snapshot.frame.generation)
        || !world_gpu_abi::isValid(snapshot.frame, snapshot.recipe)
        || metrics.activeInstanceCount != snapshot.frame.activeInstanceCount) return false;
    if (external) {
        if (metrics.externalSlotIndex >= kSlotCount || metrics.readbackBytes != headerBytes) return false;
    } else if (!world_gpu_abi::isValid(snapshot)) {
        return false;
    }
    // Ticketless publication remains available to headless metadata tooling;
    // the native render bridge requires a real ticket before touching a slot.
    if (ticket.owner && (!external || ticket.slotIndex != metrics.externalSlotIndex
        || ticket.generation != metrics.generation
        || ticket.status() != WorldPresentationHandoff::Status::Ready)) return false;
    
    if (lastPublishedGeneration_ != 0U && !generationStrictlyNewer(metrics.generation, lastPublishedGeneration_)) {
        return false;
    }

    const std::size_t slotIndex = metrics.generation % kSlotCount;
    // Write metadata first. The staging-ring publication below is the visible
    // boundary, so readers never observe a new snapshot with old metrics.
    metricsRing_[slotIndex] = metrics;
    ticketRing_[slotIndex] = ticket;
    if (external) {
        // Publication is serialized with the render loop. Only the completed
        // header is host data; copying the unused instance array here would
        // move 1.5 MiB per generation without transferring a single GPU result.
        auto& metadata = stagingRing_.slotData(slotIndex);
        metadata.frame = snapshot.frame;
        metadata.recipe = snapshot.recipe;
        stagingRing_.commit(slotIndex, metrics.generation);
    } else {
        stagingRing_.write(slotIndex, snapshot, metrics.generation);
    }
    lastPublishedGeneration_ = metrics.generation;
    return true;
}

WorldSnapshotDelivery SculptureWorldProducer::latestDelivery() const noexcept {
    std::uint64_t generation = 0U;
    const auto* snapshot = stagingRing_.readLatest(&generation);
    if (snapshot == nullptr || generation == 0U
        || generation > std::numeric_limits<std::uint32_t>::max()) {
        return {};
    }
    const std::uint32_t slotIndex =
        static_cast<std::uint32_t>(stagingRing_.latestSlotIndex());
    const WorldGenerationMetrics& metrics = metricsRing_[slotIndex];
    return {
        .snapshot = snapshot,
        .metrics = metrics.matches(static_cast<std::uint32_t>(generation))
            ? &metrics : nullptr,
        .generation = static_cast<std::uint32_t>(generation),
        // `slotIndex` is the external Vulkan/OpenCL buffer identity. The
        // staging ring has its own private index and must never leak into the
        // renderer's semaphore selection.
        .slotIndex = metrics.matches(static_cast<std::uint32_t>(generation))
            ? metrics.externalSlotIndex : WorldSnapshotDelivery::kNoSlot,
        .presentationTicket = ticketRing_[slotIndex],
    };
}

std::uint32_t SculptureWorldProducer::latestGeneration() const noexcept {
    return latestDelivery().generation;
}

void SculptureWorldProducer::reset() noexcept {
    stagingRing_.reset();
    metricsRing_ = {};
    ticketRing_ = {};
    lastPublishedGeneration_ = 0U;
}

} // namespace epi_sculpt
