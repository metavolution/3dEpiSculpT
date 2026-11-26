#pragma once

#include "SculptureAudio.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: UNIFIED AUDIO SYNESTHESIA DIRECTOR
// -----------------------------------------------------------------------------
// In epimorphic procedural computing and acoustic-geometric resonance, audio
// is not merely an accompaniment—it is a first-class continuous modulation driver.
//
// Previously, executables relied on inline, ad-hoc synthetic trigonometric pulses
// (e.g. fmod(t * 2.0f, 1.0f)), scattering signal generation across client loops
// and preventing unified mode switching, MIDI/OSC injection, or live capture fallback.
//
// SculptureAudioFeedDirector encapsulates:
// 1. Multi-Feed Source Switching:
//    - SyntheticMetronome: 4/4 electronic dance pulse (kick, snare, hat, sub-bass sweep).
//    - GenerativePolyrhythm: Euclidean layered cross-rhythms (3:2, 7/8, poly-accents).
//    - AmbientDroneDrift: Incommensurate harmonic sine waves with slow continuous drift.
//    - LiveHardwareCapture: Real microphone / line capture via SculptureAudio.
//    - ExternalStream: Programmatic / network injection via MIDI or OSC.
// 2. Calibrated Ballistic Smoothing:
//    Integrated AudioEnvelopeFollower with asymmetric attack/release times per band.
// 3. Transient Impulse Extraction:
//    Dynamic threshold tracking to trigger rhythmic geometric mutations and lighting flashes.
// -----------------------------------------------------------------------------

enum class AudioFeedMode : std::uint8_t {
    SyntheticMetronome = 0, // 4/4 electronic dance pulse
    GenerativePolyrhythm,  // Euclidean layered cross-rhythms (3:2, 7/8)
    AmbientDroneDrift,     // Continuous slow-moving harmonic acoustic drone
    LiveHardwareCapture,   // Real hardware microphone capture via SculptureAudio
    ExternalStream         // Externally injected signal context (MIDI / OSC)
};

[[nodiscard]] constexpr std::string_view audioFeedModeName(AudioFeedMode mode) noexcept {
    switch (mode) {
    case AudioFeedMode::SyntheticMetronome:   return "SyntheticMetronome";
    case AudioFeedMode::GenerativePolyrhythm: return "GenerativePolyrhythm";
    case AudioFeedMode::AmbientDroneDrift:    return "AmbientDroneDrift";
    case AudioFeedMode::LiveHardwareCapture:  return "LiveHardwareCapture";
    case AudioFeedMode::ExternalStream:        return "ExternalStream";
    default:                                  return "Unknown";
    }
}

struct AudioFeedDirectorConfig {
    // Keep visual execution silent unless a real or explicitly injected source is present.
    AudioFeedMode defaultMode = AudioFeedMode::LiveHardwareCapture;
    // Synthetic generators are a development/future-reactive opt-in, never a default driver.
    bool enableSyntheticSources = false;
    float defaultBpm = 120.0f;
    float minBpm = 40.0f;
    float maxBpm = 280.0f;
    float masterGain = 1.0f;
    bool enableEnvelopeSmoothing = true;
    AudioEnvelopeFollowerConfig envelopeConfig{};
    bool enableTransientDetection = true;
};

struct AudioFeedTelemetry {
    AudioFeedMode mode = AudioFeedMode::LiveHardwareCapture;
    float bpm = 120.0f;
    float beatPhase = 0.0f;        // [0.0, 1.0) within current beat
    float barPhase = 0.0f;         // [0.0, 1.0) within 4/4 bar
    std::uint64_t totalBeats = 0;
    bool isKickBeat = false;
    bool isSnareBeat = false;
    AudioSignalContext rawContext{};
    AudioSignalContext smoothedContext{};
    float aggregateEnergy = 0.0f;
    bool hasHardwareAttached = false;
    bool hardwareSignalActive = false;
    bool volumetricCoupled = false;
    float volumetricDensityMod = 0.0f;
    float volumetricEnergyMod = 0.0f;
};

