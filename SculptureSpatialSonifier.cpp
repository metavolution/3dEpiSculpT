#include "SculptureSpatialSonifier.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace epi_sculpt {

namespace {

constexpr float kTwoPi = 2.0f * static_cast<float>(std::numbers::pi);

} // namespace

SculptureSpatialSonifier::SculptureSpatialSonifier()
    : SculptureSpatialSonifier(SonifierVoiceConfig{}) {}

SculptureSpatialSonifier::SculptureSpatialSonifier(const SonifierVoiceConfig& config)
    : config_(config)
{
    reset();
}

void SculptureSpatialSonifier::reset() noexcept {
    waveguidePhase_ = 0.0f;
    waveguideFreq_ = config_.basePitchHz;
    delayBuffer_.fill(0.0f);
    delayWriteIdx_ = 0;

    fmCarrierPhase_ = 0.0f;
    fmModulatorPhase_ = 0.0f;
    fmCarrierFreq_ = config_.basePitchHz * 2.0f;
    fmModulatorFreq_ = config_.basePitchHz * 3.0f;
    fmIndex_ = 1.0f;

    // Initialize 6 resonant harmonic modes
    const float baseF = config_.basePitchHz;
    modalResonators_[0] = {baseF * 1.0f,  20.0f, 0.0f, 0.0f};
    modalResonators_[1] = {baseF * 1.498f, 25.0f, 0.0f, 0.0f}; // Perfect Fifth
    modalResonators_[2] = {baseF * 2.0f,  30.0f, 0.0f, 0.0f}; // Octave
    modalResonators_[3] = {baseF * 2.67f, 35.0f, 0.0f, 0.0f};
    modalResonators_[4] = {baseF * 3.0f,  40.0f, 0.0f, 0.0f};
    modalResonators_[5] = {baseF * 4.23f, 45.0f, 0.0f, 0.0f};

    constexpr std::array<float, 8> kJustRatios = {1.0f, 1.125f, 1.25f, 1.333333f, 1.5f, 1.666667f, 1.875f, 2.0f};
    for (std::size_t k = 0; k < 8; ++k) {
        octonionPhases_[k] = 0.0f;
        octonionFreqs_[k] = baseF * kJustRatios[k];
        octonionAmps_[k] = 0.5f;
    }

    itdDelayL_.fill(0.0f);
    itdDelayR_.fill(0.0f);
    itdWriteIdx_ = 0;
    smoothedPanL_ = 0.707f;
    smoothedPanR_ = 0.707f;
    smoothedItdSamples_ = 0.0f;
    spectralBands_.fill(0.0f);
}

GeometricAudioFeatures SculptureSpatialSonifier::extractFeatures(
    const SculptureSnapshot& snapshot
) noexcept {
    GeometricAudioFeatures feat{};
    if (snapshot.samples.empty()) return feat;

    float sumX = 0.0f, sumY = 0.0f, sumZ = 0.0f;
    float sumVx = 0.0f, sumVy = 0.0f, sumVz = 0.0f;
    float sumLyap = 0.0f;
    float sumCurv = 0.0f;
    float sumEnergy = 0.0f;
    std::array<float, 8> octMoments{};
    std::size_t activeCount = 0;

    for (const auto& s : snapshot.samples) {
        if (s.radius <= 0.0f) continue;
        activeCount++;

        sumX += s.x;
        sumY += s.y;
        sumZ += s.z;

        sumVx += s.vx;
        sumVy += s.vy;
        sumVz += s.vz;

        float vMag = std::sqrt(s.vx * s.vx + s.vy * s.vy + s.vz * s.vz);
        sumLyap += std::log1p(vMag);

        float nMag = std::sqrt(s.nx * s.nx + s.ny * s.ny + s.nz * s.nz);
        sumCurv += nMag;

        sumEnergy += (s.r + s.g + s.b) * 0.3333f;

        octMoments[0] += 1.0f;
        octMoments[1] += std::abs(s.x);
        octMoments[2] += std::abs(s.y);
        octMoments[3] += std::abs(s.z);
        octMoments[4] += std::abs(s.vx);
        octMoments[5] += std::abs(s.vy);
        octMoments[6] += std::abs(s.vz);
        octMoments[7] += vMag;
    }

    feat.activeCount = activeCount;
    if (activeCount == 0) return feat;

    const float invCount = 1.0f / static_cast<float>(activeCount);
    feat.centroid = {sumX * invCount, sumY * invCount, sumZ * invCount};
    feat.meanVelocity = {sumVx * invCount, sumVy * invCount, sumVz * invCount};
    feat.lyapunovIndex = sumLyap * invCount;
    feat.surfaceCurvature = sumCurv * invCount;
    feat.spectralEnergy = sumEnergy * invCount;

    for (std::size_t k = 0; k < 8; ++k) {
        feat.octonionicMoments[k] = octMoments[k] * invCount;
    }

    // Second pass: Radius of Gyration
    float sumDistSq = 0.0f;
    for (const auto& s : snapshot.samples) {
        if (s.radius <= 0.0f) continue;
        float dx = s.x - feat.centroid.x;
        float dy = s.y - feat.centroid.y;
        float dz = s.z - feat.centroid.z;
        sumDistSq += (dx * dx + dy * dy + dz * dz);
    }
    feat.radiusOfGyration = std::sqrt(sumDistSq * invCount);

    return feat;
}

