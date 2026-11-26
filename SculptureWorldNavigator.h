#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <span>
#include <optional>
#include <memory>
#include <functional>
#include <filesystem>

#include "SculptureWorldRecipe.h"
#include "SculptureWorldFraming.h"
#include "SculptureWorldTourChoreographer.h"
#include "SculptureProgramSchema.h"
#include "SculpturePipelineCoordinator.h"
#include "SculptureFrameProducer.h"
#include "SculptureStateRecorder.h"
#include "SculptureWorldRenderBridge.h"
#include "SculptureWorldObserver.h"
#include "SculptureEventBus.h"
#include "SculptureSerializer.h"
#include "mgfx/Engine.h"
#include "mgfx/Camera.h"
#include "mgfx/Drawable.h"
#include "mgfx/pbr/Atmosphere.h"
#include "mgfx/pbr/Light.h"
#include "mgfx/vk/FrameLoop.h"
#include "EpimorphicReliefGenerator.h"
#include "SculpturePerformanceMonitor.h"
#include "SculptureMorphSpace.h"
#include "SculptureHudWidgets.h"
#include "Sculpture4DPolychoronHomotopy.h"
#include "SculptureVolumetricFieldCoupler.h"
#include "SculptureParameterComposer.h"
#include "SculptureWorldTransitionController.h"
#include "SculptureHybridManifoldSynthesizer.h"
#include "SculptureWorldTelemetryFormatter.h"

