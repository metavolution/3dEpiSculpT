#pragma once

// -----------------------------------------------------------------------------
// Educational Annotation: DECLARATIVE PLATFORM INPUT KEY-ACTION MAPPER
// -----------------------------------------------------------------------------
// Translates platform-specific raw keyboard events (e.g. GLFW key code, action,
// and modifier bitmask) into semantic NavigatorAction commands.
//
// Architectural Benefits:
//  1. Decoupling: Presentation loops and UI layers don't hardcode input logic.
//  2. Uniformity: Demos, test fixtures, VR headsets, and remote controllers
//     share the same canonical key mapping table.
//  3. Rebindability: Custom key bindings can override or extend defaults.
//  4. Fault-Tolerance: Unrecognized keys or invalid modifiers are cleanly
//     ignored with zero side-effects.
// -----------------------------------------------------------------------------

#include "SculptureWorldNavigator.h"

#include <GLFW/glfw3.h>
#include <optional>

namespace epi_sculpt {

class SculptureKeyActionMapper {
public:
    [[nodiscard]] static std::optional<ActionCommand> mapGlfwKeyToAction(
        int key,
        int action,
        int mods
    ) noexcept {
        if (action != GLFW_PRESS && action != GLFW_REPEAT) {
            return std::nullopt;
        }

        const bool isPress = (action == GLFW_PRESS);
        const bool shift = (mods & GLFW_MOD_SHIFT) != 0;
        const bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
        const bool alt = (mods & GLFW_MOD_ALT) != 0;

        // 0. Undo & Redo History System (docu/project-vision.txt)
        if (isPress) {
            if (ctrl && (key == GLFW_KEY_Z || key == GLFW_KEY_Y)) {
                if (key == GLFW_KEY_Y || shift) {
                    return ActionCommand{NavigatorAction::Redo};
                }
                return ActionCommand{NavigatorAction::Undo};
            }
        }

        // 1. Core Navigation & Autopilot Tours
        if (isPress) {
            if (!alt && (key == GLFW_KEY_RIGHT || (!shift && key == GLFW_KEY_N))) return ActionCommand{NavigatorAction::NextWorld};
            if (!alt && (key == GLFW_KEY_LEFT || (shift && key == GLFW_KEY_N))) return ActionCommand{NavigatorAction::PrevWorld};
            if (!alt && key == GLFW_KEY_P) return ActionCommand{NavigatorAction::ToggleStereoSBS};
            if (key == GLFW_KEY_T || key == GLFW_KEY_O) return ActionCommand{NavigatorAction::ToggleTour};
            if (!ctrl && key == GLFW_KEY_Y) {
                return shift ? ActionCommand{NavigatorAction::CycleCategoryTour}
                             : ActionCommand{NavigatorAction::CycleTour};
            }
        }

        // 2. Multimodal Audio Synesthesia & Dynamic Sound
        if (isPress) {
            if (key == GLFW_KEY_U) {
                return shift ? ActionCommand{NavigatorAction::ToggleAudioFeed}
                             : ActionCommand{NavigatorAction::CycleAudioFeedMode};
            }
        }

        // 3. Procedural Geometry & Parameter Mutation
        if (isPress) {
            if (key == GLFW_KEY_M) {
                if (ctrl) return ActionCommand{NavigatorAction::SyncMorphSpaceCategory};
                if (alt)  return ActionCommand{NavigatorAction::CycleMorphSpaceBank};
                return ActionCommand{NavigatorAction::Mutate};
            }
            if (key == GLFW_KEY_C) return ActionCommand{NavigatorAction::RandomizePalette};
            if (key == GLFW_KEY_BACKSPACE || key == GLFW_KEY_DELETE) return ActionCommand{NavigatorAction::ResetParameters};
            if (key == GLFW_KEY_I) return ActionCommand{NavigatorAction::CycleInnerMap};
            if (key == GLFW_KEY_K) {
                return shift ? ActionCommand{NavigatorAction::CycleMirrorTopology}
                             : ActionCommand{NavigatorAction::CycleReduction};
            }
            if (key == GLFW_KEY_G) {
                if (shift || ctrl) return ActionCommand{NavigatorAction::ToggleAutoResolutionScaling};
                return ActionCommand{NavigatorAction::CycleResolutionScale};
            }
        }

        // 4. 4D Foliation Spacetime & SO(4) Rotor Kinematics
        if (isPress) {
            if (key == GLFW_KEY_4) return ActionCommand{NavigatorAction::Toggle4DFoliationWave};
            if (key == GLFW_KEY_X) {
                if (shift) return ActionCommand{NavigatorAction::CyclePolychoronSource};
                if (ctrl)  return ActionCommand{NavigatorAction::CyclePolychoronTarget};
                if (alt)   return ActionCommand{NavigatorAction::StepPolychoronMorph, 0.25f};
                return ActionCommand{NavigatorAction::Cycle4DManifold};
            }
            if (!ctrl && key == GLFW_KEY_Z) return ActionCommand{NavigatorAction::TogglePaletteAgent};
            if (key == GLFW_KEY_BACKSLASH) return ActionCommand{NavigatorAction::Reset4DRotors};
        }
        // Continuous Rotor & Slice scrubbing (fires on press and repeat)
        if (key == GLFW_KEY_LEFT_BRACKET) return ActionCommand{NavigatorAction::AdjustLatentW, -0.05f};
        if (key == GLFW_KEY_RIGHT_BRACKET) return ActionCommand{NavigatorAction::AdjustLatentW, +0.05f};
        if (key == GLFW_KEY_SEMICOLON) return ActionCommand{NavigatorAction::AdjustRotorAlpha, -0.05f};
        if (key == GLFW_KEY_APOSTROPHE) return ActionCommand{NavigatorAction::AdjustRotorAlpha, +0.05f};

        // 5. Educational Learn Mode & Diagnostics
        if (isPress) {
            if (key == GLFW_KEY_L) {
                return shift ? ActionCommand{NavigatorAction::ResetLearnMode}
                             : ActionCommand{NavigatorAction::CycleLearnStage};
            }
            if (key == GLFW_KEY_J) return ActionCommand{NavigatorAction::CycleDiagnostic};

            // Direct stage solo selection (Alt + 1..6) and reset (Alt + 0)
            if (alt && key >= GLFW_KEY_1 && key <= GLFW_KEY_6) {
                return ActionCommand{NavigatorAction::SoloStage, static_cast<float>(key - GLFW_KEY_1)};
            }
            if (alt && key == GLFW_KEY_0) {
                return ActionCommand{NavigatorAction::ResetLearnStages};
            }

            // Function keys: Shift+F1..F6 for Solo Stage, Ctrl+F1..F6 for Accumulative Stage
            if (shift && key >= GLFW_KEY_F1 && key <= GLFW_KEY_F6) {
                return ActionCommand{NavigatorAction::SoloStage, static_cast<float>(key - GLFW_KEY_F1)};
            }
            if (ctrl && key >= GLFW_KEY_F1 && key <= GLFW_KEY_F6) {
                return ActionCommand{NavigatorAction::SetLearnAccumulative, static_cast<float>(key - GLFW_KEY_F1)};
            }
        }

        // 6. Camera Framing & World Presentation Projections
        if (isPress) {
            if (key == GLFW_KEY_R || key == GLFW_KEY_F) return ActionCommand{NavigatorAction::ResetCamera};
            if (key == GLFW_KEY_V) {
                if (ctrl && alt) return ActionCommand{NavigatorAction::ToggleVolumetricCoupling};
                if (ctrl && shift) return ActionCommand{NavigatorAction::ToggleVolumetricBackend};
                if (alt) return ActionCommand{NavigatorAction::ToggleVolumetricAdvection};
                if (ctrl) return ActionCommand{NavigatorAction::CycleVolumetricResolution};
                if (shift) return ActionCommand{NavigatorAction::ToggleVolumetricFeedback};
                return ActionCommand{NavigatorAction::CyclePresentationMode};
            }
            if (key == GLFW_KEY_W && shift) return ActionCommand{NavigatorAction::ToggleVolumetricWrap};
            if (key == GLFW_KEY_B) return ActionCommand{NavigatorAction::CycleReliefProjection};
            if (key == GLFW_KEY_A) {
                if (shift) return ActionCommand{NavigatorAction::ToggleAcousticSynesthesia};
                return ActionCommand{NavigatorAction::ToggleAtmosphere};
            }
        }

        // 7. Simulation Chronos & Playback Speed Controls
        if (isPress) {
            if (key == GLFW_KEY_KP_ADD || key == GLFW_KEY_EQUAL) return ActionCommand{NavigatorAction::IncreaseSpeed};
            if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS) return ActionCommand{NavigatorAction::DecreaseSpeed};
            if (key == GLFW_KEY_0) return ActionCommand{NavigatorAction::ResetSpeed};
            if (!alt && (key == GLFW_KEY_PERIOD || key == GLFW_KEY_KP_MULTIPLY)) return ActionCommand{NavigatorAction::TogglePause};
        }

