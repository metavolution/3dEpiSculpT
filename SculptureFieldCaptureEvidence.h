#pragma once

#include "SculptureCaptureIdentity.h"
#include "mgfx/ProceduralFieldAppearance.h"
#include "mgfx/ProceduralFieldContentIdentity.h"
#include "mgfx/ProceduralFieldMapping.h"
#include "mgfx/ProceduralGeometryRecipe.h"
#include "mgfx/vk/CaptureLifecycle.h"

#include <optional>
#include <string>

namespace epi_sculpt {

// Candidate copied from the payload prepared for this frame attempt. Neither
// collection nor this value alone proves that the queue accepted it.
struct SculptureFieldCaptureCandidate {
    std::uint64_t attempt = 0;
    mgfx::ProceduralFieldContentIdentity identity{};
    mgfx::ProceduralFieldMapping mapping{};
    mgfx::ProceduralFieldAppearance appearance{};
    mgfx::ProceduralGeometryRecipe recipe = mgfx::ProceduralGeometryRecipe::Count;
    [[nodiscard]] bool valid() const noexcept {
        return attempt != 0 && identity.known() && mapping.valid() && appearance.valid() &&
            recipe == mgfx::ProceduralGeometryRecipe::Field;
    }
};

struct SculptureFieldCaptureEvidence {
    SculptureFieldCaptureCandidate field{};
    mgfx::vk::CapturedViewInputs view{};
    std::uint32_t width = 0, height = 0;
    std::uint64_t submissionSerial = 0;
};

// Consume only the completed readback joined to the frame's accepted candidate.
// Publisher namespaces are process-local; these values must not be compared
// across process lifetimes without a separate durable identity contract.
inline std::optional<SculptureFieldCaptureEvidence> qualifyFieldCapture(
    const std::optional<SculptureFieldCaptureCandidate>& candidate, bool accepted,
    const SculptureCaptureIdentity& frame, const mgfx::vk::CaptureOutcome& outcome,
    bool mono) noexcept {
    if (!mono || !accepted || !candidate || !candidate->valid() ||
        outcome.status != mgfx::vk::CaptureStatus::Completed || !outcome.image.pixels ||
        !frame.matchesReadback(outcome.submissionSerial, outcome.submitted) ||
        outcome.image.width == 0 || outcome.image.height == 0 ||
        !outcome.viewInputs.familyKnown || outcome.viewInputs.viewCount != 1 ||
        !outcome.viewInputs.valid()) return std::nullopt;
    return SculptureFieldCaptureEvidence{*candidate, outcome.viewInputs,
        outcome.image.width, outcome.image.height, outcome.submissionSerial};
}

// Deliberately excludes time, lighting, post and other scene state. It reports
// equality of recorded Field/view inputs only, never pixel equality.
inline bool matchedFieldCapture(const SculptureFieldCaptureEvidence& a,
                                const SculptureFieldCaptureEvidence& b) noexcept {
    return a.field.valid() && b.field.valid() && a.submissionSerial != 0 && b.submissionSerial != 0 &&
        a.width != 0 && a.height != 0 && a.width == b.width && a.height == b.height &&
        a.view.familyKnown && b.view.familyKnown && a.view.viewCount == 1 && b.view.viewCount == 1 &&
        a.view.valid() && b.view.valid() && a.field.identity == b.field.identity &&
        a.field.mapping == b.field.mapping && a.field.appearance == b.field.appearance &&
        a.field.recipe == b.field.recipe &&
        a.view.packedViewProjection == b.view.packedViewProjection &&
        a.view.eyePosition == b.view.eyePosition;
}

// One bounded in-process reference. Refused captures never replace it.
struct SculptureFieldCaptureTracker {
    std::optional<SculptureFieldCaptureEvidence> previous;
    std::string previousPath;
};

} // namespace epi_sculpt