namespace epi_sculpt {

class SculptureFluidAtmosphereManager;
class SculptureAudioFeedDirector;
struct SculptureRay;
struct SculptureRayHit;

struct CuratedThematicTour {
    std::string id;
    std::string title;
    WorldTourMode mode = WorldTourMode::CuratedPlaylist;
    std::vector<std::string> presetIds{};
};

// -----------------------------------------------------------------------------
// Educational Annotation: UNIFIED COMMAND CENTER ACTION SYSTEM
// -----------------------------------------------------------------------------
// Architectural North Star (docu/project-vision.txt:103-111):
//   Input Events -> CmdCenter -> State -> OpenCL Engine -> OpenGL/Vulkan Renderer
//
// In interactive generative mathematics and multidimensional foliation engines,
// inputs arrive from diverse channels: keyboard strokes, mouse drags, MIDI CC knobs,
// OSC network datagrams, automated test suites, and autonomous AI director scripts.
//
// Having frontends branch on low-level key codes or mutate internal state through
// scattered ad-hoc callbacks violates architectural cohesion and prevents deterministic
// recording or headless replay.
//
// NavigatorAction reifies every user interaction and system transformation into a
// discrete, typed command token. When paired with dispatchAction(action, value),
// the entire simulation operates as a mathematically predictable state machine:
//   State_{t+1} = Transduce(State_t, NavigatorAction, value)
// -----------------------------------------------------------------------------
enum class NavigatorAction : std::uint8_t {
    NextWorld = 0,
    PrevWorld,
    ToggleTour,
    CycleTour,
    StepTourNext,
    StepTourPrev,
    Mutate,
    RandomizePalette,
    ResetParameters,
    CycleInnerMap,
    CycleReduction,
    CycleMirrorTopology,
    SetMirrorTopology,      // value parameter: MirrorCellTopology enum value [0, 3]
    ResetCamera,
    CyclePresentationMode,
    CycleReliefProjection,
    CycleResolutionScale,
    ToggleStagedGovernor,
    Toggle4DFoliationWave,
    Cycle4DManifold,
    CyclePolychoronSource,
    CyclePolychoronTarget,
    StepPolychoronMorph,
    CycleRotorSpeed,
    TogglePaletteAgent,
    CyclePaletteAgentMode,
    CycleLearnStage,
    ResetLearnMode,
    CycleDiagnostic,
    IncreaseSpeed,
    DecreaseSpeed,
    ResetSpeed,
    TogglePause,
    AdjustLatentW,          // value parameter: delta W (e.g. +0.05 / -0.05)
    AdjustRotorAlpha,       // value parameter: delta Alpha in radians
    AdjustRotorBeta,        // value parameter: delta Beta in radians
    Reset4DRotors,
    ToggleSequenceRecording,
    RequestSnapshot,
    ReloadLatestSnapshot,
    ToggleTimeline,
    TogglePerfMonitor,
    CycleTargetFps,
    ResetPerfStats,
    RevertToBaseline,
    ExportDiffPatch,
    ToggleAtmosphere,
    CycleAudioFeedMode,
    AdjustAudioBpm,
    ToggleAudioFeed,
    BuildBeatSyncedTimeline,
    ExportTimelineScript,
    TogglePerformanceDeck,
    CrossfadeDeck,
    TriggerAutoCrossfade,
    SwapPerformanceDecks,
    LoadDeckA,
    LoadDeckB,
    SetVariableResolutionScale,
    ToggleAutoResolutionScaling,
    PlayCategoryTour,
    CycleCategoryTour,
    CycleVolumetricResolution,
    ToggleVolumetricFeedback,
    ResetVolumetricMemory,
    ToggleVolumetricWrap,
    ToggleVolumetricBackend,
    ToggleVolumetricCoupling,
    ToggleVolumetricAdvection,
    ToggleAcousticSynesthesia,
    Undo,
    Redo,
    ToggleMorphSpace,
    CycleMorphSpaceMotion,
    SetMorphSpaceX,
    SetMorphSpaceY,
    NudgeMorphSpaceX,
    NudgeMorphSpaceY,
    CycleMorphSpaceBank,
    ToggleMorphSpacePath,
    SoloStage1,
    SoloStage2,
    SoloStage3,
    SoloStage4,
    SoloStage5,
    SoloStage6,
    ResetLearnStages,
    SoloStage,
    SetLearnAccumulative,
    SyncMorphSpaceCategory,
    BindMorphSpaceCategory,
    ReloadPresetsAndKernel,
    ToggleStereoSBS
};

[[nodiscard]] constexpr std::string_view actionName(NavigatorAction action) noexcept {
    switch (action) {
    case NavigatorAction::ReloadPresetsAndKernel: return "ReloadPresetsAndKernel";
    case NavigatorAction::NextWorld: return "NextWorld";
    case NavigatorAction::PrevWorld: return "PrevWorld";
    case NavigatorAction::ToggleTour: return "ToggleTour";
    case NavigatorAction::CycleTour: return "CycleTour";
    case NavigatorAction::StepTourNext: return "StepTourNext";
    case NavigatorAction::StepTourPrev: return "StepTourPrev";
    case NavigatorAction::Mutate: return "Mutate";
    case NavigatorAction::RandomizePalette: return "RandomizePalette";
    case NavigatorAction::ResetParameters: return "ResetParameters";
    case NavigatorAction::CycleInnerMap: return "CycleInnerMap";
    case NavigatorAction::CycleReduction: return "CycleReduction";
    case NavigatorAction::CycleMirrorTopology: return "CycleMirrorTopology";
    case NavigatorAction::SetMirrorTopology: return "SetMirrorTopology";
    case NavigatorAction::ResetCamera: return "ResetCamera";
    case NavigatorAction::CyclePresentationMode: return "CyclePresentationMode";
    case NavigatorAction::CycleReliefProjection: return "CycleReliefProjection";
    case NavigatorAction::CycleResolutionScale: return "CycleResolutionScale";
    case NavigatorAction::ToggleStagedGovernor: return "ToggleStagedGovernor";
    case NavigatorAction::Toggle4DFoliationWave: return "Toggle4DFoliationWave";
    case NavigatorAction::Cycle4DManifold: return "Cycle4DManifold";
    case NavigatorAction::CyclePolychoronSource: return "CyclePolychoronSource";
    case NavigatorAction::CyclePolychoronTarget: return "CyclePolychoronTarget";
    case NavigatorAction::StepPolychoronMorph: return "StepPolychoronMorph";
    case NavigatorAction::CycleRotorSpeed: return "CycleRotorSpeed";
    case NavigatorAction::TogglePaletteAgent: return "TogglePaletteAgent";
    case NavigatorAction::CyclePaletteAgentMode: return "CyclePaletteAgentMode";
    case NavigatorAction::CycleLearnStage: return "CycleLearnStage";
    case NavigatorAction::ResetLearnMode: return "ResetLearnMode";
    case NavigatorAction::CycleDiagnostic: return "CycleDiagnostic";
    case NavigatorAction::IncreaseSpeed: return "IncreaseSpeed";
    case NavigatorAction::DecreaseSpeed: return "DecreaseSpeed";
    case NavigatorAction::ResetSpeed: return "ResetSpeed";
    case NavigatorAction::TogglePause: return "TogglePause";
    case NavigatorAction::AdjustLatentW: return "AdjustLatentW";
    case NavigatorAction::AdjustRotorAlpha: return "AdjustRotorAlpha";
    case NavigatorAction::AdjustRotorBeta: return "AdjustRotorBeta";
    case NavigatorAction::Reset4DRotors: return "Reset4DRotors";
    case NavigatorAction::ToggleSequenceRecording: return "ToggleSequenceRecording";
    case NavigatorAction::RequestSnapshot: return "RequestSnapshot";
    case NavigatorAction::ReloadLatestSnapshot: return "ReloadLatestSnapshot";
    case NavigatorAction::ToggleTimeline: return "ToggleTimeline";
    case NavigatorAction::TogglePerfMonitor: return "TogglePerfMonitor";
    case NavigatorAction::CycleTargetFps: return "CycleTargetFps";
    case NavigatorAction::ResetPerfStats: return "ResetPerfStats";
    case NavigatorAction::RevertToBaseline: return "RevertToBaseline";
    case NavigatorAction::ExportDiffPatch: return "ExportDiffPatch";
    case NavigatorAction::ToggleAtmosphere: return "ToggleAtmosphere";
    case NavigatorAction::CycleAudioFeedMode: return "CycleAudioFeedMode";
    case NavigatorAction::AdjustAudioBpm: return "AdjustAudioBpm";
    case NavigatorAction::ToggleAudioFeed: return "ToggleAudioFeed";
    case NavigatorAction::BuildBeatSyncedTimeline: return "BuildBeatSyncedTimeline";
    case NavigatorAction::ExportTimelineScript: return "ExportTimelineScript";
    case NavigatorAction::TogglePerformanceDeck: return "TogglePerformanceDeck";
    case NavigatorAction::CrossfadeDeck: return "CrossfadeDeck";
    case NavigatorAction::TriggerAutoCrossfade: return "TriggerAutoCrossfade";
    case NavigatorAction::SwapPerformanceDecks: return "SwapPerformanceDecks";
    case NavigatorAction::LoadDeckA: return "LoadDeckA";
    case NavigatorAction::LoadDeckB: return "LoadDeckB";
    case NavigatorAction::SetVariableResolutionScale: return "SetVariableResolutionScale";
    case NavigatorAction::ToggleAutoResolutionScaling: return "ToggleAutoResolutionScaling";
    case NavigatorAction::PlayCategoryTour: return "PlayCategoryTour";
    case NavigatorAction::CycleCategoryTour: return "CycleCategoryTour";
    case NavigatorAction::CycleVolumetricResolution: return "CycleVolumetricResolution";
    case NavigatorAction::ToggleVolumetricFeedback: return "ToggleVolumetricFeedback";
    case NavigatorAction::ResetVolumetricMemory: return "ResetVolumetricMemory";
    case NavigatorAction::ToggleVolumetricWrap: return "ToggleVolumetricWrap";
    case NavigatorAction::ToggleVolumetricBackend: return "ToggleVolumetricBackend";
    case NavigatorAction::ToggleVolumetricCoupling: return "ToggleVolumetricCoupling";
    case NavigatorAction::ToggleVolumetricAdvection: return "ToggleVolumetricAdvection";
    case NavigatorAction::ToggleAcousticSynesthesia: return "ToggleAcousticSynesthesia";
    case NavigatorAction::Undo: return "Undo";
    case NavigatorAction::Redo: return "Redo";
    case NavigatorAction::ToggleMorphSpace: return "ToggleMorphSpace";
    case NavigatorAction::CycleMorphSpaceMotion: return "CycleMorphSpaceMotion";
    case NavigatorAction::SetMorphSpaceX: return "SetMorphSpaceX";
    case NavigatorAction::SetMorphSpaceY: return "SetMorphSpaceY";
    case NavigatorAction::NudgeMorphSpaceX: return "NudgeMorphSpaceX";
    case NavigatorAction::NudgeMorphSpaceY: return "NudgeMorphSpaceY";
    case NavigatorAction::CycleMorphSpaceBank: return "CycleMorphSpaceBank";
    case NavigatorAction::ToggleMorphSpacePath: return "ToggleMorphSpacePath";
    case NavigatorAction::SoloStage1: return "SoloStage1";
    case NavigatorAction::SoloStage2: return "SoloStage2";
    case NavigatorAction::SoloStage3: return "SoloStage3";
    case NavigatorAction::SoloStage4: return "SoloStage4";
    case NavigatorAction::SoloStage5: return "SoloStage5";
    case NavigatorAction::SoloStage6: return "SoloStage6";
    case NavigatorAction::ResetLearnStages: return "ResetLearnStages";
    case NavigatorAction::SoloStage: return "SoloStage";
    case NavigatorAction::SetLearnAccumulative: return "SetLearnAccumulative";
    case NavigatorAction::SyncMorphSpaceCategory: return "SyncMorphSpaceCategory";
    case NavigatorAction::BindMorphSpaceCategory: return "BindMorphSpaceCategory";
    case NavigatorAction::ToggleStereoSBS: return "ToggleStereoSBS";
    default: return "Unknown";
    }
}

// -----------------------------------------------------------------------------
// Educational Annotation: UNIFIED COMMAND CENTER ACTION COMMAND
// -----------------------------------------------------------------------------
// Represents a discrete or continuous state mutation command tokenized from
// platform input (GLFW, Touch, MIDI, OSC, Timeline, Script).
// -----------------------------------------------------------------------------
struct ActionCommand {
    NavigatorAction action = NavigatorAction::NextWorld;
    float value = 0.0f;
};

// -----------------------------------------------------------------------------
// Educational Annotation: UNIFIED WORLD FRAME EXECUTION PACKET
// -----------------------------------------------------------------------------
// Real-time world execution couples multiple temporal and geometric rates:
//  - CPU wall-clock cadence
//  - Asynchronous OpenCL GPU simulation production
//  - Dynamic orbital camera geodesics and smooth transitions
//  - Real-time telemetry tracking
//
// SculptureWorldFrame bundles the complete per-frame state output from
// updateFrame(), decoupling rendering loops from internal ticking mechanics.
// -----------------------------------------------------------------------------
struct SculptureWorldFrame {
    std::optional<SculpturePipelineFrame> pipelineFrame{};
    mgfx::Camera camera{};
    bool isNewGeneration = false;
    std::uint32_t generation = 0U;
    std::size_t instanceCount = 0U;
};

// -----------------------------------------------------------------------------
// Educational Annotation: DYNAMIC AUDIO-REACTIVE WORLD LIGHTING PACKET
// -----------------------------------------------------------------------------
// Standardized high-level lighting bundle unifying analytical atmospheric scattering
// and directional PBR key lights driven by acoustic resonance physics.
// -----------------------------------------------------------------------------
struct SculptureDynamicLighting {
    mgfx::pbr::Atmosphere atmosphere{};
    mgfx::pbr::LightSourceRegistry lights{};
};

// -----------------------------------------------------------------------------
// Educational Annotation: DEDICATED WORLD CATALOG NAVIGATOR & EXECUTION FACADE
// -----------------------------------------------------------------------------
// In the epiC architecture, dedicated procedural 3D worlds (spanned by 2.5D,
// 3.5D, and 4D mathematical projections) require coordinated orchestration across:
//  1. Catalog Discovery: Resolving authored preset identities & aliases.
//  2. PBR Material Palettes: Authoritative 3-band physical surface parameters.
//  3. Camera Framing: Autonomous orbital geodesics tailored to the geometry.
//  4. Closed-Loop OpenCL Execution: Asynchronous GPU simulation & telemetry.
//  5. Zero-Stall State Persistence: Spacebar snapshot bundles (.png, .world, .epimode).
//
// SculptureWorldNavigator encapsulates these responsibilities into a clean,
// single-responsibility controller, dramatically simplifying application and
// demo loops while preventing architectural drift.
// -----------------------------------------------------------------------------
class SculptureWorldNavigator {
public:
    explicit SculptureWorldNavigator(
        mgfx::Engine* engine,
        SculpturePipelineCoordinator& coordinator,
        const class SculpturePresetLibrary* presetLibrary = nullptr,
        std::string_view initialPresetName = "",
        std::filesystem::path presetDirectory = {}
    );

