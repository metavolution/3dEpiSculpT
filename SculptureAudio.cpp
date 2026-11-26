#ifndef EPI_SCULPT_AUDIO_IO_ENABLED
#define EPI_SCULPT_AUDIO_IO_ENABLED 1
#endif

#if EPI_SCULPT_AUDIO_IO_ENABLED
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#endif

#include "SculptureAudio.h"
#include "SculptureAudioColorSync.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

#ifndef EPI_SCULPT_PI
#define EPI_SCULPT_PI 3.14159265358979323846f
#endif

namespace epi_sculpt {

namespace {

struct Biquad {
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    void setLowpass(float cutoffHz, float sampleRate, float q = 0.707f) noexcept {
        const float w0 = 2.0f * EPI_SCULPT_PI * cutoffHz / sampleRate;
        const float alpha = std::sin(w0) / (2.0f * q);
        const float cosw0 = std::cos(w0);
        const float a0 = 1.0f + alpha;
        b0 = ((1.0f - cosw0) * 0.5f) / a0;
        b1 = (1.0f - cosw0) / a0;
        b2 = ((1.0f - cosw0) * 0.5f) / a0;
        a1 = (-2.0f * cosw0) / a0;
        a2 = (1.0f - alpha) / a0;
    }

    void setBandpass(float centerHz, float sampleRate, float q = 1.0f) noexcept {
        const float w0 = 2.0f * EPI_SCULPT_PI * centerHz / sampleRate;
        const float alpha = std::sin(w0) / (2.0f * q);
        const float cosw0 = std::cos(w0);
        const float a0 = 1.0f + alpha;
        b0 = (alpha * 1.414f) / a0;
        b1 = 0.0f;
        b2 = (-alpha * 1.414f) / a0;
        a1 = (-2.0f * cosw0) / a0;
        a2 = (1.0f - alpha) / a0;
    }

    void setHighpass(float cutoffHz, float sampleRate, float q = 0.707f) noexcept {
        const float w0 = 2.0f * EPI_SCULPT_PI * cutoffHz / sampleRate;
        const float alpha = std::sin(w0) / (2.0f * q);
        const float cosw0 = std::cos(w0);
        const float a0 = 1.0f + alpha;
        b0 = ((1.0f + cosw0) * 0.5f) / a0;
        b1 = (-(1.0f + cosw0)) / a0;
        b2 = ((1.0f + cosw0) * 0.5f) / a0;
        a1 = (-2.0f * cosw0) / a0;
        a2 = (1.0f - alpha) / a0;
    }

    float process(float in) noexcept {
        const float out = b0 * in + z1;
        z1 = b1 * in - a1 * out + z2;
        z2 = b2 * in - a2 * out;
        return out;
    }

    void reset() noexcept {
        z1 = 0.0f;
        z2 = 0.0f;
    }
};

} // namespace

struct SculptureAudio::Impl {
#if EPI_SCULPT_AUDIO_IO_ENABLED
    ma_device device;
#endif
    bool initialized = false;

    // Atomically published multi-band energy signals (0.0 to 1.0)
    std::atomic<float> currentEnergy{0.0f};
    std::atomic<float> subBassEnergy{0.0f};
    std::atomic<float> bassEnergy{0.0f};
    std::atomic<float> midEnergy{0.0f};
    std::atomic<float> trebleEnergy{0.0f};
    std::atomic<float> spectralFlux{0.0f};
    std::atomic<bool> activeSignal{false};

    // Filter bank
    std::uint32_t configuredSampleRate = 0;
    Biquad filterSub;
    Biquad filterBass;
    Biquad filterMid;
    Biquad filterTreble;

    float prevTotalEnergy = 0.0f;

    // Smoothed envelope values
    float envTotal = 0.0f;
    float envSub = 0.0f;
    float envBass = 0.0f;
    float envMid = 0.0f;
    float envTreble = 0.0f;
    float envFlux = 0.0f;

