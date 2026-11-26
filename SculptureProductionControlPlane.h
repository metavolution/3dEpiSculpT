#pragma once

#include "SculptureProductionReceipt.h"
#include "SculptureProductionSchedule.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace epi_sculpt {

// Resource-free application control plane for production journaling.
//
// This class deliberately does *not* decide cadence, own producers, submit GPU
// work, or retain native resources. Its job is smaller and more mechanical:
// turn one already-decided production opportunity into a stable schedule
// identity and, when work is due, begin the matching observational receipt.
// Bounded copies of finalized selected publications preserve provenance after
// journal turnover, but never make an evicted production attempt mutable again.
//
// Centralizing this transition removes four subtly different copies of the
// same ritual from SculptureApplication and gives future producers one obvious
// integration point:
//
//   producer/cadence policy -> Opportunity -> ScheduleSet + ReceiptLedger
//
// Acceptance, deferral and publication are still reported later by the actual
// producer/transport boundaries. In particular, record() never implies that
// mathematical work ran.
class SculptureProductionControlPlane final {
public:
    struct Opportunity final {
        SculptureProductionRoute route = SculptureProductionRoute::CompactInstances;
        // The route descriptor owns the legal submission boundary. Opportunity
        // callers therefore cannot create route/phase combinations that the
        // runtime has no producer transaction for.
        std::uint64_t frameSequence = 0U;
        SculptureProductionTiming timing{};
        std::uint64_t generationIntent = 0U;
        std::uint64_t resetEpoch = 0U;
        std::uint64_t semanticLeaseEpoch = 0U;
    };

    // A schedule identity is never reused, including after ledger eviction.
    // Non-due observations do not consume an identity and remain recordable.
    [[nodiscard]] bool scheduleSequenceExhausted() const noexcept {
        return scheduleSequence_ == std::numeric_limits<std::uint64_t>::max();
    }

    // Appends exactly one resource-free opportunity to the frame-local set.
    // Reject a full set, duplicate route, mixed frame, or due opportunity with
    // no legal phase before allocating a schedule identity.
    [[nodiscard]] SculptureProductionSchedule* record(
        SculptureProductionScheduleSet& schedules,
        const Opportunity& opportunity) noexcept {
        if (!schedules.canAppend(opportunity.route, opportunity.frameSequence)) return nullptr;
        const SculptureProductionRouteDescriptor descriptor =
            describeProductionRoute(opportunity.route);
        if (opportunity.timing.submit
            && (!descriptor.schedulable || scheduleSequenceExhausted())) return nullptr;

        SculptureProductionSchedule schedule{
            .route = opportunity.route,
            .phase = opportunity.timing.submit
                ? descriptor.submissionPhase
                : SculptureProductionSubmissionPhase::None,
            .scheduleSequence = opportunity.timing.submit
                ? allocateScheduleSequence()
                : 0U,
            .frameSequence = opportunity.frameSequence,
            .timing = opportunity.timing,
            .generationIntent = opportunity.generationIntent,
            .resetEpoch = opportunity.resetEpoch,
            .semanticLeaseEpoch = opportunity.semanticLeaseEpoch,
        };

        SculptureProductionSchedule* recorded = schedules.append(schedule);
        if (recorded == nullptr) return nullptr;
        if (recorded->submitDue()) {
            // A due schedule and its receipt are born as one control-plane
            // transaction. If this ever fails, the schedule contract itself is
            // inconsistent and the caller must treat record() as rejected.
            if (receipts_.begin(*recorded) == nullptr) {
                static_cast<void>(schedules.removeLastIf(recorded->scheduleSequence));
                return nullptr;
            }
        }
        return recorded;
    }

    [[nodiscard]] bool accept(
        std::uint64_t scheduleSequence,
        std::uint64_t committedGeneration,
        std::uint64_t committedResetEpoch = 0U,
        std::uint64_t committedSemanticLeaseEpoch = 0U) noexcept {
        return receipts_.accept(
            scheduleSequence,
            committedGeneration,
            committedResetEpoch,
            committedSemanticLeaseEpoch);
    }

    [[nodiscard]] bool defer(
        std::uint64_t scheduleSequence,
        SculptureProductionDeferralReason reason) noexcept {
        return receipts_.defer(scheduleSequence, reason);
    }