    SculptureWorldNavigator(
        mgfx::Engine& engine,
        SculpturePipelineCoordinator& coordinator,
        const class SculpturePresetLibrary* presetLibrary = nullptr,
        std::string_view initialPresetName = "",
        std::filesystem::path presetDirectory = {}
    ) : SculptureWorldNavigator(&engine, coordinator, presetLibrary, initialPresetName,
                                std::move(presetDirectory)) {}

    ~SculptureWorldNavigator();

    // Dynamic catalog querying & state
    [[nodiscard]] std::size_t activeIndex() const noexcept { return activeIndex_; }
    [[nodiscard]] std::size_t count() const noexcept { return availableWorlds_.size(); }
    [[nodiscard]] const WorldCatalogEntry& activeEntry() const noexcept { return *availableWorlds_[activeIndex_]; }
    [[nodiscard]] const WorldCatalogEntry* catalogEntryAt(std::size_t index) const noexcept {
        return index < availableWorlds_.size() ? availableWorlds_[index] : nullptr;
    }
    [[nodiscard]] const WorldRecipe& activeRecipe() const noexcept { return currentRecipe_; }
    [[nodiscard]] const std::string& activePresetName() const noexcept { return currentPresetName_; }
    [[nodiscard]] const std::string& activePresetRevision() const noexcept { return currentPresetRevision_; }
    [[nodiscard]] const SculptureProgramParams& activeParams() const noexcept { return currentParams_; }
    [[nodiscard]] const SculptureProgramParams& baselineParams() const noexcept { return baselineParams_; }
    [[nodiscard]] const std::vector<mgfx::MaterialHandle>& materialBands() const noexcept { return bands_; }

    // Navigation & smooth parameter morphing
    bool selectWorld(std::size_t index, float transitionSeconds = 0.0f);
    bool selectWorld(std::string_view presetId, float transitionSeconds = 0.0f);
    void nextWorld(float transitionSeconds = 0.0f);
    void prevWorld(float transitionSeconds = 0.0f);

    // Bubblesphere mirror-cell grammar is recipe state, not a renderer knob.
    // A change is consumed by the next cadence-approved OpenCL generation.
    [[nodiscard]] bool canConfigureMirrorTopology() const noexcept;
    bool setMirrorTopology(MirrorCellTopology topology) noexcept;
    bool cycleMirrorTopology() noexcept;

    // Inter-world transition inspection & control
    [[nodiscard]] bool isTransitioning() const noexcept { return transitionState_.active; }
    [[nodiscard]] float transitionProgress() const noexcept { return transitionState_.progress; }
    [[nodiscard]] const WorldTransitionState& transitionState() const noexcept { return transitionState_; }
    void cancelTransition() noexcept;

    // Framing & camera synchronization
    [[nodiscard]] SculptureWorldFrameHint activeFrameHint() const noexcept { return makeWorldFrameHint(currentRecipe_); }
    void resetCameraFraming() noexcept;
    [[nodiscard]] mgfx::Camera computeFrameCamera(float fovYRad = 0.959931f) const noexcept;

    // Fault-tolerant closed-loop execution tick (Cadence -> AutoDirector -> OpenCL GPU -> Telemetry)
    [[nodiscard]] std::optional<SculpturePipelineFrame> tick(
        SculptureFrameProducer& producer,
        float wallDeltaSeconds
    );

    // Unified synchronous frame update combining closed-loop simulation & camera positioning
    [[nodiscard]] SculptureWorldFrame updateFrame(
        SculptureFrameProducer& producer,
        float wallDeltaSeconds,
        float fovYRad = 0.959931f
    );
    [[nodiscard]] SculptureWorldFrame updateFrame(
        float wallDeltaSeconds,
        float fovYRad = 0.959931f
    );

