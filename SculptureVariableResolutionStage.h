/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - VARIABLE-RESOLUTION MULTI-STAGE UPSCALER
 * ============================================================================
 *
 * Educational Annotation: MULTI-RATE SPATIAL RENDERING & TEXTURE UPSAMPLING
 * ----------------------------------------------------------------------------
 * Governed by docu/project-vision.txt lines 173 & 222-228:
 *  "High-Resolution Performance... Variable-resolution stages... High ROI pattern:
 *   run seed stage at lower resolution (1/2 or 1/4),
 *   keep main feedback at full quality where needed,
 *   upscale/interpolate lower-res auxiliary stages via hardware texture filtering...
 *   8k/120fps target with multi-stage resolution, mesh/grid, and interop."
 *
 * Architectural Mechanics:
 *  1. Frequency Decoupling:
 *     High-frequency spatial features (specular reflections, fractal borders,
 *     thin-film iridescence) require native full viewport resolution.
 *     Low-frequency spatial potentials (smooth volumetric fog, reaction-diffusion,
 *     soft WCA seeds) have massive spatial autocorrelation.
 *
 *  2. Compute Savings:
 *     Evaluating the auxiliary stage at Half resolution (1/2 scale in X and Y)
 *     requires only 25% of the pixel evaluations (4x compute reduction).
 *     Quarter resolution (1/4 scale) requires only 6.25% (16x compute reduction).
 *
 *  3. Hardware-Equivalent Bilinear / Bicubic Upsampling:
 *     Continuous normalized texture coordinate sampling (u, v) in [0, 1]^2
 *     with SIMD-friendly bilinear interpolation reconstructs a smooth,
 *     band-limited full-resolution layer for seamless composite blending.
 * ============================================================================
 */

#pragma once

#include "SculptureProgramSchema.h"
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <string>

namespace epi_sculpt {

enum class UpsampleFilter : std::uint8_t {
    Bilinear = 0,
    SmoothstepBilinear = 1,
    CatmullRomBicubic = 2
};

struct VariableResolutionMetrics {
    std::uint32_t fullWidth = 0;
    std::uint32_t fullHeight = 0;
    std::uint32_t auxWidth = 0;
    std::uint32_t auxHeight = 0;
    StageResolutionScale activeScale = StageResolutionScale::Half;
    float pixelSavingsRatio = 4.0f; // e.g. 4.0x for Half, 16.0x for Quarter
    std::uint64_t totalUpsampledFrames = 0;
};

class SculptureVariableResolutionStage {
public:
    SculptureVariableResolutionStage() noexcept = default;

    SculptureVariableResolutionStage(
        std::uint32_t fullWidth,
        std::uint32_t fullHeight,
        StageResolutionScale scale = StageResolutionScale::Half,
        UpsampleFilter filter = UpsampleFilter::Bilinear
    );

    // Reconfigures full-resolution dimensions and downscale tier
    void configure(
        std::uint32_t fullWidth,
        std::uint32_t fullHeight,
        StageResolutionScale scale = StageResolutionScale::Half
    );

    // Dynamic resolution scaling (triggers internal buffer reallocation if scale changed)
    void setResolutionScale(StageResolutionScale scale);
    [[nodiscard]] StageResolutionScale resolutionScale() const noexcept { return scale_; }

    void setFilter(UpsampleFilter filter) noexcept { filter_ = filter; }
    [[nodiscard]] UpsampleFilter filter() const noexcept { return filter_; }

    // Direct access to downsampled auxiliary stage buffer (RGBA32F, size = auxWidth * auxHeight * 4)
    [[nodiscard]] std::vector<float>& auxiliaryBuffer() noexcept { return auxBuffer_; }
    [[nodiscard]] const std::vector<float>& auxiliaryBuffer() const noexcept { return auxBuffer_; }

    // Direct access to reconstructed full-resolution buffer (RGBA32F, size = fullWidth * fullHeight * 4)
    [[nodiscard]] const std::vector<float>& upsampledBuffer() const noexcept { return upsampledBuffer_; }

    // Reconstructs the full-resolution buffer from the current auxiliary stage buffer
    void executeUpsample();

    // Composites the upsampled auxiliary layer onto an external full-resolution carrier
    // dst = lerp(carrier, upsampled, alpha * opacity) or additive if isAdditive = true
    void compositeOnto(
        const std::vector<float>& carrierFrame,
        std::vector<float>& outDestination,
        float opacity = 1.0f,
        bool isAdditive = false
    ) const;

    // Ingests an incoming slice (either at full or downscaled resolution) and produces
    // an upsampled / standardized full-resolution output buffer ready for spatial injection.
    [[nodiscard]] const std::vector<float>& processAndUpsample(
        const std::vector<float>& inputBuffer,
        std::uint32_t inputWidth,
        std::uint32_t inputHeight,
        std::uint32_t targetFullWidth,
        std::uint32_t targetFullHeight,
        StageResolutionScale scale
    );

    // Continuous normalized sampling (u, v in [0, 1]) from the auxiliary buffer
    [[nodiscard]] std::array<float, 4> sampleNormalized(float u, float v) const noexcept;

    // Dimension queries
    [[nodiscard]] std::uint32_t fullWidth() const noexcept { return fullWidth_; }
    [[nodiscard]] std::uint32_t fullHeight() const noexcept { return fullHeight_; }
    [[nodiscard]] std::uint32_t auxiliaryWidth() const noexcept { return auxWidth_; }
    [[nodiscard]] std::uint32_t auxiliaryHeight() const noexcept { return auxHeight_; }
    [[nodiscard]] const VariableResolutionMetrics& metrics() const noexcept { return metrics_; }

    // Human-readable diagnostic HUD string
    [[nodiscard]] std::string formatStatus() const;

private:
    void recomputeDimensions();

    std::uint32_t fullWidth_ = 0;
    std::uint32_t fullHeight_ = 0;
    std::uint32_t auxWidth_ = 0;
    std::uint32_t auxHeight_ = 0;
    StageResolutionScale scale_ = StageResolutionScale::Half;
    UpsampleFilter filter_ = UpsampleFilter::Bilinear;

    std::vector<float> auxBuffer_{};        // Downsampled auxiliary stage buffer (RGBA32F)
    std::vector<float> upsampledBuffer_{};  // Reconstructed full-resolution buffer (RGBA32F)
    VariableResolutionMetrics metrics_{};
};

} // namespace epi_sculpt