class SculptureAudioFeedDirector {
public:
    explicit SculptureAudioFeedDirector(const AudioFeedDirectorConfig& config = {});
    ~SculptureAudioFeedDirector() = default;

    // Source feed mode control
    void setMode(AudioFeedMode mode) noexcept;
    [[nodiscard]] AudioFeedMode mode() const noexcept { return mode_; }
    void cycleMode() noexcept;

    // Tempo & rhythm control
    void setBpm(float bpm) noexcept;
    void adjustBpm(float deltaBpm) noexcept;
    [[nodiscard]] float bpm() const noexcept { return bpm_; }

    // Master gain & bypass
    void setMasterGain(float gain) noexcept;
    [[nodiscard]] float masterGain() const noexcept { return masterGain_; }
    void setEnabled(bool enabled) noexcept { enabled_ = enabled; }
    [[nodiscard]] bool isEnabled() const noexcept { return enabled_; }
    void toggleEnabled() noexcept { enabled_ = !enabled_; }

    // Hardware capture device coupling
    void attachHardwareAudio(std::shared_ptr<SculptureAudio> audio) noexcept {
        hardwareAudio_ = std::move(audio);
    }
    [[nodiscard]] SculptureAudio* hardwareAudio() noexcept { return hardwareAudio_.get(); }
    [[nodiscard]] const SculptureAudio* hardwareAudio() const noexcept { return hardwareAudio_.get(); }

    // External signal stream injection (MIDI / OSC)
    void injectExternalSignal(const AudioSignalContext& context) noexcept;

    // Main synchronous tick: computes raw bands, applies ballistics, updates telemetry
    [[nodiscard]] AudioSignalContext update(float wallDeltaSeconds, float simulatedTime) noexcept;

    // 3D Volumetric Recursive Memory Reverse Modulation
    void modulateFromVolumetric(float meanDensity, float peakEnergy) noexcept;
    void setVolumetricCouplingEnabled(bool enabled) noexcept { volumetricCouplingEnabled_ = enabled; }
    [[nodiscard]] bool isVolumetricCouplingEnabled() const noexcept { return volumetricCouplingEnabled_; }
    [[nodiscard]] float volumetricDensityModulation() const noexcept { return volumetricDensityModulation_; }
    [[nodiscard]] float volumetricEnergyModulation() const noexcept { return volumetricEnergyModulation_; }

    // Telemetry & diagnostics
    [[nodiscard]] const AudioFeedTelemetry& telemetry() const noexcept { return telemetry_; }
    [[nodiscard]] const AudioSignalContext& currentSignalContext() const noexcept { return telemetry_.smoothedContext; }
    [[nodiscard]] std::string formatHudStatus() const;

private:
    [[nodiscard]] AudioSignalContext generateMetronome(float t, float dt) noexcept;
    [[nodiscard]] AudioSignalContext generatePolyrhythm(float t, float dt) noexcept;
    [[nodiscard]] AudioSignalContext generateAmbientDrone(float t, float dt) noexcept;
    [[nodiscard]] AudioSignalContext sampleHardware(float /*dt*/) noexcept;

    AudioFeedDirectorConfig config_{};
    AudioFeedMode mode_ = AudioFeedMode::LiveHardwareCapture;
    float bpm_ = 120.0f;
    float masterGain_ = 1.0f;
    bool enabled_ = true;

    bool volumetricCouplingEnabled_ = true;
    float volumetricDensityModulation_ = 0.0f;
    float volumetricEnergyModulation_ = 0.0f;

    std::shared_ptr<SculptureAudio> hardwareAudio_{};
    AudioSignalContext externalContext_{};

    AudioEnvelopeFollower envelopeFollower_;
    AudioFeedTelemetry telemetry_{};

    float accumulatedBeatPhase_ = 0.0f;
    float previousSimulatedTime_ = 0.0f;
    std::uint64_t beatCounter_ = 0;
};

} // namespace epi_sculpt