    // -------------------------------------------------------------------------
    // Educational Annotation: UNIFIED FAULT-TOLERANT RENDER FRAME EXECUTION FLOW
    // -------------------------------------------------------------------------
    // Orchestrates the complete end-to-end rendering pipeline in a single call:
    //  1. Drains and dispatches pending Command Center action queue.
    //  2. Updates dynamic audio-reactive environmental atmosphere and PBR lights.
    //  3. Steps closed-loop simulation math, camera geodesics, and fluid substrates.
    //  4. Coordinates snapshot captures and continuous sequence video recording.
    //  5. Issues Vulkan engine runFrame with canonical model presentation matrix,
    //     multi-pass presentation (points / relief / mist), and telemetry HUD.
    // Provides complete exception shielding: if a transient GPU compilation or
    // allocation failure occurs, the frame catches the error, retains the last valid
    // generation, and prevents host application crash.
    // -------------------------------------------------------------------------
    SculptureWorldFrame renderFrame(
        mgfx::Engine& engine,
        mgfx::vk::FrameLoop& loop,
        SculptureFrameProducer& producer,
        float wallDeltaSeconds,
        float fovYRad = 0.959931f,
        const std::function<void(mgfx::Drawable&)>& secondaryDraw = nullptr
    );
    // Streamlined zero-boilerplate render overload executing coordinator-driven simulation without external producer
    SculptureWorldFrame renderFrame(
        mgfx::Engine& engine,
        mgfx::vk::FrameLoop& loop,
        float wallDeltaSeconds,
        float fovYRad = 0.959931f,
        const std::function<void(mgfx::Drawable&)>& secondaryDraw = nullptr
    );

    // Application-owned navigation is applied before capture and presentation.
    // Numerical evolution and the authored camera remain independent.
    void setCameraController(std::function<mgfx::Camera(const mgfx::Camera&, float)> controller) {
        cameraController_ = std::move(controller);
    }

    // Lossless atomic snapshot capture (PNG + .world + .epimode + .epi + manifest)
    void requestSnapshot() noexcept;
    [[nodiscard]] bool isSnapshotRequested() const noexcept { return snapshotRequested_; }
    [[nodiscard]] bool consumeSnapshotRequest() noexcept {
        const bool req = snapshotRequested_;
        snapshotRequested_ = false;
        return req;
    }
    void clearSnapshotRequested() noexcept { snapshotRequested_ = false; }

    [[nodiscard]] SculptureWorldMode makeCaptureWorldMode(const mgfx::Camera& cam, std::uint32_t generation = 0U) const;
    void requestSnapshot(
        mgfx::vk::FrameLoop& frameLoop,
        const mgfx::Camera& cam,
        std::uint32_t generation,
        const std::string& directory = "snapshots"
    );

    // Bidirectional snapshot ingestion & restoration (.world, .epimode)
    SnapshotPaths saveSnapshotBundle(
        const std::string& directory = "snapshots",
        const std::string& baseName = "",
        const mgfx::Camera* cameraOverride = nullptr
    );
    bool loadSnapshotBundle(const std::string& filepathOrManifest);
    bool loadSnapshot(const std::string& filepath);
    bool applyWorldMode(const SculptureWorldMode& worldMode);
    [[nodiscard]] std::string_view latestSnapshotPath() const noexcept { return latestSnapshotPath_; }

    // State parameter drift inspection & delta patching
    [[nodiscard]] SculptureStateDiff diffFromBaseline(float epsilon = 1e-5f) const;
    bool applyPatch(const SculptureStateDiff& diff, std::string_view tag = "patch");

    // Epimorphic Micro-Patch Export & Ingestion (.epidiff)
    bool exportBaselineDiff(const std::string& filepath) const;
    bool importDiffPatch(const std::string& filepath);

    // -------------------------------------------------------------------------
    // Live Parameter History Ring (Undo/Redo)
    // -------------------------------------------------------------------------
    void pushHistory(std::string_view tag = "Manual");
    bool undo();
    bool redo();
    void clearHistory() noexcept;
    [[nodiscard]] bool canUndo() const noexcept { return !undoStack_.empty(); }
    [[nodiscard]] bool canRedo() const noexcept { return !redoStack_.empty(); }
    [[nodiscard]] std::size_t undoDepth() const noexcept { return undoStack_.size(); }
    [[nodiscard]] std::size_t redoDepth() const noexcept { return redoStack_.size(); }

    // Snapshot camera override queries
    [[nodiscard]] bool hasSavedCameraOverride() const noexcept { return hasSavedCameraOverride_; }
    [[nodiscard]] mgfx::Camera savedCameraOverride(float fovYRad = 0.959931f) const noexcept;
    void clearSavedCameraOverride() noexcept { hasSavedCameraOverride_ = false; }

    // -------------------------------------------------------------------------
    // Deterministic Cinematic Sequence Recorder & Video Archiving
    // -------------------------------------------------------------------------
    bool startSequenceRecording(std::string_view sessionName = "");
    bool stopSequenceRecording();
    bool toggleSequenceRecording(std::string_view sessionName = "");
    [[nodiscard]] bool isSequenceRecording() const noexcept;
    [[nodiscard]] SequenceRecorderStats sequenceRecordingStats() const;
    bool recordFrame(
        const mgfx::vk::CaptureResult& res,
        const mgfx::Camera& cam,
        std::string_view customTag = ""
    );
    bool recordFrame(
        std::uint32_t width,
        std::uint32_t height,
        const std::uint8_t* pixels,
        const mgfx::Camera& cam,
        bool isBgra = true,
        std::string_view customTag = ""
    );

    // Educational HUD telemetry formatting (automatic GPU generation sync or explicit)
    void formatHud(char* buffer, std::size_t size) const noexcept;
    void formatHud(
        char* buffer,
        std::size_t size,
        std::uint32_t generation,
        std::size_t instanceCount
    ) const noexcept;

    // Zero-boilerplate HUD telemetry drawing directly to active render pass
    void renderHud(
        mgfx::Drawable& draw,
        const mgfx::math::vec2& position = {28.0f, 28.0f},
        float fontSize = 22.0f,
        const mgfx::math::vec4& color = mgfx::math::vec4(1.0f)
    ) const noexcept;

    // Real-time GPU presentation pipeline integration
    void setExternalTransport(const class SculptureWorldExternalTransport* transport) noexcept;
    void render(
        mgfx::Drawable& draw,
        const WorldSnapshotDelivery& delivery,
        const mgfx::math::mat4& rootTransform = mgfx::math::mat4(1.0f)
    );
    // Simplified zero-boilerplate render overload dispatching the latest delivery cached during tick()
    void render(
        mgfx::Drawable& draw,
        const mgfx::math::mat4& rootTransform = mgfx::math::mat4(1.0f)
    );

    // -------------------------------------------------------------------------
    // Educational Annotation: ZERO-WASTE BOUNDED INSTANCE STREAM & COUPLING
    // -------------------------------------------------------------------------
    // Unifies procedural world instances from OpenCL dispatch or 4D Polychoron
    // Homotopy synthesis into the presentation bridge, optionally advecting
    // them through continuous 3D volumetric fluid memory fields.
    // Bounds memory operations to active instances, preventing 1.5 MB redundant copies.
    // -------------------------------------------------------------------------
    [[nodiscard]] const WorldSnapshotDelivery& prepareInstanceDelivery(
        const WorldSnapshotDelivery& delivery
    );

    // Procedural 4D Polychoron Homotopy instance stream synthesis
    [[nodiscard]] WorldSnapshotDelivery synthesizePolychoronDelivery() const;

    // 4D Polychoron Homotopy watertight strut manifold polygonization
    void rebuildPolychoronMesh();
    [[nodiscard]] std::shared_ptr<const mgfx::vk::Mesh> polychoronMesh() const noexcept { return polychoronMesh_; }

