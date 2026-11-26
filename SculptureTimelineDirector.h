#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>
#include <filesystem>

#include "SculptureWorldNavigator.h"
#include "ExperimentParameterEasing.h"
#include "mgfx/Camera.h"

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: DETERMINISTIC TIMELINE KEYFRAMING & MACRO CHOREOGRAPHY
// -----------------------------------------------------------------------------
// In accordance with the Project Vision (docu/project-vision.txt:148, 263-267):
//   "Path-based animation system for parameter evolution"
//   "Keyframe-based animation system & timeline-based editing"
//   "Procedural animation generators"
//   "Script-based programmatic control"
//
// SculptureTimelineDirector provides an enterprise-grade animation director:
//  1. Discrete Action Cues:
//     Fires structured NavigatorAction tokens at exact timestamps t_k (e.g. world
//     transitions, manifold switches, palette bursts, atmosphere toggles).
//  2. Continuous Parameter Tracks:
//     Interpolates continuous channels across Hermite, Smoothstep, and Cubic splines:
//     - 4D foliation coordinates: "latentW", "rotorAlpha", "rotorBeta"
//     - Spherical cinematic camera: "cameraDistance", "cameraAzimuth", "cameraElevation"
//     - Camera look-at targets: "cameraTargetX", "cameraTargetY", "cameraTargetZ"
//     - Macro kinetics: "speedMultiplier", "speed", "audioBpm"
//  3. Procedural Timeline Generators:
//     Synthesizes complete, mathematically synchronized scripts from musical tempo
//     (BPM) and phrase intervals, aligning visual world cuts, 4D waves, and orbital
//     camera moves with musical beats.
//  4. Full Bidirectional JSON Serialization:
//     Saves and loads versionable .timeline.json assets with millisecond performance.
// -----------------------------------------------------------------------------

struct TimelineActionCue {
    float timestampSeconds = 0.0f;
    NavigatorAction action = NavigatorAction::NextWorld;
    float value = 0.0f;
    std::string description{};
};

struct TimelineFloatKeyframe {
    float timeSeconds = 0.0f;
    float value = 0.0f;
    EasingType easing = EasingType::Smoothstep;
};

struct TimelineChannelTrack {
    std::string channelName{}; // e.g. "latentW", "rotorAlpha", "rotorBeta", "cameraDistance", "cameraAzimuth", "cameraElevation"
    std::vector<TimelineFloatKeyframe> keyframes{};

    [[nodiscard]] float evaluate(float t) const noexcept;
};

struct WorldTimelineScript {
    std::string id = "default_script";
    std::string title = "Untitled Timeline";
    std::string description{};
    float durationSeconds = 30.0f;
    bool loop = false;
    std::vector<TimelineActionCue> actionCues{};
    std::vector<TimelineChannelTrack> tracks{};

    void sortCues() noexcept;
};

struct TimelinePlayheadStatus {
    bool isPlaying = false;
    bool isPaused = false;
    float currentTime = 0.0f;
    float totalDuration = 0.0f;
    float progress = 0.0f; // [0.0, 1.0]
    std::size_t cuesExecuted = 0U;
    std::size_t totalCues = 0U;
    std::string activeScriptTitle{};
    std::string nextCueDescription{};
    float nextCueTimeRemaining = 0.0f;
    bool hasCameraOverride = false;
};

struct TimelineCameraState {
    bool active = false;
    float distance = 4.2f;
    float azimuth = 0.0f;      // Horizontal orbital angle (radians)
    float elevation = 0.0f;    // Vertical elevation angle (radians)
    float targetX = 0.0f;
    float targetY = 0.0f;
    float targetZ = 0.0f;
};

class SculptureTimelineDirector {
public:
    SculptureTimelineDirector() noexcept = default;
    explicit SculptureTimelineDirector(WorldTimelineScript script);

    // Timeline configuration & loading
    void loadScript(WorldTimelineScript script);
    bool loadScriptFromJson(std::string_view jsonStr);
    bool loadScriptFromFile(const std::filesystem::path& path);

    // Full bidirectional JSON serialization & disk export
    [[nodiscard]] std::string serializeToJson() const;
    static std::string serializeScriptToJson(const WorldTimelineScript& script);
    bool saveScriptToFile(const std::filesystem::path& path) const;
    static bool saveScriptToFile(const std::filesystem::path& path, const WorldTimelineScript& script);

    // Procedural animation and beat-synchronized script generators
    static WorldTimelineScript generateBeatSynchronizedScript(
        const std::vector<std::string>& presetNames,
        float bpm = 120.0f,
        std::uint32_t barsPerWorld = 8U,
        bool loop = true
    );

    static WorldTimelineScript generateOrbitalCinematicScript(
        std::string_view presetName,
        float durationSeconds = 30.0f,
        float baseRadius = 4.2f,
        bool loop = true
    );

    // Transport controls
    void play() noexcept;
    void pause() noexcept;
    void resume() noexcept;
    void stop() noexcept;
    void togglePlayPause() noexcept;
    void seek(float timeSeconds) noexcept;

    // Execution tick (advances playhead, dispatches cues, updates continuous tracks)
    void tick(SculptureWorldNavigator& navigator, float deltaSeconds);

    // Camera override inspection
    [[nodiscard]] bool hasCameraOverride() const noexcept { return cameraState_.active; }
    [[nodiscard]] const TimelineCameraState& cameraState() const noexcept { return cameraState_; }
    [[nodiscard]] mgfx::Camera computeCamera(float fovYRad = 0.959931f) const noexcept;

    // Inspection & status
    [[nodiscard]] bool isPlaying() const noexcept { return isPlaying_; }
    [[nodiscard]] bool isPaused() const noexcept { return isPaused_; }
    [[nodiscard]] float currentTime() const noexcept { return currentTime_; }
    [[nodiscard]] float totalDuration() const noexcept { return script_.durationSeconds; }
    [[nodiscard]] const WorldTimelineScript& script() const noexcept { return script_; }
    [[nodiscard]] TimelinePlayheadStatus status() const noexcept;
    [[nodiscard]] std::string formatHud() const;

private:
    void dispatchPendingCues(SculptureWorldNavigator& navigator, float prevTime, float currTime);
    void applyContinuousTracks(SculptureWorldNavigator& navigator, float time);

    WorldTimelineScript script_{};
    bool isPlaying_ = false;
    bool isPaused_ = false;
    float currentTime_ = 0.0f;
    std::size_t nextCueIndex_ = 0U;
    std::size_t cuesExecutedCount_ = 0U;
    TimelineCameraState cameraState_{};
};

} // namespace epi_sculpt
