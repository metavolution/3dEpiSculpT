#include "SculptureParameterSources.h"
#include "SculptureParameterComposer.h"

namespace epi_sculpt {

SculptureParameterSource SculptureParameterSources::source() const noexcept {
    if (deckActive_) return SculptureParameterSource::PerformanceDeck;
    if (morphSpace_.isActive()) return SculptureParameterSource::MorphSpace;
    return cinematic_ ? SculptureParameterSource::Cinematic : SculptureParameterSource::PresetMorph;
}

SculptureProgramParams SculptureParameterSources::composeBase(float timeSeconds, float& compositionDelta,
    SculptureCompositionObservation* observation) {
    SculptureProgramParams params;
    switch (source()) {
    case SculptureParameterSource::PerformanceDeck:
        params = deck_.composeMasterParams(observation ? &observation->programMacros : nullptr);
        SculptureParameterComposer::applyDownbeatKickModulation(params, deck_.downbeatEnvelope());
        deck_.applyMasterFx(params, timeSeconds, compositionDelta);
        return params; // The deck already evaluated program macros, before its post-blend FX.
    case SculptureParameterSource::MorphSpace:
        params = morphSpace_.synthesizeParams();
        break;
    case SculptureParameterSource::Cinematic:
        params = sequence_.currentParams();
        break;
    case SculptureParameterSource::PresetMorph:
        params = presetMorph_.currentParams();
        break;
    }
    const bool admitted = deck_.macroEngine().tryApplyToParams(params);
    if (observation) observation->programMacros = compositionAdmission(admitted);
    return params;
}

bool SculptureParameterSources::advanceControllers(float simulationDelta,
    SculptureCompositionObservation* observation) noexcept {
    const bool playlistWasActive = presetMorph_.isPlaylistActive();
    presetMorph_.update(simulationDelta);
    const bool admitted = deck_.tryUpdate(simulationDelta,
        observation ? &observation->deckMacros : nullptr);
    if (observation) observation->deckClock = compositionAdmission(admitted);
    if (morphSpace_.isActive()) morphSpace_.update(simulationDelta);
    return playlistWasActive;
}

void SculptureParameterSources::advanceCinematic(float simulationDelta, bool preserveLoadedState) {
    sequence_playback::advance(cinematic_, sequence_, presetMorph_, simulationDelta, preserveLoadedState);
}

void SculptureParameterSources::enterCinematic() noexcept {
    sequence_playback::enter(cinematic_, sequence_);
}

void SculptureParameterSources::leaveCinematic(bool preserveLoadedState) {
    sequence_playback::leave(cinematic_, sequence_, presetMorph_, preserveLoadedState);
}

bool SculptureParameterSources::acceptSequenceLoad(bool loaded) noexcept {
    return sequence_playback::acceptLoad(loaded, cinematic_, sequence_, presetMorph_);
}

TimelineControlOutcome SculptureParameterSources::timelineControl(const EventTimelineControl& command) noexcept {
    return applyTimelineControl(command, cinematic_, sequence_, presetMorph_);
}

void SculptureParameterSources::deactivateAlternates() noexcept {
    cinematic_ = false;
    deckActive_ = false;
    morphSpace_.setActive(false);
}

} // namespace epi_sculpt