    // Canonical world turntable presentation model matrix
    [[nodiscard]] mgfx::math::mat4 computePresentationTransform(
        float simulatedTime,
        float rotationSpeed = 0.1f,
        const mgfx::math::vec3& translation = {0.0f, -2.0f, 0.0f},
        float scale = 1.5f
    ) const noexcept;
    [[nodiscard]] mgfx::math::mat4 computePresentationTransform() const noexcept;

    // Dynamic environmental lighting calculation & application
    [[nodiscard]] SculptureDynamicLighting computeDynamicLighting(
        float simulatedTime,
        const AudioResonanceMetrics& metrics,
        const mgfx::math::vec3& lightDir = mgfx::math::vec3(0.3f, -0.7f, -0.6f)
    ) const noexcept;

    void applyDynamicLighting(
        mgfx::Engine& engine,
        float simulatedTime,
        const AudioResonanceMetrics& metrics,
        const mgfx::math::vec3& lightDir = mgfx::math::vec3(0.3f, -0.7f, -0.6f)
    ) const noexcept;

    [[nodiscard]] const WorldSnapshotDelivery& latestDelivery() const noexcept { return latestDelivery_; }
    [[nodiscard]] SculptureWorldRenderBridge& renderBridge() noexcept { return worldBridge_; }
    [[nodiscard]] const SculptureWorldRenderBridge& renderBridge() const noexcept { return worldBridge_; }
    [[nodiscard]] std::uint32_t renderedGeneration() const noexcept { return renderedGeneration_; }
    [[nodiscard]] std::size_t renderedInstanceCount() const noexcept;

    // Volumetric Optical Raymarching Facade
    [[nodiscard]] SculptureRayHit marchVolumetricRay(const SculptureRay& ray) const;

    // Transport & simulation playback rate controls
    void increaseSpeed(float factor = 1.2f) noexcept;
    void decreaseSpeed(float factor = 1.2f) noexcept;
    void resetSpeed() noexcept;
    [[nodiscard]] float speedMultiplier() const noexcept;
    void togglePause() noexcept;
    void setPaused(bool paused) noexcept;
    [[nodiscard]] bool isPaused() const noexcept;

    // Interactive parameter mutation & modulation facade
    void mutate(float mutationScale = 0.22f, float durationSeconds = 2.0f);
    void randomizePalette(float durationSeconds = 1.5f);
    void resetParameters(float durationSeconds = 1.2f);
    void cycleInnerMap(float durationSeconds = 1.0f);
    void cycleReduction(float durationSeconds = 1.0f);
    [[nodiscard]] bool areParametersMutated() const noexcept;

    // Autonomous World Tour & Thematic Playlists
    void enableTour(bool active = true);
    void toggleTour() noexcept;
    [[nodiscard]] bool isTourActive() const noexcept;
    [[nodiscard]] class SculptureWorldTourChoreographer* tourChoreographer() noexcept { return tourChoreographer_.get(); }
    [[nodiscard]] const class SculptureWorldTourChoreographer* tourChoreographer() const noexcept { return tourChoreographer_.get(); }
    void setTourPlaylist(std::vector<std::string> presetIds, std::string_view tourName = "Custom Playlist");

    // Thematic Tour Playlist selection & cycling
    void cycleCuratedTour() noexcept;
    bool selectCuratedTour(std::size_t index) noexcept;
    bool selectCuratedTour(std::string_view tourId) noexcept;
    void cycleTourMode() noexcept;
    void stepTourNext();
    void stepTourPrev();
    [[nodiscard]] std::size_t activeCuratedTourIndex() const noexcept { return activeCuratedTourIndex_; }
    [[nodiscard]] const CuratedThematicTour& activeCuratedTour() const noexcept { return curatedTours_[activeCuratedTourIndex_]; }
    [[nodiscard]] std::size_t curatedTourCount() const noexcept { return curatedTours_.size(); }
    [[nodiscard]] std::span<const CuratedThematicTour> availableCuratedTours() const noexcept { return curatedTours_; }

    // Multi-Category Preset Library Tour Ingestion & Thematic Choreography
    std::vector<std::string> availableTourCategories();
    bool selectCategoryTour(std::string_view category, float durationSeconds = 3.5f, float dwellSeconds = 12.0f);
    void cycleCategoryTour();
    [[nodiscard]] std::string_view activeTourCategory() const noexcept { return activeTourCategory_; }
    [[nodiscard]] bool isCategoryTourActive() const noexcept;
    void ensurePresetLibraryLoaded();

    // Variable-resolution staged compute governor & dynamic scaling
    void cycleResolutionScale() noexcept;
    void toggleStagedGovernor() noexcept;
    void setVariableResolutionScale(StageResolutionScale scale) noexcept;
    void setAutoResolutionScalingEnabled(bool enabled) noexcept;
    [[nodiscard]] bool isAutoResolutionScalingEnabled() const noexcept;
    [[nodiscard]] StageResolutionScale resolutionScale() const noexcept;
    [[nodiscard]] bool isStagedGovernorEnabled() const noexcept;
    void recordPerformanceSample(float frameIntervalMs, float cpuMs = 0.0f, float gpuMs = 0.0f) noexcept;

    // Pipeline Coordinator & Volumetric Epimorphism Memory
    [[nodiscard]] const SculpturePipelineCoordinator& coordinator() const noexcept { return coordinator_; }
    [[nodiscard]] SculpturePipelineCoordinator& coordinator() noexcept { return coordinator_; }
    [[nodiscard]] const SculptureVolumetricMemory* volumetricMemory() const noexcept { return coordinator_.volumetricMemory(); }
    [[nodiscard]] SculptureVolumetricMemory* volumetricMemory() noexcept { return coordinator_.volumetricMemory(); }

    // 3D Volumetric Recursive Memory Interactive Controls
    void cycleVolumetricResolution() noexcept;
    void toggleVolumetricFeedback() noexcept;
    void resetVolumetricMemory() noexcept;
    void toggleVolumetricWrap() noexcept;
    void toggleVolumetricBackend() noexcept;
    [[nodiscard]] bool isVolumetricGpuActive() const noexcept;
    bool initVolumetricGpu(cl_context context, cl_device_id device, const std::string& kernelSourceOrPath = "") {
        return coordinator_.initVolumetricGpu(context, device, kernelSourceOrPath);
    }
    bool initVolumetricGpu(SculptureFrameProducer& producer, const std::string& kernelSourceOrPath = "") noexcept;
    void releaseVolumetricGpu() noexcept {
        coordinator_.releaseVolumetricGpu();
    }

    // 3D Volumetric Field Coupler & Physical Advection Controls
    [[nodiscard]] const SculptureVolumetricFieldCoupler& volumetricCoupler() const noexcept { return volumetricCoupler_; }
    [[nodiscard]] SculptureVolumetricFieldCoupler& volumetricCoupler() noexcept { return volumetricCoupler_; }
    void toggleVolumetricCoupling() noexcept;
    void toggleVolumetricAdvection() noexcept;
    [[nodiscard]] bool isVolumetricCouplingEnabled() const noexcept { return volumetricCoupler_.config().enableCoupling; }

