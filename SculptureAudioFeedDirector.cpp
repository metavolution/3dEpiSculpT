#include "SculptureAudioFeedDirector.h"

#include <algorithm>
#include <cstdio>
#include <cmath>

namespace epi_sculpt {

SculptureAudioFeedDirector::SculptureAudioFeedDirector(const AudioFeedDirectorConfig& config)
    : config_(config),
      mode_(config.defaultMode),
      bpm_(std::clamp(config.defaultBpm, config.minBpm, config.maxBpm)),
      masterGain_(std::max(0.0f, config.masterGain)),
      envelopeFollower_(config.envelopeConfig) {
    telemetry_.mode = mode_;
    telemetry_.bpm = bpm_;
}

void SculptureAudioFeedDirector::setMode(AudioFeedMode mode) noexcept {
    mode_ = mode;
    telemetry_.mode = mode_;
}

void SculptureAudioFeedDirector::cycleMode() noexcept {
    const auto nextMode = static_cast<AudioFeedMode>(
        (static_cast<std::uint8_t>(mode_) + 1U) % 5U
    );
    setMode(nextMode);
}

void SculptureAudioFeedDirector::setBpm(float bpm) noexcept {
    bpm_ = std::clamp(bpm, config_.minBpm, config_.maxBpm);
    telemetry_.bpm = bpm_;
}

void SculptureAudioFeedDirector::adjustBpm(float deltaBpm) noexcept {
    setBpm(bpm_ + deltaBpm);
}

void SculptureAudioFeedDirector::setMasterGain(float gain) noexcept {
    masterGain_ = std::max(0.0f, gain);
}

void SculptureAudioFeedDirector::injectExternalSignal(const AudioSignalContext& context) noexcept {
    externalContext_ = context;
}

void SculptureAudioFeedDirector::modulateFromVolumetric(float meanDensity, float peakEnergy) noexcept {
    if (!volumetricCouplingEnabled_) {
        volumetricDensityModulation_ = 0.0f;
        volumetricEnergyModulation_ = 0.0f;
        return;
    }
    const float targetDensity = std::clamp(meanDensity * 2.5f, 0.0f, 1.0f);
    const float targetEnergy = std::clamp(peakEnergy * 0.8f, 0.0f, 1.0f);
    volumetricDensityModulation_ = volumetricDensityModulation_ * 0.85f + targetDensity * 0.15f;
    volumetricEnergyModulation_ = volumetricEnergyModulation_ * 0.85f + targetEnergy * 0.15f;
}

AudioSignalContext SculptureAudioFeedDirector::generateMetronome(float t, float /*dt*/) noexcept {
    AudioSignalContext raw{};
    const float beatSec = 60.0f / std::max(bpm_, 1.0f);
    const float barSec = beatSec * 4.0f;

    // Fractional phases
    const float beatP = std::fmod(t, beatSec) / beatSec;
    const float barP = std::fmod(t, barSec) / barSec;

    // 1. Downbeat Kick (Quarter-note punch with pitch-drop envelope)
    if (beatP < 0.18f) {
        const float kickEnv = 1.0f - (beatP / 0.18f);
        raw.subBass = std::clamp(kickEnv * kickEnv * 1.2f, 0.0f, 1.0f);
        raw.bass = std::clamp(kickEnv * 0.9f, 0.0f, 1.0f);
    }

    // 2. Offbeat Snare / Clap (Beats 2 and 4 in 4/4 time: bar phase ~0.25-0.35 and ~0.75-0.85)
    const float snareP1 = std::fabs(barP - 0.25f);
    const float snareP2 = std::fabs(barP - 0.75f);
    const float snareDist = std::min(snareP1, snareP2);
    if (snareDist < 0.04f) {
        const float snareEnv = 1.0f - (snareDist / 0.04f);
        raw.mid = std::clamp(snareEnv * 0.95f, 0.0f, 1.0f);
    }

    // 3. 16th-Note Hi-Hat Pattern (Treble tick with subtle velocity groove)
    const float hatP = std::fmod(t, beatSec * 0.25f) / (beatSec * 0.25f);
    if (hatP < 0.10f) {
        const float hatEnv = 1.0f - (hatP / 0.10f);
        raw.treble = std::clamp(hatEnv * 0.65f, 0.0f, 1.0f);
    }

    // 4. Spectral Flux (Instantaneous onset dynamics)
    raw.spectralFlux = std::clamp(raw.subBass * 0.7f + raw.bass * 0.4f + raw.mid * 0.6f + raw.treble * 0.3f, 0.0f, 1.0f);

    if (volumetricCouplingEnabled_) {
        raw.subBass = std::clamp(raw.subBass * (1.0f + volumetricDensityModulation_ * 0.40f), 0.0f, 1.0f);
        raw.spectralFlux = std::clamp(raw.spectralFlux * (1.0f + volumetricEnergyModulation_ * 0.35f), 0.0f, 1.0f);
    }

    return raw;
}

AudioSignalContext SculptureAudioFeedDirector::generatePolyrhythm(float t, float /*dt*/) noexcept {
    AudioSignalContext raw{};
    const float baseHz = bpm_ / 60.0f;

    // Layer 1: 3-against-2 Euclidean Sub-Bass Pulse
    const float phase32 = std::fmod(t * baseHz * 1.5f, 1.0f);
    if (phase32 < 0.15f) {
        const float env = 1.0f - (phase32 / 0.15f);
        raw.subBass = env * env;
    }

    // Layer 2: 7/8 Euclidean Rhythm on Mid & Bass (K=4, N=7 accents)
    const float phase7 = std::fmod(t * baseHz * 1.75f, 1.0f);
    const int step7 = static_cast<int>(t * baseHz * 1.75f) % 7;
    const bool is7Accent = (step7 == 0 || step7 == 2 || step7 == 4 || step7 == 5);
    if (phase7 < 0.12f) {
        const float env = 1.0f - (phase7 / 0.12f);
        if (is7Accent) {
            raw.bass = std::max(raw.bass, env * 0.9f);
            raw.mid = std::max(raw.mid, env * 0.85f);
        } else {
            raw.mid = std::max(raw.mid, env * 0.4f);
        }
    }

    // Layer 3: 5/4 Shimmer on Treble
    const float phase5 = std::fmod(t * baseHz * 1.25f, 1.0f);
    if (phase5 < 0.08f) {
        raw.treble = 1.0f - (phase5 / 0.08f);
    }

    raw.spectralFlux = std::clamp(raw.subBass * 0.5f + raw.bass * 0.5f + raw.mid * 0.7f + raw.treble * 0.4f, 0.0f, 1.0f);

    if (volumetricCouplingEnabled_) {
        raw.bass = std::clamp(raw.bass * (1.0f + volumetricDensityModulation_ * 0.35f), 0.0f, 1.0f);
        raw.mid = std::clamp(raw.mid * (1.0f + volumetricEnergyModulation_ * 0.35f), 0.0f, 1.0f);
    }

    return raw;
}

AudioSignalContext SculptureAudioFeedDirector::generateAmbientDrone(float t, float /*dt*/) noexcept {
    AudioSignalContext raw{};

    // Incommensurate irrational frequency oscillators
    constexpr float kFreq1 = 0.070f;
    constexpr float kFreq2 = 0.113f;
    constexpr float kFreq3 = 0.187f;
    constexpr float kFreq4 = 0.293f;

    // Sub-bass tidal breathing
    raw.subBass = 0.45f + 0.40f * std::sin(t * kFreq1 * 6.2831853f);

    // Warm modal bass harmonics
    raw.bass = 0.40f + 0.35f * std::cos(t * kFreq2 * 6.2831853f) * std::sin(t * kFreq1 * 3.1415926f);

    // Mid-range singing formant swells
    raw.mid = 0.30f + 0.25f * std::sin(t * kFreq3 * 6.2831853f + 1.57f);

    // Treble crystalline shimmer
    raw.treble = 0.25f + 0.20f * std::cos(t * kFreq4 * 6.2831853f);

    // Oceanic spectral flux
    raw.spectralFlux = 0.15f + 0.10f * std::sin(t * kFreq1 * 0.5f * 6.2831853f);

    if (volumetricCouplingEnabled_) {
        raw.subBass = std::clamp(raw.subBass + volumetricDensityModulation_ * 0.25f, 0.0f, 1.0f);
        raw.treble = std::clamp(raw.treble + volumetricEnergyModulation_ * 0.25f, 0.0f, 1.0f);
        raw.spectralFlux = std::clamp(raw.spectralFlux + (volumetricDensityModulation_ + volumetricEnergyModulation_) * 0.15f, 0.0f, 1.0f);
    }

    return raw;
}

AudioSignalContext SculptureAudioFeedDirector::sampleHardware(float dt) noexcept {
    if (hardwareAudio_ != nullptr && hardwareAudio_->hasActiveSignal()) {
        telemetry_.hasHardwareAttached = true;
        telemetry_.hardwareSignalActive = true;
        return hardwareAudio_->getBallisticContext(dt);
    }

    // No device/signal is a neutral control source.  Synthetic sources remain
    // explicit modes guarded by AudioFeedDirectorConfig::enableSyntheticSources.
    telemetry_.hasHardwareAttached = (hardwareAudio_ != nullptr);
    telemetry_.hardwareSignalActive = false;
    return {};
}

AudioSignalContext SculptureAudioFeedDirector::update(float wallDeltaSeconds, float simulatedTime) noexcept {
    if (!enabled_) {
        telemetry_.rawContext = {};
        telemetry_.smoothedContext = {};
        telemetry_.aggregateEnergy = 0.0f;
        telemetry_.isKickBeat = false;
        telemetry_.isSnareBeat = false;
        return {};
    }

    const float dt = std::clamp(wallDeltaSeconds, 0.0001f, 0.1f);
    previousSimulatedTime_ = simulatedTime;

    AudioSignalContext raw{};
    switch (mode_) {
    case AudioFeedMode::SyntheticMetronome:
        if (config_.enableSyntheticSources) raw = generateMetronome(simulatedTime, dt);
        break;
    case AudioFeedMode::GenerativePolyrhythm:
        if (config_.enableSyntheticSources) raw = generatePolyrhythm(simulatedTime, dt);
        break;
    case AudioFeedMode::AmbientDroneDrift:
        if (config_.enableSyntheticSources) raw = generateAmbientDrone(simulatedTime, dt);
        break;
    case AudioFeedMode::LiveHardwareCapture:
        raw = sampleHardware(dt);
        break;
    case AudioFeedMode::ExternalStream:
        raw = externalContext_;
        break;
    }

    // Scale by master gain
    raw.subBass *= masterGain_;
    raw.bass *= masterGain_;
    raw.mid *= masterGain_;
    raw.treble *= masterGain_;
    raw.spectralFlux *= masterGain_;

    // Ballistic envelope smoothing
    AudioSignalContext smoothed = raw;
    if (config_.enableEnvelopeSmoothing) {
        smoothed = envelopeFollower_.update(raw, dt);
    }

    // Beat phase & cadence tracking
    const float beatSec = 60.0f / std::max(bpm_, 1.0f);
    const float barSec = beatSec * 4.0f;
    telemetry_.beatPhase = std::fmod(simulatedTime, beatSec) / beatSec;
    telemetry_.barPhase = std::fmod(simulatedTime, barSec) / barSec;

    const std::uint64_t currentBeats = static_cast<std::uint64_t>(simulatedTime / beatSec);
    if (currentBeats > beatCounter_) {
        beatCounter_ = currentBeats;
        telemetry_.totalBeats = currentBeats;
    }

    telemetry_.isKickBeat = (raw.subBass > 0.6f);
    telemetry_.isSnareBeat = (raw.mid > 0.5f);
    telemetry_.rawContext = raw;
    telemetry_.smoothedContext = smoothed;
    telemetry_.aggregateEnergy = (smoothed.subBass + smoothed.bass + smoothed.mid + smoothed.treble) * 0.25f;
    telemetry_.volumetricCoupled = volumetricCouplingEnabled_;
    telemetry_.volumetricDensityMod = volumetricDensityModulation_;
    telemetry_.volumetricEnergyMod = volumetricEnergyModulation_;

    return smoothed;
}

std::string SculptureAudioFeedDirector::formatHudStatus() const {
    if (!enabled_) {
        return "AUDIO: [MUTED]";
    }

    char buf[128];
    const char* modeAbbrev = "SYNTH";
    switch (mode_) {
    case AudioFeedMode::SyntheticMetronome:   modeAbbrev = "4/4"; break;
    case AudioFeedMode::GenerativePolyrhythm: modeAbbrev = "POLY"; break;
    case AudioFeedMode::AmbientDroneDrift:    modeAbbrev = "DRONE"; break;
    case AudioFeedMode::LiveHardwareCapture:  modeAbbrev = telemetry_.hardwareSignalActive ? "MIC(LIVE)" : "MIC(IDLE)"; break;
    case AudioFeedMode::ExternalStream:        modeAbbrev = "EXT"; break;
    }

    // Metric bar meters [Sub, Mid, Trb]
    const int subBars = std::clamp(static_cast<int>(telemetry_.smoothedContext.subBass * 3.99f), 0, 3);
    const int midBars = std::clamp(static_cast<int>(telemetry_.smoothedContext.mid * 3.99f), 0, 3);
    const int trbBars = std::clamp(static_cast<int>(telemetry_.smoothedContext.treble * 3.99f), 0, 3);

    const char* const subStr = (subBars == 3) ? "|||" : (subBars == 2) ? "|| " : (subBars == 1) ? "|  " : ".  ";
    const char* const midStr = (midBars == 3) ? "|||" : (midBars == 2) ? "|| " : (midBars == 1) ? "|  " : ".  ";
    const char* const trbStr = (trbBars == 3) ? "|||" : (trbBars == 2) ? "|| " : (trbBars == 1) ? "|  " : ".  ";

    if (volumetricCouplingEnabled_) {
        std::snprintf(buf, sizeof(buf), "AUDIO: %s %.0f BPM [%s|%s|%s] [SYN:VOL]",
                      modeAbbrev, bpm_, subStr, midStr, trbStr);
    } else {
        std::snprintf(buf, sizeof(buf), "AUDIO: %s %.0f BPM [%s|%s|%s]",
                      modeAbbrev, bpm_, subStr, midStr, trbStr);
    }
    return std::string(buf);
}

} // namespace epi_sculpt
