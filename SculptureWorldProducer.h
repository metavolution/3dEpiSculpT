// Delivery-only triple-buffer owner for a completed OpenCL procedural world.
// It deliberately performs no mathematical evaluation and has no CPU fallback.

#pragma once

#include "SculptureStagingRing.h"
#include "SculptureWorldGpuAbi.h"
#include "SculptureWorldPerformance.h"
#include "SculptureWorldPresentationTicket.h"

#include <array>
#include <cstdint>

namespace epi_sculpt {

struct WorldSnapshotDelivery {
    static constexpr std::uint32_t kNoSlot = 0xFFFFFFFFU;

    // External deliveries expose only frame/recipe here; instances live in
    // the shared GPU slot. This pointer is borrowed until publication/reset.
    const world_gpu_abi::WorldSnapshotGpu* snapshot = nullptr;
    const WorldGenerationMetrics* metrics = nullptr;
    std::uint32_t generation = 0U;
    std::uint32_t slotIndex = kNoSlot;
    // Synchronization sidecar, separate from immutable OpenCL-authored metadata.
    WorldPresentationTicket presentationTicket{};

    [[nodiscard]] bool usesExternalMemory() const noexcept {
        return slotIndex != kNoSlot;
    }

    // A host publication has no external slot. Validate its publication facts
    // independently of native recording permission, which requires a ticket.
    [[nodiscard]] explicit operator bool() const noexcept {
        return snapshot != nullptr && metrics != nullptr && generation != 0U
            && metrics->matches(generation) && snapshot->frame.generation == generation
            && metrics->activeInstanceCount == snapshot->frame.activeInstanceCount
            && slotIndex == metrics->externalSlotIndex
            && (!usesExternalMemory() || slotIndex < WorldPresentationHandoff::kSlotCount);
    }
};

class SculptureWorldProducer {
public:
    static constexpr std::size_t kSlotCount = 3U;

    // Called after OpenCL completes the GPU world and its host metadata header.
    // A stale or malformed completion is rejected rather than becoming visible.
    [[nodiscard]] bool publishOpenClCompletion(
        const world_gpu_abi::WorldSnapshotGpu& snapshot,
        const WorldGenerationMetrics& metrics,
        const WorldPresentationTicket& ticket = {}) noexcept;

    [[nodiscard]] WorldSnapshotDelivery latestDelivery() const noexcept;
    [[nodiscard]] std::uint32_t latestGeneration() const noexcept;
    void reset() noexcept;

private:
    SculptureStagingRing<world_gpu_abi::WorldSnapshotGpu, kSlotCount> stagingRing_{};
    std::array<WorldGenerationMetrics, kSlotCount> metricsRing_{};
    std::array<WorldPresentationTicket, kSlotCount> ticketRing_{};
    std::uint32_t lastPublishedGeneration_ = 0U;
};

} // namespace epi_sculpt
