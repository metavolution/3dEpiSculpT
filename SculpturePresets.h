// epiC-style named parameter bundles and deterministic morph policy.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include <string>
#include <string_view>

#include "SculptureMorphEasing.h"
#include "SculptureProgramSchema.h"
#include "SculptureModulator.h"

namespace epi_sculpt {

struct SculpturePreset {
    std::string_view name;
    std::string_view description;
    SculptureProgramParams params;
    std::vector<std::string> routingMap;
};

[[nodiscard]] std::span<const SculpturePreset> sculpturePresets() noexcept;

// Matches epiC VisualMode morph semantics: scalar/complex-affine values are
// continuous; integral and enum-valued program choices snap at the midpoint.
[[nodiscard]] SculptureProgramParams blendProgramParams(
    const SculptureProgramParams& source,
    const SculptureProgramParams& target,
    float amount) noexcept;

[[nodiscard]] SculptureWorldState blendWorldState(
    const SculptureWorldState& source,
    const SculptureWorldState& target,
    float amount) noexcept;

[[nodiscard]] SculptureProgramParams splineProgramParams(
    const SculptureProgramParams& p0,
    const SculptureProgramParams& p1,
    const SculptureProgramParams& p2,
    const SculptureProgramParams& p3,
    float t) noexcept;

[[nodiscard]] SculptureWorldState splineWorldState(
    const SculptureWorldState& s0,
    const SculptureWorldState& s1,
    const SculptureWorldState& s2,
    const SculptureWorldState& s3,
    float t) noexcept;

// Shared authored playback policy. Numeric values are persisted; keep them stable.
enum class TourPlaybackMode : std::uint8_t {
    StepAndDwell = 0,      // Discrete cue visits with authored dwell intervals
    ContinuousSpline = 1  // Continuous interpolation through the tour's targets
};

struct SpringDynamicsParams {
    float dampingRatio = 0.707f;    // zeta: < 1.0 underdamped, = 1.0 critical, > 1.0 overdamped
    float naturalFrequency = 6.0f;  // omega_n: angular natural frequency (rad/s)
    float initialVelocity = 0.0f;   // v0
};

struct CubicBezierParams {
    float p1x = 0.25f;
    float p1y = 0.10f;
    float p2x = 0.25f;
    float p2y = 1.00f;
};

struct MorphEnvelopeConfig {
    MorphEasing easing = MorphEasing::Smoothstep;
    SpringDynamicsParams spring{};
    CubicBezierParams bezier{};
    float overshootScale = 1.0f;
    float stageDelay = 0.0f; // Staggered delay for categorical stage snaps
};

// -----------------------------------------------------------------------------
// Educational Annotation: Path-Based Animation Sequencer
// Generalizes the A-to-B morphing into a full timeline of keyframes. This enables
// programmatic scripting of visual states (cinematic camera moves, evolving
// topologies) in alignment with the 'Path-based animation system' vision.
// -----------------------------------------------------------------------------
enum class KeyframeEasing : std::uint8_t {
    Linear = 0,
    Smoothstep = 1,
    CatmullRom = 2,
    CubicHermite = 3,
    Exponential = 4
};

struct SequenceKeyframe {
    SculptureProgramParams params{};
    SculptureWorldState state{};
    float durationSeconds = 3.0f;
    KeyframeEasing easing = KeyframeEasing::CatmullRom;
    std::vector<std::string> routingMap{};
};

class SculptureSequencer {
public:
    static constexpr float kMinimumKeyframeDuration = 0.001f;

    SculptureSequencer() = default;
    
    void addKeyframe(const SculptureProgramParams& params,
                     const SculptureWorldState& state,
                     float duration,
                     KeyframeEasing easing = KeyframeEasing::CatmullRom,
                     std::vector<std::string> routingMap = {}) noexcept;
    void addKeyframe(std::size_t presetIndex,
                     float duration,
                     KeyframeEasing easing = KeyframeEasing::CatmullRom) noexcept;
    void clear() noexcept;

    // Fresh stopped authoring candidate with this sequencer's loop policy.
    // Copies no keyframes, routing or cached samples; callers publish it only
    // after their own admission succeeds. The live sequencer stays untouched.
    [[nodiscard]] SculptureSequencer makeReplacementCandidate() const noexcept;

