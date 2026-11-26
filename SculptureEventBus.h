#pragma once

#include <variant>
#include <vector>
#include <functional>
#include <string>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include "SculptureProgramSchema.h"
#include "SculptureSerializer.h"
#include "SculptureReliefContours.h"
#include "SculptureTimelineCommand.h"

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: DECOUPLED EVENT ARCHITECTURE
// -----------------------------------------------------------------------------
// This defines a strictly decoupled message-passing bus. By forcing all inputs
// (keyboard, mouse, file drops) to be expressed as Semantic Events rather than 
// direct mutations, we unlock powerful capabilities: 
// 1. We can attach a headless MIDI/OSC controller that simply posts events.
// 2. We can record a stream of events and replay them perfectly.
// 3. Components don't need pointer spaghetti to talk to each other.

struct EventPresetSelect {
    std::size_t index = 0;
    float durationSeconds = 3.0f;
};
struct EventPresetCycle { int offset; };
struct EventWorldCatalogSelect {
    std::size_t catalogIndex = 0U;
    float durationSeconds = 1.2f;
};
struct EventToggleState { 
    enum class Target { Pause, Cinematic, Temporal, Topology4D, VoxelMode, RecordSequence, StereoVR };
    Target target;
};
struct EventTemporalReset {};
struct EventLearnStage { std::uint32_t maskBit; };
struct EventPlaybackRate {
    enum class Action { Increase, Decrease, Reset };
    Action action;
};
struct EventFileAction {
    enum class Action { LoadState, SaveState, LoadSequence, ExportGeometry, ExportStandaloneHtml, ExportReliefMesh };
    Action action;
    std::string path;
};
struct EventCapture {};
struct EventSteering { float deltaLatentW; float deltaSimTime; };
struct EventMutation {
    enum class Action { ToggleTour, StepTourNext, StepTourPrev, ToggleTourPause, ToggleTourPlaybackMode, StepMutation, ResetBase, AdjustLatentW, Toggle4D, AdjustRotorAlpha, AdjustRotorBeta, AdjustRotorTheta, AdjustTopologyBlend, AdjustHue, AdjustSeedAlpha, SetLatentWOffset, SetRotorAlphaOffset, SetRotorBetaOffset, SetRotorThetaOffset, SetTopologyBlendOffset, SetHueOffset, SetSeedAlphaOffset };
    
    Action action;
    float strength = -1.0f;
};
struct EventReloadShaders {};
struct EventMorphEasingCycle {};
struct EventTourCycle { int direction = 1; };
struct EventCameraModeCycle { int offset = 1; };
struct EventSonificationModeCycle {};
struct EventSonificationVolume { float targetVolume; };
struct EventRenderStrategyCycle {};
struct EventResolutionScaleCycle {};
struct EventTriggerProfileSession {};
struct EventUpsampleFilterCycle {};
struct EventSpatialGesture {
    bool isLeftHand = false;
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    float quatW = 1.0f, quatX = 0.0f, quatY = 0.0f, quatZ = 0.0f;
    float trigger = 0.0f;
    float grip = 0.0f;
    bool valid = true;
};
struct EventStereoRigConfig {
    float ipd = 0.064f;
    float convergenceDist = 2.5f;
    bool toeIn = true;
};
struct EventFoliationManifold {
    std::uint8_t manifoldType = 0;
};
struct EventLodGovernorToggle {};
struct EventReliefProjectionCycle { int direction = 1; };
struct EventReliefProjectionSelect { int projectionMode = -1; }; // -1 for auto, 0..5 for specific ReliefManifoldProjection
struct EventReliefContourControl { ReliefContourAction action = ReliefContourAction::Toggle; };
struct EventAtmosphereProfileCycle { int direction = 1; };
struct EventGestureFilterReset {};
struct EventKnotBraid {
    float deltaP = 0.0f;
    float deltaQ = 0.0f;
    float deltaPhase = 0.0f;
    std::size_t numStrands = 3U;
};
struct EventRotorNudge {
    float deltaAlpha = 0.0f;
    float deltaBeta = 0.0f;
    float deltaTheta4D = 0.0f;
};
struct EventMultiTrackControl {
    enum class Action {
        TogglePlay,
        ToggleMute,
        ToggleSolo,
        ResetAll,
        SetLayerSpeed
    };
    Action action = Action::TogglePlay;
    std::uint8_t layerIndex = 0; // 0: Geometry, 1: Dynamics, 2: Material, 3: Atmosphere
    float speedMultiplier = 1.0f;
};

