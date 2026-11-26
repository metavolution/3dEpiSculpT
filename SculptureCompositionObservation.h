#pragma once

#include <cstdint>

namespace epi_sculpt {

// Facts about this evaluation's checked authoring stages, never control policy.
// Admitted includes valid no-ops; it is not a whole-frame numerical guarantee.
enum class SculptureCompositionAdmission : std::uint8_t {
    NotEvaluated,
    Admitted,
    Held
};

[[nodiscard]] constexpr SculptureCompositionAdmission compositionAdmission(bool admitted) noexcept {
    return admitted ? SculptureCompositionAdmission::Admitted : SculptureCompositionAdmission::Held;
}

// Copied into the immutable frame. No pointers, history or retained live owner.
// A skipped stage stays NotEvaluated rather than claiming success or refusal.
struct SculptureCompositionObservation {
    SculptureCompositionAdmission deckClock = SculptureCompositionAdmission::NotEvaluated;
    SculptureCompositionAdmission deckMacros = SculptureCompositionAdmission::NotEvaluated;
    SculptureCompositionAdmission programMacros = SculptureCompositionAdmission::NotEvaluated;
    SculptureCompositionAdmission manualRotors = SculptureCompositionAdmission::NotEvaluated;

    [[nodiscard]] constexpr bool hasHeldStages() const noexcept {
        return deckClock == SculptureCompositionAdmission::Held
            || deckMacros == SculptureCompositionAdmission::Held
            || programMacros == SculptureCompositionAdmission::Held
            || manualRotors == SculptureCompositionAdmission::Held;
    }
};

} // namespace epi_sculpt
