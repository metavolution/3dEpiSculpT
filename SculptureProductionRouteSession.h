#pragma once

#include "SculptureProductionRoute.h"

namespace epi_sculpt {

// Tracks only which mathematical producer families are currently active.
// Resource/cadence owners remain elsewhere. The value returned from observe()
// tells the application which *inactive* cadence histories became stale at a
// route boundary, so the frame dispatcher no longer resets unrelated producers
// on every display tick.
struct SculptureProductionRouteTransition final {
    bool routeChanged = false;
    bool canonicalActivityChanged = false;
    bool implicitActivityChanged = false;
    bool resetCompactCadence = false;
    bool resetWorldCadence = false;
    bool resetCanonicalCadence = false;
    bool resetImplicitCadence = false;
};

class SculptureProductionRouteSession final {
public:
    [[nodiscard]] SculptureProductionRouteTransition observe(
        SculptureProductionRoute nextRoute,
        bool implicitPresentationActive) noexcept {
        const bool nextCanonical =
            nextRoute == SculptureProductionRoute::CanonicalFeedback;
        const bool previousCanonical = initialized_ &&
            route_ == SculptureProductionRoute::CanonicalFeedback;

        SculptureProductionRouteTransition transition{};
        transition.routeChanged = !initialized_ || route_ != nextRoute;
        transition.canonicalActivityChanged =
            !initialized_ ? nextCanonical : previousCanonical != nextCanonical;
        transition.implicitActivityChanged =
            !initialized_ || implicitPresentationActive_ != implicitPresentationActive;

        if (transition.routeChanged) {
            // A producer that is not selected by the new primary route must not
            // resume later with temporal history accumulated before this route
            // boundary. Reset once here rather than repeatedly while inactive.
            transition.resetCompactCadence =
                nextRoute != SculptureProductionRoute::CompactInstances;
            transition.resetWorldCadence =
                nextRoute != SculptureProductionRoute::DedicatedWorld;
            transition.resetCanonicalCadence =
                nextRoute != SculptureProductionRoute::CanonicalFeedback;
        }

        // Canonical recursive state has explicit enter/leave semantics. Its
        // cadence restarts on either boundary; the application separately owns
        // reset-epoch invalidation because that also touches published fields.
        if (transition.canonicalActivityChanged) {
            transition.resetCanonicalCadence = true;
        }

        // Implicit observation can coexist with compact production in unified
        // composite presentation. Leaving that presentation invalidates only
        // observation cadence, not compact mathematical history.
        transition.resetImplicitCadence =
            transition.implicitActivityChanged && !implicitPresentationActive;

        route_ = nextRoute;
        implicitPresentationActive_ = implicitPresentationActive;
        initialized_ = true;
        return transition;
    }

    void reset() noexcept {
        initialized_ = false;
        route_ = SculptureProductionRoute::CompactInstances;
        implicitPresentationActive_ = false;
    }

    [[nodiscard]] bool initialized() const noexcept { return initialized_; }
    [[nodiscard]] SculptureProductionRoute route() const noexcept { return route_; }
    [[nodiscard]] bool implicitPresentationActive() const noexcept {
        return implicitPresentationActive_;
    }

private:
    bool initialized_ = false;
    SculptureProductionRoute route_ = SculptureProductionRoute::CompactInstances;
    bool implicitPresentationActive_ = false;
};

} // namespace epi_sculpt
