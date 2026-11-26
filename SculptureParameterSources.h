#pragma once

#include "SculptureCompositionObservation.h"
#include "SculptureMorphChoreographer.h"
#include "SculptureMorphSpace.h"
#include "SculpturePerformanceDeck.h"
#include "SculptureTimelineControl.h"

namespace epi_sculpt {

enum class SculptureParameterSource { PresetMorph, Cinematic, MorphSpace, PerformanceDeck };

// The source controllers and their precedence have one lifetime. Native asset
// retirement, producer admission and presentation are deliberately not members.
class SculptureParameterSources {
public:
    [[nodiscard]] SculptureParameterSource source() const noexcept;
    // Borrowed observation output records only this call's program macro result.
    [[nodiscard]] SculptureProgramParams composeBase(float timeSeconds, float& compositionDelta,
        SculptureCompositionObservation* observation = nullptr);

    // Preserve the preset -> deck -> active morph-space update order. Return the
    // pre-update playlist fact so the host can reconcile a just-completed cue.
    // Optional output records deck clock/macros without retaining diagnostics.
    [[nodiscard]] bool advanceControllers(float simulationDelta,
        SculptureCompositionObservation* observation = nullptr) noexcept;
    void advanceCinematic(float simulationDelta, bool preserveLoadedState);
    void enterCinematic() noexcept;
    void leaveCinematic(bool preserveLoadedState);
    [[nodiscard]] bool acceptSequenceLoad(bool loaded) noexcept;
    [[nodiscard]] TimelineControlOutcome timelineControl(const EventTimelineControl& command) noexcept;

    [[nodiscard]] bool cinematic() const noexcept { return cinematic_; }
    [[nodiscard]] bool deckActive() const noexcept { return deckActive_; }
    void setDeckActive(bool active) noexcept { deckActive_ = active; }
    // A decoded parameter moment takes over before its controllers are restored.
    // The old cinematic handoff is deferred to advanceCinematic, as before.
    void deactivateAlternates() noexcept;

    // Explicit editor/import ports to the single owned controllers. These are
    // transitional mutation surfaces, not additional cached state owners.
    [[nodiscard]] SculptureMorphChoreographer& presetMorph() noexcept { return presetMorph_; }
    [[nodiscard]] const SculptureMorphChoreographer& presetMorph() const noexcept { return presetMorph_; }
    [[nodiscard]] SculptureSequencer& sequence() noexcept { return sequence_; }
    [[nodiscard]] const SculptureSequencer& sequence() const noexcept { return sequence_; }
    [[nodiscard]] SculpturePerformanceDeck& deck() noexcept { return deck_; }
    [[nodiscard]] const SculpturePerformanceDeck& deck() const noexcept { return deck_; }
    [[nodiscard]] SculptureMorphSpace& morphSpace() noexcept { return morphSpace_; }
    [[nodiscard]] const SculptureMorphSpace& morphSpace() const noexcept { return morphSpace_; }

private:
    SculptureMorphChoreographer presetMorph_;
    SculptureSequencer sequence_;
    SculpturePerformanceDeck deck_;
    SculptureMorphSpace morphSpace_;
    bool cinematic_ = false;
    bool deckActive_ = false;
};

struct SculptureSourceAdvance {
    float simulationDelta = 0.0f;
    bool playlistWasActive = false;
    SculptureCompositionObservation composition{};
};

} // namespace epi_sculpt
