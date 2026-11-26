#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <vector>

namespace epi_sculpt {

enum class SculptureDiagnosticView : std::uint8_t {
    Depth, SurfaceNormal, TraceEffort, HitMask, Transmittance, AmbientOcclusion,
    DepthContours
};

struct SculptureDiagnosticDescriptor {
    SculptureDiagnosticView view;
    std::string_view id;
    std::string_view label;
    std::string_view explanation;
};

// A derived-observation catalog, not another mathematical mode registry.
inline constexpr std::array kSculptureDiagnosticViews{
    SculptureDiagnosticDescriptor{SculptureDiagnosticView::Depth, "depth", "Surface depth",
        "Hit depth in camera-space world units; near is white, far is black, misses are black."},
    SculptureDiagnosticDescriptor{SculptureDiagnosticView::SurfaceNormal, "normal", "Surface normals",
        "Hit normal direction mapped from [-1,1] to RGB [0,1]; misses are black."},
    SculptureDiagnosticDescriptor{SculptureDiagnosticView::TraceEffort, "effort", "Tracing effort",
        "Primary distance evaluations / step budget: blue, cyan, yellow, red. Includes misses; excludes shadow/fog work."},
    SculptureDiagnosticDescriptor{SculptureDiagnosticView::HitMask, "hits", "Surface hit mask",
        "Stored hit flag: white for hits, black for misses."},
    SculptureDiagnosticDescriptor{SculptureDiagnosticView::Transmittance, "transmittance", "Volume transmittance",
        "Stored optical transmittance: white is transparent, black is opaque. Includes misses."},
    SculptureDiagnosticDescriptor{SculptureDiagnosticView::AmbientOcclusion, "ao", "Surface ambient occlusion",
        "Stored hit ambient-light visibility: white is unoccluded; misses are black."},
    SculptureDiagnosticDescriptor{SculptureDiagnosticView::DepthContours, "depth-contours", "Camera-depth contours",
        "Cyan constant-depth bands over grayscale depth. Spacing is in camera-space world units, anchored at zero; misses are black."},
};

[[nodiscard]] constexpr const SculptureDiagnosticDescriptor* sculptureDiagnosticDescriptor(
    SculptureDiagnosticView view) noexcept {
    for (const auto& descriptor : kSculptureDiagnosticViews) {
        if (descriptor.view == view) return &descriptor;
    }
    return nullptr;
}

struct SculptureDiagnosticSettings {
    SculptureDiagnosticView view = SculptureDiagnosticView::TraceEffort;
    // Explicit stable bounds: camera motion/other frames cannot renormalize depth.
    float depthNear = 0.0f;
    float depthFar = 20.0f;
    // Contour-only controls. Half-width is a fraction of one spacing interval;
    // coverage fades linearly from the cyan center to the grayscale background.
    float depthContourSpacing = 1.0f;
    float depthContourHalfWidth = 0.04f;

    [[nodiscard]] bool valid() const noexcept {
        return sculptureDiagnosticDescriptor(view) != nullptr
            && std::isfinite(depthNear) && std::isfinite(depthFar)
            && depthNear >= 0.0f && depthFar > depthNear
            && (view != SculptureDiagnosticView::DepthContours
                || (std::isfinite(depthContourSpacing) && depthContourSpacing > 0.0f
                    && std::isfinite(depthContourHalfWidth) && depthContourHalfWidth > 0.0f
                    && depthContourHalfWidth <= 0.5f));
    }
};

struct SculptureDiagnosticImage {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    // Display RGB in [0,1], not scene-linear radiance or recursive field state.
    // Magenta marks invalid measurements; background rules are view-specific.
    std::vector<float> rgb;

    [[nodiscard]] bool empty() const noexcept { return rgb.empty(); }
};

} // namespace epi_sculpt
