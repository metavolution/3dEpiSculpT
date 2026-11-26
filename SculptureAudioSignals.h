#pragma once

#include <array>
#include <cmath>

namespace epi_sculpt {

struct AudioSignalContext {
    float subBass = 0.0f;
    float bass = 0.0f;
    float mid = 0.0f;
    float treble = 0.0f;
    float spectralFlux = 0.0f;
};

// One assembled input for an application evaluation. Raw color excitation and
// ballistic resonance/routing envelopes deliberately have different semantics.
struct SculptureAudioObservation {
    bool active = false;
    AudioSignalContext raw{};
    AudioSignalContext ballistic{};
    std::array<float, 3> modalFrequencies{};
};

[[nodiscard]] inline std::array<float, 3> acousticModalFrequencies(
    const AudioSignalContext& signal) noexcept {
    return {2.0f + std::floor(signal.bass * 4.0f),
            1.0f + std::floor(signal.mid * 3.0f),
            3.0f + std::floor(signal.treble * 5.0f)};
}

} // namespace epi_sculpt
