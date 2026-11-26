#include "SculptureAudioResonance.h"

#include <cmath>
#include <algorithm>
#include <numbers>

namespace epi_sculpt {

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;
} // namespace

SculptureAudioResonance::SculptureAudioResonance(const AudioResonanceConfig& config)
    : config_(config) {
    reset();
}

void SculptureAudioResonance::reset() noexcept {
    metrics_ = AudioResonanceMetrics{};
    modalPhases_.fill(0.0f);
    modalAmplitudes_.fill(0.1f);
    smoothedCentroid_ = 440.0f;
    accumulatedEnergy_ = 0.0f;

    // Initialize 8-node Kuramoto harmonic lattice with octave ratios
    constexpr float kHarmonics[kKuramotoNodes] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
    for (std::size_t k = 0; k < kKuramotoNodes; ++k) {
        kuramotoNaturalFreqs_[k] = kTwoPi * (55.0f * kHarmonics[k]); // A1 harmonics (55 Hz, 110 Hz, 165 Hz, ...)
        kuramotoPhases_[k] = static_cast<float>(k) * (kTwoPi / static_cast<float>(kKuramotoNodes));
    }
}

std::array<float, 6> SculptureAudioResonance::computeModalFrequencies(
    const SculptureBoundingVolume& bounds
) const noexcept {
    float lx = 2.0f;
    float ly = 2.0f;
    float lz = 2.0f;

    if (bounds.valid) {
        lx = std::max(0.5f, bounds.maxBounds.x - bounds.minBounds.x);
        ly = std::max(0.5f, bounds.maxBounds.y - bounds.minBounds.y);
        lz = std::max(0.5f, bounds.maxBounds.z - bounds.minBounds.z);
    }

    const float c = config_.speedOfSound;
    const float halfC = c * 0.5f;

    const float invLx2 = 1.0f / (lx * lx);
    const float invLy2 = 1.0f / (ly * ly);
    const float invLz2 = 1.0f / (lz * lz);

    // Fundamental and low-order axial & tangential modes [n, m, l]
    return {
        halfC * std::sqrt(invLx2),                       // (1, 0, 0)
        halfC * std::sqrt(invLy2),                       // (0, 1, 0)
        halfC * std::sqrt(invLz2),                       // (0, 0, 1)
        halfC * std::sqrt(invLx2 + invLy2),              // (1, 1, 0)
        halfC * std::sqrt(invLx2 + invLz2),              // (1, 0, 1)
        halfC * std::sqrt(invLy2 + invLz2)               // (0, 1, 1)
    };
}

float SculptureAudioResonance::evaluatePressureAt(
    const SculptureVec3& pos,
    const SculptureBoundingVolume& bounds,
    float /*time*/
) const noexcept {
    float lx = 2.0f;
    float ly = 2.0f;
    float lz = 2.0f;
    SculptureVec3 minB{-1.0f, -1.0f, -1.0f};

    if (bounds.valid) {
        lx = std::max(0.5f, bounds.maxBounds.x - bounds.minBounds.x);
        ly = std::max(0.5f, bounds.maxBounds.y - bounds.minBounds.y);
        lz = std::max(0.5f, bounds.maxBounds.z - bounds.minBounds.z);
        minB = bounds.minBounds;
    }

    const float normX = (pos.x - minB.x) / lx;
    const float normY = (pos.y - minB.y) / ly;
    const float normZ = (pos.z - minB.z) / lz;

    // Mode mode spatial weight products
    const float spatialWeights[6] = {
        std::cos(1.0f * kPi * normX),
        std::cos(1.0f * kPi * normY),
        std::cos(1.0f * kPi * normZ),
        std::cos(1.0f * kPi * normX) * std::cos(1.0f * kPi * normY),
        std::cos(1.0f * kPi * normX) * std::cos(1.0f * kPi * normZ),
        std::cos(1.0f * kPi * normY) * std::cos(1.0f * kPi * normZ)
    };

    float totalPressure = 0.0f;
    for (std::size_t k = 0; k < 6; ++k) {
        totalPressure += modalAmplitudes_[k] * spatialWeights[k] * std::cos(modalPhases_[k]);
    }

    return totalPressure;
}

