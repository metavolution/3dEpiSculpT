#pragma once

#include "SculptureProductionSchedule.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

namespace epi_sculpt {

// Route-neutral execution facts for one scheduled production opportunity.
//
// This is intentionally a control-plane/read-model contract. It never owns a
// producer, native handle, semaphore, imported allocation, or delivery payload.
// Producers/transports remain the authority for resource lifetime and queue
// acceptance; the receipt only records what they reported.
enum class SculptureProductionAttemptOutcome : std::uint8_t {
    Pending,
    Deferred,
    Accepted,
};

enum class SculptureProductionDeferralReason : std::uint8_t {
    None,
    ProducerBackpressure,
    ProducerRejected,
    FrameSubmissionRejected,
    TransportBackpressure,
    TransportRejected,
    // Appended to retain the existing numeric values in observational exports.
    FrameNotRecorded,
    FrameAborted,
    CadenceProposalStale,
    GenerationExhausted,
};

enum class SculptureProductionPublicationOutcome : std::uint8_t {
    None,
    Pending,
    Published,
    Rejected,
};

enum class SculptureProductionTransport : std::uint8_t {
    Unknown,
    HostStaged,
    ExternalMemory,
};

// None denotes an attempt that has not been deferred. Reject unrecognized
// values as well, so the causal read model never invents a deferral cause.
[[nodiscard]] constexpr bool isProductionDeferralReason(
    SculptureProductionDeferralReason reason) noexcept {
    switch (reason) {
    case SculptureProductionDeferralReason::ProducerBackpressure:
    case SculptureProductionDeferralReason::ProducerRejected:
    case SculptureProductionDeferralReason::FrameSubmissionRejected:
    case SculptureProductionDeferralReason::TransportBackpressure:
    case SculptureProductionDeferralReason::TransportRejected:
    case SculptureProductionDeferralReason::FrameNotRecorded:
    case SculptureProductionDeferralReason::FrameAborted:
    case SculptureProductionDeferralReason::CadenceProposalStale:
    case SculptureProductionDeferralReason::GenerationExhausted:
        return true;
    case SculptureProductionDeferralReason::None:
        return false;
    }
    return false;
}

[[nodiscard]] constexpr std::string_view name(
    SculptureProductionAttemptOutcome outcome) noexcept {
    switch (outcome) {
    case SculptureProductionAttemptOutcome::Pending: return "pending";
    case SculptureProductionAttemptOutcome::Deferred: return "deferred";
    case SculptureProductionAttemptOutcome::Accepted: return "accepted";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view name(
    SculptureProductionDeferralReason reason) noexcept {
    switch (reason) {
    case SculptureProductionDeferralReason::None: return "none";
    case SculptureProductionDeferralReason::ProducerBackpressure: return "producer-backpressure";
    case SculptureProductionDeferralReason::ProducerRejected: return "producer-rejected";
    case SculptureProductionDeferralReason::FrameSubmissionRejected: return "frame-submission-rejected";
    case SculptureProductionDeferralReason::TransportBackpressure: return "transport-backpressure";
    case SculptureProductionDeferralReason::TransportRejected: return "transport-rejected";
    case SculptureProductionDeferralReason::FrameNotRecorded: return "frame-not-recorded";
    case SculptureProductionDeferralReason::FrameAborted: return "frame-aborted";
    case SculptureProductionDeferralReason::CadenceProposalStale: return "cadence-proposal-stale";
    case SculptureProductionDeferralReason::GenerationExhausted: return "generation-exhausted";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view name(
    SculptureProductionPublicationOutcome outcome) noexcept {
    switch (outcome) {
    case SculptureProductionPublicationOutcome::None: return "none";
    case SculptureProductionPublicationOutcome::Pending: return "pending";
    case SculptureProductionPublicationOutcome::Published: return "published";
    case SculptureProductionPublicationOutcome::Rejected: return "rejected";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view name(
    SculptureProductionTransport transport) noexcept {
    switch (transport) {
    case SculptureProductionTransport::Unknown: return "unknown";
    case SculptureProductionTransport::HostStaged: return "host-staged";
    case SculptureProductionTransport::ExternalMemory: return "external-memory";
    }
    return "unknown";
}

struct SculptureProductionPublicationIdentity final {
    static constexpr std::uint32_t kNoSlot =
        std::numeric_limits<std::uint32_t>::max();

    std::uint64_t generation = 0U;
    std::uint64_t resetEpoch = 0U;
    std::uint32_t slotIndex = kNoSlot;
    // Transport-owned epochs/serials are copied as scalar identity only. They
    // do not grant ownership or permission to touch the underlying resource.
    std::uint64_t transportLeaseEpoch = 0U;
    std::uint64_t allocationGeneration = 0U;
    std::uint64_t readySerial = 0U;
    SculptureProductionTransport transport = SculptureProductionTransport::Unknown;

    [[nodiscard]] bool valid() const noexcept {
        return generation != 0U;
    }

    [[nodiscard]] constexpr bool operator==(
        const SculptureProductionPublicationIdentity&) const noexcept = default;
};

struct SculptureProductionReceipt final {
    // Stable identity copied from the schedule. A value of zero means no
    // receipt was created because no submission opportunity was due.
    std::uint64_t scheduleSequence = 0U;
    std::uint64_t frameSequence = 0U;
    SculptureProductionRoute route = SculptureProductionRoute::CompactInstances;
    SculptureProductionSubmissionPhase phase = SculptureProductionSubmissionPhase::None;

    std::uint64_t generationIntent = 0U;
    std::uint64_t resetEpoch = 0U;
    std::uint64_t semanticLeaseEpoch = 0U;
    float simulationTime = 0.0f;
    float simulationDeltaSeconds = 0.0f;
    bool cadenceDue = false;
    bool parameterTriggered = false;

    SculptureProductionAttemptOutcome attempt = SculptureProductionAttemptOutcome::Pending;
    SculptureProductionDeferralReason deferralReason = SculptureProductionDeferralReason::None;

    // Keep accepted identity explicit instead of inferring it from intent.
    // Intent is a proposal; only a producer-accepted mathematical step may
    // populate committedGeneration. Transport attempts never consume it.
    std::uint64_t committedGeneration = 0U;
    std::uint64_t committedResetEpoch = 0U;
    std::uint64_t committedSemanticLeaseEpoch = 0U;

    SculptureProductionPublicationOutcome publicationOutcome =
        SculptureProductionPublicationOutcome::None;
    SculptureProductionPublicationIdentity publication{};

    [[nodiscard]] bool pending() const noexcept {
        return scheduleSequence != 0U
            && attempt == SculptureProductionAttemptOutcome::Pending;
    }

    [[nodiscard]] bool accepted() const noexcept {
        return attempt == SculptureProductionAttemptOutcome::Accepted
            && committedGeneration != 0U;
    }

    [[nodiscard]] bool deferred() const noexcept {
        return attempt == SculptureProductionAttemptOutcome::Deferred;
    }

    [[nodiscard]] bool published() const noexcept {
        return accepted()
            && publicationOutcome == SculptureProductionPublicationOutcome::Published
            && publication.valid();
    }

    [[nodiscard]] constexpr bool operator==(
        const SculptureProductionReceipt&) const noexcept = default;

    [[nodiscard]] static SculptureProductionReceipt begin(
        const SculptureProductionSchedule& schedule) noexcept {
        return {
            .scheduleSequence = schedule.scheduleSequence,
            .frameSequence = schedule.frameSequence,
            .route = schedule.route,
            .phase = schedule.phase,
            .generationIntent = schedule.generationIntent,
            .resetEpoch = schedule.resetEpoch,
            .semanticLeaseEpoch = schedule.semanticLeaseEpoch,
            .simulationTime = schedule.timing.simulationTime,
            .simulationDeltaSeconds = schedule.timing.simulationDeltaSeconds,
            .cadenceDue = schedule.timing.cadenceDue,
            .parameterTriggered = schedule.timing.parameterTriggered,
        };
    }
};

// Bounded application-thread ledger. The GPU pipeline itself already bounds
// accepted in-flight work to a handful of slots; this larger history window is
// for causal observability, not resource lifetime. Overwrite is deterministic
// by schedule sequence and never changes producer behavior.
template <std::size_t Capacity = 128U>
class SculptureProductionReceiptLedger final {
    static_assert(Capacity > 0U);

public:
    [[nodiscard]] SculptureProductionReceipt* begin(
        const SculptureProductionSchedule& schedule) noexcept {
        // Schedule identities are monotonically assigned by the control plane.
        // A duplicate or stale begin must not overwrite a committed receipt,
        // even after its former ring slot has been reused for newer history.
        if (!schedule.submitDue() || schedule.scheduleSequence == 0U
            || schedule.scheduleSequence <= latestScheduleSequence_) return nullptr;
        SculptureProductionReceipt& receipt = slot(schedule.scheduleSequence);
        receipt = SculptureProductionReceipt::begin(schedule);
        latestScheduleSequence_ = schedule.scheduleSequence;
        return &receipt;
    }

    [[nodiscard]] bool defer(
        std::uint64_t scheduleSequence,
        SculptureProductionDeferralReason reason) noexcept {
        if (!isProductionDeferralReason(reason)) return false;
        SculptureProductionReceipt* receipt = findMutable(scheduleSequence);
        if (receipt == nullptr || !receipt->pending()) return false;
        receipt->attempt = SculptureProductionAttemptOutcome::Deferred;
        receipt->deferralReason = reason;
        receipt->publicationOutcome = SculptureProductionPublicationOutcome::None;
        return true;
    }

    [[nodiscard]] bool accept(
        std::uint64_t scheduleSequence,
        std::uint64_t committedGeneration,
        std::uint64_t committedResetEpoch = 0U,
        std::uint64_t committedSemanticLeaseEpoch = 0U) noexcept {
        SculptureProductionReceipt* receipt = findMutable(scheduleSequence);
        if (receipt == nullptr || !receipt->pending() || committedGeneration == 0U) {
            return false;
        }
        receipt->attempt = SculptureProductionAttemptOutcome::Accepted;
        receipt->deferralReason = SculptureProductionDeferralReason::None;
        receipt->committedGeneration = committedGeneration;
        receipt->committedResetEpoch = committedResetEpoch;
        receipt->committedSemanticLeaseEpoch = committedSemanticLeaseEpoch;
        receipt->publicationOutcome = SculptureProductionPublicationOutcome::Pending;
        return true;
    }

    [[nodiscard]] bool rejectPublication(std::uint64_t scheduleSequence) noexcept {
        SculptureProductionReceipt* receipt = findMutable(scheduleSequence);
        // A delayed failure is evidence about a pending publication attempt;
        // it must not erase a delivery that was already observed as published.
        // A later valid delivery may still be observed after a rejection.
        if (receipt == nullptr || !receipt->accepted()
            || receipt->publicationOutcome != SculptureProductionPublicationOutcome::Pending) return false;
        receipt->publicationOutcome = SculptureProductionPublicationOutcome::Rejected;
        receipt->publication = {};
        return true;
    }

    // Reconciles an immutable delivery identity with the accepted production
    // that created it. Async compact/world paths may publish in a later display
    // frame, so matching is by committed generation (and canonical reset epoch)
    // rather than by current FrameContext.
    [[nodiscard]] bool publish(
        SculptureProductionRoute route,
        const SculptureProductionPublicationIdentity& identity) noexcept {
        if (!identity.valid()) return false;

        SculptureProductionReceipt* match = nullptr;
        for (SculptureProductionReceipt& receipt : receipts_) {
            if (!receipt.accepted() || receipt.route != route
                || receipt.committedGeneration != identity.generation) {
                continue;
            }
            if (route == SculptureProductionRoute::CanonicalFeedback
                && receipt.committedResetEpoch != identity.resetEpoch) {
                continue;
            }
            if (match == nullptr || receipt.scheduleSequence > match->scheduleSequence) {
                match = &receipt;
            }
        }
        if (match == nullptr) return false;

        match->publication = identity;
        match->publicationOutcome = SculptureProductionPublicationOutcome::Published;
        return true;
    }

    [[nodiscard]] const SculptureProductionReceipt* find(
        std::uint64_t scheduleSequence) const noexcept {
        if (scheduleSequence == 0U) return nullptr;
        const SculptureProductionReceipt& receipt = slot(scheduleSequence);
        return receipt.scheduleSequence == scheduleSequence ? &receipt : nullptr;
    }

    [[nodiscard]] const SculptureProductionReceipt* findPublished(
        SculptureProductionRoute route,
        std::uint64_t generation,
        std::uint64_t resetEpoch = 0U) const noexcept {
        const SculptureProductionReceipt* match = nullptr;
        for (const SculptureProductionReceipt& receipt : receipts_) {
            if (!receipt.published() || receipt.route != route
                || receipt.publication.generation != generation) {
                continue;
            }
            if (route == SculptureProductionRoute::CanonicalFeedback
                && receipt.publication.resetEpoch != resetEpoch) {
                continue;
            }
            if (match == nullptr || receipt.scheduleSequence > match->scheduleSequence) {
                match = &receipt;
            }
        }
        return match;
    }

    [[nodiscard]] const SculptureProductionReceipt* latest() const noexcept {
        return find(latestScheduleSequence_);
    }

private:
    [[nodiscard]] static constexpr std::size_t indexOf(
        std::uint64_t scheduleSequence) noexcept {
        return static_cast<std::size_t>(scheduleSequence % Capacity);
    }

    [[nodiscard]] SculptureProductionReceipt& slot(
        std::uint64_t scheduleSequence) noexcept {
        return receipts_[indexOf(scheduleSequence)];
    }

    [[nodiscard]] const SculptureProductionReceipt& slot(
        std::uint64_t scheduleSequence) const noexcept {
        return receipts_[indexOf(scheduleSequence)];
    }

    [[nodiscard]] SculptureProductionReceipt* findMutable(
        std::uint64_t scheduleSequence) noexcept {
        if (scheduleSequence == 0U) return nullptr;
        SculptureProductionReceipt& receipt = slot(scheduleSequence);
        return receipt.scheduleSequence == scheduleSequence ? &receipt : nullptr;
    }

    std::array<SculptureProductionReceipt, Capacity> receipts_{};
    std::uint64_t latestScheduleSequence_ = 0U;
};

} // namespace epi_sculpt
