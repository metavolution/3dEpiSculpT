#pragma once

#include "SculptureLiveCommand.h"
#include "SculptureProgram.h"
#include "mgfx/Camera.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

namespace epi_sculpt {

// Shared live-performance controls for the focused 3D demos.
//
// The 2D runtime's transport language is deliberately preserved:
//   T / Shift+T       walk a curated tour forward / backward
//   numpad *          pause / resume
//   numpad + / -      increase / decrease animation speed
//   numpad /          reset animation speed
//
// In three dimensions the continuous gestures become camera operations while
// arrows and brackets steer stable mathematical program coordinates. Input is
// polled so this layer does not take GLFW's window user pointer away from mGFX.
class SculptureLiveControls {
public:
    enum class SimulationClockPolicy {
        AdvanceWithInput,
        // beginFrame proposes a delta; the host commits via seekSimulationTime
        // only after its mathematical producer accepts that opportunity.
        HostCommitted,
    };

    explicit SculptureLiveControls(GLFWwindow* window, bool pointerOnly = false)
        : window_(window), previousScrollCallback_(
              glfwSetScrollCallback(window_, &SculptureLiveControls::scrollCallback)),
          pointerOnly_(pointerOnly) {
        scrollOwner_ = this;
        glfwGetCursorPos(window_, &lastCursorX_, &lastCursorY_);
        if (!pointerOnly_) printHelp();
    }

    SculptureLiveControls(const SculptureLiveControls&) = delete;
    SculptureLiveControls& operator=(const SculptureLiveControls&) = delete;

    ~SculptureLiveControls() {
        if (scrollOwner_ == this) {
            scrollOwner_ = nullptr;
            glfwSetScrollCallback(window_, previousScrollCallback_);
        }
    }

    void beginFrame(float realDeltaSeconds,
                    SimulationClockPolicy clockPolicy = SimulationClockPolicy::AdvanceWithInput) {
        const float dt = std::isfinite(realDeltaSeconds)
            ? std::clamp(realDeltaSeconds, 0.0f, 0.25f) : 0.0f;
        // Catalog hosts already own keyboard commands and simulation time.
        // Share mouse navigation without issuing a second set of commands.
        if (pointerOnly_) {
            if (pressedOnce(GLFW_KEY_HOME)) manualCameraActive_ = false;
            pollPointer();
            return;
        }

        constexpr std::array kSharedCommandKeys{
            GLFW_KEY_Q, GLFW_KEY_ESCAPE, GLFW_KEY_KP_MULTIPLY,
            GLFW_KEY_KP_ADD, GLFW_KEY_KP_SUBTRACT, GLFW_KEY_KP_DIVIDE,
            GLFW_KEY_EQUAL, GLFW_KEY_MINUS, GLFW_KEY_T, GLFW_KEY_SPACE,
            GLFW_KEY_P, GLFW_KEY_L, GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2,
            GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5, GLFW_KEY_6,
            GLFW_KEY_F1, GLFW_KEY_F2, GLFW_KEY_F3, GLFW_KEY_F4,
            GLFW_KEY_F5, GLFW_KEY_F6, GLFW_KEY_F7, GLFW_KEY_F8, GLFW_KEY_F9,
            GLFW_KEY_F10, GLFW_KEY_F11, GLFW_KEY_U};
        const int mods = currentModifiers();
        for (const int key : kSharedCommandKeys) {
            if (!pressedOnce(key)) continue;
            if (const auto command = sculptureLiveCommandFromKey(
                    key, mods, SculptureLiveInputProfile::FocusedDemo)) {
                dispatch(*command);
            }
        }

        if (pressedOnce(GLFW_KEY_H)) {
            expandedHelp_ = !expandedHelp_;
            if (expandedHelp_) printHelp();
        }
        if (pressedOnce(GLFW_KEY_HOME)) {
            manualCameraActive_ = false;
            std::fprintf(stderr, "[3D controls] restored authored camera\n");
        }
        if (pressedOnce(GLFW_KEY_BACKSPACE)) {
            manualCameraActive_ = false;
            latentWOffset_ = 0.0f;
            topologyBlendOffset_ = 0.0f;
            hueRotationOffset_ = 0.0f;
            stageMask_ = learn_mode::kAllStages;
            learnModeActive_ = false;
            resolutionScale_ = StageResolutionScale::Full;
            std::fprintf(stderr, "[3D controls] restored camera and parameter offsets\n");
        }

        const float parameterRate = dt * (fastModifier() ? 1.0f : 0.35f);
        latentWOffset_ += axis(GLFW_KEY_UP, GLFW_KEY_DOWN) * parameterRate;
        topologyBlendOffset_ += axis(GLFW_KEY_RIGHT, GLFW_KEY_LEFT) * parameterRate;
        hueRotationOffset_ += axis(GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_LEFT_BRACKET)
            * parameterRate * 0.5f;
        latentWOffset_ = std::clamp(latentWOffset_, -2.0f, 2.0f);
        topologyBlendOffset_ = std::clamp(topologyBlendOffset_, -1.0f, 1.0f);

        pollPointer();

        simulationDeltaSeconds_ = paused_ ? 0.0f : dt * timeScale_;
        if (clockPolicy == SimulationClockPolicy::AdvanceWithInput) {
            if (const auto next = proposeSimulationAdvance(simulationDeltaSeconds_)) {
                simulationTime_ = *next;
            } else {
                simulationDeltaSeconds_ = 0.0f;
            }
        }
    }

private:
    void pollPointer() {
        double cursorX = 0.0;
        double cursorY = 0.0;
        glfwGetCursorPos(window_, &cursorX, &cursorY);
        const float dx = static_cast<float>(cursorX - lastCursorX_);
        const float dy = static_cast<float>(cursorY - lastCursorY_);
        lastCursorX_ = cursorX;
        lastCursorY_ = cursorY;

        orbitDelta_ = {};
        panDelta_ = {};
        if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            orbitDelta_ = {dx, dy};
        }
        if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            panDelta_ = {dx, dy};
        }
        frameScroll_ = pendingScroll_;
        pendingScroll_ = 0.0f;

    }