void SculptureAudioResonance::rasterizePressureField3D(
    std::vector<float>& outVoxelGrid,
    std::uint32_t voxelDim,
    const SculptureBoundingVolume& bounds,
    float time
) const noexcept {
    const std::size_t totalVoxels = static_cast<std::size_t>(voxelDim) * voxelDim * voxelDim;
    if (outVoxelGrid.size() != totalVoxels) {
        outVoxelGrid.resize(totalVoxels, 0.0f);
    }
    if (voxelDim == 0) return;

    const SculptureVec3 minB = bounds.valid ? bounds.minBounds : SculptureVec3{-1.5f, -1.5f, -1.5f};
    const SculptureVec3 maxB = bounds.valid ? bounds.maxBounds : SculptureVec3{ 1.5f,  1.5f,  1.5f};
    const float stepX = (maxB.x - minB.x) / static_cast<float>(voxelDim);
    const float stepY = (maxB.y - minB.y) / static_cast<float>(voxelDim);
    const float stepZ = (maxB.z - minB.z) / static_cast<float>(voxelDim);

    std::size_t idx = 0;
    for (std::uint32_t z = 0; z < voxelDim; ++z) {
        float posZ = minB.z + (static_cast<float>(z) + 0.5f) * stepZ;
        for (std::uint32_t y = 0; y < voxelDim; ++y) {
            float posY = minB.y + (static_cast<float>(y) + 0.5f) * stepY;
            for (std::uint32_t x = 0; x < voxelDim; ++x) {
                float posX = minB.x + (static_cast<float>(x) + 0.5f) * stepX;
                const SculptureVec3 samplePos{posX, posY, posZ};
                outVoxelGrid[idx++] = evaluatePressureAt(samplePos, bounds, time);
            }
        }
    }
}

void SculptureAudioResonance::step(
    const AudioSignalContext& audio,
    const GeometricAudioFeatures& geometry,
    const SculptureBoundingVolume& bounds,
    float /*time*/,
    float deltaSeconds
) noexcept {
    const float dt = std::clamp(deltaSeconds, 0.0001f, 0.05f);

    // 1. Calculate eigenmodes
    metrics_.modalFrequenciesHz = computeModalFrequencies(bounds);

    // 2. Modulate Q-factor & damping from geometry metrics
    const float curvatureClamped = std::clamp(geometry.surfaceCurvature * 0.5f, 0.0f, 1.0f);
    const float lyapunovClamped = std::clamp(geometry.lyapunovIndex * 0.2f, 0.0f, 1.0f);
    metrics_.resonanceQFactor = std::lerp(config_.minQFactor, config_.maxQFactor, curvatureClamped);
    metrics_.bioAdaptiveDamping = std::clamp(0.02f + 0.10f * (1.0f - lyapunovClamped), 0.01f, 0.25f);

    // 3. Audio excitation mapping
    const float targetAmplitudes[6] = {
        audio.subBass * 1.2f + audio.spectralFlux * 0.4f,
        audio.bass * 1.0f + audio.spectralFlux * 0.3f,
        (audio.subBass + audio.bass) * 0.5f,
        audio.mid * 0.8f + audio.spectralFlux * 0.5f,
        audio.treble * 0.6f + audio.spectralFlux * 0.4f,
        (audio.mid + audio.treble) * 0.5f
    };

    // Smooth modal amplitudes with acoustic cavity resonance envelope
    float totalEnergy = 0.0f;
    for (std::size_t k = 0; k < 6; ++k) {
        const float decay = std::exp(-metrics_.bioAdaptiveDamping * dt * 20.0f);
        modalAmplitudes_[k] = modalAmplitudes_[k] * decay + targetAmplitudes[k] * (1.0f - decay) * config_.baseResonanceGain;

        // Advance phase
        const float omega = kTwoPi * metrics_.modalFrequenciesHz[k];
        modalPhases_[k] = std::fmod(modalPhases_[k] + omega * dt, kTwoPi);

        totalEnergy += modalAmplitudes_[k] * modalAmplitudes_[k];
    }
    metrics_.acousticEnergy = totalEnergy;

    // 4. Calculate spectral centroid
    const float sumAudio = audio.subBass + audio.bass + audio.mid + audio.treble + 1e-4f;
    const float rawCentroid = (80.0f * audio.subBass + 250.0f * audio.bass + 1200.0f * audio.mid + 5000.0f * audio.treble) / sumAudio;
    smoothedCentroid_ = std::lerp(smoothedCentroid_, rawCentroid, std::clamp(dt * 15.0f, 0.0f, 1.0f));
    metrics_.spectralCentroidHz = smoothedCentroid_;

    // 5. Center pressure evaluation
    const SculptureVec3 center = bounds.valid ? bounds.center : SculptureVec3{0.0f, 0.0f, 0.0f};
    metrics_.instantaneousPressure = evaluatePressureAt(center, bounds, 0.0f);

    // 6. Step Kuramoto Coupled Oscillator Lattice:
    // Order parameter: R * exp(i * psi) = 1/M * sum(exp(i * theta_j))
    float sumCos = 0.0f;
    float sumSin = 0.0f;
    for (std::size_t j = 0; j < kKuramotoNodes; ++j) {
        sumCos += std::cos(kuramotoPhases_[j]);
        sumSin += std::sin(kuramotoPhases_[j]);
    }
    const float invM = 1.0f / static_cast<float>(kKuramotoNodes);
    sumCos *= invM;
    sumSin *= invM;

    const float orderR = std::clamp(std::sqrt(sumCos * sumCos + sumSin * sumSin), 0.0f, 1.0f);
    const float meanPsi = std::atan2(sumSin, sumCos);
    metrics_.kuramotoOrderR = orderR;
    metrics_.kuramotoMeanPhase = meanPsi;
    metrics_.spectralEntrainment = orderR * (1.0f - metrics_.bioAdaptiveDamping);

    // Update each oscillator phase under mean-field coupling and spectral audio driving:
    const float couplingK = config_.kuramotoCouplingK;
    const float audioBandExcitations[kKuramotoNodes] = {
        audio.subBass, audio.bass, (audio.subBass + audio.bass) * 0.5f,
        audio.mid, (audio.mid + audio.treble) * 0.5f, audio.treble,
        audio.spectralFlux, (audio.mid + audio.spectralFlux) * 0.5f
    };

    for (std::size_t k = 0; k < kKuramotoNodes; ++k) {
        const float dTheta = kuramotoNaturalFreqs_[k]
            + couplingK * orderR * std::sin(meanPsi - kuramotoPhases_[k])
            + audioBandExcitations[k] * 12.0f;
        kuramotoPhases_[k] = std::fmod(kuramotoPhases_[k] + dTheta * dt, kTwoPi);
        if (kuramotoPhases_[k] < 0.0f) kuramotoPhases_[k] += kTwoPi;
    }
}