    // Multimodal Closed-Loop Audio-Volumetric Synesthesia
    void toggleAcousticSynesthesia() noexcept;
    void setAcousticSynesthesiaEnabled(bool enabled) noexcept;
    [[nodiscard]] bool isAcousticSynesthesiaEnabled() const noexcept;

    // CPU reference/diagnostic accessors, not the live OpenCL path.
    // The coordinator owns simulation time and ray-step quality. Camera inputs
    // describe observation; previous-camera pointers are borrowed for this call.
    [[nodiscard]] SculptureGBuffer renderImplicitGBuffer(
        std::uint32_t width,
        std::uint32_t height,
        const SculptureVec3& cameraPos,
        const SculptureVec3& cameraTarget,
        const SculptureVec3& cameraUp = {0.0f, 1.0f, 0.0f},
        float fovRadians = 1.04719755f,
        std::uint32_t threadCount = 4,
        const SculptureVec3* prevCameraPos = nullptr,
        const SculptureVec3* prevCameraTarget = nullptr
    ) const;

    [[nodiscard]] SculptureStereoGBufferPair renderStereoImplicitGBuffer(
        std::uint32_t eyeWidth,
        std::uint32_t eyeHeight,
        const SculptureStereoRig& rig,
        const SculptureVec3& cameraPos,
        const SculptureVec3& cameraTarget,
        const SculptureVec3& cameraUp = {0.0f, 1.0f, 0.0f},
        float fovRadians = 1.04719755f,
        std::uint32_t threadCount = 4,
        const SculptureVec3* prevCameraPos = nullptr,
        const SculptureVec3* prevCameraTarget = nullptr
    ) const;

    // -------------------------------------------------------------------------
    // Educational Learn Mode & 6-Stage Pipeline Decomposition
    // -------------------------------------------------------------------------
    void cycleLearnStage() noexcept;
    void cycleDiagnosticStage() noexcept;
    void resetLearnMode() noexcept;
    void toggleLearnStage(learn_mode::Stage stage) noexcept;
    void setLearnStageSolo(learn_mode::Stage stage) noexcept;
    void setLearnStageAccumulative(learn_mode::Stage maxStage) noexcept;
    [[nodiscard]] std::uint32_t stageMask() const noexcept;
    [[nodiscard]] bool isLearnModeActive() const noexcept;
    [[nodiscard]] learn_mode::DiagnosticStage diagnosticStage() const noexcept;
    [[nodiscard]] std::string getLearnModePedagogicalReport() const;
    [[nodiscard]] std::string_view activeStageName(learn_mode::Stage stage) const noexcept;
    [[nodiscard]] std::string_view activeStageFormula(learn_mode::Stage stage) const noexcept;
    void soloStage(learn_mode::Stage stage) noexcept { setLearnStageSolo(stage); }
    void resetLearnStages() noexcept { resetLearnMode(); }

    // -------------------------------------------------------------------------
    // 4D Foliation Spacetime, SO(4) Rotor Kinematics & Living Palette Agent
    // -------------------------------------------------------------------------
    void toggle4DFoliationWave() noexcept;
    void set4DFoliationWaveEnabled(bool enabled) noexcept;
    [[nodiscard]] bool is4DFoliationWaveEnabled() const noexcept;

    void cycle4DManifoldType() noexcept;
    void set4DManifoldType(Manifold4DType type) noexcept;
    [[nodiscard]] Manifold4DType active4DManifoldType() const noexcept;

    void cycleRotorSpeed() noexcept;
    [[nodiscard]] float rotorSpeed() const noexcept;

    // Interactive 4D Hyperplane Slicing & SO(4) Rotor Scrubbing
    void adjustLatentW(float delta) noexcept;
    void setLatentW(float w) noexcept;
    [[nodiscard]] float latentW() const noexcept;

    void adjustRotorAlpha(float delta) noexcept;
    void setRotorAlpha(float alpha) noexcept;
    [[nodiscard]] float rotorAlpha() const noexcept;

    void adjustRotorBeta(float delta) noexcept;
    void setRotorBeta(float beta) noexcept;
    [[nodiscard]] float rotorBeta() const noexcept;

    void adjustRotorTheta4D(float delta) noexcept;
    void setRotorTheta4D(float theta) noexcept;
    [[nodiscard]] float rotorTheta4D() const noexcept;

    void reset4DRotors() noexcept;

    void togglePaletteAgent() noexcept;
    void setPaletteAgentEnabled(bool enabled) noexcept;
    [[nodiscard]] bool isPaletteAgentEnabled() const noexcept;
    void cyclePaletteAgentMode() noexcept;
    [[nodiscard]] PaletteAgentMode paletteAgentMode() const noexcept;
    void stimulatePaletteAgent(float intensity = 0.5f);

    // -------------------------------------------------------------------------
    // 4D Polychoron Homotopy & Coxeter Geodesic Morphing
    // -------------------------------------------------------------------------
    void setPolychoronHomotopy(PolychoronKind source, PolychoronKind target, float progress = 0.0f) noexcept;
    void setPolychoronMorphProgress(float progress) noexcept;
    [[nodiscard]] float polychoronMorphProgress() const noexcept;
    void cyclePolychoronSource() noexcept;
    void cyclePolychoronTarget() noexcept;
    [[nodiscard]] PolychoronKind polychoronSource() const noexcept;
    [[nodiscard]] PolychoronKind polychoronTarget() const noexcept;
    [[nodiscard]] std::vector<ProjectedVertex3D> samplePolychoronHomotopy() const;
    [[nodiscard]] float coxeterOrderParameter() const noexcept;
    [[nodiscard]] Sculpture4DPolychoronHomotopy& polychoronHomotopy() noexcept { return polychoronHomotopy_; }
    [[nodiscard]] const Sculpture4DPolychoronHomotopy& polychoronHomotopy() const noexcept { return polychoronHomotopy_; }

    // -------------------------------------------------------------------------
    // 2.5D / 3.5D Epimorphic Relief Manifold Presentation
    // -------------------------------------------------------------------------
    void setPresentationMode(WorldPresentationMode mode) noexcept;
    [[nodiscard]] WorldPresentationMode presentationMode() const noexcept { return presentationMode_; }
    void cyclePresentationMode() noexcept;

    void setReliefProjection(ReliefManifoldProjection projection) noexcept;
    [[nodiscard]] ReliefManifoldProjection reliefProjection() const noexcept { return reliefConfig_.projection; }
    void cycleReliefProjection() noexcept;

    void setReliefConfig(const EpimorphicReliefConfig& config) noexcept;
    [[nodiscard]] const EpimorphicReliefConfig& reliefConfig() const noexcept { return reliefConfig_; }

    // Candidate preparation precedes all publication changes. Unavailable output
    // retains the previous selection and pending request; allocation errors propagate.
    void rebuildReliefMesh();
    [[nodiscard]] std::shared_ptr<const mgfx::vk::Mesh> activeReliefMesh() const noexcept { return reliefMesh_; }
    [[nodiscard]] const EpimorphicReliefData& activeReliefData() const noexcept { return currentReliefData_; }

