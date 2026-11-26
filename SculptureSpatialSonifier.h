#pragma once

#include "SculptureProgramSchema.h"
#include "SculptureIsosurface.h"

#include <array>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <string_view>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: BIDIRECTIONAL SPATIAL SONIFICATION & HARMONIC RESONA
// -----------------------------------------------------------------------------
// Translates 3D/4D mathematical geometry and dynamical attractor orbits directly
// into spatialized audio waveforms:
//
// 1. Geometric Feature Analysis:
//    - Centroid C = 1/N sum(p_i)
//    - Gyration Radius R_g = sqrt(1/N sum(||p_i - C||^2))
//    - Mean Orbit Velocity v_mean
//    - Lyapunov Divergence Index Lambda = 1/N sum(ln(1 + ||v_i||))
//    - Surface Curvature proxy from normals kappa = 1/N sum(||n_i||)
//
// 2. Multi-Voice Acoustic Synthesizer:
//    - Voice 0: Waveguide String Physical Model (tuned to 4D winding harmonics)
//    - Voice 1: Dynamic Dual-Operator FM Chime Array (carrier: centroid, mod: Lyapunov)
//    - Voice 2: 3D Helmholtz Acoustic Modal Resonators (f_nml = c/2 * sqrt((n/Lx)^2 + ...))
//    - Voice 3: Binaural 3D Spatializer (ITD & ILD panning based on 3D centroid position)
// -----------------------------------------------------------------------------

struct GeometricAudioFeatures {
    SculptureVec3 centroid{0.0f, 0.0f, 0.0f};
    SculptureVec3 meanVelocity{0.0f, 0.0f, 0.0f};
    float radiusOfGyration = 0.0f;
    float lyapunovIndex = 0.0f;
    float surfaceCurvature = 0.0f;
    float spectralEnergy = 0.0f;
    std::array<float, 8> octonionicMoments{};
    std::size_t activeCount = 0;
};

struct SonifierVoiceConfig {
    float masterGain = 0.8f;
    float waveguideGain = 0.4f;
    float fmChimeGain = 0.35f;
    float modalResonatorGain = 0.3f;
    float octonionicClusterGain = 0.25f;
    float basePitchHz = 110.0f; // A2
    bool enableBinaural = true;
    bool enableWaveguide = true;
    bool enableFM = true;
    bool enableModal = true;
    bool enableOctonionicCluster = true;
};

class SculptureSpatialSonifier {
public:
    SculptureSpatialSonifier();
    explicit SculptureSpatialSonifier(const SonifierVoiceConfig& config);
    ~SculptureSpatialSonifier() = default;

    // Feature extraction from live sculpture snapshot
    [[nodiscard]] static GeometricAudioFeatures extractFeatures(
        const SculptureSnapshot& snapshot
    ) noexcept;

    // Feature extraction from polygon isosurface mesh
    [[nodiscard]] static GeometricAudioFeatures extractFeaturesFromMesh(
        const IsosurfaceMesh& mesh
    ) noexcept;

    // Updates internal synthesis parameters from geometric audio features
    void updateFeatures(const GeometricAudioFeatures& features) noexcept;

    // Direct sample generation: fills outStereo (interleaved L, R) with frameCount frames
    void generateAudioBlock(
        float* outStereo,
        std::size_t frameCount,
        std::uint32_t sampleRate = 48000
    ) noexcept;

    // Extracts instantaneous 4-band RMS audio energy (sub, bass, mid, high)
    // suitable for real-time visual parameter modulation feedback
    [[nodiscard]] std::array<float, 4> currentSpectralBands() const noexcept;

    // Configuration
    void setConfig(const SonifierVoiceConfig& config) noexcept;
    [[nodiscard]] const SonifierVoiceConfig& config() const noexcept;
    [[nodiscard]] const GeometricAudioFeatures& currentFeatures() const noexcept;

    void reset() noexcept;

private:
    SonifierVoiceConfig config_{};
    GeometricAudioFeatures features_{};
    std::array<float, 4> spectralBands_{0.0f, 0.0f, 0.0f, 0.0f};

    // Voice 0: Waveguide string oscillator
    float waveguidePhase_ = 0.0f;
    float waveguideFreq_ = 110.0f;
    std::array<float, 1024> delayBuffer_{};
    std::size_t delayWriteIdx_ = 0;

    // Voice 1: Dual-operator FM chime
    float fmCarrierPhase_ = 0.0f;
    float fmModulatorPhase_ = 0.0f;
    float fmCarrierFreq_ = 220.0f;
    float fmModulatorFreq_ = 110.0f;
    float fmIndex_ = 1.0f;

    // Voice 2: 3D Modal Resonators (6 resonant modes)
    struct Resonator {
        float f = 200.0f;
        float q = 15.0f;
        float y1 = 0.0f;
        float y2 = 0.0f;
    };
    std::array<Resonator, 6> modalResonators_{};

    // Voice 4: Octonionic Harmonic Cluster (8-voice microtonal Just-Intonation chord)
    std::array<float, 8> octonionPhases_{};
    std::array<float, 8> octonionFreqs_{};
    std::array<float, 8> octonionAmps_{};

    // Voice 3: Binaural spatial delay & panning state
    float smoothedPanL_ = 0.707f;
    float smoothedPanR_ = 0.707f;
    float smoothedItdSamples_ = 0.0f;
    std::array<float, 128> itdDelayL_{};
    std::array<float, 128> itdDelayR_{};
    std::size_t itdWriteIdx_ = 0;
};

} // namespace epi_sculpt
