#pragma once

#include "SculptureDiagnosticView.h"
#include "SculptureGBuffer.h"

#include <limits>
#include <span>
#include <utility>

namespace epi_sculpt {
namespace detail {
// Tiny 5x7 uppercase caption glyphs, stored by column, bit zero at the top.
// Keeps the explicit CPU export independent of renderer/font/asset lifetimes.
inline std::array<std::uint8_t,5> diagnosticCaptionGlyph(char c) noexcept {
    switch (c) {
    case 'a': return {0x7e,0x11,0x11,0x11,0x7e};
    case 'b': return {0x7f,0x49,0x49,0x49,0x36};
    case 'c': return {0x3e,0x41,0x41,0x41,0x22};
    case 'd': return {0x7f,0x41,0x41,0x22,0x1c};
    case 'e': return {0x7f,0x49,0x49,0x49,0x41};
    case 'f': return {0x7f,0x09,0x09,0x09,0x01};
    case 'g': return {0x3e,0x41,0x49,0x49,0x7a};
    case 'h': return {0x7f,0x08,0x08,0x08,0x7f};
    case 'i': return {0x41,0x41,0x7f,0x41,0x41};
    case 'j': return {0x20,0x40,0x41,0x3f,0x01};
    case 'k': return {0x7f,0x08,0x14,0x22,0x41};
    case 'l': return {0x7f,0x40,0x40,0x40,0x40};
    case 'm': return {0x7f,0x02,0x0c,0x02,0x7f};
    case 'n': return {0x7f,0x02,0x04,0x08,0x7f};
    case 'o': return {0x3e,0x41,0x41,0x41,0x3e};
    case 'p': return {0x7f,0x09,0x09,0x09,0x06};
    case 'q': return {0x3e,0x41,0x51,0x21,0x5e};
    case 'r': return {0x7f,0x09,0x19,0x29,0x46};
    case 's': return {0x46,0x49,0x49,0x49,0x31};
    case 't': return {0x01,0x01,0x7f,0x01,0x01};
    case 'u': return {0x3f,0x40,0x40,0x40,0x3f};
    case 'v': return {0x1f,0x20,0x40,0x20,0x1f};
    case 'w': return {0x7f,0x20,0x18,0x20,0x7f};
    case 'x': return {0x63,0x14,0x08,0x14,0x63};
    case 'y': return {0x07,0x08,0x70,0x08,0x07};
    case 'z': return {0x61,0x51,0x49,0x45,0x43};
    case '-': return {0x08,0x08,0x08,0x08,0x08};
    default: return {0x02,0x01,0x51,0x09,0x06}; // Visible unknown glyph.
    }
}

inline std::array<float,3> diagnosticCaptionPixel(
    std::string_view label, std::uint64_t x, std::uint64_t y) noexcept {
    const auto character=x/6U, glyphX=x%6U;
    if (y==0 || y>=8 || character>=label.size() || glyphX>=5) return {};
    const auto glyph=diagnosticCaptionGlyph(label[character]);
    const float ink=(glyph[glyphX] & (1U<<(y-1U))) ? 1.0f : 0.0f;
    return {ink,ink,ink};
}

// Resource-free derived layout. A render request and a completed-buffer export
// share this admission boundary; neither layout preparation nor export owns time.
struct DiagnosticAtlasLayout {
    static constexpr std::uint32_t captionHeight=9;
    std::array<SculptureDiagnosticSettings,kSculptureDiagnosticViews.size()> settings{};
    std::array<std::string_view,kSculptureDiagnosticViews.size()> labels{};
    std::size_t count=0, columns=0, rows=0;
    std::uint32_t sourceWidth=0, sourceHeight=0, width=0, height=0, gutter=0;
    std::uint64_t tileWidth=0, tileHeight=0;
    [[nodiscard]] bool empty() const noexcept { return sourceWidth==0 || sourceHeight==0; }
};

[[nodiscard]] inline DiagnosticAtlasLayout prepareDiagnosticAtlasLayout(
    std::uint32_t sourceWidth, std::uint32_t sourceHeight,
    std::span<const SculptureDiagnosticSettings> views,
    std::uint32_t requestedColumns, std::uint32_t gutter,
    std::span<const std::string_view> labels = {}) {
    if (views.empty() || views.size()>kSculptureDiagnosticViews.size() || requestedColumns==0)
        throw std::invalid_argument("diagnostic atlas requires a bounded nonempty tile selection and columns");
    if (!labels.empty() && labels.size()!=views.size())
        throw std::invalid_argument("diagnostic captions must match tile selection");
    DiagnosticAtlasLayout layout;
    layout.count=views.size();
    layout.columns=std::min<std::size_t>(requestedColumns,layout.count);
    layout.rows=(layout.count+layout.columns-1U)/layout.columns;
    layout.sourceWidth=sourceWidth; layout.sourceHeight=sourceHeight;
    layout.gutter=gutter; layout.tileWidth=sourceWidth;
    for (std::size_t i=0; i<layout.count; ++i) {
        layout.settings[i]=views[i];
        if (!layout.settings[i].valid()) throw std::invalid_argument("invalid raymarch diagnostic settings");
        layout.labels[i]=labels.empty()
            ? sculptureDiagnosticDescriptor(layout.settings[i].view)->id : labels[i];
        // Callers use short static captions; reject impossible label arithmetic
        // before deriving the width, even on narrower size_t platforms.
        if (layout.labels[i].empty() || layout.labels[i].size()>std::numeric_limits<std::uint32_t>::max()/6U)
            throw std::length_error("diagnostic caption exceeds image limits");
        layout.tileWidth=std::max(layout.tileWidth,static_cast<std::uint64_t>(layout.labels[i].size()*6U-1U));
    }
    if (layout.empty()) return layout;
    // Refuse impossible source/output shapes before a caller allocates or renders.
    (void)checkedRaymarchElementCount<4>(sourceWidth,sourceHeight);
    layout.tileHeight=static_cast<std::uint64_t>(sourceHeight)+DiagnosticAtlasLayout::captionHeight;
    const auto width=layout.columns*(layout.tileWidth+gutter)+gutter;
    const auto height=layout.rows*(layout.tileHeight+gutter)+gutter;
    if (width>std::numeric_limits<std::uint32_t>::max() || height>std::numeric_limits<std::uint32_t>::max())
        throw std::length_error("diagnostic atlas extent exceeds image limits");
    layout.width=static_cast<std::uint32_t>(width); layout.height=static_cast<std::uint32_t>(height);
    (void)checkedRaymarchElementCount<3>(layout.width,layout.height);
    return layout;
}

// One admitted grid-to-source mapping for both same-source atlases and paired
// comparison sheets. p = sourceY * sourceWidth + sourceX; the typed reader owns
// pixel meaning, while this layer owns only captions, gutters and addressing.
// The encoder consumes the returned callable within this row. The layout and
// borrowed reader inputs must remain stable for the synchronous publication.
template<class ReadPixel>
[[nodiscard]] inline auto prepareDiagnosticAtlasRow(
    const DiagnosticAtlasLayout& layout, std::uint32_t y, ReadPixel readPixel) {
    constexpr auto captionHeight=DiagnosticAtlasLayout::captionHeight;
    const auto row=y<layout.gutter ? layout.rows : (y-layout.gutter)/(layout.tileHeight+layout.gutter);
    const auto localY=y<layout.gutter ? layout.tileHeight : (y-layout.gutter)%(layout.tileHeight+layout.gutter);
    const auto rowBase=row*layout.columns;
    const bool visible=rowBase<layout.count && localY<layout.tileHeight;
    const auto sourceOffset=localY>=captionHeight && localY<layout.tileHeight
        ? static_cast<std::size_t>(localY-captionHeight)*layout.sourceWidth : 0U;
    return [&layout,rowBase,localY,visible,sourceOffset,readPixel=std::move(readPixel)]
        (std::uint32_t x) -> std::array<float,3> {
        if (!visible || x<layout.gutter) return {};
        const auto column=(x-layout.gutter)/(layout.tileWidth+layout.gutter);
        const auto localX=(x-layout.gutter)%(layout.tileWidth+layout.gutter);
        const auto index=rowBase+column;
        if (column>=layout.columns || index>=layout.count || localX>=layout.tileWidth) return {};
        if (localY<captionHeight) return diagnosticCaptionPixel(layout.labels[index],localX,localY);
        if (localX>=layout.sourceWidth) return {};
        return readPixel(index,sourceOffset+static_cast<std::size_t>(localX));
    };
}
} // namespace detail
} // namespace epi_sculpt
