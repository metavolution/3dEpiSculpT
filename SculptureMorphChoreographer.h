/**
 * ============================================================================
 * EPI2 - SCULPTURE MORPH CHOREOGRAPHER & GEODESIC HOMOTOPY ENGINE
 * ============================================================================
 *
 * Educational Annotation: Continuous Non-Linear Morphing & Geodesic Easing
 * -----------------------------------------------------------------------
 * In epimorphic fractal spaces, naive linear interpolation (lerp) between two
 * distinct presets causes manifold collapse, gimbal locking on 4D rotations,
 * and desaturation artifacts across the OKLCH color wheel.
 *
 * This module provides:
 * 1. Geodesic Shortest-Arc Polar Interpolation for OKLCH hue angles.
 * 2. Continuous Topological Homotopy blending between distinct inner maps:
 *      F_morph(p, lambda) = (1 - lambda) * F_A(p) + lambda * F_B(p)
 * 3. Multi-curve easing & spring physics solver for organic transitions.
 * 4. Automated Playlist / Preset Tour sequencer with customizable dwell times.
 *
 * ============================================================================
 */

#pragma once

#include "SculptureProgramSchema.h"
#include "SculpturePresets.h"

#include <cmath>
#include <filesystem>
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <cstdint>

namespace epi_sculpt {

enum class CatmullRomType : std::uint8_t {
    Centripetal = 0, // alpha = 0.5 (prevents cusps and self-intersections)
    Uniform = 1,     // alpha = 0.0 (standard cubic spline)
    Chordal = 2      // alpha = 1.0 (smoother on long spans)
};

struct MorphTarget {
    SculptureProgramParams params{};
    std::string name = "Target";
    // Empty for ad-hoc parameter-only targets. Library-built tours retain the
    // stable asset ID so the application can switch discrete world delivery
    // metadata without asking this interpolation class to own render state.
    std::string presetId;
    float durationSeconds = 3.0f;
    float dwellSeconds = 5.0f; // Dwell time before next transition in sequencer
    MorphEasing easing = MorphEasing::Smoothstep;
    MorphEnvelopeConfig envelope{};
    // Revision of the source that supplied these owned parameters. Never
    // refresh this from a later catalog without rematerializing the cue.
    std::string presetRevision;
};

class SculptureMorphChoreographer {
public:
    SculptureMorphChoreographer();
    explicit SculptureMorphChoreographer(const SculptureProgramParams& initialParams);

    // Dynamic Transition Control
    void setSource(const SculptureProgramParams& source) noexcept;
    void setTarget(const SculptureProgramParams& target, float durationSeconds = 3.0f, MorphEasing easing = MorphEasing::Smoothstep) noexcept;
    void setTarget(const SculptureProgramParams& target, float durationSeconds, const MorphEnvelopeConfig& envelope) noexcept;
    void setTargetPreset(const SculpturePreset& preset, float durationSeconds = 3.0f, MorphEasing easing = MorphEasing::Smoothstep) noexcept;
    void setTargetPreset(const SculpturePreset& preset, float durationSeconds, const MorphEnvelopeConfig& envelope) noexcept;

    // Reset immediately to target state with zero transition
    void snapToTarget() noexcept;

    // Advance morph then dwell from one time budget; enter at most one new cue.
    void update(float deltaSeconds) noexcept;

    // State Inspection
    [[nodiscard]] bool isActive() const noexcept { return active_; }
    [[nodiscard]] float progress() const noexcept { return progress_; }
    [[nodiscard]] float normalizedTime() const noexcept { return duration_ > 0.0f ? std::clamp(elapsed_ / duration_, 0.0f, 1.0f) : 1.0f; }
    [[nodiscard]] const SculptureProgramParams& currentParams() const noexcept { return currentParams_; }
    [[nodiscard]] const MorphEnvelopeConfig& envelopeConfig() const noexcept { return envelopeConfig_; }
    void setEnvelopeConfig(const MorphEnvelopeConfig& config) noexcept { envelopeConfig_ = config; easing_ = config.easing; }

