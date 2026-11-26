#pragma once

#include "SculptureDiagnosticComparisonAnalysis.h"
#include "SculptureDiagnosticExport.h"
#include "SculptureDiagnosticLayout.h"

#include <cstdint>
#include <optional>

namespace epi_sculpt {

namespace detail {
inline constexpr std::array<std::string_view,3> kDiagnosticComparisonLabels{"before","after","difference"};

// Value-only admission shared by completed exports and explicit render requests.
[[nodiscard]] inline DiagnosticAtlasLayout prepareDiagnosticComparisonLayout(
    std::uint32_t width, std::uint32_t height,
    const SculptureDiagnosticComparisonSettings& settings, std::uint32_t gutter) {
    if (!settings.valid()) throw std::invalid_argument("invalid diagnostic comparison settings");
    const std::array views{settings.diagnostic,settings.diagnostic,settings.diagnostic};
    auto labels=kDiagnosticComparisonLabels;
    if (settings.differenceMode==SculptureDiagnosticDifferenceMode::ChangeMask) labels[2]="change-mask";
    return prepareDiagnosticAtlasLayout(width,height,views,3,gutter,labels);
}

[[nodiscard]] inline bool publishDiagnosticComparison(
    const std::string& filepath, const SculptureGBuffer& before, const SculptureGBuffer& after,
    const SculptureDiagnosticComparisonSettings& frozen, const DiagnosticAtlasLayout& layout) {
    if (before.width!=layout.sourceWidth || before.height!=layout.sourceHeight
        || after.width!=layout.sourceWidth || after.height!=layout.sourceHeight)
        throw std::invalid_argument("diagnostic comparison sources do not match admitted extent");
    if (layout.empty()) return false;
    const auto& labels=layout.labels;
    const detail::CheckedDiagnosticObservation first(before,frozen.diagnostic);
    const detail::CheckedDiagnosticObservation second(after,frozen.diagnostic);
    const auto summary=frozen.includeChangeSummary
        ? std::optional{summarizeDiagnosticChanges(first,second,before.width,before.height,frozen.differenceThreshold)}
        : std::nullopt;
    constexpr auto captionHeight=detail::DiagnosticAtlasLayout::captionHeight;
    const auto* descriptor=sculptureDiagnosticDescriptor(frozen.diagnostic.view);

    return detail::publishDiagnosticPpm(filepath,layout.width,layout.height,[&](std::ostream& stream) {
        stream << "# epic-diagnostic-comparison=1\n# source=completed-gbuffer-pair\n"
               << "# view=" << descriptor->id
               << "\n# correspondence=same-pixel-index\n# difference="
               << (frozen.differenceMode==SculptureDiagnosticDifferenceMode::ChangeMask
                   ? "thresholded-display-rgb-mask" : "absolute-display-rgb")
               << "\n# difference-gain=" << frozen.differenceGain;
        if (frozen.differenceMode==SculptureDiagnosticDifferenceMode::ChangeMask)
            stream << "\n# difference-threshold=" << frozen.differenceThreshold
                   << "\n# threshold-domain=unquantized-display-rgb-max-channel"
                   << "\n# threshold-rule=strictly-greater\n# difference-gain-applied=false"
                   << "\n# mask-colors=unchanged-black,changed-white";
        if (summary) {
            const auto& bounds=summary->changedBounds;
            stream << "\n# change-summary=unquantized-display-rgb"
                   << "\n# change-threshold=" << frozen.differenceThreshold
                   << "\n# compared-pixels=" << summary->comparedPixels
                   << "\n# changed-pixels=" << summary->changedPixels
                   << "\n# changed-fraction=" << summary->changedFraction()
                   << "\n# maximum-channel-difference=" << summary->maximumDifference
                   << "\n# changed-bounds=" << bounds.x << ' ' << bounds.y << ' ' << bounds.width << ' ' << bounds.height
                   << "\n# changed-bounds-space=source-pixel-xywh-half-open";
        }
        stream
               << "\n# invalid-measurements=diagnostic-view-colors\n"
               << "# native-presentation=unconfirmed\n# encoding=display-rgb8\n"
               << "# tile-width=" << layout.tileWidth << "\n# tile-height=" << layout.tileHeight
               << "\n# gutter=" << layout.gutter << "\n# caption-height=" << captionHeight << '\n';
        detail::writeDiagnosticSettingsMetadata(stream,frozen.diagnostic);
        for (std::size_t i=0; i<layout.count; ++i)
            stream << "# panel=" << labels[i] << " x=" << layout.gutter+i*(layout.tileWidth+layout.gutter)
                   << " y=" << layout.gutter+captionHeight
                   << " width=" << first.width() << " height=" << first.height() << '\n';
    },[&](std::uint32_t y) {
        return prepareDiagnosticAtlasRow(layout,y,[&](std::size_t column,std::size_t p) -> std::array<float,3> {
            if (column==0) return first.pixel(p);
            if (column==1) return second.pixel(p);
            const auto a=first.pixel(p), b=second.pixel(p);
            if (frozen.differenceMode==SculptureDiagnosticDifferenceMode::ChangeMask) {
                const float maximum=diagnosticComparisonMaximumDifference(a,b);
                const float changed=maximum>frozen.differenceThreshold ? 1.0f : 0.0f;
                return {changed,changed,changed};
            }
            std::array<float,3> difference{};
            for (std::size_t c=0; c<difference.size(); ++c)
                difference[c]=std::min(1.0f,std::abs(b[c]-a[c])*frozen.differenceGain);
            return difference;
        });
    });
}
} // namespace detail

// Explicit cold comparison of two stable, co-registered completed observations.
// All three panels share one frozen diagnostic recipe. The third panel is the
// absolute DISPLAY-color difference times gain, clamped to [0,1], or an opt-in
// black/white change mask using a strict max-channel threshold before gain.
// Both include the converter's invalid colors: equal colors always give black.
// No raw-field error, automatic alignment, production or native receipt is inferred.
// Invalid settings/extents/channels throw before filesystem work. Empty matching
// sources or path/IO refusal return false; failed publication preserves old output.
[[nodiscard]] inline bool exportSculptureDiagnosticComparisonPpm(
    const std::string& filepath, const SculptureGBuffer& before, const SculptureGBuffer& after,
    const SculptureDiagnosticComparisonSettings& settings = {}, std::uint32_t gutter=2) {
    if (!file_publication_detail::admittedFilePath(filepath)) return false;
    const auto frozen=settings;
    const auto layout=detail::prepareDiagnosticComparisonLayout(before.width,before.height,frozen,gutter);
    return detail::publishDiagnosticComparison(filepath,before,after,frozen,layout);
}

} // namespace epi_sculpt
