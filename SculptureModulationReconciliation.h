#pragma once

#include "SculptureModulationEngine.h"
#include "SculptureModulationRouting.h"

namespace epi_sculpt::modulation_routing {

enum class ReconcileOutcome { Unchanged, Installed, Rejected };

// Owner-thread application policy, with no state or IO of its own. The caller
// retains the engine and distinct installed/last-request vectors. Only installed
// text describes capture truth; a rejected request is cached solely for deduplication.
// Optional diagnostics describe only this attempt and are cleared on no-op paths;
// they are not retained state or a reason to retry/relog an unchanged request.
[[nodiscard]] inline ReconcileOutcome reconcile(
    const std::vector<std::string>& authored, bool audioEnabled,
    SculptureModulationEngine& engine, std::vector<std::string>& installed,
    std::vector<std::string>& lastRequested, ModulationRouteDiagnostic* diagnostic = nullptr) {
    if (diagnostic) *diagnostic = {};
    if (matches(authored, audioEnabled, lastRequested)) return ReconcileOutcome::Unchanged;

    std::vector<std::string> effectiveRoutes = resolve(authored, audioEnabled);

    // Returning to installed intent after rejection must retain smoothing.
    if (effectiveRoutes == installed) {
        lastRequested.swap(effectiveRoutes);
        return ReconcileOutcome::Unchanged;
    }

    // Prepare both text snapshots before the engine can commit. Publication
    // afterward uses nonthrowing swaps, preserving graph/text agreement.
    auto installedRoutes = effectiveRoutes;
    ModulationRouteDiagnostic attempt;
    const bool accepted = engine.replaceRoutes(effectiveRoutes, attempt);
    if (diagnostic) *diagnostic = attempt;
    if (accepted) installed.swap(installedRoutes);
    lastRequested.swap(effectiveRoutes);
    return accepted ? ReconcileOutcome::Installed : ReconcileOutcome::Rejected;
}

} // namespace epi_sculpt::modulation_routing