public:

    // Remove the prior frame's control overlay before a demo recomputes its
    // authored state. This prevents persistent params from accumulating the
    // same operator offset every frame.
    void restoreProgramBaseline(SculptureProgramParams& params) noexcept {
        params.latentW -= appliedLatentWOffset_;
        params.topologyBlend -= appliedTopologyBlendOffset_;
        params.hueRotation -= appliedHueRotationOffset_;
        params.stageMask = baselineStageMask_;
        params.auxiliaryResolutionScale = baselineResolutionScale_;
        appliedLatentWOffset_ = 0.0f;
        appliedTopologyBlendOffset_ = 0.0f;
        appliedHueRotationOffset_ = 0.0f;
    }

    void applyProgramOffsets(SculptureProgramParams& params) noexcept {
        baselineStageMask_ = params.stageMask;
        baselineResolutionScale_ = params.auxiliaryResolutionScale;
        const float baseLatentW = params.latentW;
        const float baseTopologyBlend = params.topologyBlend;
        params.latentW = std::clamp(baseLatentW + latentWOffset_, -4.0f, 4.0f);
        params.topologyBlend = std::clamp(
            baseTopologyBlend + topologyBlendOffset_, 0.0f, 1.0f);
        params.hueRotation += hueRotationOffset_;
        appliedLatentWOffset_ = params.latentW - baseLatentW;
        appliedTopologyBlendOffset_ = params.topologyBlend - baseTopologyBlend;
        appliedHueRotationOffset_ = hueRotationOffset_;
        if (learnModeActive_ || stageMask_ != learn_mode::kAllStages) {
            params.stageMask = stageMask_;
        }
        if (resolutionScale_ != StageResolutionScale::Full) {
            params.auxiliaryResolutionScale = resolutionScale_;
        }
    }

    [[nodiscard]] mgfx::Camera applyCamera(
        const mgfx::Camera& authoredCamera, float realDeltaSeconds) {
        const bool keyboardMove = !pointerOnly_ && (axis(GLFW_KEY_W, GLFW_KEY_S) != 0.0f
            || axis(GLFW_KEY_D, GLFW_KEY_A) != 0.0f
            || axis(GLFW_KEY_E, GLFW_KEY_C) != 0.0f);
        const bool pointerMove = glm::length2(orbitDelta_) > 0.0f
            || glm::length2(panDelta_) > 0.0f || frameScroll_ != 0.0f;

        if (!manualCameraActive_ && (keyboardMove || pointerMove)) {
            manualCamera_ = authoredCamera;
            manualCameraActive_ = true;
            std::fprintf(stderr, "[3D controls] manual camera engaged (Home restores authored view)\n");
        }
        if (!manualCameraActive_) return authoredCamera;

        const float dt = std::clamp(realDeltaSeconds, 0.0f, 0.1f);
        mgfx::math::vec3 view = manualCamera_.target - manualCamera_.eye;
        float distance = std::max(glm::length(view), 0.05f);
        mgfx::math::vec3 forward = view / distance;
        mgfx::math::vec3 right = safeNormalize(glm::cross(forward, manualCamera_.up),
                                                {1.0f, 0.0f, 0.0f});
        mgfx::math::vec3 cameraUp = safeNormalize(glm::cross(right, forward),
                                                   {0.0f, 1.0f, 0.0f});

        if (glm::length2(orbitDelta_) > 0.0f) {
            mgfx::math::vec3 offset = manualCamera_.eye - manualCamera_.target;
            const auto yaw = glm::angleAxis(-orbitDelta_.x * 0.0045f,
                                            mgfx::math::vec3{0.0f, 1.0f, 0.0f});
            offset = yaw * offset;
            right = safeNormalize(glm::cross(-safeNormalize(offset, -forward),
                                              mgfx::math::vec3{0.0f, 1.0f, 0.0f}), right);
            const auto pitch = glm::angleAxis(-orbitDelta_.y * 0.0045f, right);
            const mgfx::math::vec3 pitched = pitch * offset;
            const float pole = std::abs(glm::dot(safeNormalize(pitched, offset),
                                                  mgfx::math::vec3{0.0f, 1.0f, 0.0f}));
            if (pole < 0.985f) offset = pitched;
            manualCamera_.eye = manualCamera_.target + offset;
        }

        view = manualCamera_.target - manualCamera_.eye;
        distance = std::max(glm::length(view), 0.05f);
        forward = view / distance;
        right = safeNormalize(glm::cross(forward, manualCamera_.up), right);
        cameraUp = safeNormalize(glm::cross(right, forward), cameraUp);

        if (glm::length2(panDelta_) > 0.0f) {
            const float panScale = std::max(distance, 0.5f) * 0.0015f;
            const mgfx::math::vec3 pan = (-right * panDelta_.x + cameraUp * panDelta_.y)
                * panScale;
            manualCamera_.eye += pan;
            manualCamera_.target += pan;
        }

        if (frameScroll_ != 0.0f) {
            const float newDistance = std::clamp(
                distance * std::exp(-frameScroll_ * 0.13f), 0.08f, 250.0f);
            manualCamera_.eye = manualCamera_.target - forward * newDistance;
            distance = newDistance;
        }

        const float movement = (pointerOnly_ ? 0.0f : std::max(distance * 0.7f, 0.8f)) * dt
            * (fastModifier() ? 4.0f : 1.0f);
        const mgfx::math::vec3 translation =
            forward * axis(GLFW_KEY_W, GLFW_KEY_S) * movement
            + right * axis(GLFW_KEY_D, GLFW_KEY_A) * movement
            + cameraUp * axis(GLFW_KEY_E, GLFW_KEY_C) * movement;
        manualCamera_.eye += translation;
        manualCamera_.target += translation;
        manualCamera_.up = cameraUp;
        return manualCamera_;
    }

    [[nodiscard]] static mgfx::Camera composeBasicTourCamera(
        const mgfx::Camera& base, int tourIndex) noexcept {
        constexpr int kTourViews = 8;
        const int wrapped = ((tourIndex % kTourViews) + kTourViews) % kTourViews;
        if (wrapped == 0) return base;

        mgfx::Camera camera = base;
        const mgfx::math::vec3 offset = base.eye - base.target;
        const float yaw = static_cast<float>(wrapped) * mgfx::math::kTwoPi
            / static_cast<float>(kTourViews);
        const auto rotation = glm::angleAxis(yaw, mgfx::math::vec3{0.0f, 1.0f, 0.0f});
        camera.eye = camera.target + rotation * offset;
        return camera;
    }

    [[nodiscard]] int consumeTourStep() noexcept {
        const int step = pendingTourStep_;
        pendingTourStep_ = 0;
        return step;
    }

    [[nodiscard]] bool consumePresentationToggle() noexcept {
        return consume(pendingPresentationToggle_);
    }

    [[nodiscard]] bool consumeCapture() noexcept { return consume(pendingCapture_); }

    [[nodiscard]] float simulationTime() const noexcept { return static_cast<float>(simulationTime_); }
    // Seek the mathematical clock without manufacturing an evolution step.
    bool seekSimulationTime(float time) noexcept {
        if (!std::isfinite(time)) return false;
        simulationTime_ = time;
        simulationDeltaSeconds_ = 0.0f;
        return true;
    }
    // Host-committed clocks prepare without mutation; the producer's actual
    // acceptance is the boundary at which the caller commits this value.
    [[nodiscard]] std::optional<double> proposeSimulationAdvance(float delta) const noexcept {
        if (!std::isfinite(delta) || delta < 0.0f) return std::nullopt;
        const double next = simulationTime_ + static_cast<double>(delta);
        constexpr double limit = std::numeric_limits<float>::max();
        if (!std::isfinite(next) || next < -limit || next > limit) return std::nullopt;
        return next;
    }
    bool commitSimulationTime(double time) noexcept {
        constexpr double limit = std::numeric_limits<float>::max();
        if (!std::isfinite(time) || time < -limit || time > limit) return false;
        simulationTime_ = time;
        simulationDeltaSeconds_ = 0.0f;
        return true;
    }
    [[nodiscard]] float simulationDeltaSeconds() const noexcept {
        return simulationDeltaSeconds_;
    }
    [[nodiscard]] float timeScale() const noexcept { return timeScale_; }
    void setTimeScale(float scale) noexcept {
        if (std::isfinite(scale))
            timeScale_ = std::clamp(scale, kLivePlaybackRateMinimum, kLivePlaybackRateMaximum);
    }
    void increasePlaybackRate() noexcept {
        timeScale_ = std::min(kLivePlaybackRateMaximum, timeScale_ * kLivePlaybackRateStep);
    }
    void decreasePlaybackRate() noexcept {
        timeScale_ = std::max(kLivePlaybackRateMinimum, timeScale_ / kLivePlaybackRateStep);
    }
    void resetPlaybackRate() noexcept {
        timeScale_ = 1.0f;
    }
    [[nodiscard]] bool paused() const noexcept { return paused_; }
    void setPaused(bool paused) noexcept { paused_ = paused; }
    void togglePause() noexcept { paused_ = !paused_; }
    [[nodiscard]] bool manualCameraActive() const noexcept { return manualCameraActive_; }

    [[nodiscard]] bool learnModeActive() const noexcept { return learnModeActive_; }
    [[nodiscard]] std::uint32_t stageMask() const noexcept { return stageMask_; }
    [[nodiscard]] StageResolutionScale resolutionScale() const noexcept { return resolutionScale_; }
    [[nodiscard]] bool lodGovernorActive() const noexcept { return lodGovernorActive_; }

    [[nodiscard]] std::string hud(std::string_view pipeline,
                                  std::string_view cameraName) const {
        char status[192];
        std::snprintf(status, sizeof(status),
                      "  |  CAM %s:%.*s  |  %s%.2fx",
                      manualCameraActive_ ? "MANUAL" : "TOUR",
                      static_cast<int>(cameraName.size()), cameraName.data(),
                      paused_ ? "PAUSED  |  " : "", timeScale_);
        std::string result(pipeline);
        result += status;
        if (resolutionScale_ != StageResolutionScale::Full) {
            char resStatus[64];
            std::snprintf(resStatus, sizeof(resStatus), "  |  RES %s", name(resolutionScale_).data());
            result += resStatus;
        }
        if (learnModeActive_ || (stageMask_ & learn_mode::kAllStages) != learn_mode::kAllStages) {
            char learnStatus[192];
            const auto diag = learn_mode::diagnostic(stageMask_);
            std::snprintf(learnStatus, sizeof(learnStatus),
                          "\nLEARN: 1:Xform(%c) 2:Reduce(%c) 3:Smpl(%c) 4:Color(%c) 5:Seed(%c) 6:Blend(%c) | Diag: %s",
                          learn_mode::enabled(stageMask_, learn_mode::Stage::Transform) ? '+' : '-',
                          learn_mode::enabled(stageMask_, learn_mode::Stage::Reduce) ? '+' : '-',
                          learn_mode::enabled(stageMask_, learn_mode::Stage::FeedbackSample) ? '+' : '-',
                          learn_mode::enabled(stageMask_, learn_mode::Stage::Color) ? '+' : '-',
                          learn_mode::enabled(stageMask_, learn_mode::Stage::Seed) ? '+' : '-',
                          learn_mode::enabled(stageMask_, learn_mode::Stage::Blend) ? '+' : '-',
                          learn_mode::diagnosticDescription(diag));
            result += learnStatus;
        }
        result += "\nT/Shift+T tour  |  LMB orbit  RMB pan  wheel dolly  |  WASD + E/C move  |  H help  |  Q quit";
        if (expandedHelp_) {
            result += "\nArrows latent-W/topology  |  [/] hue  |  numpad */+/- transport  |  numpad / speed reset";
            result += "\nP render path (advanced)  |  L learn mode  |  1..6/F1..F6 stages  |  0/U diagnostic  |  Shift+L/F7 reset stages";
            result += "\nF9 resolution scale  |  F10 LOD governor  |  Space capture  |  Home camera reset  |  Backspace reset all offsets";
        }
        return result;
    }

    static void printHelp() {
        std::fprintf(stderr,
            "\n3D live controls:\n"
            "  T / Shift+T       next / previous authored camera-tour view\n"
            "  Left drag          orbit around target\n"
            "  Right drag         pan camera and target\n"
            "  Mouse wheel        dolly in / out\n"
            "  W/A/S/D + E/C      move forward/left/back/right + up/down\n"
            "  Shift              accelerate camera or parameter motion\n"
            "  Arrows              latent-W up/down; topology blend left/right\n"
            "  [ / ]               hue rotation\n"
            "  Numpad *            pause / resume animation\n"
            "  + / Numpad +        increase animation speed by 1.2x\n"
            "  - / Numpad -        decrease animation speed by 1.2x\n"
            "  Numpad /            reset animation speed to 1x\n"
            "  L / Shift+L        toggle educational learn mode / reset all stages to enabled\n"
            "  1..6 / F1..F6      isolate/toggle stages (Transform, Reduce, Sample, Color, Seed, Blend)\n"
            "  0 / U              cycle diagnostic view (Transform, Reduce, Seed, Material, Energy, Trap)\n"
            "  F8                 cycle geodesic morph easing (Smoothstep, Spring, Exp, Bezier, etc.)\n"
            "  F9                 cycle resolution scale (Full -> Half -> Quarter -> Eighth)\n"
            "  F10                toggle adaptive LOD governor\n"
            "  P                   switch render path (advanced demos)\n"
            "  Space               capture PNG + state\n"
            "  Home / Backspace    reset camera / reset camera and parameter offsets\n"
            "  H                    toggle expanded HUD help\n"
            "  Q / Escape           quit immediately\n\n");
    }

