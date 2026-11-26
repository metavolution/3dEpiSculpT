#pragma once

#include "SculptureAudioColorSync.h"
#include "SculptureAudioResonance.h"
#include "SculptureAudioSignals.h"

#include <optional>

namespace epi_sculpt {

// An owned numerical observation, not a borrow of live control state. Copying
// resonance retains bounded phases/amplitudes/metrics; no device or grid storage.
struct SculptureAcousticMistObservation {
    SculptureAudioResonance resonance;
    SculptureBoundingVolume bounds;
    float timeSeconds = 0.0f;
};

class SculptureAudioModulation {
public:
    // Call before apply(): rotor entrainment uses the previous evaluation's
    // resonance metrics, while routing consumes the metrics after this step.
    [[nodiscard]] float rotorEntrainment(bool active) const noexcept {
        return active ? 1.0f + 0.4f * resonance_.metrics().kuramotoOrderR : 1.0f;
    }

    [[nodiscard]] std::optional<SculptureAcousticMistObservation> apply(
        SculptureProgramParams& params,
        const SculptureAudioObservation& audio,
        float timeSeconds,
        float deltaSeconds) noexcept;

    [[nodiscard]] const AudioResonanceMetrics& metrics() const noexcept {
        return resonance_.metrics();
    }

private:
    AudioReactiveColorSync color_{};
    SculptureAudioResonance resonance_{};
};

} // namespace epi_sculpt