    // Sonification Synth State
#if EPI_SCULPT_AUDIO_IO_ENABLED
    ma_device playbackDevice;
#endif
    bool playbackInitialized = false;
    std::atomic<SonificationMode> sonifMode{SonificationMode::Mute};
    std::atomic<float> sonifVolume{0.0f};
    std::atomic<float> targetOrbit{0.0f};
    std::atomic<float> targetMagnitude{0.0f};
    std::atomic<float> targetMorph{0.0f};
    std::atomic<float> targetLatentW{0.0f};
    std::atomic<std::uint32_t> targetInnerMap{0U};

    // Synthesizer running state
    float currentOrbit = 0.0f;
    float currentMagnitude = 0.0f;
    float currentMorph = 0.0f;
    float currentLatentW = 0.0f;
    float phaseBase = 0.0f;
    float phaseMod = 0.0f;
    float phaseHarm2 = 0.0f;
    float phaseHarm3 = 0.0f;
    float phaseHarm5 = 0.0f;
    float phaseSub = 0.0f;
    float panPhase = 0.0f;

    std::uint32_t noiseState = 1337;
    Biquad windLowpass;
    Biquad windBandpass;
    AudioEnvelopeFollower envelopeFollower;

    void ensureFilters(std::uint32_t sampleRate) {
        if (configuredSampleRate == sampleRate) return;
        configuredSampleRate = sampleRate;
        const float sr = static_cast<float>(sampleRate > 0 ? sampleRate : 48000);
        filterSub.setLowpass(120.0f, sr, 0.707f);
        filterBass.setBandpass(250.0f, sr, 1.2f);
        filterMid.setBandpass(1200.0f, sr, 0.9f);
        filterTreble.setHighpass(3500.0f, sr, 0.707f);
    }

    void processSamples(const float* samples, std::size_t numSamples, std::uint32_t sampleRate) {
        if (!samples || numSamples == 0) return;

        ensureFilters(sampleRate);

        float sumTotalSq = 0.0f;
        float sumSubSq = 0.0f;
        float sumBassSq = 0.0f;
        float sumMidSq = 0.0f;
        float sumTrebleSq = 0.0f;

        for (std::size_t i = 0; i < numSamples; ++i) {
            const float s = samples[i];
            sumTotalSq += s * s;

            const float subSample = filterSub.process(s);
            const float bassSample = filterBass.process(s);
            const float midSample = filterMid.process(s);
            const float trebleSample = filterTreble.process(s);

            sumSubSq += subSample * subSample;
            sumBassSq += bassSample * bassSample;
            sumMidSq += midSample * midSample;
            sumTrebleSq += trebleSample * trebleSample;
        }

        const float invCount = 1.0f / static_cast<float>(numSamples);
        const float rmsTotal = std::sqrt(sumTotalSq * invCount);
        const float rmsSub = std::sqrt(sumSubSq * invCount);
        const float rmsBass = std::sqrt(sumBassSq * invCount);
        const float rmsMid = std::sqrt(sumMidSq * invCount);
        const float rmsTreble = std::sqrt(sumTrebleSq * invCount);

        // Spectral flux / onset detection: positive difference of total energy
        const float rawFlux = std::max(0.0f, rmsTotal - prevTotalEnergy);
        prevTotalEnergy = rmsTotal;

        // Envelope follower dynamics: instant attack (0.7), smooth decay (0.90)
        auto follow = [](float currentEnv, float targetRms) {
            if (targetRms > currentEnv) {
                return currentEnv + 0.70f * (targetRms - currentEnv);
            }
            return currentEnv * 0.90f;
        };

        envTotal = follow(envTotal, rmsTotal * 3.0f);
        envSub = follow(envSub, rmsSub * 3.5f);
        envBass = follow(envBass, rmsBass * 3.5f);
        envMid = follow(envMid, rmsMid * 3.5f);
        envTreble = follow(envTreble, rmsTreble * 3.5f);
        envFlux = follow(envFlux, rawFlux * 5.0f);

        // Publish normalized [0.0, 1.0] signals atomically
        currentEnergy.store(std::clamp(envTotal, 0.0f, 1.0f), std::memory_order_relaxed);
        subBassEnergy.store(std::clamp(envSub, 0.0f, 1.0f), std::memory_order_relaxed);
        bassEnergy.store(std::clamp(envBass, 0.0f, 1.0f), std::memory_order_relaxed);
        midEnergy.store(std::clamp(envMid, 0.0f, 1.0f), std::memory_order_relaxed);
        trebleEnergy.store(std::clamp(envTreble, 0.0f, 1.0f), std::memory_order_relaxed);
        spectralFlux.store(std::clamp(envFlux, 0.0f, 1.0f), std::memory_order_relaxed);

        activeSignal.store(envTotal > 0.015f, std::memory_order_relaxed);
    }

