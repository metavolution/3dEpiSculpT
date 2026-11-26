#pragma once

#include "SculptureDiagnosticExport.h"
#include "SculptureDiagnosticLayout.h"

#include <optional>

namespace epi_sculpt {

// All tiles observe the same completed source; order follows the public catalog.
inline constexpr auto kSculptureDiagnosticAtlasViews = [] {
    std::array<SculptureDiagnosticSettings, kSculptureDiagnosticViews.size()> settings{};
    for (std::size_t i=0; i<settings.size(); ++i) settings[i].view=kSculptureDiagnosticViews[i].view;
    return settings;
}();

namespace detail {
[[nodiscard]] inline bool publishDiagnosticAtlas(
    const std::string& filepath, const SculptureGBuffer& source, const DiagnosticAtlasLayout& layout) {
    if (source.width!=layout.sourceWidth || source.height!=layout.sourceHeight)
        throw std::invalid_argument("diagnostic atlas source does not match admitted layout");
    if (layout.empty()) return false;
    std::array<std::optional<CheckedDiagnosticObservation>,kSculptureDiagnosticViews.size()> tiles;
    const auto count=layout.count, columns=layout.columns, rows=layout.rows;
    const auto tileWidth=layout.tileWidth, tileHeight=layout.tileHeight;
    const auto atlasWidth=layout.width, atlasHeight=layout.height, gutter=layout.gutter;
    constexpr auto captionHeight=DiagnosticAtlasLayout::captionHeight;
    for (std::size_t i=0; i<count; ++i) tiles[i].emplace(source,layout.settings[i]);

    return detail::publishDiagnosticPpm(filepath,atlasWidth,atlasHeight,[&](std::ostream& stream) {
        stream << "# epic-diagnostic-atlas=1\n# source=completed-gbuffer\n"
               << "# native-presentation=unconfirmed\n# encoding=display-rgb8\n"
               << "# columns=" << columns << "\n# rows=" << rows
               << "\n# tile-width=" << tileWidth << "\n# tile-height=" << tileHeight
               << "\n# gutter=" << gutter << "\n# caption-height=" << captionHeight << '\n';
        for (std::size_t i=0; i<count; ++i) {
            stream << "# tile=" << i << " view=" << layout.labels[i]
                   << " x=" << gutter+(i%columns)*(tileWidth+gutter)
                   << " y=" << gutter+(i/columns)*(tileHeight+gutter)+captionHeight
                   << " width=" << source.width << " height=" << source.height << '\n';
            detail::writeDiagnosticSettingsMetadata(stream,tiles[i]->settings());
        }
    },[&](std::uint32_t y) {
        return prepareDiagnosticAtlasRow(layout,y,[&](std::size_t index,std::size_t p) {
            return tiles[index]->pixel(p);
        });
    });
}
} // namespace detail

// Labeled, row-major contact sheet of real completed channels. No rendering,
// readback, source mutation or presentation confirmation is performed here.
// Select 1..catalog-size tiles; repeats allow different bounds for the same view.
// Settings freeze before publication; callers keep the source stable until return.
// Empty source/path refusal returns false. Invalid views/layout/channels throw
// before filesystem work. Failed publication preserves the previous destination.
[[nodiscard]] inline bool exportSculptureDiagnosticAtlasPpm(
    const std::string& filepath, const SculptureGBuffer& source,
    std::span<const SculptureDiagnosticSettings> views = kSculptureDiagnosticAtlasViews,
    std::uint32_t requestedColumns = 3, std::uint32_t gutter = 2) {
    if (!file_publication_detail::admittedFilePath(filepath)) return false;
    const auto layout=detail::prepareDiagnosticAtlasLayout(
        source.width,source.height,views,requestedColumns,gutter);
    return detail::publishDiagnosticAtlas(filepath,source,layout);
}

} // namespace epi_sculpt