GeometricAudioFeatures SculptureSpatialSonifier::extractFeaturesFromMesh(
    const IsosurfaceMesh& mesh
) noexcept {
    GeometricAudioFeatures feat{};
    if (mesh.vertices.empty()) return feat;

    float sumX = 0.0f, sumY = 0.0f, sumZ = 0.0f;
    float sumCurv = 0.0f;
    const std::size_t count = mesh.vertices.size();

    for (const auto& v : mesh.vertices) {
        sumX += v.position.x;
        sumY += v.position.y;
        sumZ += v.position.z;
        float nMag = std::sqrt(v.normal.x * v.normal.x + v.normal.y * v.normal.y + v.normal.z * v.normal.z);
        sumCurv += nMag;
    }

    const float invCount = 1.0f / static_cast<float>(count);
    feat.activeCount = count;
    feat.centroid = {sumX * invCount, sumY * invCount, sumZ * invCount};
    feat.surfaceCurvature = sumCurv * invCount;
    feat.lyapunovIndex = 0.5f;
    feat.spectralEnergy = 0.6f;

    for (std::size_t k = 0; k < 8; ++k) {
        feat.octonionicMoments[k] = 0.5f;
    }

    float sumDistSq = 0.0f;
    for (const auto& v : mesh.vertices) {
        float dx = v.position.x - feat.centroid.x;
        float dy = v.position.y - feat.centroid.y;
        float dz = v.position.z - feat.centroid.z;
        sumDistSq += (dx * dx + dy * dy + dz * dz);
    }
    feat.radiusOfGyration = std::sqrt(sumDistSq * invCount);

    return feat;
}

void SculptureSpatialSonifier::updateFeatures(const GeometricAudioFeatures& features) noexcept {
    features_ = features;

    // 1. Waveguide String Frequency
    const float basePitch = config_.basePitchHz;
    waveguideFreq_ = std::clamp(basePitch * (0.8f + features.radiusOfGyration * 0.4f), 40.0f, 660.0f);

    // 2. FM Synthesis Parameters
    fmCarrierFreq_ = std::clamp(basePitch * 2.0f * (0.9f + std::abs(features.centroid.x) * 0.3f), 80.0f, 1200.0f);
    fmModulatorFreq_ = fmCarrierFreq_ * (1.5f + 0.5f * std::clamp(features.surfaceCurvature, 0.0f, 2.0f));
    fmIndex_ = std::clamp(0.5f + features.lyapunovIndex * 3.0f, 0.1f, 6.0f);

    // 3. Modal Resonator Tuning
    const float cavityScale = std::max(features.radiusOfGyration, 0.5f);
    modalResonators_[0].f = (basePitch * 1.00f) / cavityScale;
    modalResonators_[1].f = (basePitch * 1.50f) / cavityScale;
    modalResonators_[2].f = (basePitch * 2.00f) / cavityScale;
    modalResonators_[3].f = (basePitch * 2.67f) / cavityScale;
    modalResonators_[4].f = (basePitch * 3.14f) / cavityScale;
    modalResonators_[5].f = (basePitch * 4.25f) / cavityScale;

    // 4. Octonionic Harmonic Cluster Tuning
    constexpr std::array<float, 8> kJustRatios = {1.0f, 1.125f, 1.25f, 1.333333f, 1.5f, 1.666667f, 1.875f, 2.0f};
    for (std::size_t k = 0; k < 8; ++k) {
        octonionFreqs_[k] = basePitch * kJustRatios[k] * (0.95f + 0.1f * features_.octonionicMoments[k]);
        octonionAmps_[k] = std::clamp(0.2f + 0.8f * features_.octonionicMoments[k], 0.05f, 1.0f);
    }

    // 5. Binaural Spatial Panning & Inter-aural Time Difference (ITD)
    float azimuth = std::atan2(features.centroid.x, -features.centroid.z);
    float normAzimuth = std::clamp(azimuth / 3.14159265f, -1.0f, 1.0f); // [-1 (left), +1 (right)]
    float panAngle = (normAzimuth + 1.0f) * 0.25f * 3.14159265f; // [0, pi/2]

    float targetPanL = std::cos(panAngle);
    float targetPanR = std::sin(panAngle);
    float targetItd = normAzimuth * 24.0f;

    // Smooth panning transitions to avoid audio clicks
    smoothedPanL_ = std::lerp(smoothedPanL_, targetPanL, 0.15f);
    smoothedPanR_ = std::lerp(smoothedPanR_, targetPanR, 0.15f);
    smoothedItdSamples_ = std::lerp(smoothedItdSamples_, targetItd, 0.15f);
}