struct EventPerformanceDeckControl {
    enum class Action {
        SetCrossfadePosition,
        NudgeCrossfade,
        TapTempo,
        SetBpm,
        NudgeBpm,
        TriggerAutoCrossfade,
        TriggerBeatSyncedCrossfade,
        ToggleTourBeatSync,
        SwapDecks,
        StoreCue,
        RecallCue,
        RecallCueToOpposite,
        TriggerTapeStop,
        TriggerBeatRoll,
        TriggerChaosBurst,
        SetMasterFilter,
        ToggleMacroEngine,
        SetMacroMasterAmount
    };
    Action action = Action::NudgeCrossfade;
    float value = 0.0f;          // Position [0..1], Nudge delta, BPM, or duration/bar count
    std::size_t bankIndex = 0;   // Cue bank index [0..9]
    std::uint8_t targetDeck = 0; // 0: DeckA, 1: DeckB
};

struct EventMidiProfile {
    std::string profileName; // "dj", "launchpad", "korg", "apc", "keyboard"
};

struct EventSequenceRecorderControl {
    enum class Action {
        Start,
        Stop,
        Toggle
    };
    Action action = Action::Toggle;
    std::string sessionName{};
};

struct EventSnapshotControl {
    enum class Action {
        Request,
        ReloadLatest
    };
    Action action = Action::Request;
    std::string directory{"snapshots"};
};

struct EventPaletteAgentControl {
    enum class Action {
        Toggle,
        SetMode,
        Stimulate,
        Reseed
    };
    Action action = Action::Toggle;
    std::uint8_t mode = 0; // 0=Auto, 1=DynSync, 2=Harmonic, 3=Exp
    float intensity = 0.5f;
    std::uint32_t seed = 0;
};

struct EventPerformancePacingControl {
    enum class Action {
        ToggleMonitor,
        CycleTargetTier,
        SetTargetTier,
        ResetStats,
        SetResolutionScale,
        ToggleAutoResolution
    };
    Action action = Action::CycleTargetTier;
    std::uint8_t targetTier = 0; // 0=60, 1=120, 2=144, 3=240, 4=Unlimited
    // StageResolutionScale divisors, matching the actual consumer and telemetry.
    std::uint8_t resolutionScale = static_cast<std::uint8_t>(StageResolutionScale::Full);
};

struct EventSceneRenderScaleControl {
    enum class Action : std::uint8_t {
        Cycle,
        Set,
        ToggleAuto
    };
    Action action = Action::Cycle;
    float scale = 1.0f;
};

struct EventPerformanceMetrics {
    float fps = 0.0f;
    float frameTimeMs = 0.0f;
    float gpuTimeMs = 0.0f;
    float jitterMs = 0.0f;
    float low1PercentFps = 0.0f;
    std::uint32_t hitchCount = 0U;
    bool budgetExceeded = false;
};

struct EventParameterPatch {
    SculptureStateDiff diff{};
    std::string tag{"patch"};
};

struct EventHistoryControl {
    enum class Action : std::uint8_t {
        Undo,
        Redo,
        Clear
    };
    Action action = Action::Undo;
};

struct EventMorphSpaceControl {
    enum class Action : std::uint8_t {
        SetCursor,
        NudgeCursor,
        SetMotionMode,
        CycleMotionMode,
        SetMotionSpeed,
        SetShepardPower,
        ToggleActive,
        LoadBank,
        CycleBank,
        TogglePath,
        ClearPath,
        AddWaypoint,
        SyncCategory,
        BindCategory
    };
    Action action = Action::SetCursor;
    float cursorX = 0.5f;
    float cursorY = 0.5f;
    std::uint8_t mode = 0;
    float speed = 0.5f;
    float shepardPower = 2.0f;
    std::size_t bankIndex = 0;
    std::string bankName{};
    std::string category{};
    float dwellSec = 2.0f;
    float travelSec = 2.0f;
    bool loop = true;
};

