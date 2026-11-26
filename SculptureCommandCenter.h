#pragma once

#include "SculptureEventBus.h"
#include "SculptureLiveCommand.h"
#include "SculptureReliefContourInput.h"
#include "SculptureProgramSchema.h"
#include "Sculpture4DFoliation.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: DATA-DRIVEN INPUT ROUTING
// -----------------------------------------------------------------------------
// Translates platform input into renderer-neutral semantic events. mGFX owns
// GLFW's window user pointer, so this adapter uses a window-keyed registry and
// chains/restores callbacks instead of replacing mGFX's hidden window state.
// 
// By using a data-driven approach for key bindings (rather than monolithic 
// if-else blocks), the execution flow is simplified, highly extendable, and 
// can be dynamically reconfigured at runtime without recompilation.
// -----------------------------------------------------------------------------

class SculptureCommandCenter {
public:
    SculptureCommandCenter(GLFWwindow* window, SculptureEventBus* eventBus)
        : window_(window), eventBus_(eventBus) {
        glfwGetCursorPos(window_, &lastCursorX_, &lastCursorY_);
        instances_[window_] = this;
        previousKeyCallback_ = glfwSetKeyCallback(window_, keyCallback);
        previousDropCallback_ = glfwSetDropCallback(window_, dropCallback);
    }

    SculptureCommandCenter(const SculptureCommandCenter&) = delete;
    SculptureCommandCenter& operator=(const SculptureCommandCenter&) = delete;

    ~SculptureCommandCenter() {
        const auto found = instances_.find(window_);
        if (found != instances_.end() && found->second == this) {
            glfwSetKeyCallback(window_, previousKeyCallback_);
            glfwSetDropCallback(window_, previousDropCallback_);
            instances_.erase(found);
        }
    }

