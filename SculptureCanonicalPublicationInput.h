#pragma once

#include "EpiFeedbackFieldDelivery.h"
#include "epi/mode/visual_mode.hpp"
#include "epi/backend/seed_image_input.hpp"

#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <utility>

namespace epi_sculpt {

class SculptureCanonicalPublicationInputs;

// Application-owned semantic evidence, prepared before producer invocation.
// Only the publication store can stamp completion; readers receive const owners.
// This is neither a checkpoint nor a producer/scheduler/native-resource owner.
class SculptureCanonicalPublicationInput final {
public:
    SculptureCanonicalPublicationInput(epi::VisualMode resolvedMode,
        std::string source, std::string preset, std::string revision,
        std::uint64_t generation, std::uint64_t epoch, std::uint64_t receipt,
        float controlTime, float delta, std::uint64_t imageSelectionRevision = 0U,
        epi::SeedImageInput::Owner imageInput = {})
        : mode(std::move(resolvedMode)), sourceStateId(std::move(source)),
          presetId(std::move(preset)), presetRevision(std::move(revision)),
          generation(generation), resetEpoch(epoch), productionReceiptSequence(receipt),
          acceptedControlTime(controlTime), productionDelta(delta),
          seedImageSelectionRevision(imageSelectionRevision), seedImageInput(std::move(imageInput)) {}

    const epi::VisualMode mode;
    const std::string sourceStateId;
    const std::string presetId;
    const std::string presetRevision;
    const std::uint64_t generation;
    const std::uint64_t resetEpoch;
    const std::uint64_t productionReceiptSequence;
    const float acceptedControlTime;
    const float productionDelta;
    const std::uint64_t seedImageSelectionRevision;
    const epi::SeedImageInput::Owner seedImageInput;

    struct Publication final {
        std::uint32_t width = 0U, height = 0U;
        float kernelTime = 0.0f;
        EpiFeedbackFieldFormat format = EpiFeedbackFieldFormat::Rgba32Linear;
        EpiFeedbackFieldTransport transport = EpiFeedbackFieldTransport::HostStaged;
        std::optional<EpiFeedbackFieldExternalLease> externalLease;
    };
    [[nodiscard]] const Publication& publication() const noexcept { return publication_; }
    [[nodiscard]] bool published() const noexcept { return published_; }

    [[nodiscard]] bool matches(const EpiFeedbackFieldDelivery& delivery,
                               std::uint64_t receipt) const noexcept {
        if (!published_ || !matchesInput(delivery) || receipt != productionReceiptSequence)
            return false;
        const auto& field = delivery.descriptor;
        if (publication_.width != field.width || publication_.height != field.height
            || publication_.kernelTime != field.simulationTime || publication_.format != field.format
            || publication_.transport != delivery.transport) return false;
        const auto& a = publication_.externalLease;
        const auto& b = delivery.externalLease;
        return a.has_value() == b.has_value() && (!a ||
            (a->slotIndex == b->slotIndex && a->allocationGeneration == b->allocationGeneration
             && a->readySerial == b->readySerial));
    }

private:
    friend class SculptureCanonicalPublicationInputs;
    [[nodiscard]] bool matchesInput(const EpiFeedbackFieldDelivery& delivery) const noexcept {
        return generation != 0U && productionReceiptSequence != 0U && !sourceStateId.empty()
            && std::isfinite(acceptedControlTime) && std::isfinite(productionDelta)
            && productionDelta >= 0.0f && delivery.isConsumable()
            // This host owns global semantic generations, not a preset player's
            // engine-local session. A player needs an explicit input adapter.
            && delivery.descriptor.producerSession == 0U
            && std::isfinite(delivery.descriptor.simulationTime)
            && generation == delivery.descriptor.generation
            && resetEpoch == delivery.descriptor.resetEpoch
            && sourceStateId == delivery.descriptor.sourceStateId;
    }
    bool published_ = false;
    Publication publication_{};
};

// Paired with the application's existing delivery ring. No maps, unbounded
// journal or serialization: at most one owner per selectable ring entry.
class SculptureCanonicalPublicationInputs final {
public:
    static constexpr std::size_t kCapacity = EpiFeedbackFieldDeliveryRing::kSlotCount;
    using Candidate = std::shared_ptr<SculptureCanonicalPublicationInput>;
    using Owner = std::shared_ptr<const SculptureCanonicalPublicationInput>;

    [[nodiscard]] bool publish(EpiFeedbackFieldDeliveryRing& fields,
        std::optional<EpiFeedbackFieldDelivery> delivery,
        EpiFeedbackFieldTransport expectedTransport, Candidate candidate) {
        if (!candidate || candidate->published_ || !delivery
            || !candidate->matchesInput(*delivery)) return false;
        // Native acceptance has already settled. Do not retain input for a
        // rejected publication or overwrite evidence for an older selected field.
        if (!publishCanonicalFeedbackDelivery(fields, std::move(delivery), expectedTransport))
            return false;
        const auto& field = *fields.latest();
        candidate->publication_ = {
            field.descriptor.width, field.descriptor.height, field.descriptor.simulationTime,
            field.descriptor.format, field.transport, field.externalLease};
        candidate->published_ = true;
        // Everything after ring publication is allocation-free and nonthrowing.
        entries_[next_] = std::move(candidate);
        next_ = (next_ + 1U) % kCapacity;
        return true;
    }

    [[nodiscard]] Owner find(const EpiFeedbackFieldDelivery& delivery,
                             std::uint64_t receipt) const noexcept {
        for (const auto& entry : entries_)
            if (entry && entry->matches(delivery, receipt)) return entry;
        return {};
    }

    [[nodiscard]] bool invalidate(EpiFeedbackFieldDeliveryRing& fields,
                                 std::uint64_t epoch) noexcept {
        if (!fields.invalidate(epoch)) return false;
        entries_ = {};
        next_ = 0U;
        return true;
    }

private:
    std::array<Owner, kCapacity> entries_{};
    std::size_t next_ = 0U;
};

} // namespace epi_sculpt