struct EventAtmosphereControl {
    enum class Action : std::uint8_t {
        Toggle,
        Enable,
        Disable
    };
    Action action = Action::Toggle;
};

struct EventAudioFeedControl {
    enum class Action : std::uint8_t {
        CycleMode,
        SetMode,
        AdjustBpm,
        SetBpm,
        ToggleEnabled,
        SetGain
    };
    Action action = Action::CycleMode;
    std::uint8_t mode = 0;
    float value = 0.0f;
};

struct EventThematicTourControl {
    enum class Action : std::uint8_t {
        SelectTour,
        CycleTour,
        SelectCategory,
        CycleCategory,
        ToggleAutopilot
    };
    Action action = Action::CycleCategory;
    std::string identifier{};
    float dwellSeconds = 12.0f;
    float transitionSeconds = 3.5f;
};

struct EventVolumetricControl {
    enum class Action : std::uint8_t {
        ToggleFeedback,
        CycleResolution,
        SetResolution,
        ResetMemory,
        ToggleTorusWrap,
        ToggleWcaSeed,
        SetDecayFactor,
        ToggleAcousticCoupling,
        ToggleBackend
    };
    Action action = Action::CycleResolution;
    // StageResolutionScale divisors: 1=Full, 2=Half, 4=Quarter, 8=Eighth.
    std::uint8_t resolutionTier = static_cast<std::uint8_t>(StageResolutionScale::Half);
    float decayFactor = 0.94f;
};

struct EventVolumetricTelemetry {
    std::uint32_t activeDimension = 64U;
    std::size_t activeVoxelCount = 262144U;
    std::size_t bufferBytes = 8388608U;
    std::uint64_t stepCount = 0U;
    float simulationTime = 0.0f;
    float meanDensity = 0.0f;
    float peakEnergy = 0.0f;
    float macroOccupancyRatio = 0.0f;
    // Same divisor encoding as control and the actual volume descriptor.
    std::uint8_t resolutionTier = static_cast<std::uint8_t>(StageResolutionScale::Half);
    bool feedbackEnabled = true;
    bool gpuAccelerated = false;
    const char* backendName = "CPU-Oracle";
};

using SculptureEvent = std::variant<
    EventPresetSelect,
    EventPresetCycle,
    EventWorldCatalogSelect,
    EventToggleState,
    EventTemporalReset,
    EventLearnStage,
    EventPlaybackRate,
    EventFileAction,
    EventCapture,
    EventSteering,
    EventMutation,
    EventReloadShaders,
    EventMorphEasingCycle,
    EventTourCycle,
    EventCameraModeCycle,
    EventSonificationModeCycle,
    EventSonificationVolume,
    EventRenderStrategyCycle,
    EventResolutionScaleCycle,
    EventUpsampleFilterCycle,
    EventSpatialGesture,
    EventStereoRigConfig,
    EventFoliationManifold,
    EventLodGovernorToggle,
    EventAtmosphereProfileCycle,
    EventGestureFilterReset,
    EventTriggerProfileSession,
    EventKnotBraid,
    EventRotorNudge,
    EventMultiTrackControl,
    EventPerformanceDeckControl,
    EventMidiProfile,
    EventTimelineControl,
    EventSequenceRecorderControl,
    EventSnapshotControl,
    EventPaletteAgentControl,
    EventPerformancePacingControl,
    EventSceneRenderScaleControl,
    EventPerformanceMetrics,
    EventParameterPatch,
    EventAtmosphereControl,
    EventAudioFeedControl,
    EventThematicTourControl,
    EventVolumetricControl,
    EventVolumetricTelemetry,
    EventHistoryControl,
    EventMorphSpaceControl,
    EventReliefProjectionCycle,
    EventReliefProjectionSelect,
    EventReliefContourControl
