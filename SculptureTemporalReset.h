#pragma once

#include "SculptureProductionRoute.h"

#include <cstdint>

namespace epi_sculpt {

using SculptureResetContributions = std::uint8_t;

[[nodiscard]] constexpr SculptureResetContributions temporalResetContribution(
    SculptureProductionRoute route) noexcept {
    switch (route) {
    case SculptureProductionRoute::CompactInstances: return 1U << 0U;
    case SculptureProductionRoute::CanonicalFeedback:
    case SculptureProductionRoute::CanonicalFeedbackUnavailable: return 1U << 1U;
    case SculptureProductionRoute::DedicatedWorld: return 1U << 2U;
    case SculptureProductionRoute::ImplicitField: return 1U << 3U;
    }
    return 0U;
}

[[nodiscard]] constexpr SculptureResetContributions temporalResetContributions(
    SculptureProductionDecision production,
    SculpturePresentationOwnership ownership) noexcept {
    return static_cast<SculptureResetContributions>(temporalResetContribution(production.route)
        | (includesImplicitField(ownership)
            ? temporalResetContribution(SculptureProductionRoute::ImplicitField) : 0U));
}

// A reset is a command addressed to active contributions, not durable preset
// state. Freeze this value with the frame so later acceptance acknowledges the
// exact command that was attempted, even if live input has since changed.
struct SculptureTemporalResetIntent final {
    std::uint64_t sequence = 0U;
    SculptureResetContributions contributions = 0U;

    [[nodiscard]] constexpr bool pending() const noexcept {
        return sequence != 0U && contributions != 0U;
    }

    [[nodiscard]] constexpr bool pending(SculptureProductionRoute route) const noexcept {
        return sequence != 0U && (contributions & temporalResetContribution(route)) != 0U;
    }
};

// Application control-plane bookkeeping only. Admission, resources and clocks
// remain with their existing owners. Rejected/deferred work never calls ack.
class SculptureTemporalResetControl final {
public:
    void retain(SculptureResetContributions active) noexcept {
        // A command belongs to the current histories; do not replay it if a
        // departed contribution is selected again in some later session state.
        intent_.contributions &= active;
    }

    void request(SculptureResetContributions active) noexcept {
        ++intent_.sequence;
        if (intent_.sequence == 0U) ++intent_.sequence;
        // Several resets before acceptance coalesce into the newest fresh start.
        intent_.contributions = active;
    }

    [[nodiscard]] SculptureTemporalResetIntent intent() const noexcept {
        return intent_;
    }

    void acknowledge(
        SculptureTemporalResetIntent attempted,
        SculptureProductionRoute route) noexcept {
        if (attempted.sequence != intent_.sequence || !attempted.pending(route)) return;
        intent_.contributions &= static_cast<SculptureResetContributions>(
            ~temporalResetContribution(route));
    }

private:
    SculptureTemporalResetIntent intent_{};
};

} // namespace epi_sculpt
