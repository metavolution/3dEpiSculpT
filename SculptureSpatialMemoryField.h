#pragma once

#include "SculptureProgramSchema.h"
#include "Sculpture3DProceduralWorld.h"

#include <vector>
#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <string_view>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: RECURSIVE 3D SPATIAL MEMORY & FLUID ADVECTION FIELD
// -----------------------------------------------------------------------------
// In 2D epimorphic feedback, past state is stored in an image2d_t texture buffer.
// In true 3D procedural worlds, memory is stored across a camera-independent
// double-buffered 3D volumetric field.
//
// Each voxel evaluates:
// 1. Semi-Lagrangian 3D advection: backtracing along kinetic velocity vectors.
// 2. Continuous 3D Trilinear sampling of density, velocity, and spectral hue.
// 3. Divergence-free 3D curl turbulence and kinetic viscosity dissipation.
// 4. Higher-dimensional 4D seed injection (HyperSphere4D / Quaternionic pulse).
// 5. Exponential temporal blend and spatial persistence.
// -----------------------------------------------------------------------------

struct SpatialVoxel {
    float density = 0.0f;       // Optical scalar density / matter presence [0, 1]
    float vx = 0.0f;            // 3D kinetic velocity vector X
    float vy = 0.0f;            // 3D kinetic velocity vector Y
    float vz = 0.0f;            // 3D kinetic velocity vector Z
    float hue = 0.0f;           // OKLCH / Spectral wavelength [0, 1]
    float energy = 0.0f;        // Thermal / kinetic excitation
    float chemicalU = 1.0f;     // Gray-Scott Reaction-Diffusion U
    float chemicalV = 0.0f;     // Gray-Scott Reaction-Diffusion V
};

struct SpatialFieldSample {
    float density = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float vz = 0.0f;
    float hue = 0.0f;
    float energy = 0.0f;
    float chemicalU = 1.0f;
    float chemicalV = 0.0f;
};

class SculptureSpatialMemoryField {
public:
    static constexpr std::size_t kDefaultResolution = 40U; // 40^3 = 64,000 voxels (cache-friendly & realtime)

    explicit SculptureSpatialMemoryField(std::size_t resolution = kDefaultResolution,
                                         float worldExtent = 2.0f);
    ~SculptureSpatialMemoryField();
    SculptureSpatialMemoryField(SculptureSpatialMemoryField&&) noexcept;
    SculptureSpatialMemoryField& operator=(SculptureSpatialMemoryField&&) noexcept;

    // Reconfigures dimensions and resets both histories after complete preparation.
    // Shape/geometry refusal or allocation failure preserves the current field.
    // Construction uses the same admission; finite minima remain 8 voxels/axis
    // and 0.1 world extent. Replacement temporarily retains old and new buffers.
    void resize(std::size_t resolution, float worldExtent = 2.0f);

    // Resets both history and active fields to zero
    void reset() noexcept;

    // Advances the recursive field with a positive finite delta and finite time.
    // Other temporal inputs are no-ops: voxels, summaries and buffer selection
    // remain unchanged. Admitted deltas retain the [0.0001, 0.1]-second clamp.
    void evolve(float deltaSeconds, const SculptureProgramParams& params, float simTime = 0.0f);

    // Continuous Trilinear Sample of the 3D memory field in normalized world coordinates [-extent, +extent]
    [[nodiscard]] SpatialFieldSample sampleTrilinear(float x, float y, float z) const noexcept;

    // Cross-Domain Holographic Slice Ingestion & Extraction (2D <-> 3D Coupling)
    void injectTextureSlice(
        const std::vector<float>& inTextureRgba,
        std::uint32_t width,
        std::uint32_t height,
        float zPlaneOffset = 0.0f,
        float weight = 0.5f
    );

    void extractTextureSlice(
        std::vector<float>& outTextureRgba,
        std::uint32_t width,
        std::uint32_t height,
        float zPlaneOffset = 0.0f
    ) const;

    void modulatePotentialField(
        const std::vector<float>& scalarGrid,
        float strength = 1.0f
    );

    // Grid properties
    [[nodiscard]] std::size_t resolution() const noexcept { return resolution_; }
    [[nodiscard]] std::size_t totalVoxels() const noexcept { return totalVoxels_; }
    [[nodiscard]] float worldExtent() const noexcept { return worldExtent_; }
    [[nodiscard]] float voxelSize() const noexcept { return voxelSize_; }
    // Borrow the same current field that sampling reads. Reacquire after mutation;
    // resize invalidates element references, and evolution changes the selected vector.
    [[nodiscard]] const std::vector<SpatialVoxel>& activeField() const noexcept {
        return readBuffer_ == fieldA_.data() ? fieldA_ : fieldB_;
    }

    // Summary statistics for sonification & telemetry
    [[nodiscard]] float totalEnergy() const noexcept { return totalEnergy_; }
    [[nodiscard]] float meanDensity() const noexcept { return meanDensity_; }

private:
    [[nodiscard]] std::size_t index(std::size_t x, std::size_t y, std::size_t z) const noexcept {
        return (z * resolution_ + y) * resolution_ + x;
    }

    [[nodiscard]] const SpatialVoxel& readVoxel(std::size_t x, std::size_t y, std::size_t z) const noexcept {
        return readBuffer_[index(x, y, z)];
    }

    // 3D Divergence-Free Curl Noise Generator
    [[nodiscard]] static std::array<float, 3> evaluateCurlNoise(float x, float y, float z, float freq, float strength) noexcept;

    std::size_t resolution_ = kDefaultResolution;
    std::size_t totalVoxels_ = kDefaultResolution * kDefaultResolution * kDefaultResolution;
    float worldExtent_ = 2.0f;
    float voxelSize_ = 0.1f;

    std::vector<SpatialVoxel> fieldA_;
    std::vector<SpatialVoxel> fieldB_;
    SpatialVoxel* readBuffer_ = nullptr;
    SpatialVoxel* writeBuffer_ = nullptr;

    float totalEnergy_ = 0.0f;
    float meanDensity_ = 0.0f;
    Sculpture3DProceduralWorld proceduralWorld_{};
};

} // namespace epi_sculpt