    // Easing, Spring Dynamics & Geodesic Interpolation Primitives
    [[nodiscard]] static float evaluateEasing(float t, MorphEasing easing) noexcept;
    [[nodiscard]] static float evaluateSpring(float t, float zeta, float omega_n, float v0 = 0.0f) noexcept;
    [[nodiscard]] static float evaluateCubicBezier(float t, float p1x, float p1y, float p2x, float p2y) noexcept;
    [[nodiscard]] static float evaluateEnvelope(float t, const MorphEnvelopeConfig& config) noexcept;
    [[nodiscard]] static float shortestAngleDelta(float a, float b) noexcept;
    [[nodiscard]] static float interpolateHue(float hueA, float hueB, float t) noexcept;
    [[nodiscard]] static float shortestAngleDeltaRad(float a, float b) noexcept;
    [[nodiscard]] static float interpolateAngleRad(float angleA, float angleB, float t) noexcept;
    [[nodiscard]] static std::array<float, 4> quaternionSlerp(
        const std::array<float, 4>& qA,
        const std::array<float, 4>& qB,
        float t
    ) noexcept;
    static void interpolateRotorSO4(
        float alphaA, float betaA, float thetaA,
        float alphaB, float betaB, float thetaB,
        float t,
        float& outAlpha, float& outBeta, float& outTheta
    ) noexcept;
    [[nodiscard]] static ComplexAffine interpolateAffine(const ComplexAffine& a, const ComplexAffine& b, float t) noexcept;
    [[nodiscard]] static SculptureProgramParams interpolateParams(
        const SculptureProgramParams& a,
        const SculptureProgramParams& b,
        float t
    ) noexcept;

    // Multi-Keyframe Catmull-Rom Spline & SQUAD Evaluation
    [[nodiscard]] static float evaluateCatmullRomSpline(
        float p0, float p1, float p2, float p3,
        float t,
        CatmullRomType type = CatmullRomType::Centripetal
    ) noexcept;

    [[nodiscard]] static ComplexAffine interpolateAffineSpline(
        const ComplexAffine& a0, const ComplexAffine& a1,
        const ComplexAffine& a2, const ComplexAffine& a3,
        float t
    ) noexcept;

    [[nodiscard]] static std::array<float, 4> quaternionSquad(
        const std::array<float, 4>& q0,
        const std::array<float, 4>& q1,
        const std::array<float, 4>& q2,
        const std::array<float, 4>& q3,
        float t
    ) noexcept;

    static void interpolateRotorSO4Squad(
        const std::array<float, 3>& r0,
        const std::array<float, 3>& r1,
        const std::array<float, 3>& r2,
        const std::array<float, 3>& r3,
        float t,
        float& outAlpha, float& outBeta, float& outTheta
    ) noexcept;

    [[nodiscard]] static SculptureProgramParams interpolateParamsSpline(
        const SculptureProgramParams& p0,
        const SculptureProgramParams& p1,
        const SculptureProgramParams& p2,
        const SculptureProgramParams& p3,
        float t
    ) noexcept;

