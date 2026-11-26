#pragma once

#include <cstdint>

#include "SculptureProgramSchema.h"
#include "SculptureSampleHistory.h"

namespace epi_sculpt {

// History is meaningful only within one evolution family. Keeping this policy
// independent of OpenCL/Vulkan lets future SDF or mesh outputs add a family
// without changing producer-slot ownership code.
[[nodiscard]] inline SculptureTemporalFamily temporalFamily(
    const SculptureProgramParams& params) noexcept {
    return params.outputMode == 1U && params.outputModeB == 1U
        ? SculptureTemporalFamily::Voxel
        : SculptureTemporalFamily::Surface;
}

// Temporal reset establishes a new world-memory epoch. No prior output or
// spatial field may cross that boundary into the next producer submission.
[[nodiscard]] inline bool shouldUseTemporalHistory(
    const SculptureProgramParams& params) noexcept {
    return !params.temporalReset;
}

[[nodiscard]] inline bool canUseTemporalHistory(
    const SculptureProgramParams& request,
    SculptureTemporalFamily historyFamily) noexcept {
    return shouldUseTemporalHistory(request)
        && temporalFamily(request) == historyFamily;
}

// A changed sampling scale starts a cold source-sample domain. Spatial field
// memory has its own stable lattice and retains the family/reset policy above.
[[nodiscard]] inline bool canUseSourceSampleHistory(
    const SculptureProgramParams& request, SculptureTemporalFamily historyFamily,
    StageResolutionScale historyScale) noexcept {
    return canUseTemporalHistory(request, historyFamily)
        && request.auxiliaryResolutionScale == historyScale;
}

} // namespace epi_sculpt