    void synthesizeFrames(float* outStereo, std::size_t frameCount, std::uint32_t sampleRate) {
        if (!outStereo || frameCount == 0) return;

        const SonificationMode mode = sonifMode.load(std::memory_order_relaxed);
        if (mode == SonificationMode::Mute) {
            std::fill_n(outStereo, frameCount * 2, 0.0f);
            return;
        }

        const float volume = sonifVolume.load(std::memory_order_relaxed);
        const float tgtO = targetOrbit.load(std::memory_order_relaxed);
        const float tgtM = targetMagnitude.load(std::memory_order_relaxed);
        const float tgtMorph = targetMorph.load(std::memory_order_relaxed);
        const float tgtW = targetLatentW.load(std::memory_order_relaxed);
        const std::uint32_t inner = targetInnerMap.load(std::memory_order_relaxed);
        const float sr = static_cast<float>(sampleRate > 0 ? sampleRate : 48000);
        const float dt = (2.0f * EPI_SCULPT_PI) / sr;

        for (std::size_t i = 0; i < frameCount; ++i) {
            currentOrbit += (tgtO - currentOrbit) * 0.002f;
            currentMagnitude += (tgtM - currentMagnitude) * 0.002f;
            currentMorph += (tgtMorph - currentMorph) * 0.002f;
            currentLatentW += (tgtW - currentLatentW) * 0.002f;

            float leftSample = 0.0f;
            float rightSample = 0.0f;

            const float freqBase = 55.0f + currentMagnitude * 165.0f + std::abs(currentLatentW) * 20.0f;

            if (mode == SonificationMode::DroneFM) {
                const float freqMod = freqBase * (1.5f + currentMorph * 0.5f);
                const float modIndex = currentOrbit * 4.0f + 0.5f;

                phaseMod += freqMod * dt;
                if (phaseMod > 2.0f * EPI_SCULPT_PI) phaseMod -= 2.0f * EPI_SCULPT_PI;

                const float mod = std::sin(phaseMod) * modIndex;

                phaseBase += freqBase * dt;
                if (phaseBase > 2.0f * EPI_SCULPT_PI) phaseBase -= 2.0f * EPI_SCULPT_PI;

                float wave = std::tanh(std::sin(phaseBase + mod) * (1.0f + currentOrbit * 2.0f));
                float amp = std::clamp(currentMagnitude * 0.35f * volume, 0.0f, 0.5f);

                leftSample = wave * amp;
                rightSample = std::tanh(std::sin(phaseBase + mod + 0.35f) * (1.0f + currentOrbit * 2.0f)) * amp;
            } else if (mode == SonificationMode::HarmonicModal) {
                float wSub = 0.30f;
                float wFund = 0.50f;
                float wEven = (inner == 1U || inner == 2U) ? 0.35f : 0.10f;
                float wOdd = (inner == 5U || inner == 6U) ? 0.40f : 0.15f;
                float wFifth = 0.20f + currentMorph * 0.15f;

                phaseSub += (freqBase * 0.5f) * dt;
                phaseBase += freqBase * dt;
                phaseHarm2 += (freqBase * 2.0f) * dt;
                phaseHarm3 += (freqBase * 3.0f) * dt;
                phaseHarm5 += (freqBase * 5.0f) * dt;

                if (phaseSub > 2.0f * EPI_SCULPT_PI) phaseSub -= 2.0f * EPI_SCULPT_PI;
                if (phaseBase > 2.0f * EPI_SCULPT_PI) phaseBase -= 2.0f * EPI_SCULPT_PI;
                if (phaseHarm2 > 2.0f * EPI_SCULPT_PI) phaseHarm2 -= 2.0f * EPI_SCULPT_PI;
                if (phaseHarm3 > 2.0f * EPI_SCULPT_PI) phaseHarm3 -= 2.0f * EPI_SCULPT_PI;
                if (phaseHarm5 > 2.0f * EPI_SCULPT_PI) phaseHarm5 -= 2.0f * EPI_SCULPT_PI;

                float rawModal = std::sin(phaseSub) * wSub
                               + std::sin(phaseBase) * wFund
                               + std::sin(phaseHarm2) * wEven
                               + std::sin(phaseHarm3) * wOdd
                               + std::sin(phaseHarm5) * wFifth;

                float wave = std::tanh(rawModal * (1.2f + currentOrbit));
                float amp = std::clamp(volume * 0.4f, 0.0f, 0.5f);

                leftSample = wave * amp;
                rightSample = wave * amp;
            } else if (mode == SonificationMode::ChromaChord) {
                const float f1 = freqBase;
                const float f2 = freqBase * 1.25f;
                const float f3 = freqBase * 1.50f;

                phaseBase += f1 * dt;
                phaseHarm2 += f2 * dt;
                phaseHarm3 += f3 * dt;

                if (phaseBase > 2.0f * EPI_SCULPT_PI) phaseBase -= 2.0f * EPI_SCULPT_PI;
                if (phaseHarm2 > 2.0f * EPI_SCULPT_PI) phaseHarm2 -= 2.0f * EPI_SCULPT_PI;
                if (phaseHarm3 > 2.0f * EPI_SCULPT_PI) phaseHarm3 -= 2.0f * EPI_SCULPT_PI;

                float chord = (std::sin(phaseBase) + std::sin(phaseHarm2) + std::sin(phaseHarm3)) * 0.333f;
                float amp = std::clamp(volume * 0.35f, 0.0f, 0.5f);

                panPhase += 0.25f * dt;
                if (panPhase > 2.0f * EPI_SCULPT_PI) panPhase -= 2.0f * EPI_SCULPT_PI;
                float panL = 0.5f + 0.3f * std::sin(panPhase);
                float panR = 1.0f - panL;

                leftSample = chord * amp * panL * 2.0f;
                rightSample = chord * amp * panR * 2.0f;
            } else if (mode == SonificationMode::AerodynamicWind) {
                noiseState = noiseState * 1664525 + 1013904223;
                float noise = (static_cast<float>(noiseState) / 4294967296.0f) * 2.0f - 1.0f;
                
                if ((i & 63) == 0) {
                    float windCutoff = 200.0f + currentMagnitude * 2500.0f + currentOrbit * 1000.0f;
                    float windResonance = 1.0f + currentMorph * 5.0f;
                    windLowpass.setLowpass(windCutoff, sr, 0.707f);
                    windBandpass.setBandpass(windCutoff * 1.5f + std::abs(currentLatentW) * 500.0f, sr, windResonance);
                }
                
                float filtered = windLowpass.process(noise);
                filtered += windBandpass.process(noise) * 0.5f;
                
                float amp = std::clamp(currentMagnitude * volume * 0.8f, 0.0f, 0.5f);
                
                panPhase += 0.1f * dt;
                if (panPhase > 2.0f * EPI_SCULPT_PI) panPhase -= 2.0f * EPI_SCULPT_PI;
                float panL = std::clamp(0.5f + 0.3f * std::sin(panPhase) + currentLatentW * 0.2f, 0.1f, 0.9f);
                float panR = 1.0f - panL;

                leftSample = filtered * amp * panL * 2.0f;
                rightSample = filtered * amp * panR * 2.0f;
            }

            outStereo[i * 2 + 0] = leftSample;
            outStereo[i * 2 + 1] = rightSample;
        }
    }

#if EPI_SCULPT_AUDIO_IO_ENABLED
    static void capture_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        static_cast<void>(pOutput);
        if (pInput == nullptr) return;