    // Playlist / Automated Preset Sequencer & Tour Choreography
    void clearPlaylist() noexcept;
    // Prepare targets off to the side; commit ownership without allocation.
    // Resets playlist runtime like clearPlaylist; keeps the current morph/source.
    void replacePlaylist(std::vector<MorphTarget> targets) noexcept;
    void addPlaylistPreset(const SculpturePreset& preset, float durationSeconds = 3.0f, float dwellSeconds = 6.0f, MorphEasing easing = MorphEasing::Smoothstep);
    void addPlaylistPreset(const SculpturePreset& preset, float durationSeconds, float dwellSeconds, const MorphEnvelopeConfig& envelope);
    void addPlaylistAsset(const struct SculpturePresetAsset& asset, float durationSeconds = 3.0f, float dwellSeconds = 6.0f, MorphEasing easing = MorphEasing::Smoothstep);
    void addPlaylistAsset(const struct SculpturePresetAsset& asset, float durationSeconds, float dwellSeconds, const MorphEnvelopeConfig& envelope);
    std::size_t addPlaylistDirectory(const std::filesystem::path& directoryPath, float durationSeconds = 3.0f, float dwellSeconds = 6.0f, MorphEasing easing = MorphEasing::Smoothstep);
    // Admit the populated timeline before changing clocks, flags or morph state.
    // Empty or unrepresentable spline starts reject; valid indices wrap.
    [[nodiscard]] bool tryStartPlaylistFrom(std::size_t index, bool loop = true) noexcept;
    // Compatibility entry points use the same admission and retain state on rejection.
    void startPlaylist(bool loop = true) noexcept;
    // Spline playback starts at the wrapped segment's time origin.
    void startPlaylistFrom(std::size_t index, bool loop = true) noexcept;
    // Settle an active tour at its visible value; retain cues for restart.
    // An inactive playlist does not cancel an unrelated standalone morph.
    void stopPlaylist() noexcept;
    // Explicit steps reject invalid spline timelines without changing playback.
    // Admitted steps preserve flags and rebase the selected spline segment.
    // Without a transition, settle source/current/target at the selected cue.
    void stepPlaylistNext(bool startTransition = true) noexcept;
    void stepPlaylistPrevious(bool startTransition = true) noexcept;
    void togglePlaylistPause() noexcept;
    void setPlaylistPaused(bool paused) noexcept;
    // Restore selection/flags; inactive captures keep their current parameters
    // without entering the authored cue's transition. Active restoration rejects
    // invalid spline timelines before changing state. This is not phase replay.
    void restorePlaylistRuntime(std::size_t index, bool loop, bool paused,
                                float dwellSeconds, bool active = true) noexcept;
    // Active mode changes restart this cue's destination phase, retaining flags.
    // Same-mode requests are no-ops; inactive morphs keep their current state.
    // Invalid modes or unrepresentable spline timelines reject before mutation.
    // An empty playlist may select policy before its targets are authored.
    [[nodiscard]] bool trySetTourPlaybackMode(TourPlaybackMode mode) noexcept;
    // Compatibility entry point; use the checked form when reporting admission.
    void setTourPlaybackMode(TourPlaybackMode mode) noexcept;
    [[nodiscard]] TourPlaybackMode tourPlaybackMode() const noexcept { return tourMode_; }
    [[nodiscard]] bool isPlaylistActive() const noexcept { return playlistActive_; }
    [[nodiscard]] bool isPlaylistPaused() const noexcept { return playlistPaused_; }
    [[nodiscard]] bool playlistLoops() const noexcept { return playlistLoop_; }
    [[nodiscard]] std::size_t playlistIndex() const noexcept { return playlistIndex_; }
    [[nodiscard]] std::size_t playlistSize() const noexcept { return playlist_.size(); }
    [[nodiscard]] const MorphTarget* playlistTarget(std::size_t index) const noexcept {
        return index < playlist_.size() ? &playlist_[index] : nullptr;
    }
    [[nodiscard]] const std::vector<MorphTarget>& playlist() const noexcept { return playlist_; }
    [[nodiscard]] std::string_view currentPlaylistTargetName() const noexcept;
    [[nodiscard]] float playlistDwellTimer() const noexcept { return playlistDwellTimer_; }
    [[nodiscard]] float playlistDwellProgress() const noexcept;
    [[nodiscard]] float splineTotalDuration() const noexcept;

private:
    // Empty timelines may select a mode before authoring; entry also requires a cue.
    [[nodiscard]] bool isPlaylistTimingValid(TourPlaybackMode mode) const noexcept;
    void enterPlaylistCue(std::size_t index, bool startTransition) noexcept;

    SculptureProgramParams sourceParams_{};
    SculptureProgramParams targetParams_{};
    SculptureProgramParams currentParams_{};

    float elapsed_ = 0.0f;
    float duration_ = 3.0f;
    float progress_ = 1.0f;
    MorphEasing easing_ = MorphEasing::Smoothstep;
    MorphEnvelopeConfig envelopeConfig_{};
    bool active_ = false;

    // Playlist sequencer state
    std::vector<MorphTarget> playlist_;
    std::size_t playlistIndex_ = 0;
    float playlistDwellTimer_ = 0.0f;
    float splineGlobalTime_ = 0.0f;
    TourPlaybackMode tourMode_ = TourPlaybackMode::StepAndDwell;
    bool playlistActive_ = false;
    bool playlistPaused_ = false;
    bool playlistLoop_ = true;
};

} // namespace epi_sculpt