    // Configure wrapping even before keyframes exist. Does not move/sample the
    // cursor or change playback status; activation remains the caller's choice.
    void setLooping(bool loop) noexcept;
    void play(bool loop = true) noexcept;
    // Resume the selected phase with configured wrapping; does not rewind.
    void resume() noexcept;
    void pause() noexcept;
    // Select and sample immediately, including while paused, without changing
    // playback status. Nonfinite requests/totals leave the selected state intact.
    void seek(float timelineSeconds) noexcept;
    // Select a cue's start and sample immediately, preserving activation/loop
    // policy. Invalid indices or unrepresentable timelines leave state intact.
    [[nodiscard]] bool seekKeyframe(std::size_t index) noexcept;
    // Direction uses its sign. Looping wraps; nonlooping edges/zero reject.
    [[nodiscard]] bool stepKeyframe(int direction) noexcept;
    // While playing, advance by a finite nonnegative delta; zero refreshes the sample.
    // Each interval consumes its own duration; nonlooping playback holds the last
    // authored keyframe. Invalid deltas/time sums leave playback unchanged.
    void update(float deltaSeconds) noexcept;
    
    [[nodiscard]] const SculptureProgramParams& currentParams() const noexcept;
    [[nodiscard]] const SculptureWorldState& currentState() const noexcept;
    [[nodiscard]] const std::vector<std::string>& currentRoutingMap() const noexcept;
    [[nodiscard]] std::span<const SequenceKeyframe> keyframes() const noexcept {
        return keyframes_;
    }
    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] std::size_t keyframeCount() const noexcept;
    [[nodiscard]] std::size_t currentIndex() const noexcept;
    [[nodiscard]] float totalDuration() const noexcept;
    [[nodiscard]] float currentTimelineTime() const noexcept;

private:
    // Refresh params/state/routing without moving clocks or changing playback.
    void sampleCurrentPosition() noexcept;

    std::vector<SequenceKeyframe> keyframes_;
    SculptureProgramParams currentParams_{};
    SculptureWorldState currentState_{};
    std::vector<std::string> currentRoutingMap_{};
    std::size_t currentIndex_ = 0;
    float elapsedInCurrentFrame_ = 0.0f;
    float timelineTime_ = 0.0f;
    bool playing_ = false;
    bool loop_ = true;
};

// -----------------------------------------------------------------------------
// Educational Annotation: Modulator Engine (LFO)
// Generates continuous control signals (Sine, Triangle, Noise) to procedurally
// drive parameter fields over time, enabling living, breathing topologies.
// -----------------------------------------------------------------------------
enum class LFOShape : std::uint8_t {
    Sine,
    Triangle,
    Square
};

class SculptureLFO {
public:
    SculptureLFO(LFOShape shape, float frequency, float amplitude, float phaseOffset = 0.0f)
        : shape_(shape), freq_(frequency), amp_(amplitude), phase_(phaseOffset) {}

    float evaluate(float time) const noexcept;

private:
    LFOShape shape_;
    float freq_;
    float amp_;
    float phase_;
};

// -----------------------------------------------------------------------------
// Educational Annotation: Autonomous Guided Preset Tour
// -----------------------------------------------------------------------------
// Continuously journeys through the catalog of mathematical presets,
// applying smooth Hermite/Catmull-Rom morphing between visual states and
// adding generative parameter drift to avoid static repetition.
class [[deprecated("Use SculptureMorphChoreographer instead.")]] SculptureGuidedTour {
public:
    explicit SculptureGuidedTour(float dwellTimeSeconds = 6.0f, float morphDurationSeconds = 2.5f)
        : dwellTimeSeconds_(dwellTimeSeconds), morphDurationSeconds_(morphDurationSeconds),
          dwellRemaining_(dwellTimeSeconds) {}

    void start() noexcept;
    void stop() noexcept;
    void toggle() noexcept;
    void update(float deltaSeconds, class SculptureMorphChoreographer& morph) noexcept;