void SculptureAudioResonance::applyResonanceModulation(
    SculptureProgramParams& params,
    float /*time*/,
    float /*deltaSeconds*/
) const noexcept {
    if (!config_.enableHarmonicFeedback) return;

    const float strength = config_.couplingStrength;

    // Modulate seed pulse frequency and amplitude with primary standing wave
    params.seedPulseFrequency = std::clamp(
        params.seedPulseFrequency + (metrics_.modalFrequenciesHz[0] * 0.001f) * strength, 
        0.02f, 8.0f
    );
    params.seedPulseAmplitude = std::clamp(
        params.seedPulseAmplitude + std::abs(metrics_.instantaneousPressure) * 0.45f * strength, 
        0.0f, 2.5f
    );

    // Modulate thin-film optical interference thickness with cavity acoustic pressure
    params.filmThickness = std::clamp(
        params.filmThickness + metrics_.instantaneousPressure * 120.0f * strength, 
        100.0f, 1200.0f
    );

    // Modulate chromatic breath from spectral centroid
    params.chromaBreath = std::clamp(
        params.chromaBreath + (metrics_.spectralCentroidHz / 4000.0f) * 0.25f * strength, 
        0.0f, 1.0f
    );

    // Kuramoto harmonic synchronization drives 4D latent coordinate and temporal feedback
    params.latentW = std::clamp(
        params.latentW + (metrics_.kuramotoOrderR - 0.5f) * 0.35f * strength,
        -4.0f, 4.0f
    );
    params.temporalFeedbackBlend = std::clamp(
        params.temporalFeedbackBlend + metrics_.spectralEntrainment * 0.20f * strength,
        0.0f, 0.95f
    );

    // If 4D Hyper-Foliation is active, couple Kuramoto mean-field phase and cavity pressure
    // directly to SO(4) isoclinic rotors and ZW planar hyper-angles
    if (params.enable4D > 0U) {
        const float phaseMod = std::sin(metrics_.kuramotoMeanPhase) * metrics_.spectralEntrainment;
        params.rotorAlpha = std::clamp(params.rotorAlpha + phaseMod * 0.25f * strength, -6.283185f, 6.283185f);
        params.rotorBeta = std::clamp(params.rotorBeta + (metrics_.instantaneousPressure * 0.15f) * strength, -6.283185f, 6.283185f);
        params.rotorTheta4D = std::clamp(params.rotorTheta4D + (metrics_.kuramotoOrderR * 0.10f) * strength, -3.14159f, 3.14159f);
    }
}

} // namespace epi_sculpt
