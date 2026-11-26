#pragma once

#include "SculptureAudioSignals.h"
#include "SculptureProgramSchema.h"

#include <algorithm>
#include <cmath>

namespace epi_sculpt {

class SculptureAudio;

class AudioReactiveColorSync {
public:
    struct Config {
        float attackTimeSeconds = 0.015f;   // 15ms fast attack
        float releaseTimeSeconds = 0.120f;  // 120ms smooth decay
        float fluxThreshold = 0.08f;
        float chromaSensitivity = 0.50f;
        float lumaSensitivity = 0.35f;
        float hueDriftRate = 0.05f;
    };

    AudioReactiveColorSync() = default;
    explicit AudioReactiveColorSync(const Config& config)
        : config_(config) {}

    void setConfig(const Config& config) noexcept { config_ = config; }
    [[nodiscard]] const Config& config() const noexcept { return config_; }

    // Device adapter implemented with SculptureAudio, outside the pure math closure.
    void update(SculptureProgramParams& params, const SculptureAudio& audio, float dt) noexcept;

    void update(SculptureProgramParams& params, const AudioSignalContext& audio, float dt) noexcept {
        const float targetSubBass = audio.subBass;
        const float targetBass = audio.bass;
        const float targetMid = audio.mid;
        const float targetTreble = audio.treble;
        const float targetFlux = audio.spectralFlux;

        smoothedSubBass_ = smoothFollower(smoothedSubBass_, targetSubBass, dt, config_.attackTimeSeconds, config_.releaseTimeSeconds);
        smoothedBass_ = smoothFollower(smoothedBass_, targetBass, dt, config_.attackTimeSeconds, config_.releaseTimeSeconds);
        smoothedMid_ = smoothFollower(smoothedMid_, targetMid, dt, config_.attackTimeSeconds, config_.releaseTimeSeconds);
        smoothedTreble_ = smoothFollower(smoothedTreble_, targetTreble, dt, config_.attackTimeSeconds, config_.releaseTimeSeconds);
        smoothedFlux_ = smoothFollower(smoothedFlux_, targetFlux, dt, 0.005f, 0.080f);

        transientOnset_ = (smoothedFlux_ > config_.fluxThreshold);

        // Map audio spectrum into GPU parameter block
        params.audioSpectrum = {
            smoothedSubBass_,
            smoothedBass_,
            smoothedMid_,
            smoothedTreble_
        };

        // Modulate perceptual OKLCH color dynamics
        params.chromaBreath = std::clamp(params.chromaBreath + smoothedBass_ * config_.chromaSensitivity, 0.0f, 1.0f);
        params.lumaDrift = std::clamp(params.lumaDrift + smoothedTreble_ * config_.lumaSensitivity, 0.0f, 1.0f);

        // 4D Latent-W Audio Reactivity
        if (params.enable4D) {
            params.latentW += smoothedMid_ * 0.15f * dt;
        }

        if (transientOnset_) {
            accumulatedHueShift_ += config_.hueDriftRate * smoothedFlux_ * (dt * 60.0f);
            params.paletteBlend = std::clamp(params.paletteBlend + 0.15f * smoothedFlux_, 0.0f, 1.0f);
        }
        params.hueRotation = std::fmod(params.hueRotation + accumulatedHueShift_, 1.0f);

        // ---------------------------------------------------------------------
        // NEW FEATURE: Procedural Material/Environment Audio-Modulation
        // ---------------------------------------------------------------------
        
        // 1. Participating Media (Volumetric Fog) pulses dynamically with Bass energy
        params.volumetricDensityScale += smoothedBass_ * 1.5f;

        // 2. Thin-Film Optical Wave Iridescence shimmers via structural thickness modulation
        params.filmThickness += (smoothedMid_ * 150.0f + smoothedTreble_ * 150.0f);

        // 3. Fluid Advection & Vector Curl Turbulence erupt on Spectral Flux transients
        params.advectionSpeed += smoothedFlux_ * 1.5f;
        params.curlStrength += smoothedFlux_ * 3.0f;
        
        // 4. Kinetic velocity trails persist longer during massive SubBass drops
        params.trailDecay = std::clamp(params.trailDecay + smoothedSubBass_ * 0.04f, 0.0f, 0.999f);

        // 5. Geometry structural expansion (organic scale breathing) on SubBass
        params.radiusScale += smoothedSubBass_ * 0.15f;
    }

    [[nodiscard]] float smoothedSubBass() const noexcept { return smoothedSubBass_; }
    [[nodiscard]] float smoothedBass() const noexcept { return smoothedBass_; }
    [[nodiscard]] float smoothedMid() const noexcept { return smoothedMid_; }
    [[nodiscard]] float smoothedTreble() const noexcept { return smoothedTreble_; }
    [[nodiscard]] float smoothedFlux() const noexcept { return smoothedFlux_; }
    [[nodiscard]] bool transientOnset() const noexcept { return transientOnset_; }

    [[nodiscard]] static float smoothFollower(float current, float target, float dt, float attack, float release) noexcept {
        const float timeConstant = (target > current) ? attack : release;
        if (timeConstant <= 1.0e-5f || dt <= 0.0f) return target;
        const float alpha = 1.0f - std::exp(-dt / timeConstant);
        return current + alpha * (target - current);
    }

private:
    Config config_;
    float smoothedSubBass_ = 0.0f;
    float smoothedBass_ = 0.0f;
    float smoothedMid_ = 0.0f;
    float smoothedTreble_ = 0.0f;
    float smoothedFlux_ = 0.0f;
    bool transientOnset_ = false;
    float accumulatedHueShift_ = 0.0f;
};

} // namespace epi_sculpt
