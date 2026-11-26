#pragma once

#include <mgfx/ProceduralFieldChannel.h>

namespace epi_sculpt {

enum class SculptureFieldPalette { BluePink, Ice, Ember, Mono };

// Presentation-only controls; these do not alter the produced scientific field.
struct SculptureFieldPresentationOptions {
    SculptureFieldPalette palette = SculptureFieldPalette::BluePink;
    mgfx::ProceduralFieldChannel channel = mgfx::ProceduralFieldChannel::Density;
    float minimum = 0.0f;
    float maximum = 1.0f;
    float densityScale = 0.12f;
    float extinctionRatio = 2.5f;
    float anisotropy = 0.65f;
    float pulse = 0.2f;
};

} // namespace epi_sculpt