        Impl* impl = static_cast<Impl*>(pDevice->pUserData);
        const float* samples = static_cast<const float*>(pInput);
        const std::size_t numSamples = frameCount * pDevice->capture.channels;

        impl->processSamples(samples, numSamples, pDevice->sampleRate);
    }

    static void playback_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        static_cast<void>(pInput);
        Impl* impl = static_cast<Impl*>(pDevice->pUserData);
        float* out = static_cast<float*>(pOutput);
        impl->synthesizeFrames(out, frameCount, pDevice->sampleRate);
    }
#endif
};

SculptureAudio::SculptureAudio() : impl_(std::make_unique<Impl>()) {}

SculptureAudio::~SculptureAudio() {
    shutdown();
}

bool SculptureAudio::hardwareIoCompiled() noexcept {
#if EPI_SCULPT_AUDIO_IO_ENABLED
    return true;
#else
    return false;
#endif
}

bool SculptureAudio::initialize(const SculptureAudioConfig& config) {
    impl_->sonifMode.store(config.sonificationMode, std::memory_order_relaxed);
    impl_->sonifVolume.store(
        std::clamp(config.sonificationVolume, 0.0f, 1.0f),
        std::memory_order_relaxed);

#if EPI_SCULPT_AUDIO_IO_ENABLED
    if (config.enableCapture) {
        ma_device_config captureConfig = ma_device_config_init(ma_device_type_capture);
        captureConfig.capture.format   = ma_format_f32;
        captureConfig.capture.channels = 1;
        captureConfig.sampleRate       = 48000;
        captureConfig.dataCallback     = Impl::capture_data_callback;
        captureConfig.pUserData        = impl_.get();

        if (ma_device_init(nullptr, &captureConfig, &impl_->device) != MA_SUCCESS) {
            std::cerr << "[Audio] Failed to initialize capture device." << std::endl;
        } else if (ma_device_start(&impl_->device) == MA_SUCCESS) {
            impl_->initialized = true;
        } else {
            ma_device_uninit(&impl_->device);
        }
    }

    // Playback is a separate, explicit opt-in. Capture-only visual modulation
    // cannot accidentally start the historical sonification drone.
    if (config.enablePlayback) {
        ma_device_config playbackConfig = ma_device_config_init(ma_device_type_playback);
        playbackConfig.playback.format   = ma_format_f32;
        playbackConfig.playback.channels = 2;
        playbackConfig.sampleRate        = 48000;
        playbackConfig.dataCallback      = Impl::playback_data_callback;
        playbackConfig.pUserData         = impl_.get();

        if (ma_device_init(nullptr, &playbackConfig, &impl_->playbackDevice) != MA_SUCCESS) {
            std::cerr << "[Audio] Failed to initialize playback device." << std::endl;
        } else if (ma_device_start(&impl_->playbackDevice) == MA_SUCCESS) {
            impl_->playbackInitialized = true;
        } else {
            ma_device_uninit(&impl_->playbackDevice);
        }
    }

    return impl_->initialized || impl_->playbackInitialized;
#else
    static_cast<void>(config);
    return false;
#endif
}

