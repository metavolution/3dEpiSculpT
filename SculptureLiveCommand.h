#pragma once

#include "SculptureLiveTransport.h"
#include "SculptureLearnMode.h"

#include <GLFW/glfw3.h>

#include <cstdint>
#include <optional>

namespace epi_sculpt {

// Small semantic vocabulary shared by the full sculpture application and the
// focused high-performance demos. Hardware-specific keys are translated once;
// each frontend remains responsible for applying the resulting intent to its
// own authoritative state.
enum class SculptureLiveCommandType {
    Quit,
    TogglePause,
    IncreasePlaybackRate,
    DecreasePlaybackRate,
    ResetPlaybackRate,
    StepCameraTour,
    Capture,
    CyclePresentation,
    ToggleLearnMode,
    ToggleStage,
    CycleDiagnostic,
    ResetStages,
    CycleResolutionScale,
    ToggleLodGovernor,
    CycleMorphEasing,
    ExportRelief,
    CycleReliefProjection,
    TriggerProfileSession,
};

struct SculptureLiveCommand {
    SculptureLiveCommandType type = SculptureLiveCommandType::Quit;
    int direction = 0;
    std::uint32_t stageBit = 0U;
};

enum class SculptureLiveInputProfile {
    FullApplication,
    FocusedDemo,
};

[[nodiscard]] inline std::optional<SculptureLiveCommand>
sculptureLiveCommandFromKey(
    int key,
    int mods,
    SculptureLiveInputProfile profile) noexcept {
    if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) {
        return SculptureLiveCommand{SculptureLiveCommandType::Quit};
    }
    if (key == GLFW_KEY_KP_MULTIPLY) {
        return SculptureLiveCommand{SculptureLiveCommandType::TogglePause};
    }
    if (key == GLFW_KEY_KP_ADD
        || (profile == SculptureLiveInputProfile::FocusedDemo && key == GLFW_KEY_EQUAL)) {
        return SculptureLiveCommand{SculptureLiveCommandType::IncreasePlaybackRate};
    }
    if (key == GLFW_KEY_KP_SUBTRACT
        || (profile == SculptureLiveInputProfile::FocusedDemo && key == GLFW_KEY_MINUS)) {
        return SculptureLiveCommand{SculptureLiveCommandType::DecreasePlaybackRate};
    }
    if (key == GLFW_KEY_KP_DIVIDE) {
        return SculptureLiveCommand{SculptureLiveCommandType::ResetPlaybackRate};
    }
    if (key == GLFW_KEY_T) {
        const int direction = (mods & GLFW_MOD_SHIFT) != 0 ? -1 : 1;
        return SculptureLiveCommand{
            SculptureLiveCommandType::StepCameraTour, direction};
    }
    if (key == GLFW_KEY_SPACE) {
        if ((mods & GLFW_MOD_SHIFT) != 0) {
            return SculptureLiveCommand{SculptureLiveCommandType::ExportRelief};
        }
        return SculptureLiveCommand{SculptureLiveCommandType::Capture};
    }
    if (key == GLFW_KEY_P) {
        return SculptureLiveCommand{SculptureLiveCommandType::CyclePresentation};
    }
    if (key == GLFW_KEY_L) {
        if ((mods & GLFW_MOD_SHIFT) != 0) {
            return SculptureLiveCommand{SculptureLiveCommandType::ResetStages};
        }
        return SculptureLiveCommand{SculptureLiveCommandType::ToggleLearnMode};
    }
    if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F6) {
        const auto stage = static_cast<learn_mode::Stage>(key - GLFW_KEY_F1);
        return SculptureLiveCommand{
            SculptureLiveCommandType::ToggleStage, 0, learn_mode::bit(stage)};
    }
    if (key == GLFW_KEY_F7) {
        return SculptureLiveCommand{SculptureLiveCommandType::ResetStages};
    }
    if (key == GLFW_KEY_F8) {
        return SculptureLiveCommand{SculptureLiveCommandType::CycleMorphEasing};
    }
    if (key == GLFW_KEY_F9) {
        return SculptureLiveCommand{SculptureLiveCommandType::CycleResolutionScale};
    }
    if (key == GLFW_KEY_F10) {
        return SculptureLiveCommand{SculptureLiveCommandType::ToggleLodGovernor};
    }
    if (key == GLFW_KEY_F11) {
        return SculptureLiveCommand{SculptureLiveCommandType::TriggerProfileSession};
    }
    if (key == GLFW_KEY_F12) {
        const int direction = (mods & GLFW_MOD_SHIFT) != 0 ? -1 : 1;
        return SculptureLiveCommand{SculptureLiveCommandType::CycleReliefProjection, direction};
    }
    if (key == GLFW_KEY_U) {
        return SculptureLiveCommand{SculptureLiveCommandType::CycleDiagnostic};
    }
    if (profile == SculptureLiveInputProfile::FocusedDemo) {
        if (key >= GLFW_KEY_1 && key <= GLFW_KEY_6) {
            const auto stage = static_cast<learn_mode::Stage>(key - GLFW_KEY_1);
            return SculptureLiveCommand{
                SculptureLiveCommandType::ToggleStage, 0, learn_mode::bit(stage)};
        }
        if (key == GLFW_KEY_0) {
            return SculptureLiveCommand{SculptureLiveCommandType::CycleDiagnostic};
        }
    }
    return std::nullopt;
}

} // namespace epi_sculpt
