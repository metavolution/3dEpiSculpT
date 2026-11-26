#pragma once

#include "SculptureGBufferDiagnostics.h"
#include "SculptureFilePublication.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <limits>
#include <locale>
#include <ostream>
#include <string>

namespace epi_sculpt {
namespace detail {

inline void writeDiagnosticSettingsMetadata(
    std::ostream& stream, const SculptureDiagnosticSettings& settings) {
    if (settings.view == SculptureDiagnosticView::Depth
        || settings.view == SculptureDiagnosticView::DepthContours) {
        stream << "# depth-near=" << settings.depthNear << "\n# depth-far=" << settings.depthFar << '\n';
    }
    if (settings.view == SculptureDiagnosticView::DepthContours) {
        stream << "# depth-contour-spacing=" << settings.depthContourSpacing
               << "\n# depth-contour-half-width=" << settings.depthContourHalfWidth << '\n';
    }
}

// One bounded encoder/publication transaction for individual and tiled views.
// Metadata and rows consume admitted, frozen values. Preparing once per row lets
// tiled consumers resolve vertical geometry outside the per-pixel hot loop.
template<class Metadata, class PrepareRow>
[[nodiscard]] inline bool publishDiagnosticPpm(
    const std::string& filepath, std::uint32_t width, std::uint32_t height,
    const Metadata& metadata, const PrepareRow& prepareRow) {
    (void)checkedRaymarchElementCount<3>(width, height);
    return file_publication_detail::publishStreamFile(filepath, [&](std::ostream& stream) {
        stream.imbue(std::locale::classic());
        stream << std::setprecision(std::numeric_limits<float>::max_digits10) << "P6\n";
        metadata(stream);
        stream << width << ' ' << height << "\n255\n";
        std::array<unsigned char, 4096> block{};
        std::size_t used = 0;
        for (std::uint32_t y = 0; y < height; ++y) {
            const auto pixel = prepareRow(y);
            for (std::uint32_t x = 0; x < width; ++x) {
                for (float value : pixel(x)) {
                    block[used++] = static_cast<unsigned char>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
                    if (used == block.size()) {
                        stream.write(reinterpret_cast<const char*>(block.data()), used);
                        if (!stream) return false;
                        used = 0;
                    }
                }
            }
        }
        if (used != 0) stream.write(reinterpret_cast<const char*>(block.data()), used);
        return stream.good();
    });
}
} // namespace detail

// Explicit cold export of a completed observation. The converter and header use
// one frozen settings value; callers keep the source stable for this synchronous
// operation. This does not produce mathematics or confirm native presentation.
// PPM keeps pixels and explanatory metadata in one atomic file publication.
// Invalid settings/channels throw before filesystem work; empty input or IO/path
// refusal returns false. Existing destinations survive failed publication.
[[nodiscard]] inline bool exportSculptureDiagnosticPpm(
    const std::string& filepath, const SculptureGBuffer& source,
    const SculptureDiagnosticSettings& settings = {}) {
    if (!file_publication_detail::admittedFilePath(filepath)) return false;
    const detail::CheckedDiagnosticObservation observation(source, settings);
    if (observation.empty()) return false;
    const auto& frozen = observation.settings();
    const auto* descriptor = sculptureDiagnosticDescriptor(frozen.view);

    return detail::publishDiagnosticPpm(filepath, observation.width(), observation.height(),
        [&](std::ostream& stream) {
            stream << "# epic-diagnostic=1\n# view=" << descriptor->id
                   << "\n# source=completed-gbuffer\n# native-presentation=unconfirmed"
                   << "\n# encoding=display-rgb8\n";
            detail::writeDiagnosticSettingsMetadata(stream, frozen);
        }, [&](std::uint32_t y) {
            const auto offset=static_cast<std::size_t>(y)*observation.width();
            return [&,offset](std::uint32_t x) { return observation.pixel(offset+x); };
        });
}

} // namespace epi_sculpt
