// Flagship-only key adapter. Produces semantic intent without mutating state.
#pragma once
#include "SculptureReliefContours.h"
#include <GLFW/glfw3.h>

namespace epi_sculpt {
[[nodiscard]] inline std::optional<ReliefContourAction> reliefContourActionFromKey(
    int key, int mods) noexcept {
    if ((mods & GLFW_MOD_ALT) == 0
        || (mods & (GLFW_MOD_CONTROL | GLFW_MOD_SUPER)) != 0) return std::nullopt;
    const bool shift = (mods & GLFW_MOD_SHIFT) != 0;
    if (key == GLFW_KEY_C)
        return shift ? ReliefContourAction::Inherit : ReliefContourAction::Toggle;
    if (shift && key == GLFW_KEY_RIGHT_BRACKET) return ReliefContourAction::CycleStyle;
    if (!shift && key == GLFW_KEY_LEFT_BRACKET) return ReliefContourAction::Coarser;
    if (!shift && key == GLFW_KEY_RIGHT_BRACKET) return ReliefContourAction::Finer;
    return std::nullopt;
}
} // namespace epi_sculpt
