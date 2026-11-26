#pragma once

#include "SculptureProgramSchema.h"
#include "epi/mode/visual_mode.hpp"
#include "epi/backend/seed_image_input.hpp"

#include <string>
#include <cstdint>

namespace epi_sculpt {

// Immutable once published by the application. The full producer input already
// includes controls resolved at capture time, so only later deviations from the
// captured projection baseline may be applied. No GPU history or native extent.
struct SculptureCanonicalFeedbackSource {
    epi::VisualMode mode;
    SculptureProgramParams projectionBaseline;
    // Recorded starting provenance, independent of today's library revision.
    // Empty means unknown; it is never repaired from a newer installed asset.
    std::string presetId;
    std::string presetRevision;
    // Live resource selection, not persisted provenance or recursive history.
    // A new admitted capture must reselect an image even at the same pathname.
    std::uint64_t seedImageSelectionRevision = 0U;
    epi::SeedImageInput::Owner seedImageInput{};
};

} // namespace epi_sculpt
