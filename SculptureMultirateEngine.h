#pragma once

#include "SculptureProgramSchema.h"
#include <array>
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <cmath>
#include <span>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: VARIABLE-RATE MULTIRATE STAGED EXECUTION
// -----------------------------------------------------------------------------
// The 8K @ 120 FPS blueprint (from docu/project-vision.txt) demands decoupled
// temporal and spatial execution rates across the 6-stage mathematical loop:
//
//   1. High-Frequency Stages (Transform, Orbit Feedback):
//      - Require full spatial resolution (50^3 voxels / 16,384 samples)
//      - Evaluated at 120+ FPS with single-cycle ALU operations.
//
//   2. Low-Frequency Stages (WCA Seed Injection, Volumetric Density, Fog):
//      - Dominated by smooth spatial gradients and diffusion potentials.
//      - Evaluated at Half (25^3) or Quarter (12^3) resolution.
//      - Reconstructed via trilinear hardware-equivalent tensor interpolation:
//          V(x,y,z) = lerp(lerp(lerp(c000, c100, u), lerp(c010, c110, u), v), ...)
// -----------------------------------------------------------------------------

inline constexpr std::size_t kFullGridDim = 50U;     // 125,000 voxels
inline constexpr std::size_t kHalfGridDim = 25U;     // 15,625 voxels
inline constexpr std::size_t kQuarterGridDim = 12U;  // 1,728 voxels

template <typename T, std::size_t Dim>
struct VoxelGrid3D {
    static constexpr std::size_t kDimension = Dim;
    static constexpr std::size_t kTotalVoxels = Dim * Dim * Dim;
    std::array<T, kTotalVoxels> data{};

    [[nodiscard]] constexpr std::size_t index(std::size_t x, std::size_t y, std::size_t z) const noexcept {
        return x + y * Dim + z * Dim * Dim;
    }

    [[nodiscard]] const T& at(std::size_t x, std::size_t y, std::size_t z) const noexcept {
        return data[index(x, y, z)];
    }

    T& at(std::size_t x, std::size_t y, std::size_t z) noexcept {
        return data[index(x, y, z)];
    }

    void fill(const T& val) noexcept {
        data.fill(val);
    }
};

struct MultirateConfig {
    StageResolutionScale seedResolution = StageResolutionScale::Half;
    StageResolutionScale atmosphereResolution = StageResolutionScale::Quarter;
    std::uint32_t seedDecimationCadence = 2;       // Update seed every N frames
    std::uint32_t atmosphereDecimationCadence = 4; // Update fog every M frames
    bool enableTrilinearFiltering = true;
    bool enableTemporalSmoothing = true;
    float temporalBlendFactor = 0.85f;
};

struct MultirateExecutionStats {
    float fullResolutionComputeMs = 0.0f;
    float auxiliaryComputeMs = 0.0f;
    float upsampleInterpolationMs = 0.0f;
    std::size_t totalVoxelsEvaluated = 0;
    std::size_t totalVoxelsSaved = 0;
    float effectiveCompressionRatio = 1.0f;
};

class SculptureMultirateEngine {
public:
    SculptureMultirateEngine();
    explicit SculptureMultirateEngine(const MultirateConfig& config);

    void setConfig(const MultirateConfig& config) noexcept { config_ = config; }
    [[nodiscard]] const MultirateConfig& config() const noexcept { return config_; }
    [[nodiscard]] const MultirateExecutionStats& stats() const noexcept { return stats_; }

    // Evaluates Stage 5 (Seed Injection) at multirate resolution and upsamples into full grid
    void executeMultirateSeedStage(
        const SculptureProgramParams& params,
        float time,
        std::uint64_t frameIndex,
        VoxelGrid3D<float, kFullGridDim>& outFullGrid
    ) noexcept;

    // Evaluates Stage 5 (Seed Injection) at multirate resolution directly into spatial vector buffer
    void executeMultirateSeedToBuffer(
        const SculptureProgramParams& params,
        float time,
        std::uint64_t frameIndex,
        std::vector<float>& outSpatialBuffer
    ) noexcept;

