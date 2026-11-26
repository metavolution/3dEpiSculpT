#pragma once

namespace epi_sculpt {

enum class PresentationRetirementResult {
    Ready,
    SculptureDrainFailed,
    CanonicalDrainFailed,
    GraphicsDrainFailed,
};

// Retirement is permitted only after every applicable owner proves completion.
// The caller retains ownership of cancellation, diagnostics and native resources.
template <class Cancel, class SculptureDrain, class CanonicalDrain,
          class GraphicsDrain, class Retire>
PresentationRetirementResult retirePresentationAfterDrains(
    Cancel&& cancel, SculptureDrain&& sculptureDrain,
    CanonicalDrain&& canonicalDrain, GraphicsDrain&& graphicsDrain,
    Retire&& retire) {
    cancel();
    if (!sculptureDrain()) return PresentationRetirementResult::SculptureDrainFailed;
    if (!canonicalDrain()) return PresentationRetirementResult::CanonicalDrainFailed;
    if (!graphicsDrain()) return PresentationRetirementResult::GraphicsDrainFailed;
    retire();
    return PresentationRetirementResult::Ready;
}

} // namespace epi_sculpt
