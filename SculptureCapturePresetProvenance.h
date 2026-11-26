#pragma once

#include "SculptureCanonicalFeedbackSource.h"
#include "SculptureCapturedWorldRecipe.h"
#include "SculptureFramePlan.h"

#include <string_view>

namespace epi_sculpt {

// Borrow only until the caller constructs its owned WorldMode. Queued capture
// callbacks retain that value, never these views or the live preset library.
struct CapturePresetProvenance {
    std::string_view id;
    std::string_view revision;
};

[[nodiscard]] inline CapturePresetProvenance resolveCapturePresetProvenance(
    const SculptureFramePlan& frame, std::string_view libraryId,
    std::string_view libraryRevision) noexcept {
    if (frame.production().requestsCanonicalFeedback()) {
        if (const auto* source = frame.capturedCanonicalSource()) {
            return {source->presetId, source->presetRevision};
        }
    }
    if (frame.production().ownsDedicatedWorldProduction()) {
        if (const auto* source = frame.capturedWorldRecipe()) {
            return {source->presetId, source->presetRevision};
        }
    }
    return {libraryId, libraryRevision};
}

} // namespace epi_sculpt