>;

[[nodiscard]] constexpr std::string_view eventTypeName(const SculptureEvent& event) noexcept {
    return std::visit([](const auto& e) -> std::string_view {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, EventPresetSelect>) return "PresetSelect";
        else if constexpr (std::is_same_v<T, EventPresetCycle>) return "PresetCycle";
        else if constexpr (std::is_same_v<T, EventWorldCatalogSelect>) return "WorldCatalogSelect";
        else if constexpr (std::is_same_v<T, EventToggleState>) return "ToggleState";
        else if constexpr (std::is_same_v<T, EventTemporalReset>) return "TemporalReset";
        else if constexpr (std::is_same_v<T, EventLearnStage>) return "LearnStage";
        else if constexpr (std::is_same_v<T, EventPlaybackRate>) return "PlaybackRate";
        else if constexpr (std::is_same_v<T, EventFileAction>) return "FileAction";
        else if constexpr (std::is_same_v<T, EventCapture>) return "Capture";
        else if constexpr (std::is_same_v<T, EventSteering>) return "Steering";
        else if constexpr (std::is_same_v<T, EventMutation>) return "Mutation";
        else if constexpr (std::is_same_v<T, EventReloadShaders>) return "ReloadShaders";
        else if constexpr (std::is_same_v<T, EventMorphEasingCycle>) return "MorphEasingCycle";
        else if constexpr (std::is_same_v<T, EventTourCycle>) return "TourCycle";
        else if constexpr (std::is_same_v<T, EventCameraModeCycle>) return "CameraModeCycle";
        else if constexpr (std::is_same_v<T, EventSonificationModeCycle>) return "SonificationModeCycle";
        else if constexpr (std::is_same_v<T, EventSonificationVolume>) return "SonificationVolume";
        else if constexpr (std::is_same_v<T, EventRenderStrategyCycle>) return "RenderStrategyCycle";
        else if constexpr (std::is_same_v<T, EventResolutionScaleCycle>) return "ResolutionScaleCycle";
        else if constexpr (std::is_same_v<T, EventUpsampleFilterCycle>) return "UpsampleFilterCycle";
        else if constexpr (std::is_same_v<T, EventSpatialGesture>) return "SpatialGesture";
        else if constexpr (std::is_same_v<T, EventStereoRigConfig>) return "StereoRigConfig";
        else if constexpr (std::is_same_v<T, EventFoliationManifold>) return "FoliationManifold";
        else if constexpr (std::is_same_v<T, EventLodGovernorToggle>) return "LodGovernorToggle";
        else if constexpr (std::is_same_v<T, EventAtmosphereProfileCycle>) return "AtmosphereProfileCycle";
        else if constexpr (std::is_same_v<T, EventGestureFilterReset>) return "GestureFilterReset";
        else if constexpr (std::is_same_v<T, EventKnotBraid>) return "KnotBraid";
        else if constexpr (std::is_same_v<T, EventRotorNudge>) return "RotorNudge";
        else if constexpr (std::is_same_v<T, EventMultiTrackControl>) return "MultiTrackControl";
        else if constexpr (std::is_same_v<T, EventPerformanceDeckControl>) return "PerformanceDeckControl";
        else if constexpr (std::is_same_v<T, EventMidiProfile>) return "MidiProfile";
        else if constexpr (std::is_same_v<T, EventTimelineControl>) return "TimelineControl";
        else if constexpr (std::is_same_v<T, EventSequenceRecorderControl>) return "SequenceRecorderControl";
        else if constexpr (std::is_same_v<T, EventSnapshotControl>) return "SnapshotControl";
        else if constexpr (std::is_same_v<T, EventPaletteAgentControl>) return "PaletteAgentControl";
        else if constexpr (std::is_same_v<T, EventPerformancePacingControl>) return "PerformancePacingControl";
        else if constexpr (std::is_same_v<T, EventSceneRenderScaleControl>) return "SceneRenderScaleControl";
        else if constexpr (std::is_same_v<T, EventPerformanceMetrics>) return "PerformanceMetrics";
        else if constexpr (std::is_same_v<T, EventParameterPatch>) return "ParameterPatch";
        else if constexpr (std::is_same_v<T, EventAtmosphereControl>) return "AtmosphereControl";
        else if constexpr (std::is_same_v<T, EventAudioFeedControl>) return "AudioFeedControl";
        else if constexpr (std::is_same_v<T, EventThematicTourControl>) return "ThematicTourControl";
        else if constexpr (std::is_same_v<T, EventVolumetricControl>) return "VolumetricControl";
        else if constexpr (std::is_same_v<T, EventVolumetricTelemetry>) return "VolumetricTelemetry";
        else if constexpr (std::is_same_v<T, EventHistoryControl>) return "HistoryControl";
        else if constexpr (std::is_same_v<T, EventMorphSpaceControl>) return "MorphSpaceControl";
        else if constexpr (std::is_same_v<T, EventReliefProjectionCycle>) return "ReliefProjectionCycle";
        else if constexpr (std::is_same_v<T, EventReliefProjectionSelect>) return "ReliefProjectionSelect";
        else if constexpr (std::is_same_v<T, EventReliefContourControl>) return "ReliefContourControl";
        else return "UnknownEvent";
    }, event);
}

