// Single authority for selecting the active visual-production route.
// The route is a control-plane decision only: producers and renderers retain
// their own resources and never infer policy from one another.
#pragma once

#include "SculpturePresentationTypes.h"

#include <string_view>

namespace epi_sculpt {

enum class SculptureProductionRoute : unsigned char {
    CompactInstances,
    CanonicalFeedback,
    // Authored canonical feedback remains its own truthful state when the
    // required producer is unavailable.  It must not silently become compact
    // instances, because that would change the mathematical authority.
    CanonicalFeedbackUnavailable,
    DedicatedWorld,
    ImplicitField,
};

// Resource owners publish only these stable, control-plane capabilities. The
// route resolver deliberately never receives a producer, transport, or native
// handle, which keeps policy testable and presentation-independent.
struct SculptureProductionCapabilities {
    bool canonicalFeedbackProducerAvailable = false;
    bool dedicatedWorldTransportAvailable = true;
};

// A route is a small control-plane fact. It states which mathematical family
// was requested and whether canonical feedback is currently available; it
// never carries a GPU handle, delivery pointer, or renderer policy.
struct SculptureProductionDecision {
    SculptureProductionRoute route = SculptureProductionRoute::CompactInstances;
    // The selected route is canonical only because an authored dedicated world
    // lacks its external-only transport. Keeping this provenance on the
    // control-plane decision makes graceful degradation observable without
    // leaking native resource state into renderers.
    bool dedicatedWorldFallback = false;

    [[nodiscard]] constexpr bool requestsCanonicalFeedback() const noexcept {
        return route == SculptureProductionRoute::CanonicalFeedback
            || route == SculptureProductionRoute::CanonicalFeedbackUnavailable;
    }

    [[nodiscard]] constexpr bool usesCanonicalFeedback() const noexcept {
        return route == SculptureProductionRoute::CanonicalFeedback;
    }

    [[nodiscard]] constexpr bool canonicalFeedbackUnavailable() const noexcept {
        return route == SculptureProductionRoute::CanonicalFeedbackUnavailable;
    }

    [[nodiscard]] constexpr bool ownsCompactProduction() const noexcept {
        return route == SculptureProductionRoute::CompactInstances;
    }

    [[nodiscard]] constexpr bool ownsDedicatedWorldProduction() const noexcept {
        return route == SculptureProductionRoute::DedicatedWorld;
    }

    [[nodiscard]] constexpr bool ownsImplicitProduction() const noexcept {
        return route == SculptureProductionRoute::ImplicitField;
    }
};

[[nodiscard]] constexpr bool requestsCanonicalFeedback(
    SculpturePresentationOwnership ownership, RenderStrategy strategy) noexcept {
    // Explicit preset intent wins. The compact + ReliefChamber combination is
    // retained for the interactive strategy toggle and older captures.
    return ownership == SculpturePresentationOwnership::CanonicalFeedback
        || (ownership == SculpturePresentationOwnership::CompactInstances
            && strategy == RenderStrategy::ReliefChamber);
}

// Producer availability is a control-plane capability, not per-frame field
// readiness. A capable canonical route may still wait for its first completed
// delivery; presentation handles that state without changing this route.
[[nodiscard]] constexpr SculptureProductionRoute resolveProductionRoute(
    SculpturePresentationOwnership ownership, RenderStrategy strategy,
    SculptureProductionCapabilities capabilities) noexcept {
    if (ownership == SculpturePresentationOwnership::ImplicitField) {
        return SculptureProductionRoute::ImplicitField;
    }
    if (ownership == SculpturePresentationOwnership::DedicatedWorld) {
        // Dedicated worlds are external-memory only. When that transport is
        // unavailable, retain OpenCL mathematical authority through the
        // canonical feedback field rather than queueing an unpresentable world
        // or silently substituting compact/CPU geometry.
        if (capabilities.dedicatedWorldTransportAvailable) {
            return SculptureProductionRoute::DedicatedWorld;
        }
        return capabilities.canonicalFeedbackProducerAvailable
            ? SculptureProductionRoute::CanonicalFeedback
            : SculptureProductionRoute::CanonicalFeedbackUnavailable;
    }
    if (requestsCanonicalFeedback(ownership, strategy)) {
        return capabilities.canonicalFeedbackProducerAvailable
            ? SculptureProductionRoute::CanonicalFeedback
            : SculptureProductionRoute::CanonicalFeedbackUnavailable;
    }
    // Unified composite is intentionally represented by multiple schedules.
    // Compact instances are its primary application route; the implicit
    // contribution is added independently at frame-recording time once the
    // concrete camera/extent are known.
    return SculptureProductionRoute::CompactInstances;
}

// Compatibility overload for lightweight callers and existing contracts.
[[nodiscard]] constexpr SculptureProductionRoute resolveProductionRoute(
    SculpturePresentationOwnership ownership, RenderStrategy strategy,
    bool canonicalFeedbackProducerAvailable,
    bool dedicatedWorldTransportAvailable = true) noexcept {
    return resolveProductionRoute(ownership, strategy, {
        canonicalFeedbackProducerAvailable,
        dedicatedWorldTransportAvailable,
    });
}

[[nodiscard]] constexpr bool isDedicatedWorldRoute(
    SculptureProductionRoute route) noexcept {
    return route == SculptureProductionRoute::DedicatedWorld;
}

[[nodiscard]] constexpr bool isCanonicalFeedbackIntentRoute(
    SculptureProductionRoute route) noexcept {
    return SculptureProductionDecision{route}.requestsCanonicalFeedback();
}

[[nodiscard]] constexpr SculptureProductionDecision resolveProductionDecision(
    SculpturePresentationOwnership ownership, RenderStrategy strategy,
    SculptureProductionCapabilities capabilities) noexcept {
    return {
        resolveProductionRoute(ownership, strategy, capabilities),
        ownership == SculpturePresentationOwnership::DedicatedWorld
            && !capabilities.dedicatedWorldTransportAvailable,
    };
}

[[nodiscard]] constexpr SculptureProductionDecision resolveProductionDecision(
    SculpturePresentationOwnership ownership, RenderStrategy strategy,
    bool canonicalFeedbackProducerAvailable,
    bool dedicatedWorldTransportAvailable = true) noexcept {
    return resolveProductionDecision(ownership, strategy, {
        canonicalFeedbackProducerAvailable,
        dedicatedWorldTransportAvailable,
    });
}

[[nodiscard]] constexpr std::string_view name(
    SculptureProductionRoute route) noexcept {
    switch (route) {
    case SculptureProductionRoute::CompactInstances: return "compact";
    case SculptureProductionRoute::CanonicalFeedback: return "canonical-relief";
    case SculptureProductionRoute::CanonicalFeedbackUnavailable:
        return "canonical-unavailable";
    case SculptureProductionRoute::DedicatedWorld: return "dedicated-world";
    case SculptureProductionRoute::ImplicitField: return "implicit";
    }
    return "unknown";
}

} // namespace epi_sculpt
