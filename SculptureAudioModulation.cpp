#include "SculptureAudioModulation.h"

namespace epi_sculpt {

std::optional<SculptureAcousticMistObservation> SculptureAudioModulation::apply(
    SculptureProgramParams& params,
    const SculptureAudioObservation& audio,
    float timeSeconds,
    float deltaSeconds) noexcept {
    if (!audio.active) {
        params.audioSpectrum = {};
        return std::nullopt;
    }

    color_.update(params, audio.raw, deltaSeconds);
    params.seedPulseFrequency = 0.05f * audio.modalFrequencies[0];
    params.audioSpectrum = {audio.ballistic.bass, audio.ballistic.mid,
                           audio.ballistic.treble, audio.ballistic.spectralFlux};

    SculptureBoundingVolume bounds{};
    bounds.minBounds = {-1.5f, -1.5f, -1.5f};
    bounds.maxBounds = { 1.5f,  1.5f,  1.5f};
    bounds.valid = true;
    GeometricAudioFeatures geometry{};
    geometry.surfaceCurvature = 0.5f;
    resonance_.step(audio.ballistic, geometry, bounds, timeSeconds, deltaSeconds);
    resonance_.applyResonanceModulation(params, timeSeconds, deltaSeconds);
    return SculptureAcousticMistObservation{resonance_, bounds, timeSeconds};
}

} // namespace epi_sculpt