    void update(float deltaSeconds) {
        const float dt = std::clamp(deltaSeconds, 0.0f, 0.1f);
        const bool fast = glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
            || glfwGetKey(window_, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        const float steeringRate = fast ? 4.0f : 1.0f;

        float deltaLatentW = 0.0f;
        float deltaSimTime = 0.0f;
        if (glfwGetKey(window_, GLFW_KEY_UP) == GLFW_PRESS) {
            deltaLatentW += 0.8f * steeringRate * dt;
        }
        if (glfwGetKey(window_, GLFW_KEY_DOWN) == GLFW_PRESS) {
            deltaLatentW -= 0.8f * steeringRate * dt;
        }

        double cursorX = 0.0;
        double cursorY = 0.0;
        glfwGetCursorPos(window_, &cursorX, &cursorY);
        const float dx = static_cast<float>(cursorX - lastCursorX_);
        const float dy = static_cast<float>(cursorY - lastCursorY_);
        lastCursorX_ = cursorX;
        lastCursorY_ = cursorY;

        // Pointer navigation belongs to SculptureLiveControls, shared with the
        // focused OpenCL/Vulkan demos.  Preserve Alt+drag as the explicit 2D
        // Morph Space gesture; unmodified left drag is now a 3D orbit gesture.
        if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            const bool altDrag = (glfwGetKey(window_, GLFW_KEY_LEFT_ALT) == GLFW_PRESS
                || glfwGetKey(window_, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS);
            if (altDrag) {
                eventBus_->post(EventMorphSpaceControl{
                    .action = EventMorphSpaceControl::Action::NudgeCursor,
                    .cursorX = dx * 0.003f,
                    .cursorY = -dy * 0.003f
                });
            }
        }

        if (deltaLatentW != 0.0f || deltaSimTime != 0.0f) {
            eventBus_->post(EventSteering{deltaLatentW, deltaSimTime});
        }
    }

private:
    void dispatchSharedCommand(const SculptureLiveCommand& command) {
        switch (command.type) {
        case SculptureLiveCommandType::Quit:
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
            break;
        case SculptureLiveCommandType::TogglePause:
            eventBus_->post(EventToggleState{EventToggleState::Target::Pause});
            break;
        case SculptureLiveCommandType::IncreasePlaybackRate:
            eventBus_->post(EventPlaybackRate{EventPlaybackRate::Action::Increase});
            break;
        case SculptureLiveCommandType::DecreasePlaybackRate:
            eventBus_->post(EventPlaybackRate{EventPlaybackRate::Action::Decrease});
            break;
        case SculptureLiveCommandType::ResetPlaybackRate:
            eventBus_->post(EventPlaybackRate{EventPlaybackRate::Action::Reset});
            break;
        case SculptureLiveCommandType::StepCameraTour:
            eventBus_->post(EventCameraModeCycle{command.direction});
            break;
        case SculptureLiveCommandType::Capture:
            // [EDUCATIONAL ANNOTATION]
            // Queue an asynchronous frame/state capture and retain this live
            // command's separate state-save action. Keyboard Space uses the
            // snapshot event below; admission/completion belongs to the app.
            eventBus_->post(EventCapture{});
            eventBus_->post(EventFileAction{EventFileAction::Action::SaveState, "user_state.epi"});
            break;
        case SculptureLiveCommandType::ExportRelief:
            // [EDUCATIONAL ANNOTATION]
            // Watertight Epimorphic Relief Export: Shift+Space triggers 3D print
            // solid mesh synthesis and exports STL and OBJ with topological validation.
            eventBus_->post(EventFileAction{EventFileAction::Action::ExportReliefMesh, "epimorphic_relief_3dprint"});
            break;
        case SculptureLiveCommandType::CycleReliefProjection:
            // [EDUCATIONAL ANNOTATION]
            // Non-Euclidean Manifold Projection Cycling: F12 / Shift+F12 cycles
            // between Planar, Poincare Disk, Klein-Beltrami, Riemann Sphere,
            // Toroidal Chamber, Hyperbolic Pseudosphere, and Auto-Deduction.
            eventBus_->post(EventReliefProjectionCycle{command.direction});
            break;
        case SculptureLiveCommandType::CyclePresentation:
            eventBus_->post(EventRenderStrategyCycle{});
            break;
        case SculptureLiveCommandType::ToggleLearnMode:
            eventBus_->post(EventLearnStage{learn_mode::kCycleDiagnosticCommand});
            break;
        case SculptureLiveCommandType::ToggleStage:
            eventBus_->post(EventLearnStage{command.stageBit});
            break;
        case SculptureLiveCommandType::CycleDiagnostic:
            eventBus_->post(EventLearnStage{learn_mode::kCycleDiagnosticCommand});
            break;
        case SculptureLiveCommandType::ResetStages:
            eventBus_->post(EventLearnStage{learn_mode::kResetStagesCommand});
            break;
        case SculptureLiveCommandType::CycleResolutionScale:
            eventBus_->post(EventResolutionScaleCycle{});
            break;
        case SculptureLiveCommandType::ToggleLodGovernor:
            eventBus_->post(EventLodGovernorToggle{});
            break;
        case SculptureLiveCommandType::CycleMorphEasing:
            eventBus_->post(EventMorphEasingCycle{});
            break;
        case SculptureLiveCommandType::TriggerProfileSession:
            eventBus_->post(EventTriggerProfileSession{});
            break;
        }
    }

    void dispatchApplicationKey(int key, int mods) {
        if (const auto action = reliefContourActionFromKey(key, mods)) {
            eventBus_->post(EventReliefContourControl{*action});
            return;
        }
        const bool shift = (mods & GLFW_MOD_SHIFT) != 0;
        const bool control = (mods & GLFW_MOD_CONTROL) != 0;
        const bool alt = (mods & GLFW_MOD_ALT) != 0;

        // Authored transport/navigation uses the same semantic vocabulary as other
        // adapters. The application resolves cinematic versus playlist ownership.
        if (control && !shift && !alt && (mods & GLFW_MOD_SUPER) == 0) {
            if (key == GLFW_KEY_ENTER) {
                eventBus_->post(EventTimelineControl{EventTimelineControl::Action::TogglePlayPause});
                return;
            }
            if (key == GLFW_KEY_BACKSPACE) {
                eventBus_->post(EventTimelineControl{EventTimelineControl::Action::Stop});
                return;
            }
            if (key == GLFW_KEY_PAGE_UP) {
                eventBus_->post(EventTimelineControl{EventTimelineControl::Action::PreviousCue});
                return;
            }
            if (key == GLFW_KEY_PAGE_DOWN) {
                eventBus_->post(EventTimelineControl{EventTimelineControl::Action::NextCue});
                return;
            }
            if (key == GLFW_KEY_HOME) {
                eventBus_->post(EventTimelineControl{
                    .action = EventTimelineControl::Action::SkipToCue, .cueIndex = 0U});
                return;
            }
        }

        // F9's plain form remains the mathematical auxiliary-resolution
        // control. Modifier forms are presentation-only and therefore must be
        // resolved before the shared command map consumes the key.
        if (control && shift && key == GLFW_KEY_F9) {
            eventBus_->post(EventSceneRenderScaleControl{
                .action = EventSceneRenderScaleControl::Action::ToggleAuto});
            return;
        }
        if (control && key == GLFW_KEY_F9) {
            eventBus_->post(EventSceneRenderScaleControl{
                .action = EventSceneRenderScaleControl::Action::Cycle});
            return;
        }
        if (shift && key == GLFW_KEY_F9) {
            eventBus_->post(EventUpsampleFilterCycle{});
            return;
        }

        if (const auto command = sculptureLiveCommandFromKey(
                key, mods, SculptureLiveInputProfile::FullApplication)) {
            dispatchSharedCommand(*command);
            return;
        }

        // -------------------------------------------------------------------------
        // Data-Driven Direct Key Evaluator
        // -------------------------------------------------------------------------
        // Shift+1..8 selects the bounded world catalog. Plain number keys keep
        // their established direct-preset behavior for existing operators.
        if (shift && key >= GLFW_KEY_1 && key <= GLFW_KEY_8) {
            eventBus_->post(EventWorldCatalogSelect{
                static_cast<std::size_t>(key - GLFW_KEY_1)});
        }
        else if (alt && key == GLFW_KEY_4) {
            static std::uint8_t currentManifold = 0;
            currentManifold = static_cast<std::uint8_t>((shift)
                ? ((currentManifold + kManifold4DTypeCount - 1) % kManifold4DTypeCount)
                : ((currentManifold + 1) % kManifold4DTypeCount));
            eventBus_->post(EventFoliationManifold{currentManifold});
        }
        else if (alt && key >= GLFW_KEY_1 && key <= GLFW_KEY_6) eventBus_->post(EventLearnStage{learn_mode::commandSoloStage(static_cast<learn_mode::Stage>(key - GLFW_KEY_1))});
        else if (alt && key == GLFW_KEY_0) eventBus_->post(EventLearnStage{learn_mode::kResetStagesCommand});
        else if (control && key == GLFW_KEY_4) eventBus_->post(EventMutation{EventMutation::Action::Toggle4D});
        else if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) eventBus_->post(EventPresetSelect{static_cast<std::size_t>(key - GLFW_KEY_1)});
        else if (key == GLFW_KEY_0) eventBus_->post(EventPresetSelect{9U});
        else if (key == GLFW_KEY_MINUS) eventBus_->post(EventPresetSelect{10U});
        else if (key == GLFW_KEY_EQUAL) eventBus_->post(EventPresetSelect{11U});
        else if (key == GLFW_KEY_LEFT_BRACKET) eventBus_->post(EventPresetSelect{12U});
        else if (key == GLFW_KEY_RIGHT_BRACKET) eventBus_->post(EventPresetSelect{13U});
        else if (key == GLFW_KEY_BACKSLASH) eventBus_->post(EventPresetSelect{14U});
        else if (key == GLFW_KEY_GRAVE_ACCENT) eventBus_->post(EventPresetSelect{15U});
        else if (shift && key == GLFW_KEY_RIGHT) eventBus_->post(EventMutation{EventMutation::Action::StepTourNext});
        else if (shift && key == GLFW_KEY_LEFT) eventBus_->post(EventMutation{EventMutation::Action::StepTourPrev});
        else if (control && key == GLFW_KEY_RIGHT) eventBus_->post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::NudgeCrossfade,
            .value = 0.05f
        });
        else if (control && key == GLFW_KEY_LEFT) eventBus_->post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::NudgeCrossfade,
            .value = -0.05f
        });
        else if (control && key == GLFW_KEY_D) eventBus_->post(EventPerformanceDeckControl{
            .action = EventPerformanceDeckControl::Action::SwapDecks
        });
        else if (key == GLFW_KEY_RIGHT) eventBus_->post(EventPresetCycle{1});
        else if (key == GLFW_KEY_LEFT) eventBus_->post(EventPresetCycle{-1});
        else if (alt && key == GLFW_KEY_T) eventBus_->post(EventMutation{EventMutation::Action::ToggleTour});
        else if (shift && key == GLFW_KEY_T) eventBus_->post(EventMutation{EventMutation::Action::ToggleTourPlaybackMode});
        else if (shift && key == GLFW_KEY_M) eventBus_->post(EventMutation{EventMutation::Action::ToggleTourPause});
        else if (control && key == GLFW_KEY_N) eventBus_->post(EventTourCycle{1});
        else if (control && key == GLFW_KEY_B) eventBus_->post(EventTourCycle{-1});
        else if (shift && key >= GLFW_KEY_F1 && key <= GLFW_KEY_F6) eventBus_->post(EventLearnStage{learn_mode::commandSoloStage(static_cast<learn_mode::Stage>(key - GLFW_KEY_F1))});
        else if (control && key >= GLFW_KEY_F1 && key <= GLFW_KEY_F6) eventBus_->post(EventLearnStage{learn_mode::commandAccumulativeStage(static_cast<learn_mode::Stage>(key - GLFW_KEY_F1))});
        else if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F6) eventBus_->post(EventLearnStage{learn_mode::bit(static_cast<learn_mode::Stage>(key - GLFW_KEY_F1))});
        else if (key == GLFW_KEY_F7) eventBus_->post(EventLearnStage{learn_mode::kResetStagesCommand});
        else if (key == GLFW_KEY_F8 || (control && key == GLFW_KEY_P)) eventBus_->post(EventFileAction{EventFileAction::Action::ExportGeometry, "sculpture_export"});
        else if (key == GLFW_KEY_X) eventBus_->post(EventRenderStrategyCycle{});
        else if (key == GLFW_KEY_F9) eventBus_->post(EventResolutionScaleCycle{});
        else if (key == GLFW_KEY_F10) eventBus_->post(EventLodGovernorToggle{});
        else if (key == GLFW_KEY_F11) eventBus_->post(EventAtmosphereProfileCycle{shift ? -1 : 1});
        // -------------------------------------------------------------------------
        // Undo & Redo History System (docu/project-vision.txt)
        // -------------------------------------------------------------------------
        else if (control && (key == GLFW_KEY_Z || key == GLFW_KEY_Y)) {
            if (key == GLFW_KEY_Y || shift) {
                eventBus_->post(EventHistoryControl{.action = EventHistoryControl::Action::Redo});
            } else {
                eventBus_->post(EventHistoryControl{.action = EventHistoryControl::Action::Undo});
            }
        }
        // -------------------------------------------------------------------------
        // 2D Vector Morph Space Navigation & Mode Cycling
        // -------------------------------------------------------------------------
        else if (control && alt && key == GLFW_KEY_F12) {
            eventBus_->post(EventMorphSpaceControl{
                .action = EventMorphSpaceControl::Action::SyncCategory
            });
        }
        else if (alt && key == GLFW_KEY_F12) {
            eventBus_->post(EventMorphSpaceControl{
                .action = EventMorphSpaceControl::Action::CycleBank
            });
        }
        else if (alt && key == GLFW_KEY_M) {
            eventBus_->post(EventMorphSpaceControl{
                .action = EventMorphSpaceControl::Action::CycleBank
            });
        }
        else if (control && shift && key == GLFW_KEY_F12) {
            eventBus_->post(EventMorphSpaceControl{
                .action = EventMorphSpaceControl::Action::TogglePath
            });
        }
        else if (shift && key == GLFW_KEY_F12) {
            eventBus_->post(EventMorphSpaceControl{
                .action = EventMorphSpaceControl::Action::CycleMotionMode
            });
        }
        else if (control && key == GLFW_KEY_F12) {
            eventBus_->post(EventMorphSpaceControl{
                .action = EventMorphSpaceControl::Action::ToggleActive
            });
        }
        else if (control && alt && key == GLFW_KEY_RIGHT) {
            eventBus_->post(EventMorphSpaceControl{
                .action = EventMorphSpaceControl::Action::NudgeCursor,
                .cursorX = 0.05f,
                .cursorY = 0.0f
            });
        }
        else if (control && alt && key == GLFW_KEY_LEFT) {
            eventBus_->post(EventMorphSpaceControl{
                .action = EventMorphSpaceControl::Action::NudgeCursor,
                .cursorX = -0.05f,
                .cursorY = 0.0f
            });
        }
        else if (control && alt && key == GLFW_KEY_UP) {
            eventBus_->post(EventMorphSpaceControl{
                .action = EventMorphSpaceControl::Action::NudgeCursor,
                .cursorX = 0.0f,
                .cursorY = 0.05f
            });
        }
        else if (control && alt && key == GLFW_KEY_DOWN) {
            eventBus_->post(EventMorphSpaceControl{
                .action = EventMorphSpaceControl::Action::NudgeCursor,
                .cursorX = 0.0f,
                .cursorY = -0.05f
            });
        }
        else if (key == GLFW_KEY_F12) eventBus_->post(EventGestureFilterReset{});
        else if (key == GLFW_KEY_U) eventBus_->post(EventLearnStage{learn_mode::kCycleDiagnosticCommand});
        else if (control && key == GLFW_KEY_R) eventBus_->post(EventToggleState{EventToggleState::Target::Temporal});
        else if (key == GLFW_KEY_R) eventBus_->post(EventTemporalReset{});
        else if (key == GLFW_KEY_Z) eventBus_->post(EventToggleState{EventToggleState::Target::RecordSequence});
        else if (key == GLFW_KEY_V) eventBus_->post(EventToggleState{EventToggleState::Target::VoxelMode});
        else if (shift && key == GLFW_KEY_Y) eventBus_->post(EventThematicTourControl{
            .action = EventThematicTourControl::Action::CycleCategory
        });
        else if (key == GLFW_KEY_Y) eventBus_->post(EventToggleState{EventToggleState::Target::StereoVR});
        else if (key == GLFW_KEY_E) eventBus_->post(EventLearnStage{learn_mode::kCycleDiagnosticCommand});
        else if (key == GLFW_KEY_H) eventBus_->post(EventReloadShaders{});
        else if (shift && key == GLFW_KEY_K) eventBus_->post(EventKnotBraid{1.0f, 0.0f, 0.0f, 3U});
        else if (control && key == GLFW_KEY_K) eventBus_->post(EventKnotBraid{0.0f, 1.0f, 0.0f, 3U});
        else if (key == GLFW_KEY_K) eventBus_->post(EventMorphEasingCycle{});
        else if (shift && key == GLFW_KEY_P) eventBus_->post(EventMultiTrackControl{EventMultiTrackControl::Action::TogglePlay});
        else if (shift && key == GLFW_KEY_G) eventBus_->post(EventMultiTrackControl{EventMultiTrackControl::Action::ToggleMute, 0});
        else if (shift && key == GLFW_KEY_D) eventBus_->post(EventMultiTrackControl{EventMultiTrackControl::Action::ToggleMute, 1});
        else if (shift && key == GLFW_KEY_C) eventBus_->post(EventMultiTrackControl{EventMultiTrackControl::Action::ToggleMute, 2});
        else if (shift && key == GLFW_KEY_A) eventBus_->post(EventMultiTrackControl{EventMultiTrackControl::Action::ToggleMute, 3});
        else if (key == GLFW_KEY_N) eventBus_->post(EventMutation{EventMutation::Action::StepMutation, 0.25f});
        else if (key == GLFW_KEY_B) eventBus_->post(EventMutation{EventMutation::Action::ResetBase});
        else if (key == GLFW_KEY_S) eventBus_->post(EventFileAction{EventFileAction::Action::SaveState, "user_state.epi"});
        else if (key == GLFW_KEY_L) eventBus_->post(EventFileAction{EventFileAction::Action::LoadState, "user_state.epi"});
        else if (shift && key == GLFW_KEY_O) eventBus_->post(EventFileAction{EventFileAction::Action::ExportStandaloneHtml, "sculpture_export.html"});
        else if (key == GLFW_KEY_O) eventBus_->post(EventFileAction{EventFileAction::Action::ExportGeometry, "sculpture_export"});
        // Spacebar: Instant Snapshot (PNG + Serialized Settings Bundle, docu/project-vision.txt:189)
        else if (key == GLFW_KEY_SPACE) eventBus_->post(EventSnapshotControl{EventSnapshotControl::Action::Request});

        // -------------------------------------------------------------------------
        // VJ Live Performance Deck & Numpad Control Surface
        // -------------------------------------------------------------------------
        // Numpad 0..9: Instant Performance Cue Banks (Shift: Store, Plain: Recall)
        else if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) {
            const std::size_t bank = static_cast<std::size_t>(key - GLFW_KEY_KP_0);
            if (shift) {
                eventBus_->post(EventPerformanceDeckControl{
                    .action = EventPerformanceDeckControl::Action::StoreCue,
                    .bankIndex = bank
                });
            } else {
                eventBus_->post(EventPerformanceDeckControl{
                    .action = EventPerformanceDeckControl::Action::RecallCueToOpposite,
                    .bankIndex = bank
                });
            }
        }
        // Numpad Divide (/): Momentary Performance FX (Plain: Tape Stop, Shift: Beat Roll, Ctrl: Chaos Burst)
        else if (key == GLFW_KEY_KP_DIVIDE) {
            if (shift) {
                eventBus_->post(EventPerformanceDeckControl{
                    .action = EventPerformanceDeckControl::Action::TriggerBeatRoll,
                    .value = 0.25f
                });
            } else if (control) {
                eventBus_->post(EventPerformanceDeckControl{
                    .action = EventPerformanceDeckControl::Action::TriggerChaosBurst,
                    .value = 1.0f
                });
            } else {
                eventBus_->post(EventPerformanceDeckControl{
                    .action = EventPerformanceDeckControl::Action::TriggerTapeStop,
                    .value = 1.2f
                });
            }
        }
        // Numpad Decimal (.): Master Isolator Filter (Plain: Reset 0.0, Shift: HP +0.6, Ctrl: LP -0.6)
        else if (key == GLFW_KEY_KP_DECIMAL) {
            const float filterVal = shift ? 0.6f : (control ? -0.6f : 0.0f);
            eventBus_->post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::SetMasterFilter,
                .value = filterVal
            });
        }
        // Numpad Multiply (*): Simulation Pause / Resume Toggle (docu/project-vision.txt:187)
        else if (key == GLFW_KEY_KP_MULTIPLY) {
            eventBus_->post(EventToggleState{EventToggleState::Target::Pause});
        }
        // Numpad Add (+): Adjust Simulation Speed (plain) or Nudge Musical BPM (with Shift)
        else if (key == GLFW_KEY_KP_ADD) {
            if (shift) {
                eventBus_->post(EventPerformanceDeckControl{
                    .action = EventPerformanceDeckControl::Action::NudgeBpm,
                    .value = 1.0f
                });
            } else {
                eventBus_->post(EventPlaybackRate{EventPlaybackRate::Action::Increase});
            }
        }
        // Numpad Subtract (-): Adjust Simulation Speed (plain) or Nudge Musical BPM (with Shift)
        else if (key == GLFW_KEY_KP_SUBTRACT) {
            if (shift) {
                eventBus_->post(EventPerformanceDeckControl{
                    .action = EventPerformanceDeckControl::Action::NudgeBpm,
                    .value = -1.0f
                });
            } else {
                eventBus_->post(EventPlaybackRate{EventPlaybackRate::Action::Decrease});
            }
        }
        // Ctrl+M: Toggle Musical Macro Automation Engine
        else if (control && key == GLFW_KEY_M) {
            eventBus_->post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::ToggleMacroEngine
            });
        }
        // Plain T: Tap-Tempo Beat Phase Synchronizer
        else if (key == GLFW_KEY_T && !shift && !control && !alt) {
            eventBus_->post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::TapTempo
            });
        }
        // Numpad Enter or Ctrl+Slash: Tap-Tempo Beat Phase Synchronizer
        else if (key == GLFW_KEY_KP_ENTER || (control && key == GLFW_KEY_SLASH)) {
            eventBus_->post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::TapTempo
            });
        }
        // Alt+X or Shift+B: Trigger Quantized 2-Bar Beat-Synchronized Crossfade (Wait Next Downbeat)
        else if ((alt && key == GLFW_KEY_X) || (shift && key == GLFW_KEY_B)) {
            eventBus_->post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::TriggerBeatSyncedCrossfade,
                .value = 2.0f
            });
        }
        // Ctrl+T: Toggle Autonomous Musical Bar-Synchronized Tour Progression
        else if (control && key == GLFW_KEY_T) {
            eventBus_->post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::ToggleTourBeatSync
            });
        }
        // Shift+X: Trigger 4-beat Musical Auto-Crossfade to Opposite Deck
        else if (shift && key == GLFW_KEY_X) {
            eventBus_->post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::TriggerAutoCrossfade,
                .value = 4.0f
            });
        }
        // Ctrl+X: Instant VJ Deck Swap (A <-> B)
        else if (control && key == GLFW_KEY_X) {
            eventBus_->post(EventPerformanceDeckControl{
                .action = EventPerformanceDeckControl::Action::SwapDecks
            });
        }

        // -------------------------------------------------------------------------
        // 4D SO(4) Rotor Hyperplane Interactive Navigation
        // Alt+Left/Right: Nudge Left-Isoclinic Rotor Alpha
        // Alt+Up/Down: Nudge Right-Isoclinic Rotor Beta
        // Alt+PageUp/PageDown: Nudge zw-Hyperplane Rotor Theta4D
        // -------------------------------------------------------------------------
        else if (alt && key == GLFW_KEY_LEFT) {
            eventBus_->post(EventRotorNudge{-0.05f, 0.0f, 0.0f});
        }
        else if (alt && key == GLFW_KEY_RIGHT) {
            eventBus_->post(EventRotorNudge{0.05f, 0.0f, 0.0f});
        }
        else if (alt && key == GLFW_KEY_UP) {
            eventBus_->post(EventRotorNudge{0.0f, 0.05f, 0.0f});
        }
        else if (alt && key == GLFW_KEY_DOWN) {
            eventBus_->post(EventRotorNudge{0.0f, -0.05f, 0.0f});
        }
        else if (alt && key == GLFW_KEY_PAGE_UP) {
            eventBus_->post(EventRotorNudge{0.0f, 0.0f, 0.05f});
        }
        else if (alt && key == GLFW_KEY_PAGE_DOWN) {
            eventBus_->post(EventRotorNudge{0.0f, 0.0f, -0.05f});
        }
    }

    void dispatchDroppedFiles(int count, const char** paths) {
        if (count <= 0 || paths == nullptr || paths[0] == nullptr) return;
        const std::string path = paths[0];
        // Capture certificates use the same bounded LoadState admission as CLI
        // loads; their image/history is descriptive, not a field checkpoint.
        if (path.ends_with(".epimode") || path.ends_with(".world")
            || path.ends_with(".epi") || path.ends_with(".manifest")) {
            eventBus_->post(EventFileAction{EventFileAction::Action::LoadState, path});
        } else if (path.ends_with(".episeq")) {
            eventBus_->post(EventFileAction{EventFileAction::Action::LoadSequence, path});
        }
    }

    static void keyCallback(
        GLFWwindow* window, int key, int scancode, int action, int mods) {
        const auto found = instances_.find(window);
        if (found == instances_.end()) return;
        SculptureCommandCenter* center = found->second;
        if (action == GLFW_PRESS) center->dispatchApplicationKey(key, mods);
        if (center->previousKeyCallback_
            && center->previousKeyCallback_ != &SculptureCommandCenter::keyCallback) {
            center->previousKeyCallback_(window, key, scancode, action, mods);
        }
    }

    static void dropCallback(GLFWwindow* window, int count, const char** paths) {
        const auto found = instances_.find(window);
        if (found == instances_.end()) return;
        SculptureCommandCenter* center = found->second;
        center->dispatchDroppedFiles(count, paths);
        if (center->previousDropCallback_
            && center->previousDropCallback_ != &SculptureCommandCenter::dropCallback) {
            center->previousDropCallback_(window, count, paths);
        }
    }

    GLFWwindow* window_ = nullptr;
    SculptureEventBus* eventBus_ = nullptr;
    GLFWkeyfun previousKeyCallback_ = nullptr;
    GLFWdropfun previousDropCallback_ = nullptr;
    double lastCursorX_ = 0.0;
    double lastCursorY_ = 0.0;

    inline static std::unordered_map<GLFWwindow*, SculptureCommandCenter*> instances_{};
};

} // namespace epi_sculpt