    [[nodiscard]] bool active() const noexcept { return active_; }
    [[nodiscard]] std::size_t currentTourIndex() const noexcept { return currentPresetIndex_; }
    [[nodiscard]] float dwellRemaining() const noexcept { return dwellRemaining_; }
    [[nodiscard]] SculptureMutator& mutator() noexcept { return mutator_; }
    [[nodiscard]] const SculptureMutator& mutator() const noexcept { return mutator_; }

private:
    bool active_ = false;
    float dwellTimeSeconds_ = 6.0f;
    float morphDurationSeconds_ = 2.5f;
    float dwellRemaining_ = 0.0f;
    std::size_t currentPresetIndex_ = 0;
    SculptureMutator mutator_{2026};
};

// -----------------------------------------------------------------------------
// Educational Annotation: N-Way Barycentric Simplex Parameter Blender
// -----------------------------------------------------------------------------
// Enables simultaneous multi-way interpolation across K distinct mathematical
// presets weighted by normalized barycentric coordinates (w_0, ..., w_{K-1})
// where sum(w_i) = 1.0 and w_i >= 0. Scalar fields are smoothly integrated,
// while discrete topological choices follow dominant simplex weights.
class SculptureBarycentricBlender {
public:
    struct WeightedPreset {
        SculptureProgramParams params;
        float weight = 0.0f;
    };

    [[nodiscard]] static SculptureProgramParams blend(
        std::span<const WeightedPreset> weightedPresets
    ) noexcept;
};

// -----------------------------------------------------------------------------
// Educational Annotation: Multi-Track Spatiotemporal Preset Choreographer
// -----------------------------------------------------------------------------
// Decouples visual generation into 4 orthogonal timeline tracks:
// 1. Geometry Track: Complex maps, fold reductions, and spatial scaling.
// 2. Dynamics Track: 4D hyper-coordinates, rotation speeds, and seed pulses.
// 3. Material Track: OKLCH color palettes, chroma breathing, and luma drift.
// 4. Atmosphere Track: Participating media fog density, anisotropy, and staging.
// Each track operates with independent playback speeds, keyframe counts, and looping.
enum class TrackLayer : std::uint8_t {
    Geometry = 0,
    Dynamics = 1,
    Material = 2,
    Atmosphere = 3
};

class SculptureMultiTrackChoreographer {
public:
    SculptureMultiTrackChoreographer();

    void setTrackKeyframes(TrackLayer layer, std::vector<SequenceKeyframe> keyframes) noexcept;
    void setTrackSpeed(TrackLayer layer, float speedMultiplier) noexcept;
    void setTrackLoop(TrackLayer layer, bool loop) noexcept;
    void setTrackMuted(TrackLayer layer, bool muted) noexcept;
    [[nodiscard]] bool isTrackMuted(TrackLayer layer) const noexcept;
    void setTrackSolo(TrackLayer layer, bool solo) noexcept;
    [[nodiscard]] bool isTrackSolo(TrackLayer layer) const noexcept;
    void clearSolos() noexcept;

    void setTrackLFO(TrackLayer layer, LFOShape shape, float freq, float amp, float phase = 0.0f) noexcept;
    void clearTrackLFO(TrackLayer layer) noexcept;

    void play() noexcept;
    void pause() noexcept;
    void update(float deltaSeconds) noexcept;
    void seek(float timelineSeconds) noexcept;

    [[nodiscard]] SculptureProgramParams composeProgramParams(
        const SculptureProgramParams& baseParams = {}
    ) const noexcept;

    [[nodiscard]] bool isPlaying() const noexcept { return playing_; }
    [[nodiscard]] float trackTime(TrackLayer layer) const noexcept;
    [[nodiscard]] bool isLayerActive(TrackLayer layer) const noexcept;
    [[nodiscard]] const SculptureSequencer& track(TrackLayer layer) const noexcept;
    [[nodiscard]] SculptureSequencer& track(TrackLayer layer) noexcept;

private:
    std::array<SculptureSequencer, 4> tracks_;
    std::array<float, 4> speeds_{1.0f, 1.0f, 1.0f, 1.0f};
    std::array<bool, 4> muted_{false, false, false, false};
    std::array<bool, 4> solo_{false, false, false, false};
    std::array<std::optional<SculptureLFO>, 4> lfos_{};
    bool playing_ = false;
    float globalTimelineTime_ = 0.0f;
};

} // namespace epi_sculpt