void SculptureSpatialSonifier::generateAudioBlock(
    float* outStereo,
    std::size_t frameCount,
    std::uint32_t sampleRate
) noexcept {
    if (!outStereo || frameCount == 0 || sampleRate == 0) return;

    const float dt = 1.0f / static_cast<float>(sampleRate);
    const float wgPhaseInc = waveguideFreq_ * dt;
    const float fmCarPhaseInc = fmCarrierFreq_ * dt;
    const float fmModPhaseInc = fmModulatorFreq_ * dt;

    // Delay line length for Karplus-Strong string
    const std::size_t delayLen = std::clamp(
        static_cast<std::size_t>(static_cast<float>(sampleRate) / std::max(waveguideFreq_, 20.0f)),
        std::size_t{2},
        delayBuffer_.size() - 1
    );

    float sumSub = 0.0f, sumBass = 0.0f, sumMid = 0.0f, sumHigh = 0.0f;

    for (std::size_t i = 0; i < frameCount; ++i) {
        float sampleMono = 0.0f;
        float subMono = 0.0f;
        float bassMono = 0.0f;
        float midMono = 0.0f;
        float highMono = 0.0f;

        // --- Voice 0: Waveguide String Physical Model ---
        if (config_.enableWaveguide) {
            std::size_t readIdx = (delayWriteIdx_ + delayBuffer_.size() - delayLen) % delayBuffer_.size();
            float delayed = delayBuffer_[readIdx];
            std::size_t nextReadIdx = (readIdx + 1) % delayBuffer_.size();
            float nextDelayed = delayBuffer_[nextReadIdx];

            // Lowpass loop filter (damping)
            float filtered = 0.5f * (delayed + nextDelayed) * 0.992f;

            // Excitation from mathematical orbit phase
            float excitation = std::sin(waveguidePhase_ * kTwoPi) * 0.15f;
            float newSample = filtered + excitation;

            delayBuffer_[delayWriteIdx_] = newSample;
            delayWriteIdx_ = (delayWriteIdx_ + 1) % delayBuffer_.size();

            float wgVal = newSample * config_.waveguideGain;
            sampleMono += wgVal;
            bassMono += wgVal;
            waveguidePhase_ += wgPhaseInc;
            if (waveguidePhase_ >= 1.0f) waveguidePhase_ -= 1.0f;
        }

        // --- Voice 1: Dual-Operator FM Chime Array ---
        if (config_.enableFM) {
            float modSignal = std::sin(fmModulatorPhase_ * kTwoPi);
            float carSignal = std::sin(fmCarrierPhase_ * kTwoPi + fmIndex_ * modSignal);
            float fmVal = carSignal * config_.fmChimeGain * (0.5f + 0.5f * features_.spectralEnergy);

            sampleMono += fmVal;
            midMono += fmVal;

            fmModulatorPhase_ += fmModPhaseInc;
            if (fmModulatorPhase_ >= 1.0f) fmModulatorPhase_ -= 1.0f;

            fmCarrierPhase_ += fmCarPhaseInc;
            if (fmCarrierPhase_ >= 1.0f) fmCarrierPhase_ -= 1.0f;
        }

        // --- Voice 2: 3D Modal Resonators ---
        if (config_.enableModal) {
            float modalSum = 0.0f;
            float impulse = (i == 0) ? (0.3f * features_.lyapunovIndex) : 0.0f;

            for (auto& res : modalResonators_) {
                float w = 2.0f * 3.14159265f * res.f * dt;
                float r = 1.0f - (3.14159265f * res.f / (res.q * static_cast<float>(sampleRate)));
                float y = 2.0f * r * std::cos(w) * res.y1 - r * r * res.y2 + impulse;
                res.y2 = res.y1;
                res.y1 = y;
                modalSum += y;
            }

            float modalVal = std::clamp(modalSum * config_.modalResonatorGain, -1.0f, 1.0f);
            sampleMono += modalVal;
            subMono += modalVal * 0.5f;
            bassMono += modalVal * 0.5f;
        }

        // --- Voice 4: Octonionic Harmonic Cluster ---
        if (config_.enableOctonionicCluster) {
            float octSum = 0.0f;
            for (std::size_t k = 0; k < 8; ++k) {
                float octVal = std::sin(octonionPhases_[k] * kTwoPi) * octonionAmps_[k];
                octSum += octVal;
                octonionPhases_[k] += octonionFreqs_[k] * dt;
                if (octonionPhases_[k] >= 1.0f) octonionPhases_[k] -= 1.0f;
            }
            float octScaled = (octSum * 0.125f) * config_.octonionicClusterGain;
            sampleMono += octScaled;
            highMono += octScaled;
        }

        // --- Voice 3: Spatial Binaural Panning & Fractional ITD ---
        itdDelayL_[itdWriteIdx_] = sampleMono * smoothedPanL_ * config_.masterGain;
        itdDelayR_[itdWriteIdx_] = sampleMono * smoothedPanR_ * config_.masterGain;

        float leftSample = itdDelayL_[itdWriteIdx_];
        float rightSample = itdDelayR_[itdWriteIdx_];

        if (config_.enableBinaural) {
            if (smoothedItdSamples_ > 0.0f) {
                int delayInt = static_cast<int>(smoothedItdSamples_);
                float frac = smoothedItdSamples_ - static_cast<float>(delayInt);
                std::size_t rIdx0 = (itdWriteIdx_ + itdDelayL_.size() - delayInt) % itdDelayL_.size();
                std::size_t rIdx1 = (itdWriteIdx_ + itdDelayL_.size() - delayInt - 1) % itdDelayL_.size();
                leftSample = std::lerp(itdDelayL_[rIdx0], itdDelayL_[rIdx1], frac);
            } else if (smoothedItdSamples_ < 0.0f) {
                float absItd = -smoothedItdSamples_;
                int delayInt = static_cast<int>(absItd);
                float frac = absItd - static_cast<float>(delayInt);
                std::size_t rIdx0 = (itdWriteIdx_ + itdDelayR_.size() - delayInt) % itdDelayR_.size();
                std::size_t rIdx1 = (itdWriteIdx_ + itdDelayR_.size() - delayInt - 1) % itdDelayR_.size();
                rightSample = std::lerp(itdDelayR_[rIdx0], itdDelayR_[rIdx1], frac);
            }
        }

        itdWriteIdx_ = (itdWriteIdx_ + 1) % itdDelayL_.size();

        // Soft saturation to prevent clipping
        leftSample = std::tanh(leftSample);
        rightSample = std::tanh(rightSample);

        outStereo[i * 2 + 0] = leftSample;
        outStereo[i * 2 + 1] = rightSample;

        sumSub += subMono * subMono;
        sumBass += bassMono * bassMono;
        sumMid += midMono * midMono;
        sumHigh += highMono * highMono;
    }

    const float invFrames = 1.0f / static_cast<float>(frameCount);
    spectralBands_[0] = std::clamp(std::sqrt(sumSub * invFrames), 0.0f, 1.0f);
    spectralBands_[1] = std::clamp(std::sqrt(sumBass * invFrames), 0.0f, 1.0f);
    spectralBands_[2] = std::clamp(std::sqrt(sumMid * invFrames), 0.0f, 1.0f);
    spectralBands_[3] = std::clamp(std::sqrt(sumHigh * invFrames), 0.0f, 1.0f);
}

std::array<float, 4> SculptureSpatialSonifier::currentSpectralBands() const noexcept {
    return spectralBands_;
}

void SculptureSpatialSonifier::setConfig(const SonifierVoiceConfig& config) noexcept {
    config_ = config;
}

const SonifierVoiceConfig& SculptureSpatialSonifier::config() const noexcept {
    return config_;
}

const GeometricAudioFeatures& SculptureSpatialSonifier::currentFeatures() const noexcept {
    return features_;
}

} // namespace epi_sculpt