        // 8. Snapshots, Video Recording & Timeline Choreography
        if (isPress) {
            if (key == GLFW_KEY_F5) return ActionCommand{NavigatorAction::ReloadPresetsAndKernel};
            if (key == GLFW_KEY_SPACE) return ActionCommand{NavigatorAction::RequestSnapshot};
            if (key == GLFW_KEY_F7) {
                if (shift) return ActionCommand{NavigatorAction::TriggerAutoCrossfade};
                if (ctrl)  return ActionCommand{NavigatorAction::SwapPerformanceDecks};
                return ActionCommand{NavigatorAction::TogglePerformanceDeck};
            }
            if (key == GLFW_KEY_F8) return ActionCommand{NavigatorAction::ReloadLatestSnapshot};
            if (key == GLFW_KEY_F9) return ActionCommand{NavigatorAction::ToggleSequenceRecording};
            if (key == GLFW_KEY_F10) {
                if (ctrl) return ActionCommand{NavigatorAction::ExportTimelineScript};
                if (shift) return ActionCommand{NavigatorAction::BuildBeatSyncedTimeline};
                return ActionCommand{NavigatorAction::ToggleTimeline};
            }
            if (key == GLFW_KEY_F11) {
                if (ctrl) return ActionCommand{NavigatorAction::ToggleAutoResolutionScaling};
                return shift ? ActionCommand{NavigatorAction::TogglePerfMonitor}
                             : ActionCommand{NavigatorAction::CycleTargetFps};
            }
        }