    // -------------------------------------------------------------------------
    // Unified Command Center: State Mutation Action Dispatcher
    // -------------------------------------------------------------------------
    // Dispatches a structured high-level action (keyboard, MIDI, OSC, script, AI)
    // to synchronously mutate navigator or pipeline state.
    // 'value' serves as an optional scalar magnitude (e.g., delta W or delta Alpha).
    // Returns true if the action was recognized and executed.
    bool dispatchAction(NavigatorAction action, float value = 0.0f) noexcept;

    // Asynchronous / decoupled action enqueueing for event threads, input mappers, and UI callbacks
    void enqueueAction(NavigatorAction action, float value = 0.0f);
    void enqueueAction(const ActionCommand& cmd);
    std::size_t drainActionQueue() noexcept;
    [[nodiscard]] std::size_t queuedActionCount() const noexcept;
    void clearActionQueue() noexcept;

    // -------------------------------------------------------------------------
    // Deterministic Macro Timeline Choreography
    // -------------------------------------------------------------------------
    bool loadTimelineScript(std::string_view jsonOrFilePath);
    void playTimeline() noexcept;
    void pauseTimeline() noexcept;
    void resumeTimeline() noexcept;
    void stopTimeline() noexcept;
    void toggleTimeline() noexcept;
    [[nodiscard]] bool isTimelinePlaying() const noexcept;
    [[nodiscard]] float timelineProgress() const noexcept;
    [[nodiscard]] class SculptureTimelineDirector* timelineDirector() noexcept { return timelineDirector_.get(); }
    [[nodiscard]] const class SculptureTimelineDirector* timelineDirector() const noexcept { return timelineDirector_.get(); }
    bool generateBeatSyncedTimeline(float bpm = 0.0f, std::uint32_t barsPerWorld = 8U);
    bool exportActiveTimeline(const std::string& filepath = "") const;

    // -------------------------------------------------------------------------
    // Real-Time Performance Monitor & Adaptive Frame-Pacing Optimizer
    // -------------------------------------------------------------------------
    [[nodiscard]] class SculpturePerformanceMonitor* performanceMonitor() noexcept { return performanceMonitor_.get(); }
    [[nodiscard]] const class SculpturePerformanceMonitor* performanceMonitor() const noexcept { return performanceMonitor_.get(); }
    void togglePerformanceMonitor() noexcept;
    void cycleTargetFps() noexcept;
    void setTargetFpsTier(TargetFrameRateTier tier) noexcept;
    [[nodiscard]] TargetFrameRateTier targetFpsTier() const noexcept;
    [[nodiscard]] float targetFps() const noexcept;

    // -------------------------------------------------------------------------
    // Reactive Observer & Decoupled EventBus System
    // -------------------------------------------------------------------------
    void addObserver(std::shared_ptr<INavigatorObserver> observer);
    void addObserver(INavigatorObserver* rawObserver);
    void removeObserver(INavigatorObserver* rawObserver);
    [[nodiscard]] std::size_t observerCount() const noexcept;
    void clearObservers() noexcept;

    // Bi-directional EventBus binding
    void bindEventBus(SculptureEventBus& eventBus);

    // Dynamic fluid physics and atmospheric mist subsystem
    void setAtmosphereManager(std::shared_ptr<SculptureFluidAtmosphereManager> atmosphere) noexcept {
        atmosphereManager_ = std::move(atmosphere);
    }
    [[nodiscard]] SculptureFluidAtmosphereManager* atmosphereManager() noexcept {
        return atmosphereManager_.get();
    }
    [[nodiscard]] const SculptureFluidAtmosphereManager* atmosphereManager() const noexcept {
        return atmosphereManager_.get();
    }

    // Dynamic audio synesthesia feed subsystem
    void setAudioFeedDirector(std::shared_ptr<SculptureAudioFeedDirector> audioDirector) noexcept {
        audioDirector_ = std::move(audioDirector);
    }
    [[nodiscard]] SculptureAudioFeedDirector* audioDirector() noexcept {
        return audioDirector_.get();
    }
    [[nodiscard]] const SculptureAudioFeedDirector* audioDirector() const noexcept {
        return audioDirector_.get();
    }

    // -------------------------------------------------------------------------
    // VJ Live Performance Deck & Master Crossfader Surface
    // -------------------------------------------------------------------------
    void setPerformanceDeckEnabled(bool enabled) noexcept;
    [[nodiscard]] bool isPerformanceDeckEnabled() const noexcept;
    void togglePerformanceDeck() noexcept;

    // -------------------------------------------------------------------------
    // Stereoscopic 3D Side-By-Side (SBS) Presentation Mode
    // -------------------------------------------------------------------------
    [[nodiscard]] bool isStereoEnabled() const noexcept { return stereoEnabled_; }
    void setStereoEnabled(bool enabled) noexcept { stereoEnabled_ = enabled; }
    void toggleStereo() noexcept { stereoEnabled_ = !stereoEnabled_; }
    [[nodiscard]] float interpupillaryDistance() const noexcept { return ipdMeters_; }
    void setInterpupillaryDistance(float ipd) noexcept { ipdMeters_ = ipd; }

    void setCrossfadePosition(float pos) noexcept;
    [[nodiscard]] float crossfadePosition() const noexcept;
    void nudgeCrossfade(float delta) noexcept;

    void triggerAutoCrossfade(float durationSeconds = 3.0f, std::optional<DeckId> targetDeck = std::nullopt) noexcept;
    void cancelAutoCrossfade() noexcept;
    [[nodiscard]] bool isAutoCrossfading() const noexcept;
    [[nodiscard]] float autoCrossfadeProgress() const noexcept;

    void swapPerformanceDecks() noexcept;
    void loadActiveToDeck(DeckId deckId);
    void loadWorldToDeck(DeckId deckId, std::size_t worldIndex);
    void loadPresetToDeck(DeckId deckId, std::string_view presetId);

    [[nodiscard]] SculpturePerformanceDeck* performanceDeck() noexcept;
    [[nodiscard]] const SculpturePerformanceDeck* performanceDeck() const noexcept;

    // -------------------------------------------------------------------------
    // Multidimensional Morph Space (Vector Synthesis Pad)
    // -------------------------------------------------------------------------
    [[nodiscard]] SculptureMorphSpace* morphSpace() noexcept { return morphSpace_.get(); }
    [[nodiscard]] const SculptureMorphSpace* morphSpace() const noexcept { return morphSpace_.get(); }
    void toggleMorphSpace() noexcept;
    [[nodiscard]] bool isMorphSpaceActive() const noexcept;
    void setMorphSpaceCursor(float x, float y) noexcept;
    void nudgeMorphSpaceCursor(float dx, float dy) noexcept;
    void cycleMorphSpaceMotion() noexcept;
    void cycleMorphSpaceBank() noexcept;
    void loadMorphSpaceBank(std::size_t index) noexcept;
    void loadMorphSpaceBank(std::string_view name) noexcept;
    [[nodiscard]] std::string_view activeMorphSpaceBankName() const noexcept;
    [[nodiscard]] std::string_view activeMorphSpaceBankTitle() const noexcept;
    void toggleMorphSpacePath() noexcept;
    [[nodiscard]] bool isMorphSpacePathPlaying() const noexcept;
    void populateMorphSpaceFromPresetLibrary() noexcept;
    bool bindMorphSpaceToCategory(std::string_view category, bool autoLoad = true) noexcept;
    bool syncMorphSpaceToActiveCategory() noexcept;
    std::size_t synthesizeCategoryMorphBanks() noexcept;

