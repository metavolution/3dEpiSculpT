#pragma once

namespace epi_sculpt {

// Route-neutral temporal facts attached to a production opportunity.
//
// This intentionally lives below both the generic schedule and the concrete
// cadence implementations. A cadence may use arbitrary private state and a
// producer may use arbitrary parameter types; the schedule only needs the
// immutable facts required for observability and later settlement.
struct SculptureProductionTiming final {
    bool submit = false;
    float simulationDeltaSeconds = 0.0f;
    float simulationTime = 0.0f;
    bool cadenceDue = false;
    bool parameterTriggered = false;
};

// Structural adapter: any proposal exposing this small vocabulary can be
// projected into the generic schedule. Keeping this helper duck-typed avoids a
// reverse dependency from control-plane contracts into producer cadence code.
template <typename TProposal>
[[nodiscard]] constexpr SculptureProductionTiming makeProductionTiming(
    const TProposal& proposal) noexcept {
    return {
        .submit = proposal.submit,
        .simulationDeltaSeconds = proposal.simulationDeltaSeconds,
        .simulationTime = proposal.simulationTime,
        .cadenceDue = proposal.cadenceDue,
        .parameterTriggered = proposal.parameterTriggered,
    };
}

} // namespace epi_sculpt
