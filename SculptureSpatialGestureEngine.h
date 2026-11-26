#pragma once

#include "SculptureProgramSchema.h"
#include "SculptureSpatialLBVH.h"
#include "SculptureSpatialSonifier.h"
#include "SculptureSpatialGestureFilter.h"
#include <array>
#include <vector>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: 6-DOF SPATIAL GESTURE & TACTILE FIELD ENGINE
// -----------------------------------------------------------------------------
// Translates 6-Degree-of-Freedom spatial controller and hand poses into direct
// Lie algebra transformations, topological metric morphs, and localized vortex
// velocity injections on the implicit epimorphic world.
//
// 1. Conformal Dual-Hand Metric Pinch/Rotate:
//    - Distance ||P_L - P_R|| modulates scale magnitude r.
//    - Vector axis P_L -> P_R modulates complex phase rotation theta.
//    - Midpoint (P_L + P_R)/2 modulates 3D world translation offset.
//
// 2. Localized Fluid Vortex Injection:
//    - Computes rotational curl velocity v_vortex = omega x (p - center)
//      and injects it directly into the variable-rate fluid grid at hand position.
//
// 3. Haptic Resonance Pulse Mapping:
//    - Maps geometric audio features (roughness, spectral centroid) into
//      tactile vibration amplitude and frequency for controller haptic actuators.
// -----------------------------------------------------------------------------

struct HapticFeedbackPulse {
    float amplitude = 0.0f;       // [0.0, 1.0] vibration intensity
    float frequencyHz = 0.0f;     // [0.0, 500.0] resonant frequency
    float durationSeconds = 0.0f;
};

struct GestureKeyframe {
    float timestampSeconds = 0.0f;
    Pose6DOF leftHand{};
    Pose6DOF rightHand{};
    GestureType gesture = GestureType::Idle;
    float customParamA = 0.0f;
    float customParamB = 0.0f;
};

enum class TrajectoryPlaybackMode : std::uint8_t {
    Once = 0,
    Loop = 1,
    PingPong = 2
};

struct GestureTrajectory {
    std::string name = "LiveGestureTrack";
    std::vector<GestureKeyframe> keyframes;
    float totalDurationSeconds = 0.0f;
    TrajectoryPlaybackMode mode = TrajectoryPlaybackMode::Loop;
    float speed = 1.0f;
};

class SculptureSpatialGestureEngine {
public:
    SculptureSpatialGestureEngine() = default;

    // Update hand/head controller poses
    void setLeftHandPose(const Pose6DOF& pose) noexcept { leftHand_ = pose; }
    void setRightHandPose(const Pose6DOF& pose) noexcept { rightHand_ = pose; }
    void setHeadPose(const Pose6DOF& pose) noexcept { headPose_ = pose; }

    [[nodiscard]] const Pose6DOF& leftHand() const noexcept { return leftHand_; }
    [[nodiscard]] const Pose6DOF& rightHand() const noexcept { return rightHand_; }
    [[nodiscard]] const Pose6DOF& headPose() const noexcept { return headPose_; }
    [[nodiscard]] GestureType activeGesture() const noexcept { return activeGesture_; }

    // Evaluates gestures and mutates sculpture program parameters and/or fluid grid
    void processGestures(
        SculptureProgramParams& params,
        std::vector<float>& spatialBuffer,
        float deltaSeconds
    );

    // Evaluates 6-DOF hand motion velocity and directly steers 4D latent rotation coordinates
    void applySpatialGestureTo4DLatent(
        SculptureProgramParams& params,
        float deltaSeconds
    ) noexcept;

    // Calculates haptic feedback pulse from geometric audio features
    [[nodiscard]] HapticFeedbackPulse computeHapticFeedback(
        const GeometricAudioFeatures& audioFeatures,
        bool isLeftHand
    ) const noexcept;

    // Telemetry getters for modulation matrix sources
    [[nodiscard]] float gestureScaleDelta() const noexcept { return gestureScaleDelta_; }
    [[nodiscard]] float gestureRotationDelta() const noexcept { return gestureRotationDelta_; }
    [[nodiscard]] float gestureVortexIntensity() const noexcept { return gestureVortexIntensity_; }

    // -------------------------------------------------------------------------
    // Spatial Trajectory Recording & Cubic Hermite Playback
    // -------------------------------------------------------------------------
    void startRecording(std::string_view trackName = "LiveGestureTrack") noexcept;
    void stopRecording() noexcept;
    void recordFrame(float deltaSeconds) noexcept;
    [[nodiscard]] bool isRecording() const noexcept { return isRecording_; }
    [[nodiscard]] const GestureTrajectory& recordedTrajectory() const noexcept { return recordedTrajectory_; }
    void setRecordedTrajectory(GestureTrajectory trajectory) noexcept;

    void startPlayback(TrajectoryPlaybackMode mode = TrajectoryPlaybackMode::Loop, float speed = 1.0f) noexcept;
    void stopPlayback() noexcept;
    void pausePlayback() noexcept { isPlaybackPaused_ = true; }
    void resumePlayback() noexcept { isPlaybackPaused_ = false; }
    [[nodiscard]] bool isPlaying() const noexcept { return isPlaying_ && !isPlaybackPaused_; }
    [[nodiscard]] float playbackTime() const noexcept { return playbackTime_; }
    [[nodiscard]] GestureKeyframe sampleTrajectory(float timeSeconds) const noexcept;
    void updatePlayback(float deltaSeconds) noexcept;

    // Cubic Hermite & Interpolation Primitives
    [[nodiscard]] static SculptureVec3 hermiteInterpolate(
        const SculptureVec3& p0, const SculptureVec3& m0,
        const SculptureVec3& p1, const SculptureVec3& m1,
        float t
    ) noexcept;

    [[nodiscard]] static Pose6DOF interpolatePose(
        const Pose6DOF& p0, const SculptureVec3& m0,
        const Pose6DOF& p1, const SculptureVec3& m1,
        float t
    ) noexcept;

    // 2nd-Order Critically Damped Spatial Filter Accessors
    [[nodiscard]] SculptureSpatialGestureFilter& gestureFilter() noexcept { return gestureFilter_; }
    [[nodiscard]] const SculptureSpatialGestureFilter& gestureFilter() const noexcept { return gestureFilter_; }
    [[nodiscard]] const FilteredSpatialHands& filteredHands() const noexcept { return filteredHands_; }

private:
    Pose6DOF leftHand_{};
    Pose6DOF rightHand_{};
    Pose6DOF headPose_{};
    Pose6DOF prevLeftHand_{};
    Pose6DOF prevRightHand_{};
    Pose6DOF prevHeadPose_{};

    // Biological Wavelet State (Respiration / Micro-Movement)
    float respirationPhase_ = 0.0f;
    float accumulatedHeadDelta_ = 0.0f;

    GestureType activeGesture_ = GestureType::Idle;
    float initialHandDistance_ = 0.0f;
    float initialHandAngle_ = 0.0f;
    bool gestureAnchorActive_ = false;

    float gestureScaleDelta_ = 0.0f;
    float gestureRotationDelta_ = 0.0f;
    float gestureVortexIntensity_ = 0.0f;

    SculptureSpatialGestureFilter gestureFilter_{};
    FilteredSpatialHands filteredHands_{};

    // Recording & Playback State
    bool isRecording_ = false;
    float recordingTimer_ = 0.0f;
    GestureTrajectory recordedTrajectory_{};

    bool isPlaying_ = false;
    bool isPlaybackPaused_ = false;
    float playbackTime_ = 0.0f;
    bool playbackPingPongForward_ = true;
};

} // namespace epi_sculpt