    // Internal observer notification helpers
    void notifyTimelineCueFired(const struct TimelineActionCue& cue);
    void notifyTimelinePlaybackChanged(bool isPlaying, bool isPaused, float currentTime, float duration);
    void broadcastPerformanceMetrics();

private:
    friend class SculptureWorldTelemetryFormatter;
    friend class SculptureWorldStatePersistence;
    friend class SculptureWorldActionDispatcher;

    void rebuildReliefMesh(const ReliefMeshRequest& request, mgfx::Drawable* draw = nullptr);
    void rebuildPolychoronMesh(const PolychoronMeshRequest& request, mgfx::Drawable* draw = nullptr);
    [[nodiscard]] PolychoronMeshRequest describePolychoronMesh() const noexcept;

    // Resolve intent before touching live state. Catalog identity describes the
    // route; the owned parameters/recipe retain an alternate asset's identity.
    struct ResolvedWorldSelection {
        std::size_t catalogIndex = 0U;
        std::string presetId;
        std::string revision;
        SculptureProgramParams params{};
        WorldRecipe recipe{};
    };
    [[nodiscard]] std::optional<ResolvedWorldSelection> resolveWorldSelection(
        std::string_view presetId);
    bool applyWorldSelection(const ResolvedWorldSelection& selection, float transitionSeconds);

    std::function<mgfx::Camera(const mgfx::Camera&, float)> cameraController_;
    template <typename Func>
    void notifyExternalObservers(Func&& func) {
        for (auto& obs : sharedObservers_) {
            if (obs) func(*obs);
        }
        for (auto* obs : rawObservers_) {
            if (obs) func(*obs);
        }
    }

    template <typename Func>
    void notifyObservers(Func&& func) {
        if (performanceMonitor_ != nullptr) {
            func(*performanceMonitor_);
        }
        notifyExternalObservers(std::forward<Func>(func));
    }

    std::vector<std::shared_ptr<INavigatorObserver>> sharedObservers_{};
    std::vector<INavigatorObserver*> rawObservers_{};

    SculptureWorldFrame renderFrameInternal(
        mgfx::Engine& engine,
        mgfx::vk::FrameLoop& loop,
        SculptureWorldFrame worldFrame,
        std::uint32_t captureGeneration,
        float wallDeltaSeconds,
        const std::function<void(mgfx::Drawable&)>& secondaryDraw
    );

    void applyRecipePalette();
    void updateTransitionPalette(float progress);
    void syncReliefProjectionForRecipe(const WorldRecipe& recipe);
    void syncAutoDirectorFraming();
    void initCuratedTours();
    void advanceTransitionAndAutopilot(float wallDeltaSeconds) noexcept;

    WorldTransitionState transitionState_{};

    mgfx::Engine* engine_ = nullptr;
    SculpturePipelineCoordinator& coordinator_;
    SculptureEventBus* boundEventBus_ = nullptr;
    const class SculpturePresetLibrary* presetLibrary_ = nullptr;
    std::unique_ptr<class SculpturePresetLibrary> ownedPresetLibrary_;
    std::filesystem::path presetDirectory_{};
    std::string activeTourCategory_;
    std::size_t activeCategoryIndex_ = 0U;
    std::vector<const WorldCatalogEntry*> availableWorlds_;
    std::size_t activeIndex_ = 0U;
    WorldRecipe currentRecipe_{};
    std::string currentPresetName_;
    std::string currentPresetRevision_;
    SculptureProgramParams currentParams_{};
    SculptureProgramParams baselineParams_{};
    std::vector<mgfx::MaterialHandle> bands_;
    std::unique_ptr<class SculptureWorldTourChoreographer> tourChoreographer_;
    std::unique_ptr<class SculptureTimelineDirector> timelineDirector_;
    std::unique_ptr<class SculpturePerformanceMonitor> performanceMonitor_;
    std::unique_ptr<SculptureMorphSpace> morphSpace_;
    std::vector<CuratedThematicTour> curatedTours_;
    std::size_t activeCuratedTourIndex_ = 0U;
    std::shared_ptr<SculptureFluidAtmosphereManager> atmosphereManager_{};
    std::shared_ptr<SculptureAudioFeedDirector> audioDirector_{};

    SculptureWorldRenderBridge worldBridge_{};
    // Observational only: the bridge owns rendering, while this pointer lets
    // the HUD report external-ring health without copying world data.
    const class SculptureWorldExternalTransport* externalTransport_ = nullptr;
    std::unique_ptr<struct SculptureWorldPrimitiveMeshes> primitiveMeshes_;
    std::uint32_t renderedGeneration_ = 0U;
    WorldSnapshotDelivery latestDelivery_{};

    WorldPresentationMode presentationMode_ = WorldPresentationMode::PointCloud;
    EpimorphicReliefConfig reliefConfig_{};
    EpimorphicReliefData currentReliefData_{};
    std::shared_ptr<const mgfx::vk::Mesh> reliefMesh_{};
    std::optional<ReliefMeshRequest> publishedReliefRequest_{};
    bool reliefMeshRebuildPending_ = false;
    bool hasSavedCameraOverride_ = false;
    mgfx::math::vec3 savedCameraEye_{0.0f, 0.0f, 14.0f};
    mgfx::math::vec3 savedCameraTarget_{0.0f, 0.0f, 0.0f};
    mgfx::math::vec3 savedCameraUp_{0.0f, 1.0f, 0.0f};
    bool snapshotRequested_ = false;
    std::string latestSnapshotPath_;
    std::vector<ActionCommand> queuedActions_{};

    // Stereoscopic 3D Side-By-Side (SBS) presentation state
    bool stereoEnabled_ = false;
    float ipdMeters_ = 0.064f;

    // Closed-loop governor and adaptive variable-resolution state
    void updateClosedLoopResolutionScaling() noexcept;
    bool autoResolutionScalingEnabled_ = true;
    std::uint32_t consecutiveHitchCount_ = 0U;
    std::uint32_t sustainedHeadroomCount_ = 0U;

    // Live Parameter History Ring stacks
    struct ParameterHistoryEntry {
        SculptureProgramParams params{};
        std::string tag;
    };
    static constexpr std::size_t kMaxHistoryDepth = 32;
    std::deque<ParameterHistoryEntry> undoStack_{};
    std::deque<ParameterHistoryEntry> redoStack_{};

    Sculpture4DPolychoronHomotopy polychoronHomotopy_{};
    std::shared_ptr<const mgfx::vk::Mesh> polychoronMesh_{};
    std::optional<PolychoronMeshRequest> publishedPolychoronRequest_{};
    bool polychoronMeshRebuildPending_ = false;
    mutable WorldGenerationMetrics polychoronMetrics_{};
    SculptureVolumetricFieldCoupler volumetricCoupler_{};
    mutable world_gpu_abi::WorldSnapshotGpu coupledSnapshot_{};
    mutable WorldSnapshotDelivery coupledDelivery_{};
};

} // namespace epi_sculpt
