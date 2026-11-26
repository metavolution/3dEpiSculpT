#pragma once

#include "SculptureProductionRoute.h"
#include "mgfx/vk/FrameOutcome.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace epi_sculpt {

struct SculpturePresentationContribution final {
    SculptureProductionRoute route = SculptureProductionRoute::CompactInstances;
    std::uint64_t generation = 0U;
    std::uint64_t resetEpoch = 0U;
    std::uint64_t productionReceiptSequence = 0U;

    [[nodiscard]] constexpr bool selected() const noexcept {
        return generation != 0U;
    }
};

// Immutable result identities are selected before recording. The renderer then
// settles the receipt with its non-exceptional frame outcome after submission
// and presentation handling. This keeps selected, submitted, and confirmed
// presentation facts distinct without teaching mGFX about epiC payloads.
// The production-receipt sequence fields are causal joins only: they never
// grant ownership of producer or transport resources.
struct SculpturePresentationReceipt {
    static constexpr std::size_t kContributionCapacity = 8U;

    SculptureProductionRoute route = SculptureProductionRoute::CompactInstances;
    std::uint64_t frameSequence = 0U;
    // Native counters such as submission serials are only monotonic inside one
    // logical device. Carry the device/surface namespace beside semantic
    // generations so persisted causal evidence cannot alias across recovery.
    std::uint64_t deviceGeneration = 0U;
    std::uint64_t surfaceGeneration = 0U;
    std::uint32_t compactGeneration = 0U;
    std::uint64_t compactProductionReceiptSequence = 0U;
    std::uint32_t worldGeneration = 0U;
    std::uint64_t worldProductionReceiptSequence = 0U;
    std::uint64_t canonicalFeedbackGeneration = 0U;
    std::uint64_t canonicalFeedbackResetEpoch = 0U;
    std::uint64_t canonicalFeedbackProductionReceiptSequence = 0U;
    std::uint32_t implicitGeneration = 0U;
    std::uint64_t implicitProductionReceiptSequence = 0U;
    mgfx::vk::FrameOutcome outcome = mgfx::vk::FrameOutcome::SkippedZeroExtent;
    // With no returned native outcome (for example an exceptional frame exit),
    // the default enum value carries no outcome evidence. Consumers must retain
    // this flag instead of promoting selection into submission/presentation.
    bool settled = false;

    // Centralized compatibility adapter from generic presentation vocabulary
    // into today's persisted route-specific fields. New orchestration code
    // should select contributions through this method rather than duplicating
    // route switches throughout render/capture/causal paths.
    void selectProduction(
        SculptureProductionRoute selectedRoute,
        std::uint64_t generation,
        std::uint64_t productionReceiptSequence,
        std::uint64_t resetEpoch = 0U) noexcept {
        switch (selectedRoute) {
        case SculptureProductionRoute::CompactInstances:
            compactGeneration = static_cast<std::uint32_t>(generation);
            compactProductionReceiptSequence = productionReceiptSequence;
            break;
        case SculptureProductionRoute::DedicatedWorld:
            worldGeneration = static_cast<std::uint32_t>(generation);
            worldProductionReceiptSequence = productionReceiptSequence;
            break;
        case SculptureProductionRoute::CanonicalFeedback:
            canonicalFeedbackGeneration = generation;
            canonicalFeedbackResetEpoch = resetEpoch;
            canonicalFeedbackProductionReceiptSequence = productionReceiptSequence;
            break;
        case SculptureProductionRoute::ImplicitField:
            implicitGeneration = static_cast<std::uint32_t>(generation);
            implicitProductionReceiptSequence = productionReceiptSequence;
            break;
        case SculptureProductionRoute::CanonicalFeedbackUnavailable:
            break;
        }
    }

    // A fixed-size value projection keeps the persisted representation stable
    // while giving causal/debug consumers one generic iterable contract.
    [[nodiscard]] std::array<SculpturePresentationContribution, kContributionCapacity>
    selectedProductions() const noexcept {
        std::array<SculpturePresentationContribution, kContributionCapacity> result{};
        std::size_t count = 0U;
        const auto append = [&](SculptureProductionRoute selectedRoute,
                                std::uint64_t generation,
                                std::uint64_t productionReceiptSequence,
                                std::uint64_t resetEpoch = 0U) {
            if (generation == 0U || count >= result.size()) return;
            result[count++] = {
                .route = selectedRoute,
                .generation = generation,
                .resetEpoch = resetEpoch,
                .productionReceiptSequence = productionReceiptSequence,
            };
        };
        append(SculptureProductionRoute::CompactInstances,
               compactGeneration, compactProductionReceiptSequence);
        append(SculptureProductionRoute::DedicatedWorld,
               worldGeneration, worldProductionReceiptSequence);
        append(SculptureProductionRoute::CanonicalFeedback,
               canonicalFeedbackGeneration, canonicalFeedbackProductionReceiptSequence,
               canonicalFeedbackResetEpoch);
        append(SculptureProductionRoute::ImplicitField,
               implicitGeneration, implicitProductionReceiptSequence);
        return result;
    }

    void settle(mgfx::vk::FrameOutcome value) noexcept {
        outcome = value;
        settled = true;
    }

    [[nodiscard]] bool submitted() const noexcept {
        return settled && mgfx::vk::frameOutcomeSubmitted(outcome);
    }

    [[nodiscard]] bool presentationConfirmed() const noexcept {
        return settled && mgfx::vk::frameOutcomePresentationConfirmed(outcome);
    }

    [[nodiscard]] constexpr bool operator==(
        const SculpturePresentationReceipt&) const noexcept = default;
};

} // namespace epi_sculpt
