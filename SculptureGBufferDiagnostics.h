#pragma once

#include "SculptureDiagnosticView.h"
#include "SculptureGBuffer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace epi_sculpt {
namespace detail {
inline bool diagnosticUnitValue(float value) noexcept {
    return std::isfinite(value) && value >= 0.0f && value <= 1.0f;
}

inline std::array<float, 3> diagnosticEffortColor(float value) noexcept {
    const float position = value * 3.0f;
    if (position < 1.0f) return {0.0f, position, 1.0f};
    if (position < 2.0f) return {position - 1.0f, 1.0f, 2.0f - position};
    return {1.0f, 3.0f - position, 0.0f};
}
} // namespace detail

namespace detail {

// Shared by owned RGB images and streaming exports. Admission and frozen settings
// belong here; consumers choose storage without duplicating pixel interpretation.
// Borrow the completed source unchanged for the reader's synchronous lifetime.
class CheckedDiagnosticObservation final {
public:
    CheckedDiagnosticObservation(const SculptureGBuffer& source,
                                 const SculptureDiagnosticSettings& settings)
        : source_(source), settings_(settings) {
        if (!settings_.valid()) throw std::invalid_argument("invalid raymarch diagnostic settings");
        if (source.empty()) return;
        const auto rgbaCount = checkedRaymarchElementCount<4>(source.width, source.height);
        rgbCount_ = checkedRaymarchElementCount<3>(source.width, source.height);
        pixels_ = rgbaCount / 4U;
        const auto requireSize = [](std::size_t actual, std::size_t expected) {
            if (actual != expected) throw std::invalid_argument("diagnostic channel does not match G-buffer extent");
        };
        const auto view = settings_.view;
        if (view == SculptureDiagnosticView::Transmittance) {
            requireSize(source.radiance.size(), rgbaCount);
        } else {
            requireSize(source.attributes.size(), rgbaCount);
            if (view == SculptureDiagnosticView::Depth || view == SculptureDiagnosticView::DepthContours)
                requireSize(source.depth.size(), pixels_);
            if (view == SculptureDiagnosticView::SurfaceNormal) requireSize(source.normal.size(), rgbaCount);
        }
        width_ = source.width;
        height_ = source.height;
    }

    [[nodiscard]] bool empty() const noexcept { return pixels_ == 0; }
    [[nodiscard]] std::uint32_t width() const noexcept { return width_; }
    [[nodiscard]] std::uint32_t height() const noexcept { return height_; }
    [[nodiscard]] std::size_t pixelCount() const noexcept { return pixels_; }
    [[nodiscard]] std::size_t rgbCount() const noexcept { return rgbCount_; }
    [[nodiscard]] const SculptureDiagnosticSettings& settings() const noexcept { return settings_; }

    // Precondition: p < pixelCount(); both consumers use the admitted bound.
    [[nodiscard]] std::array<float, 3> pixel(std::size_t p) const noexcept {
        const auto& source = source_;
        const auto& settings = settings_;
        const auto view = settings.view;
        constexpr std::array<float, 3> invalid{1.0f, 0.0f, 1.0f};
        std::array<float, 3> color{};
        const auto gray = [&color](float value) { color = {value, value, value}; };
        if (view == SculptureDiagnosticView::TraceEffort) {
            const float effort = source.attributes[p * 4U + 2U];
            color = detail::diagnosticUnitValue(effort) ? detail::diagnosticEffortColor(effort) : invalid;
        } else if (view == SculptureDiagnosticView::Transmittance) {
            const float transmission = source.radiance[p * 4U + 3U];
            if (detail::diagnosticUnitValue(transmission)) gray(transmission); else color = invalid;
        } else {
            const float hit = source.attributes[p * 4U + 3U];
            if (!detail::diagnosticUnitValue(hit)) {
                color = invalid;
            } else if (hit >= 0.5f) {
                switch (view) {
                case SculptureDiagnosticView::Depth:
                case SculptureDiagnosticView::DepthContours: {
                    const float depth = source.depth[p];
                    if (!std::isfinite(depth) || depth < 0.0f) { color = invalid; break; }
                    const double normalized = (static_cast<double>(depth) - settings.depthNear)
                        / (static_cast<double>(settings.depthFar) - settings.depthNear);
                    gray(static_cast<float>(1.0 - std::clamp(normalized, 0.0, 1.0)));
                    if (view == SculptureDiagnosticView::DepthContours) {
                        // Reduce before dividing: depth/spacing can be enormous.
                        // The zero-anchored phase never follows frame extrema or
                        // the near/far display bounds, so observations stay comparable.
                        const double spacing = settings.depthContourSpacing;
                        const double distance = std::abs(std::remainder(static_cast<double>(depth), spacing)) / spacing;
                        const float coverage = static_cast<float>(std::clamp(
                            1.0 - distance / settings.depthContourHalfWidth, 0.0, 1.0));
                        const float background = color[0] * (1.0f - coverage);
                        color = {background, background + coverage, background + coverage};
                    }
                    break;
                }
                case SculptureDiagnosticView::SurfaceNormal: {
                    const float x = source.normal[p * 4U], y = source.normal[p * 4U + 1U],
                                z = source.normal[p * 4U + 2U];
                    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
                        color = invalid; break;
                    }
                    const double length = std::hypot(static_cast<double>(x), y, z);
                    if (length == 0.0) { color = invalid; break; }
                    color = {static_cast<float>(0.5 + 0.5 * x / length),
                             static_cast<float>(0.5 + 0.5 * y / length),
                             static_cast<float>(0.5 + 0.5 * z / length)};
                    break;
                }
                case SculptureDiagnosticView::HitMask: gray(1.0f); break;
                case SculptureDiagnosticView::AmbientOcclusion: {
                    const float ao = source.attributes[p * 4U];
                    if (detail::diagnosticUnitValue(ao)) gray(ao); else color = invalid;
                    break;
                }
                default: break; // Effort/transmission handled independently of hits.
                }
            }
        }
        return color;
    }

private:
    const SculptureGBuffer& source_;
    const SculptureDiagnosticSettings settings_;
    std::uint32_t width_ = 0, height_ = 0;
    std::size_t pixels_ = 0, rgbCount_ = 0;
};
} // namespace detail

// Observe completed channels only. No mathematical work or source mutation.
[[nodiscard]] inline SculptureDiagnosticImage makeSculptureDiagnosticImage(
    const SculptureGBuffer& source, const SculptureDiagnosticSettings& settings = {}) {
    const detail::CheckedDiagnosticObservation observation(source, settings);
    if (observation.empty()) return {};
    SculptureDiagnosticImage image{observation.width(), observation.height(),
                                  std::vector<float>(observation.rgbCount())};
    for (std::size_t p = 0; p < observation.pixelCount(); ++p) {
        const auto color = observation.pixel(p);
        std::copy(color.begin(), color.end(), image.rgb.begin() + p * 3U);
    }
    return image;
}

} // namespace epi_sculpt
