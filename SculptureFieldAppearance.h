#pragma once

#include "SculptureFieldPresentationOptions.h"
#include "SculptureVolumePresentationState.h"
#include <mgfx/ProceduralFieldAppearance.h>
#include <mgfx/ProceduralGeometryRecipe.h>

namespace epi_sculpt {

// Persistence stays renderer-neutral. These adapters alone translate its stable
// scalar fields to the typed GPU presentation schema; no native owner is saved.
static_assert(static_cast<std::uint32_t>(mgfx::ProceduralGeometryRecipe::Field) == 0U);
static_assert(static_cast<std::uint32_t>(mgfx::ProceduralGeometryRecipe::EpiMorphWoodland) == 1U);
static_assert(static_cast<std::uint32_t>(mgfx::ProceduralGeometryRecipe::OctonionWoodland) == 2U);
static_assert(static_cast<std::uint32_t>(mgfx::ProceduralGeometryRecipe::MandelbulbWoodland) == 3U);
static_assert(static_cast<std::uint32_t>(mgfx::ProceduralGeometryRecipe::DielectricWoodland) == 4U);
static_assert(static_cast<std::uint32_t>(mgfx::ProceduralGeometryRecipe::Count) == 5U);
static_assert(static_cast<std::uint32_t>(mgfx::ProceduralFieldChannel::X) == 0U);
static_assert(static_cast<std::uint32_t>(mgfx::ProceduralFieldChannel::Y) == 1U);
static_assert(static_cast<std::uint32_t>(mgfx::ProceduralFieldChannel::Z) == 2U);
static_assert(static_cast<std::uint32_t>(mgfx::ProceduralFieldChannel::Density) == 3U);

[[nodiscard]] inline SculptureVolumePresentationState captureSculptureVolumePresentation(
    mgfx::ProceduralGeometryRecipe recipe, const mgfx::ProceduralFieldAppearance& appearance) noexcept {
    SculptureVolumePresentationState result{};
    result.recipe = static_cast<std::uint32_t>(recipe);
    result.colorChannel = static_cast<std::uint32_t>(appearance.colorChannel);
    for (unsigned i = 0; i < 4; ++i) {
        result.lowColorAndMinimum[i] = appearance.lowColorAndMinimum[i];
        result.highColorAndMaximum[i] = appearance.highColorAndMaximum[i];
        result.optics[i] = appearance.optics[i];
    }
    return result;
}

[[nodiscard]] inline mgfx::ProceduralFieldAppearance restoreSculptureFieldAppearance(
    const SculptureVolumePresentationState& state) noexcept {
    mgfx::ProceduralFieldAppearance result{};
    result.colorChannel = static_cast<mgfx::ProceduralFieldChannel>(state.colorChannel);
    for (unsigned i = 0; i < 4; ++i) {
        result.lowColorAndMinimum[i] = state.lowColorAndMinimum[i];
        result.highColorAndMaximum[i] = state.highColorAndMaximum[i];
        result.optics[i] = state.optics[i];
    }
    return result;
}

[[nodiscard]] inline mgfx::ProceduralFieldAppearance makeSculptureFieldAppearance(
    const SculptureFieldPresentationOptions& options) noexcept {
    mgfx::ProceduralFieldAppearance result{};
    switch (options.palette) {
    case SculptureFieldPalette::BluePink: break;
    case SculptureFieldPalette::Ice:
        result.lowColorAndMinimum = {0.01f, 0.12f, 0.2f, options.minimum};
        result.highColorAndMaximum = {0.72f, 0.95f, 1.0f, options.maximum};
        break;
    case SculptureFieldPalette::Ember:
        result.lowColorAndMinimum = {0.08f, 0.01f, 0.005f, options.minimum};
        result.highColorAndMaximum = {1.0f, 0.7f, 0.16f, options.maximum};
        break;
    case SculptureFieldPalette::Mono:
        result.lowColorAndMinimum = {0.08f, 0.08f, 0.08f, options.minimum};
        result.highColorAndMaximum = {0.95f, 0.95f, 0.95f, options.maximum};
        break;
    }
    result.lowColorAndMinimum.w = options.minimum;
    result.highColorAndMaximum.w = options.maximum;
    result.optics = {options.densityScale, options.extinctionRatio, options.anisotropy, options.pulse};
    result.colorChannel = options.channel;
    return result;
}

} // namespace epi_sculpt