void SculptureAudio::shutdown() {
#if EPI_SCULPT_AUDIO_IO_ENABLED
    if (impl_->initialized) {
        ma_device_uninit(&impl_->device);
        impl_->initialized = false;
    }
    if (impl_->playbackInitialized) {
        ma_device_uninit(&impl_->playbackDevice);
        impl_->playbackInitialized = false;
    }
#else
    impl_->initialized = false;
    impl_->playbackInitialized = false;
#endif
}

float SculptureAudio::getEnergy() const {
    return impl_->currentEnergy.load(std::memory_order_relaxed);
}

float SculptureAudio::getBand(AudioBand band) const {
    switch (band) {
        case AudioBand::SubBass: return impl_->subBassEnergy.load(std::memory_order_relaxed);
        case AudioBand::Bass:    return impl_->bassEnergy.load(std::memory_order_relaxed);
        case AudioBand::Mid:     return impl_->midEnergy.load(std::memory_order_relaxed);
        case AudioBand::Treble:  return impl_->trebleEnergy.load(std::memory_order_relaxed);
        case AudioBand::Flux:    return impl_->spectralFlux.load(std::memory_order_relaxed);
    }
    return 0.0f;
}

std::array<float, 4> SculptureAudio::getSpectrum() const {
    return {
        impl_->bassEnergy.load(std::memory_order_relaxed),
        impl_->midEnergy.load(std::memory_order_relaxed),
        impl_->trebleEnergy.load(std::memory_order_relaxed),
        impl_->spectralFlux.load(std::memory_order_relaxed)
    };
}