    // Evaluates volumetric atmosphere / optical density at multirate resolution
    void executeMultirateAtmosphereStage(
        const SculptureProgramParams& params,
        float time,
        std::uint64_t frameIndex,
        VoxelGrid3D<float, kFullGridDim>& outFullGrid
    ) noexcept;

    // Fast static trilinear sample probe from arbitrary downsampled grid
    template <std::size_t SrcDim>
    [[nodiscard]] static float sampleTrilinear(
        const VoxelGrid3D<float, SrcDim>& src,
        float u, float v, float w
    ) noexcept {
        const float cu = std::clamp(u, 0.0f, 1.0f) * static_cast<float>(SrcDim - 1);
        const float cv = std::clamp(v, 0.0f, 1.0f) * static_cast<float>(SrcDim - 1);
        const float cw = std::clamp(w, 0.0f, 1.0f) * static_cast<float>(SrcDim - 1);

        const std::size_t x0 = static_cast<std::size_t>(std::floor(cu));
        const std::size_t y0 = static_cast<std::size_t>(std::floor(cv));
        const std::size_t z0 = static_cast<std::size_t>(std::floor(cw));
        const std::size_t x1 = std::min(x0 + 1, SrcDim - 1);
        const std::size_t y1 = std::min(y0 + 1, SrcDim - 1);
        const std::size_t z1 = std::min(z0 + 1, SrcDim - 1);

        const float fx = cu - static_cast<float>(x0);
        const float fy = cv - static_cast<float>(y0);
        const float fz = cw - static_cast<float>(z0);

        const float c000 = src.at(x0, y0, z0);
        const float c100 = src.at(x1, y0, z0);
        const float c010 = src.at(x0, y1, z0);
        const float c110 = src.at(x1, y1, z0);
        const float c001 = src.at(x0, y0, z1);
        const float c101 = src.at(x1, y0, z1);
        const float c011 = src.at(x0, y1, z1);
        const float c111 = src.at(x1, y1, z1);

        const float c00 = c000 + fx * (c100 - c000);
        const float c10 = c010 + fx * (c110 - c010);
        const float c01 = c001 + fx * (c101 - c001);
        const float c11 = c011 + fx * (c111 - c011);

        const float c0 = c00 + fy * (c10 - c00);
        const float c1 = c01 + fy * (c11 - c01);

        return c0 + fz * (c1 - c0);
    }

    // Upsamples SrcDim grid to DstDim grid using hardware-accurate trilinear weighting
    template <std::size_t SrcDim, std::size_t DstDim>
    static void upsampleGrid(
        const VoxelGrid3D<float, SrcDim>& src,
        VoxelGrid3D<float, DstDim>& dst
    ) noexcept {
        constexpr float invDst = 1.0f / static_cast<float>(DstDim - 1);
        for (std::size_t z = 0; z < DstDim; ++z) {
            const float w = static_cast<float>(z) * invDst;
            for (std::size_t y = 0; y < DstDim; ++y) {
                const float v = static_cast<float>(y) * invDst;
                for (std::size_t x = 0; x < DstDim; ++x) {
                    const float u = static_cast<float>(x) * invDst;
                    dst.at(x, y, z) = sampleTrilinear<SrcDim>(src, u, v, w);
                }
            }
        }
    }

private:
    MultirateConfig config_{};
    MultirateExecutionStats stats_{};

    VoxelGrid3D<float, kHalfGridDim> halfSeedBuffer_{};
    VoxelGrid3D<float, kQuarterGridDim> quarterSeedBuffer_{};
    VoxelGrid3D<float, kHalfGridDim> halfAtmosphereBuffer_{};
    VoxelGrid3D<float, kQuarterGridDim> quarterAtmosphereBuffer_{};
    VoxelGrid3D<float, kFullGridDim> previousFullSeed_{};
};

} // namespace epi_sculpt
