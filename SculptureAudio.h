#pragma once

#include "SculptureAudioSignals.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <cmath>
#include <algorithm>

namespace epi_sculpt {

enum class AudioBand : std::uint8_t {
    SubBass = 0,
    Bass = 1,
    Mid = 2,
    Treble = 3,
    Flux = 4
};

enum class SonificationMode : std::uint8_t {
    DroneFM = 0,
    HarmonicModal = 1,
    ChromaChord = 2,
    Mute = 3,
    AerodynamicWind = 4
};

// Project-wide safety policy: visual executables never acquire an audio device
// or emit sound unless their caller makes an explicit opt-in decision.
inline constexpr bool kSculptureAudioEnabledByDefault = false;

struct SculptureAudioConfig {
    bool enableCapture = true;
    bool enablePlayback = false;
    SonificationMode sonificationMode = SonificationMode::Mute;
    float sonificationVolume = 0.0f;
};

// -----------------------------------------------------------------------------
// Educational Annotation: BALLISTIC AUDIO ENVELOPE FOLLOWER
// -----------------------------------------------------------------------------
// Translates instantaneous, noisy spectral energy into smooth musical animation
// signals. Uses asymmetric exponential attack/release ballistics:
//     y[t] = y[t-dt] + alpha_att * (x - y[t-dt])  if x > y[t-dt]
//     y[t] = y[t-dt] + alpha_rel * (x - y[t-dt])  if x <= y[t-dt]
// where alpha = 1.0 - exp(-dt / tau).
// -----------------------------------------------------------------------------
struct AudioEnvelopeFollowerConfig {
    float attackSubBass = 0.008f;  // 8ms attack (deep punch)
    float releaseSubBass = 0.120f; // 120ms release (warm sustain)
    float attackBass = 0.006f;     // 6ms attack
    float releaseBass = 0.100f;    // 100ms release
    float attackMid = 0.005f;      // 5ms attack
    float releaseMid = 0.080f;     // 80ms release
    float attackTreble = 0.002f;   // 2ms attack (crisp transients)
    float releaseTreble = 0.045f;  // 45ms release
    float attackFlux = 0.001f;     // 1ms attack (instantaneous onset)
    float releaseFlux = 0.030f;    // 30ms release
};

class AudioEnvelopeFollower {
public:
    explicit AudioEnvelopeFollower(const AudioEnvelopeFollowerConfig& config = {})
        : config_(config) {}

    void reset() noexcept {
        smoothed_ = AudioSignalContext{};
    }

    [[nodiscard]] const AudioSignalContext& update(const AudioSignalContext& raw, float deltaSeconds) noexcept {
        const float dt = std::clamp(deltaSeconds, 0.0001f, 0.1f);
        smoothed_.subBass = stepBand(raw.subBass, smoothed_.subBass, config_.attackSubBass, config_.releaseSubBass, dt);
        smoothed_.bass = stepBand(raw.bass, smoothed_.bass, config_.attackBass, config_.releaseBass, dt);
        smoothed_.mid = stepBand(raw.mid, smoothed_.mid, config_.attackMid, config_.releaseMid, dt);
        smoothed_.treble = stepBand(raw.treble, smoothed_.treble, config_.attackTreble, config_.releaseTreble, dt);
        smoothed_.spectralFlux = stepBand(raw.spectralFlux, smoothed_.spectralFlux, config_.attackFlux, config_.releaseFlux, dt);
        return smoothed_;
    }

    [[nodiscard]] const AudioSignalContext& smoothed() const noexcept { return smoothed_; }
    void setConfig(const AudioEnvelopeFollowerConfig& config) noexcept { config_ = config; }
    [[nodiscard]] const AudioEnvelopeFollowerConfig& config() const noexcept { return config_; }

private:
    static float stepBand(float target, float current, float attackTau, float releaseTau, float dt) noexcept {
        const float tau = (target > current) ? attackTau : releaseTau;
        const float alpha = 1.0f - std::exp(-dt / std::max(tau, 0.0001f));
        return current + alpha * (target - current);
    }

    AudioEnvelopeFollowerConfig config_{};
    AudioSignalContext smoothed_{};
};

class SculptureAudio {
public:
    SculptureAudio();
    ~SculptureAudio();

    [[nodiscard]] static bool hardwareIoCompiled() noexcept;

    bool initialize(const SculptureAudioConfig& config = {});
    void shutdown();

    // Returns a smoothed 0.0 - 1.0 global RMS energy value based on capture
    float getEnergy() const;

    // Returns a smoothed 0.0 - 1.0 energy for a specific frequency band
    float getBand(AudioBand band) const;

    // Returns a 4-element spectrum array [Bass, Mid, HighMid, Treble] normalized [0.0, 1.0]
    std::array<float, 4> getSpectrum() const;

    // Returns transient onset / spectral flux impulse strength [0.0 - 1.0]
    float getSpectralFlux() const;

    // Returns normalized aggregate audio signal context for modulation routing
    [[nodiscard]] AudioSignalContext getSignalContext() const noexcept;

    // Returns ballistic envelope follower context smoothed with calibrated attack/release
    [[nodiscard]] AudioSignalContext getBallisticContext(float deltaSeconds) noexcept;

    // Disabled/inactive observations do not sample or advance the envelope.
    [[nodiscard]] SculptureAudioObservation observeModulation(bool enabled, float deltaSeconds) noexcept;

    // Returns true if audio capture is active and receiving sound above noise floor
    bool hasActiveSignal() const;

    // Sonification synthesis mode and volume controls
    void setSonificationMode(SonificationMode mode);
    [[nodiscard]] SonificationMode getSonificationMode() const;
    void setSonificationVolume(float gain);
    [[nodiscard]] float getSonificationVolume() const;

    // Feed the latent state of the sculpture to drive the sonification engine
    void updateLatentState(float avgOrbit, float avgMagnitude);
    void updateSonificationState(float avgOrbit, float avgMagnitude, float morph = 0.0f, float latentW = 0.0f, std::uint32_t innerMap = 0);

    // Direct sample block analysis for offline processing / deterministic tests
    void processAudioBlock(const float* samples, std::size_t sampleCount, std::uint32_t sampleRate = 48000);

    // Direct stereo sample block generation for offline processing / deterministic tests
    void generateAudioBlock(float* outStereoSamples, std::size_t frameCount, std::uint32_t sampleRate = 48000);

    // Returns 3D acoustic standing-wave modal frequencies [n, m, l] driven by harmonic spectrum
    [[nodiscard]] std::array<float, 3> getAcousticModalFrequencies() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace epi_sculpt