float SculptureAudio::getSpectralFlux() const {
    return impl_->spectralFlux.load(std::memory_order_relaxed);
}

AudioSignalContext SculptureAudio::getSignalContext() const noexcept {
    return AudioSignalContext{
        .subBass = impl_->subBassEnergy.load(std::memory_order_relaxed),
        .bass = impl_->bassEnergy.load(std::memory_order_relaxed),
        .mid = impl_->midEnergy.load(std::memory_order_relaxed),
        .treble = impl_->trebleEnergy.load(std::memory_order_relaxed),
        .spectralFlux = impl_->spectralFlux.load(std::memory_order_relaxed)
    };
}

AudioSignalContext SculptureAudio::getBallisticContext(float deltaSeconds) noexcept {
    const AudioSignalContext raw = getSignalContext();
    return impl_->envelopeFollower.update(raw, deltaSeconds);
}

SculptureAudioObservation SculptureAudio::observeModulation(bool enabled, float deltaSeconds) noexcept {
    if (!enabled || !hasActiveSignal()) return {};
    const auto raw = getSignalContext();
    return {.active = true, .raw = raw,
            .ballistic = impl_->envelopeFollower.update(raw, deltaSeconds),
            .modalFrequencies = acousticModalFrequencies(raw)};
}

void AudioReactiveColorSync::update(
    SculptureProgramParams& params, const SculptureAudio& audio, float deltaSeconds) noexcept {
    update(params, audio.getSignalContext(), deltaSeconds);
}

bool SculptureAudio::hasActiveSignal() const {
    return impl_->activeSignal.load(std::memory_order_relaxed);
}

void SculptureAudio::setSonificationMode(SonificationMode mode) {
    impl_->sonifMode.store(mode, std::memory_order_relaxed);
}

SonificationMode SculptureAudio::getSonificationMode() const {
    return impl_->sonifMode.load(std::memory_order_relaxed);
}

void SculptureAudio::setSonificationVolume(float gain) {
    impl_->sonifVolume.store(std::clamp(gain, 0.0f, 1.0f), std::memory_order_relaxed);
}

float SculptureAudio::getSonificationVolume() const {
    return impl_->sonifVolume.load(std::memory_order_relaxed);
}

void SculptureAudio::updateLatentState(float avgOrbit, float avgMagnitude) {
    updateSonificationState(avgOrbit, avgMagnitude, 0.0f, 0.0f, 0U);
}

void SculptureAudio::updateSonificationState(float avgOrbit, float avgMagnitude, float morph, float latentW, std::uint32_t innerMap) {
    impl_->targetOrbit.store(avgOrbit, std::memory_order_relaxed);
    impl_->targetMagnitude.store(avgMagnitude, std::memory_order_relaxed);
    impl_->targetMorph.store(morph, std::memory_order_relaxed);
    impl_->targetLatentW.store(latentW, std::memory_order_relaxed);
    impl_->targetInnerMap.store(innerMap, std::memory_order_relaxed);
}

void SculptureAudio::processAudioBlock(const float* samples, std::size_t sampleCount, std::uint32_t sampleRate) {
    impl_->processSamples(samples, sampleCount, sampleRate);
}

void SculptureAudio::generateAudioBlock(float* outStereoSamples, std::size_t frameCount, std::uint32_t sampleRate) {
    impl_->synthesizeFrames(outStereoSamples, frameCount, sampleRate);
}

std::array<float, 3> SculptureAudio::getAcousticModalFrequencies() const noexcept {
    return acousticModalFrequencies(getSignalContext());
}

} // namespace epi_sculpt