using SculptureEventListener = std::function<void(const SculptureEvent&)>;
using SculptureEventTraceListener = std::function<void(std::string_view, const SculptureEvent&)>;

class SculptureEventBus {
public:
    SculptureEventBus() noexcept
        : ownerThread_(std::this_thread::get_id()) {}

    void subscribe(SculptureEventListener listener) {
        listeners_.push_back(std::move(listener));
    }

    template <typename SpecificEvent, typename Handler>
    void subscribeTo(Handler&& handler) {
        listeners_.push_back([h = std::forward<Handler>(handler)](const SculptureEvent& event) {
            if (const auto* val = std::get_if<SpecificEvent>(&event)) {
                h(*val);
            }
        });
    }

    void setTraceListener(SculptureEventTraceListener traceListener) noexcept {
        traceListener_ = std::move(traceListener);
    }

    void clearListeners() {
        listeners_.clear();
        traceListener_ = nullptr;
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pending_.clear();
    }

    [[nodiscard]] std::size_t listenerCount() const noexcept {
        return listeners_.size();
    }

    [[nodiscard]] bool onOwnerThread() const noexcept {
        return std::this_thread::get_id() == ownerThread_;
    }

    [[nodiscard]] std::size_t pendingCount() const {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        return pending_.size();
    }

    // Main-thread producers retain the original immediate response semantics.
    // Worker-thread adapters (OSC/MIDI) enqueue instead of invoking application
    // listeners concurrently. The application drains that queue at its input
    // frame boundary, making semantic state single-writer by construction.
    void post(const SculptureEvent& event) {
        if (onOwnerThread()) {
            dispatchNow(event);
            return;
        }
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pending_.push_back(event);
    }

    // Drain exactly the events that were pending when the frame boundary was
    // entered. Events arriving while listeners execute remain queued for the
    // next frame, which gives remote/MIDI ingress deterministic frame ordering.
    [[nodiscard]] std::size_t drainPending() {
        if (!onOwnerThread()) return 0U;

        std::deque<SculptureEvent> batch;
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            batch.swap(pending_);
        }

        const std::size_t count = batch.size();
        for (const SculptureEvent& event : batch) {
            dispatchNow(event);
        }
        return count;
    }

    // Semantic alias for post.
    void publish(const SculptureEvent& event) {
        post(event);
    }

private:
    void dispatchNow(const SculptureEvent& event) {
        if (traceListener_) {
            traceListener_(eventTypeName(event), event);
        }
        for (auto& listener : listeners_) {
            listener(event);
        }
    }

    std::vector<SculptureEventListener> listeners_;
    SculptureEventTraceListener traceListener_{nullptr};
    const std::thread::id ownerThread_;
    mutable std::mutex pendingMutex_;
    std::deque<SculptureEvent> pending_;
};

} // namespace epi_sculpt