    // Close unattempted opportunities after the owner frame has exited. The
    // ledger's pending-only transition makes this idempotent: accepted work,
    // publication-in-flight, and earlier route-specific deferrals are retained.
    // Native callbacks must already have settled before calling this function.
    [[nodiscard]] std::size_t deferPending(
        const SculptureProductionScheduleSet& schedules,
        SculptureProductionDeferralReason reason) noexcept {
        std::size_t count = 0U;
        for (const auto& schedule : schedules.schedules()) {
            const auto* receipt = receipts_.find(schedule.scheduleSequence);
            if (receipt && receipt->frameSequence == schedule.frameSequence
                && receipts_.defer(schedule.scheduleSequence, reason)) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] bool rejectPublication(std::uint64_t scheduleSequence) noexcept {
        return receipts_.rejectPublication(scheduleSequence);
    }

    [[nodiscard]] bool publish(
        SculptureProductionRoute route,
        const SculptureProductionPublicationIdentity& identity) noexcept {
        return receipts_.publish(route, identity);
    }

    [[nodiscard]] const SculptureProductionReceipt* find(
        std::uint64_t scheduleSequence) const noexcept {
        // Execution lookup deliberately excludes retained selection provenance.
        return receipts_.find(scheduleSequence);
    }

    [[nodiscard]] const SculptureProductionReceipt* findPublished(
        SculptureProductionRoute route,
        std::uint64_t generation,
        std::uint64_t resetEpoch = 0U) const noexcept {
        // Observation lookup may outlive execution history. This grants neither
        // resource access nor permission to retry/publish an evicted attempt.
        if (const auto* receipt = receipts_.findPublished(route, generation, resetEpoch))
            return receipt;
        return findRetainedPublication(route, generation, resetEpoch);
    }

    // Pin scalar provenance for the finalized selection, not resources or work.
    // A paused/held source can outlive the rolling execution journal while other
    // routes keep producing. Only exact, already-published receipts are retained.
    // One selected publication per route matches today's ScheduleSet/presentation
    // vocabulary; replacing it does not affect acceptance or native retirement.
    [[nodiscard]] const SculptureProductionReceipt* retainSelectedPublication(
        SculptureProductionRoute route,
        std::uint64_t generation,
        std::uint64_t scheduleSequence,
        std::uint64_t resetEpoch = 0U) noexcept {
        if (scheduleSequence == 0U || generation == 0U) return nullptr;
        const SculptureProductionReceipt* source = receipts_.find(scheduleSequence);
        if (source == nullptr) source = findRetainedPublication(route, generation, resetEpoch);
        if (source == nullptr || source->scheduleSequence != scheduleSequence
            || !matchesPublication(*source, route, generation, resetEpoch)) return nullptr;

        // Reuse this route's slot first. Empty slots are shared headroom for new
        // routes; never evict another route's selected provenance under pressure.
        SculptureProductionReceipt* empty = nullptr;
        for (auto& retained : selectedPublications_) {
            if (retained.scheduleSequence != 0U && retained.route == route) {
                retained = *source;
                return &retained;
            }
            if (empty == nullptr && retained.scheduleSequence == 0U) empty = &retained;
        }
        if (empty == nullptr) return nullptr;
        *empty = *source;
        return empty;
    }

    [[nodiscard]] const SculptureProductionReceipt* latest() const noexcept {
        return receipts_.latest();
    }

private:
    [[nodiscard]] static bool matchesPublication(
        const SculptureProductionReceipt& receipt,
        SculptureProductionRoute route,
        std::uint64_t generation,
        std::uint64_t resetEpoch) noexcept {
        return receipt.published() && receipt.route == route
            && receipt.publication.generation == generation
            && (route != SculptureProductionRoute::CanonicalFeedback
                || receipt.publication.resetEpoch == resetEpoch);
    }

    [[nodiscard]] const SculptureProductionReceipt* findRetainedPublication(
        SculptureProductionRoute route,
        std::uint64_t generation,
        std::uint64_t resetEpoch) const noexcept {
        for (const auto& receipt : selectedPublications_)
            if (matchesPublication(receipt, route, generation, resetEpoch)) return &receipt;
        return nullptr;
    }

    [[nodiscard]] std::uint64_t allocateScheduleSequence() noexcept {
        // record() checks saturation before this increment. Zero remains the
        // no-submission sentinel and no old receipt identity can be reused.
        return ++scheduleSequence_;
    }

    std::uint64_t scheduleSequence_ = 0U;
    SculptureProductionReceiptLedger<> receipts_{};
    // Bounded value copies only. find()/accept()/defer()/publish() still use the
    // rolling execution ledger, so retention cannot revive an evicted attempt.
    std::array<SculptureProductionReceipt, SculptureProductionScheduleSet::kCapacity>
        selectedPublications_{};
};

} // namespace epi_sculpt