private:
    static bool consume(bool& flag) noexcept {
        const bool value = flag;
        flag = false;
        return value;
    }

    [[nodiscard]] bool keyDown(int key) const noexcept {
        return glfwGetKey(window_, key) == GLFW_PRESS;
    }

    [[nodiscard]] float axis(int positive, int negative) const noexcept {
        return (keyDown(positive) ? 1.0f : 0.0f)
            - (keyDown(negative) ? 1.0f : 0.0f);
    }

    [[nodiscard]] bool fastModifier() const noexcept {
        return keyDown(GLFW_KEY_LEFT_SHIFT) || keyDown(GLFW_KEY_RIGHT_SHIFT);
    }

    [[nodiscard]] int currentModifiers() const noexcept {
        int mods = 0;
        if (keyDown(GLFW_KEY_LEFT_SHIFT) || keyDown(GLFW_KEY_RIGHT_SHIFT)) {
            mods |= GLFW_MOD_SHIFT;
        }
        if (keyDown(GLFW_KEY_LEFT_CONTROL) || keyDown(GLFW_KEY_RIGHT_CONTROL)) {
            mods |= GLFW_MOD_CONTROL;
        }
        if (keyDown(GLFW_KEY_LEFT_ALT) || keyDown(GLFW_KEY_RIGHT_ALT)) {
            mods |= GLFW_MOD_ALT;
        }
        return mods;
    }

    void dispatch(const SculptureLiveCommand& command) {
        switch (command.type) {
        case SculptureLiveCommandType::Quit:
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
            break;
        case SculptureLiveCommandType::TogglePause:
            paused_ = !paused_;
            std::fprintf(stderr, "[3D controls] animation %s\n",
                         paused_ ? "paused" : "resumed");
            break;
        case SculptureLiveCommandType::IncreasePlaybackRate:
            timeScale_ = std::min(
                kLivePlaybackRateMaximum, timeScale_ * kLivePlaybackRateStep);
            printTimeScale();
            break;
        case SculptureLiveCommandType::DecreasePlaybackRate:
            timeScale_ = std::max(
                kLivePlaybackRateMinimum, timeScale_ / kLivePlaybackRateStep);
            printTimeScale();
            break;
        case SculptureLiveCommandType::ResetPlaybackRate:
            timeScale_ = 1.0f;
            printTimeScale();
            break;
        case SculptureLiveCommandType::StepCameraTour:
            pendingTourStep_ += command.direction;
            manualCameraActive_ = false;
            break;
        case SculptureLiveCommandType::Capture:
            pendingCapture_ = true;
            break;
        case SculptureLiveCommandType::CyclePresentation:
            pendingPresentationToggle_ = true;
            break;
        case SculptureLiveCommandType::ToggleLearnMode:
            learnModeActive_ = !learnModeActive_;
            std::fprintf(stderr, "[3D controls] educational learn mode %s\n",
                         learnModeActive_ ? "ACTIVE" : "OFF");
            break;
        case SculptureLiveCommandType::ToggleStage:
            stageMask_ ^= command.stageBit;
            std::fprintf(stderr, "[3D controls] stage mask: 0x%02X\n",
                         stageMask_ & learn_mode::kAllStages);
            break;
        case SculptureLiveCommandType::CycleDiagnostic:
            stageMask_ = learn_mode::cycleDiagnostic(stageMask_);
            std::fprintf(stderr, "[3D controls] diagnostic: %s\n",
                         learn_mode::diagnosticDescription(learn_mode::diagnostic(stageMask_)));
            break;
        case SculptureLiveCommandType::ResetStages:
            stageMask_ = learn_mode::withAllStages(stageMask_);
            std::fprintf(stderr, "[3D controls] all stages reset to enabled (0x%02X)\n",
                         stageMask_ & learn_mode::kAllStages);
            break;
        case SculptureLiveCommandType::CycleResolutionScale: {
            switch (resolutionScale_) {
            case StageResolutionScale::Full: resolutionScale_ = StageResolutionScale::Half; break;
            case StageResolutionScale::Half: resolutionScale_ = StageResolutionScale::Quarter; break;
            case StageResolutionScale::Quarter: resolutionScale_ = StageResolutionScale::Eighth; break;
            case StageResolutionScale::Eighth: resolutionScale_ = StageResolutionScale::Full; break;
            }
            std::fprintf(stderr, "[3D controls] resolution scale: %s\n", name(resolutionScale_).data());
            break;
        }
        case SculptureLiveCommandType::ToggleLodGovernor:
            lodGovernorActive_ = !lodGovernorActive_;
            std::fprintf(stderr, "[3D controls] LOD governor: %s\n",
                         lodGovernorActive_ ? "ACTIVE" : "OFF");
            break;
        case SculptureLiveCommandType::CycleMorphEasing:
            std::fprintf(stderr, "[3D controls] cycled morph easing profile\n");
            break;
        case SculptureLiveCommandType::ExportRelief:
        case SculptureLiveCommandType::CycleReliefProjection:
        case SculptureLiveCommandType::TriggerProfileSession:
            // Handled by the full application via EventBus
            break;
        }
    }

    bool pressedOnce(int key) noexcept {
        if (key < 0 || key > GLFW_KEY_LAST) return false;
        const bool down = keyDown(key);
        const bool edge = down && !keyWasDown_[static_cast<std::size_t>(key)];
        keyWasDown_[static_cast<std::size_t>(key)] = down;
        return edge;
    }

    [[nodiscard]] static mgfx::math::vec3 safeNormalize(
        const mgfx::math::vec3& value,
        const mgfx::math::vec3& fallback) noexcept {
        const float lengthSquared = glm::length2(value);
        return lengthSquared > mgfx::math::kEpsilonNormalize
            ? value * glm::inversesqrt(lengthSquared) : fallback;
    }

    static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
        auto* owner = scrollOwner_;
        if (owner && owner->window_ == window) {
            owner->pendingScroll_ += static_cast<float>(yOffset);
            if (owner->previousScrollCallback_
                && owner->previousScrollCallback_ != &SculptureLiveControls::scrollCallback) {
                owner->previousScrollCallback_(window, xOffset, yOffset);
            }
        }
    }

    void printTimeScale() const {
        std::fprintf(stderr, "[3D controls] time speed: %.3fx\n", timeScale_);
    }

    GLFWwindow* window_ = nullptr;
    GLFWscrollfun previousScrollCallback_ = nullptr;
    std::array<bool, GLFW_KEY_LAST + 1> keyWasDown_{};
    mgfx::Camera manualCamera_{};
    mgfx::math::vec2 orbitDelta_{0.0f};
    mgfx::math::vec2 panDelta_{0.0f};
    double lastCursorX_ = 0.0;
    double lastCursorY_ = 0.0;
    float pendingScroll_ = 0.0f;
    float frameScroll_ = 0.0f;
    double simulationTime_ = 0.0;
    float simulationDeltaSeconds_ = 0.0f;
    float timeScale_ = 1.0f;
    float latentWOffset_ = 0.0f;
    float topologyBlendOffset_ = 0.0f;
    float hueRotationOffset_ = 0.0f;
    float appliedLatentWOffset_ = 0.0f;
    float appliedTopologyBlendOffset_ = 0.0f;
    float appliedHueRotationOffset_ = 0.0f;
    std::uint32_t baselineStageMask_ = learn_mode::kAllStages;
    std::uint32_t stageMask_ = learn_mode::kAllStages;
    StageResolutionScale baselineResolutionScale_ = StageResolutionScale::Full;
    StageResolutionScale resolutionScale_ = StageResolutionScale::Full;
    int pendingTourStep_ = 0;
    bool paused_ = false;
    bool expandedHelp_ = false;
    bool manualCameraActive_ = false;
    bool pendingPresentationToggle_ = false;
    bool pendingCapture_ = false;
    bool learnModeActive_ = false;
    bool lodGovernorActive_ = true;
    bool pointerOnly_ = false;

    inline static SculptureLiveControls* scrollOwner_ = nullptr;
};

} // namespace epi_sculpt