        // 9. Multidimensional 2D Morph Space Vector Synthesis & Continuous Steering
        if (isPress) {
            if (key == GLFW_KEY_F12) {
                if (alt) return ActionCommand{NavigatorAction::CycleMorphSpaceBank};
                if (ctrl && shift) return ActionCommand{NavigatorAction::ToggleMorphSpacePath};
                if (ctrl) return ActionCommand{NavigatorAction::SyncMorphSpaceCategory};
                return shift ? ActionCommand{NavigatorAction::CycleMorphSpaceMotion}
                             : ActionCommand{NavigatorAction::ToggleMorphSpace};
            }
        }
        // Continuous Morph Space & VJ Crossfader Nudging (Alt + Arrow / Comma / Period keys, fires on press and repeat)
        if (alt) {
            if (key == GLFW_KEY_COMMA)  return ActionCommand{NavigatorAction::CrossfadeDeck, -0.05f};
            if (key == GLFW_KEY_PERIOD) return ActionCommand{NavigatorAction::CrossfadeDeck, +0.05f};
            if (key == GLFW_KEY_RIGHT) return ActionCommand{NavigatorAction::NudgeMorphSpaceX, +0.05f};
            if (key == GLFW_KEY_LEFT)  return ActionCommand{NavigatorAction::NudgeMorphSpaceX, -0.05f};
            if (key == GLFW_KEY_UP)    return ActionCommand{NavigatorAction::NudgeMorphSpaceY, +0.05f};
            if (key == GLFW_KEY_DOWN)  return ActionCommand{NavigatorAction::NudgeMorphSpaceY, -0.05f};
        }

        return std::nullopt;
    }

    static void glfwKeyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int mods) {
        if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) {
            if (action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            return;
        }

        if (auto cmd = mapGlfwKeyToAction(key, action, mods)) {
            auto* nav = static_cast<SculptureWorldNavigator*>(glfwGetWindowUserPointer(window));
            if (nav != nullptr) {
                nav->enqueueAction(*cmd);
            }
        }
    }

    static void attachGlfwKeyCallback(GLFWwindow* window, SculptureWorldNavigator* navigator) {
        if (!window) return;
        glfwSetWindowUserPointer(window, navigator);
        glfwSetKeyCallback(window, glfwKeyCallback);
    }
};

} // namespace epi_sculpt
