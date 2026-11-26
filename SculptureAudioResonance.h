#pragma once

#include "SculptureProgramSchema.h"
#include "AudioResonanceMetrics.h"
#include "SculptureAudioSignals.h"
#include "SculptureSpatialSonifier.h"

#include <array>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <string_view>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: UNIFIED 3D ACOUSTIC RESONANCE & HARMONIC TENSOR
// -----------------------------------------------------------------------------
// Models the closed-loop physical coupling between 3D epimorphic sculpture
// geometry and acoustic wave mechanics:
//
// 1. 3D Helmholtz Cavity Resonance Modes:
//    The sculpture's dynamic bounding box (Lx, Ly, Lz) forms a resonant acoustic
//    cavity with eigenmodes:
//        f_{n,m,l} = (c_s / 2) * sqrt( (n/Lx)^2 + (m/Ly)^2 + (l/Lz)^2 )
//    where c_s is the medium sound velocity (343.0 m/s default).
//
// 2. Standing Wave Acoustic Pressure Field:
//    Calculates the internal spatial pressure wave:
//        p(x,y,z,t) = p0 * cos(n pi x / Lx) * cos(m pi y / Ly) * cos(l pi z / Lz) * cos(omega t)
//
// 3. Bio-Adaptive Closed Loop Coupling:
//    - Audio spectral flux excites modal standing waves.
//    - Standing wave pressure sculpts visual seed pulses and thin-film thickness.
//    - Geometric Lyapunov divergence and surface curvature modulate acoustic Q-factors.
// -----------------------------------------------------------------------------

struct AudioResonanceConfig {
    float speedOfSound = 343.0f;              // Sound velocity c_s in m/s
    float baseResonanceGain = 0.85f;          // Global feedback gain
    float minQFactor = 4.0f;                  // Minimum resonance sharpness
    float maxQFactor = 32.0f;                 // Maximum resonance sharpness
    float couplingStrength = 0.45f;           // Sound-to-geometry modulation depth
    float kuramotoCouplingK = 2.5f;           // Non-linear Kuramoto synchronization strength
    bool enableHarmonicFeedback = true;       // Closed-loop bio-adaptive feedback
};

class SculptureAudioResonance {
public:
    explicit SculptureAudioResonance(const AudioResonanceConfig& config = {});
    ~SculptureAudioResonance() = default;

    // Evaluates acoustic modal frequencies from dynamic bounding volume
    [[nodiscard]] std::array<float, 6> computeModalFrequencies(
        const SculptureBoundingVolume& bounds
    ) const noexcept;

    // Evaluates 3D standing wave pressure at spatial coordinate (p)
    [[nodiscard]] float evaluatePressureAt(
        const SculptureVec3& pos,
        const SculptureBoundingVolume& bounds,
        float time
    ) const noexcept;

    // Rasterizes 3D standing wave acoustic pressure field into a discrete cubic voxel grid
    void rasterizePressureField3D(
        std::vector<float>& outVoxelGrid,
        std::uint32_t voxelDim,
        const SculptureBoundingVolume& bounds,
        float time
    ) const noexcept;

    // Steps resonance dynamics, updating metrics from audio context and geometry features
    void step(
        const AudioSignalContext& audio,
        const GeometricAudioFeatures& geometry,
        const SculptureBoundingVolume& bounds,
        float time,
        float deltaSeconds
    ) noexcept;

    // Applies resonant acoustic modulation directly to visual parameters
    void applyResonanceModulation(
        SculptureProgramParams& params,
        float time,
        float deltaSeconds
    ) const noexcept;

    // Configuration & State Access
    void setConfig(const AudioResonanceConfig& config) noexcept { config_ = config; }
    [[nodiscard]] const AudioResonanceConfig& config() const noexcept { return config_; }
    [[nodiscard]] const AudioResonanceMetrics& metrics() const noexcept { return metrics_; }

    void reset() noexcept;

private:
    AudioResonanceConfig config_{};
    AudioResonanceMetrics metrics_{};

    // Internal modal oscillator phase states
    std::array<float, 6> modalPhases_{};
    std::array<float, 6> modalAmplitudes_{};
    float smoothedCentroid_ = 440.0f;
    float accumulatedEnergy_ = 0.0f;

    // 8-node Kuramoto Coupled Oscillator Lattice
    static constexpr std::size_t kKuramotoNodes = 8U;
    std::array<float, kKuramotoNodes> kuramotoPhases_{};
    std::array<float, kKuramotoNodes> kuramotoNaturalFreqs_{};
};

} // namespace epi_sculpt
