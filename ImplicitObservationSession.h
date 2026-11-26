#pragma once

#include "ImplicitObservationCadence.h"
#include "ImplicitProductionAttempt.h"

#include <cstdint>
#include <limits>
#include <optional>

namespace epi_sculpt {

// Small control-plane helper for standalone/demo frontends that do not own the
// flagship SculptureProductionReceipt ledger. It keeps wall-clock cadence and
// generation allocation outside presentation code while allowing the renderer
// adapter to report the real producer/submission outcome asynchronously.
//
// The flagship application intentionally uses the lower-level cadence +
// observer contracts directly because it also creates schedules and receipts.
class ImplicitObservationSession final {
public:
    [[nodiscard]] std::uint32_t nextGeneration() const noexcept {
        // Zero means no new mathematical identity is available. Reusing 1
        // could alias an older field that a consumer still remembers.
        return acceptedGeneration_ == std::numeric_limits<std::uint32_t>::max()
            ? 0U : acceptedGeneration_ + 1U;
    }

    [[nodiscard]] bool identityExhausted() const noexcept {
        return acceptedGeneration_ == std::numeric_limits<std::uint32_t>::max()
            || attemptSequence_ == std::numeric_limits<std::uint64_t>::max();
    }

    [[nodiscard]] bool beginAttempt(
        double wallSeconds,
        const ImplicitFrameRequest& request,
        float updateHz) noexcept {
        if (pending_ || identityExhausted() || request.frame.generation == 0U
            || request.frame.generation != nextGeneration()) return false;
        if (!cadence_.beginAttempt(wallSeconds, request, updateHz)) return false;
        // Never reuse an observer token; an old native callback may still
        // hold it even after its original attempt settled.
        ++attemptSequence_;
        pending_ = PendingAttempt{request, attemptSequence_};
        return true;
    }

    [[nodiscard]] ImplicitProductionAttemptObserver observer() noexcept {
        return {
            .handler = &ImplicitObservationSession::observeAttempt,
            .context = this,
            .token = pending_ ? pending_->sequence : 0U,
        };
    }

    void reset() noexcept {
        // A recorded native submission may still accept mathematical work.
        // Keep its observer join alive until settlement; dropping it here would
        // allow the same generation to be offered again after a mode switch.
        if (pending_) {
            resetAfterSettlement_ = true;
            return;
        }
        cadence_.reset();
    }

    [[nodiscard]] bool pending() const noexcept { return pending_.has_value(); }
    [[nodiscard]] std::uint32_t generation() const noexcept { return acceptedGeneration_; }

private:
    struct PendingAttempt final {
        ImplicitFrameRequest request{};
        std::uint64_t sequence = 0U;
    };

    static void observeAttempt(
        void* context,
        std::uint64_t token,
        const ImplicitProductionAttemptReport& report) noexcept {
        if (context == nullptr) return;
        static_cast<ImplicitObservationSession*>(context)->settleAttempt(token, report);
    }

    void settleAttempt(
        std::uint64_t token,
        const ImplicitProductionAttemptReport& report) noexcept {
        if (!pending_ || token != pending_->sequence) return;
        const bool validReport = report.generation == pending_->request.frame.generation;
        cadence_.completeAttempt(pending_->request, validReport && report.accepted());
        if (validReport && report.accepted()) acceptedGeneration_ = report.generation;
        pending_.reset();
        if (resetAfterSettlement_) {
            // A mode switch invalidates view reuse, but must not erase a
            // generation that the producer actually accepted.
            cadence_.reset();
            resetAfterSettlement_ = false;
        }
    }

    ImplicitObservationCadence cadence_{};
    std::optional<PendingAttempt> pending_{};
    bool resetAfterSettlement_ = false;
    std::uint64_t attemptSequence_ = 0U;
    // A generation is a mathematical execution identity, not an attempt id.
    // Rejected/backpressured attempts therefore retry the same generation.
    std::uint32_t acceptedGeneration_ = 0U;
};

} // namespace epi_sculpt
