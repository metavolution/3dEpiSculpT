#pragma once

// Shared comparison values and pure completed-observation analysis. File IO,
// layout and rendering remain in the export/capture consumers.
#include "SculptureGBufferDiagnostics.h"

#include <cstddef>
#include <cstdint>

namespace epi_sculpt {

enum class SculptureDiagnosticDifferenceMode : std::uint8_t {
    AbsoluteRgb,
    ChangeMask
};

struct SculptureDiagnosticComparisonSettings {
    SculptureDiagnosticSettings diagnostic{};
    float differenceGain=1.0f;
    SculptureDiagnosticDifferenceMode differenceMode=SculptureDiagnosticDifferenceMode::AbsoluteRgb;
    // Strict threshold on max(abs(after-before)) in unquantized display RGB.
    // Gain applies only to AbsoluteRgb; it never changes mask membership.
    float differenceThreshold=0.0f;
    // Optional CPU-only summary pass over completed observations before export.
    bool includeChangeSummary=false;

    [[nodiscard]] bool valid() const noexcept {
        return diagnostic.valid() && std::isfinite(differenceGain) && differenceGain>0.0f
            && (differenceMode==SculptureDiagnosticDifferenceMode::AbsoluteRgb
                || differenceMode==SculptureDiagnosticDifferenceMode::ChangeMask)
            && std::isfinite(differenceThreshold) && differenceThreshold>=0.0f && differenceThreshold<=1.0f;
    }
};

struct SculptureDiagnosticChangeBounds {
    std::uint32_t x=0, y=0, width=0, height=0;
    [[nodiscard]] bool empty() const noexcept { return width==0 || height==0; }
};

struct SculptureDiagnosticChangeSummary {
    std::uint32_t width=0, height=0;
    std::size_t comparedPixels=0, changedPixels=0;
    float maximumDifference=0.0f;
    SculptureDiagnosticChangeBounds changedBounds{};
    [[nodiscard]] double changedFraction() const noexcept {
        return comparedPixels==0 ? 0.0 : static_cast<double>(changedPixels)/comparedPixels;
    }
};

namespace detail {
[[nodiscard]] inline float diagnosticComparisonMaximumDifference(
    const std::array<float,3>& a, const std::array<float,3>& b) noexcept {
    return std::max({std::abs(b[0]-a[0]),std::abs(b[1]-a[1]),std::abs(b[2]-a[2])});
}

// Same membership rule as the mask; source-pixel bounds have half-open upper
// edges [x,x+width), [y,y+height). Row traversal keeps coordinates available
// without division/modulo for every changed pixel.
[[nodiscard]] inline SculptureDiagnosticChangeSummary summarizeDiagnosticChanges(
    const CheckedDiagnosticObservation& first, const CheckedDiagnosticObservation& second,
    std::uint32_t width, std::uint32_t height, float threshold) noexcept {
    SculptureDiagnosticChangeSummary result{width,height,first.pixelCount()};
    // A declared empty width may accompany a huge height. Do no row work then.
    if (result.comparedPixels==0) return result;
    std::uint32_t minX=width, minY=height, maxX=0, maxY=0;
    std::size_t p=0;
    for (std::uint32_t y=0; y<height; ++y) {
        for (std::uint32_t x=0; x<width; ++x, ++p) {
            const float difference=diagnosticComparisonMaximumDifference(first.pixel(p),second.pixel(p));
            result.maximumDifference=std::max(result.maximumDifference,difference);
            if (difference<=threshold) continue;
            ++result.changedPixels;
            minX=std::min(minX,x); minY=std::min(minY,y);
            maxX=std::max(maxX,x); maxY=std::max(maxY,y);
        }
    }
    if (result.changedPixels!=0) result.changedBounds={minX,minY,maxX-minX+1,maxY-minY+1};
    return result;
}

} // namespace detail

// Allocation-free, synchronous analysis of stable completed observations. The
// threshold applies before gain in either difference mode. All selected pixels,
// including invalid-marker colors, participate; this is not raw-field error.
// Empty matching extents return a zero summary retaining the declared extent.
// Invalid settings, mismatched extents or malformed selected channels throw.
[[nodiscard]] inline SculptureDiagnosticChangeSummary analyzeSculptureDiagnosticChanges(
    const SculptureGBuffer& before, const SculptureGBuffer& after,
    const SculptureDiagnosticComparisonSettings& settings = {}) {
    const auto frozen=settings;
    if (!frozen.valid()) throw std::invalid_argument("invalid diagnostic comparison settings");
    if (before.width!=after.width || before.height!=after.height)
        throw std::invalid_argument("diagnostic comparison sources do not match extent");
    const detail::CheckedDiagnosticObservation first(before,frozen.diagnostic), second(after,frozen.diagnostic);
    return detail::summarizeDiagnosticChanges(first,second,before.width,before.height,frozen.differenceThreshold);
}

} // namespace epi_sculpt
