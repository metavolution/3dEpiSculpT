/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - SCULPTURE PIPELINE COORDINATOR
 * ============================================================================
 *
 * Educational Annotation: UNIFIED EXECUTION PIPELINE & ARCHITECTURAL BRIDGE
 * -------------------------------------------------------------------------
 * This module coordinates the entire mathematical life-cycle of epimorphic
 * 3D visualizations, uniting four decoupled subsystems into a coherent,
 * high-performance, fault-tolerant execution flow:
 *
 *  ┌────────────────────────────────────────────────────────────────────────┐
 *  │                      1. TEMPORAL CADENCE GOVERNOR                      │
 *  │   Decouples variable presentation framerate (e.g. 120-240 Hz Vulkan)  │
 *  │   from asynchronous simulation ticks (e.g. 30-60 Hz OpenCL compute).   │
 *  └───────────────────────────────────┬────────────────────────────────────┘
 *                                      │ Dispatches simulation tick
 *                                      ▼
 *  ┌────────────────────────────────────────────────────────────────────────┐
 *  │                   2. GEODESIC MORPH CHOREOGRAPHER                      │
 *  │   Computes shortest-arc polar OKLCH transitions and homotopy blends    │
 *  │   across curated playlist tours without parameter singularities.      │
 *  └───────────────────────────────────┬────────────────────────────────────┘
 *                                      │ Interpolates parameters
 *                                      ▼
 *  ┌────────────────────────────────────────────────────────────────────────┐
 *  │                  3. LEARN MODE STAGE BITMASK ISOLATOR                  │
 *  │   Enables educational isolation and diagnostic viewing across the 6    │
 *  │   canonical pipeline stages (Transform, Reduce, Seed, Material, etc.). │
 *  └───────────────────────────────────┬────────────────────────────────────┘
 *                                      │ Shapes active stage mask
 *                                      ▼
 *  ┌────────────────────────────────────────────────────────────────────────┐
 *  │                4. ADAPTIVE QUALITY & RESOLUTION GOVERNOR               │
 *  │   Dynamically modulates raymarching steps (48-192) and downsampling    │
 *  │   tiers (Full / Half / Quarter) via hysteresis timing feedback.        │
 *  └───────────────────────────────────┬────────────────────────────────────┘
 *                                      │ Emits immutable package
 *                                      ▼
 *                      [SculpturePipelineFrame Package]
 * ============================================================================
 */

#pragma once

#include "SculptureProgramSchema.h"
#include "SculpturePresets.h"
#include "SculpturePresetLibrary.h"
#include "SculptureProducerCadence.h"
#include "SculptureMorphChoreographer.h"
#include "ImplicitQualityPolicy.h"
#include "SculptureLearnMode.h"
#include "SculptureModulationEngine.h"
#include "SculptureAudio.h"
#include "SculptureAudioResonance.h"
#include "ISculptureCoordinatorPlugin.h"
#include "SculptureWorldRecipe.h"
#include "SculptureStateRecorder.h"
#include "SculptureLiveCaptureController.h"
#include "SculptureAutoDirector.h"
#include "SculpturePerformanceDeck.h"
#include "SculptureEventBus.h"
#include "Sculpture4DFoliation.h"
#include "SculptureFieldGeometry.h"
#include "SculptureShaderSynthesizer.h"
#include "EpimorphicReliefGenerator.h"
#include "SculptureProceduralGeometryRequest.h"
#include "SculptureKnotHomotopy.h"
#include "SculptureStagedComputeGovernor.h"
#include "SculptureProgramBinaryCache.h"
#include "SculptureKernelSpecializer.h"
#include "SculptureVariableResolutionStage.h"
#include "SculptureTimelineSequencer.h"
#include "SculpturePaletteAgent.h"
#include "SculptureSequenceRecorder.h"
#include "SculptureVolumetricMemory.h"
#include "SculptureVolumetricFieldCoupler.h"
#include "Sculpture4DPolychoronHomotopy.h"
#include "SculptureRaymarcher.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <memory>

namespace epi_sculpt {

class SculptureRemoteServer;

// Immutable coordinated frame package ready for instant renderer dispatch
struct SculpturePipelineFrame {
    SculptureProgramParams params{};
    float time = 0.0f;
    float morph = 0.0f;
    float deltaSeconds = 0.0f;
    std::uint32_t generation = 0U;
    std::uint32_t raySteps = 128U;
    StageResolutionScale resolutionScale = StageResolutionScale::Full;
    std::uint32_t stageMask = learn_mode::kAllStages;
    learn_mode::DiagnosticStage diagnosticStage = learn_mode::DiagnosticStage::FullPipeline;
    float speedMultiplier = 1.0f;
    bool isPaused = false;
    bool isTourActive = false;
    std::size_t tourIndex = 0U;
    std::size_t tourSize = 0U;
};

struct SculpturePipelineCoordinatorConfig {
    SculptureProducerCadenceConfig cadence{};
    ImplicitQualityConfig quality{};
    bool autoStartTour = false;
    bool enableModulation = true;
    bool enableAudioResonance = false;
    bool enableAutoDirector = false;
    AutoDirectorConfig autoDirectorConfig{};
    std::string defaultTourName = "dimensional_horizons";
    float defaultTransitionSeconds = 3.0f;
    float defaultDwellSeconds = 5.0f;
};

struct SculpturePipelineTelemetry {
    std::uint64_t totalFramesCoordinated = 0U;
    std::uint64_t dispatchesEmitted = 0U;
    std::uint32_t currentGeneration = 0U;
    float effectiveCadenceHz = 30.0f;
    std::uint32_t activeRaySteps = 128U;
    StageResolutionScale activeResolutionScale = StageResolutionScale::Full;
    bool isMorphActive = false;
    float morphProgress = 0.0f;
    bool isTourActive = false;
    std::size_t tourIndex = 0U;
    std::size_t tourSize = 0U;
    std::uint32_t activeStageMask = learn_mode::kAllStages;
    learn_mode::DiagnosticStage activeDiagnosticStage = learn_mode::DiagnosticStage::FullPipeline;
    std::size_t activeModulationRoutes = 0U;
    bool hasAudioSignal = false;
    float kuramotoOrderR = 0.0f;
    float instantaneousPressure = 0.0f;
    float resonanceQFactor = 12.0f;
    float spectralEntrainment = 0.0f;
    float speedMultiplier = 1.0f;
    bool isPaused = false;
    // Facts about due volume attempts, distinct from offered animation time.
    std::uint64_t rejectedVolumeSteps = 0U;
    bool volumeStepRejected = false;
};

class SculpturePipelineCoordinator {
public:
    explicit SculpturePipelineCoordinator(
        SculpturePipelineCoordinatorConfig config = {},
        const SculpturePresetLibrary* library = nullptr);

    // -------------------------------------------------------------------------
    // Temporal Advancement & Frame Coordination
    // -------------------------------------------------------------------------
    // Advances offered authoring time by presentation wall time and evaluates
    // cadence; a due interval commits after optional volume production succeeds.
    // Returns an immutable frame dispatch package if a simulation tick is due,
    // or std::nullopt if the renderer should reuse current visual buffers.
    // A rejected volume attempt leaves accepted cadence and the emitted sample
    // unchanged; rejection facts are available through telemetry(). Authoring/
    // modulation time is offered control state, not rolled-back field history.
    [[nodiscard]] std::optional<SculpturePipelineFrame> update(float wallDeltaSeconds) noexcept;

    // -------------------------------------------------------------------------
    // Unified Closed-Loop Execution Dispatch
    // -------------------------------------------------------------------------
    // Evaluates cadence, morphing, learn mode, and modulation, then directly
    // executes the pipeline with closed-loop frame timing feedback.
    [[nodiscard]] std::optional<SculpturePipelineFrame> executeCoordinatedTick(
        class SculpturePipeline& pipeline,
        float wallDeltaSeconds,
        const SculptureVec3& cameraPos = {0.0f, 0.0f, 0.0f},
        const SculptureSnapshot* externalCpuSnapshot = nullptr,
        const std::vector<float>* externalGpuSpatialBuffer = nullptr
    );

    // -------------------------------------------------------------------------
    // Feedback & Closed-Loop Quality Adaptation
    // -------------------------------------------------------------------------
    // Ingests timing measurements from GPU compute / raymarch producers to
    // automatically adjust dispatch rates, step budgets, and resolution scales.
    void observeProducerMetrics(float kernelMilliseconds, float totalProducerLatencyMilliseconds) noexcept;

    // -------------------------------------------------------------------------
    // Event Bus Integration & Decoupled Semantic Control
    // -------------------------------------------------------------------------
    void bindEventBus(SculptureEventBus& eventBus);
    void unbindEventBus() noexcept { boundEventBus_ = nullptr; }
    [[nodiscard]] SculptureEventBus* boundEventBus() noexcept { return boundEventBus_; }
    [[nodiscard]] const SculptureEventBus* boundEventBus() const noexcept { return boundEventBus_; }
    [[nodiscard]] bool isBoundToEventBus(const SculptureEventBus* bus) const noexcept {
        return boundEventBus_ != nullptr && boundEventBus_ == bus;
    }
    void setRemoteServer(SculptureRemoteServer* server) noexcept { remoteServer_ = server; }
    [[nodiscard]] SculptureRemoteServer* remoteServer() noexcept { return remoteServer_; }
    [[nodiscard]] const SculptureRemoteServer* remoteServer() const noexcept { return remoteServer_; }

    // -------------------------------------------------------------------------
    // Transport, Playback Rate & Cadence Hotkey Controls
    // -------------------------------------------------------------------------
    void setSpeedMultiplier(float speed) noexcept { cadence_.setSpeedMultiplier(speed); }
    [[nodiscard]] float speedMultiplier() const noexcept { return cadence_.speedMultiplier(); }
    void increaseSpeed(float factor = 1.2f) noexcept { cadence_.increaseSpeed(factor); }
    void decreaseSpeed(float factor = 1.2f) noexcept { cadence_.decreaseSpeed(factor); }
    void resetSpeed() noexcept { cadence_.resetSpeed(); }
    void setPaused(bool paused) noexcept { cadence_.setPaused(paused); }
    void togglePause() noexcept { cadence_.togglePause(); }
    [[nodiscard]] bool paused() const noexcept { return cadence_.paused(); }
    [[nodiscard]] bool isPaused() const noexcept { return cadence_.paused(); }
    [[nodiscard]] float simulatedTime() const noexcept { return simulatedTime_; }

    // -------------------------------------------------------------------------
    // Audio Spectrum & Reactive Signal Ingestion
    // -------------------------------------------------------------------------
    void setAudioContext(const AudioSignalContext& audio) noexcept;
    void setSynestheticAudioFeedback(const class SculptureSpatialSonifier& sonifier) noexcept;
    void clearAudioContext() noexcept;
    void setAudioResonanceEnabled(bool enabled) noexcept { enableAudioResonance_ = enabled; }
    [[nodiscard]] bool audioResonanceEnabled() const noexcept { return enableAudioResonance_; }
    [[nodiscard]] const SculptureAudioResonance& audioResonance() const noexcept { return audioResonance_; }
    [[nodiscard]] SculptureAudioResonance& audioResonance() noexcept { return audioResonance_; }

    // -------------------------------------------------------------------------
    // Dynamic Modulation Matrix & Routing Maps
    // -------------------------------------------------------------------------
    void addModulationRoute(const CompiledModulationRoute& route);
    bool parseAndAddModulationRoute(std::string_view routeString);
    void loadPresetModulationRoutes(const std::vector<std::string>& routingMap);
    void clearModulationRoutes() noexcept;

    // Macro Control Channels (0..3 and Semantic Aliases) & Slew Limiter
    void setMacro(std::size_t index, float value) noexcept { modulation_.setMacro(index, value); }
    void setMacro(std::string_view name, float value) noexcept { modulation_.setMacro(name, value); }
    [[nodiscard]] float macro(std::size_t index) const noexcept { return modulation_.macro(index); }
    [[nodiscard]] float macro(std::string_view name) const noexcept { return modulation_.macro(name); }
    void resetModulationSmoothing() noexcept { modulation_.resetSmoothing(); }

    // -------------------------------------------------------------------------
    // 2D <-> 4D Holographic Texture & Scalar Potential Ingestion
    // -------------------------------------------------------------------------
    void setAuxiliary2DTexture(const std::vector<float>& textureRgba, std::uint32_t width, std::uint32_t height);
    void clearAuxiliary2DTexture() noexcept;
    [[nodiscard]] bool hasAuxiliary2DTexture() const noexcept {
        return !auxiliaryTextureRgba_.empty() && auxiliaryTextureWidth_ > 0 && auxiliaryTextureHeight_ > 0;
    }
    [[nodiscard]] const std::vector<float>& auxiliaryVoxelGrid() const noexcept {
        return auxiliaryVoxelGrid_;
    }

    [[nodiscard]] const ProceduralGeometrySample& proceduralGeometrySample() const noexcept {
        return proceduralGeometrySample_;
    }
    // Cheap source description: no field hashing, allocation or native readback.
    [[nodiscard]] ReliefMeshRequest describeEpimorphicRelief(
        const EpimorphicReliefConfig& config) const noexcept;
    [[nodiscard]] EpimorphicReliefData generateEpimorphicRelief(
        const ReliefMeshRequest& request) const;

    // -------------------------------------------------------------------------
    // 3D Volumetric Epimorphism Feedback Field (Camera-Independent Recursive Memory)
    // -------------------------------------------------------------------------
    [[nodiscard]] const SculptureVolumetricMemory* volumetricMemory() const noexcept {
        return volumetricMemory_.get();
    }
    [[nodiscard]] SculptureVolumetricMemory* volumetricMemory() noexcept {
        return volumetricMemory_.get();
    }

    // GPU OpenCL context binding for volumetric memory acceleration
    bool initVolumetricGpu(cl_context context, cl_device_id device, const std::string& kernelSourceOrPath = "") {
        if (volumetricMemory_) {
            return volumetricMemory_->initGpu(context, device, kernelSourceOrPath);
        }
        return false;
    }
    void releaseVolumetricGpu() noexcept {
        if (volumetricMemory_) {
            volumetricMemory_->releaseGpu();
        }
    }
    [[nodiscard]] bool isVolumetricGpuActive() const noexcept {
        return volumetricMemory_ && volumetricMemory_->isGpuActive();
    }

    [[nodiscard]] const SculptureVolumetricFieldCoupler& volumetricCoupler() const noexcept {
        return volumetricCoupler_;
    }
    [[nodiscard]] SculptureVolumetricFieldCoupler& volumetricCoupler() noexcept {
        return volumetricCoupler_;
    }

    // Rasterizes dynamic 3D Helmholtz acoustic standing wave pressure into a voxel grid
    void rasterizeAcousticPressureGrid3D(
        std::vector<float>& outVoxelGrid,
        std::uint32_t voxelDim,
        float time = 0.0f
    ) const noexcept;

    // -------------------------------------------------------------------------
    // 4D Foliation Manifold Mesh Synthesis & Wave Fields
    // -------------------------------------------------------------------------
    [[nodiscard]] IsosurfaceMesh generateActiveManifoldMesh(
        Manifold4DType type,
        float scale = 1.0f,
        std::size_t uDivisions = 32,
        std::size_t vDivisions = 32
    ) const;

    [[nodiscard]] IsosurfaceMesh generateActiveTwistorFibrationMesh(
        std::size_t fiberCount = 8,
        std::size_t pointsPerFiber = 32,
        float ribbonWidth = 0.03f,
        float scale = 1.0f
    ) const;

    [[nodiscard]] std::vector<float> sampleActiveKuramotoWaveField(
        const std::vector<SculptureVec3>& samplePoints
    ) const;

    [[nodiscard]] IsosurfaceMesh generateActivePolychoronHomotopyMesh(
        PolychoronKind source = PolychoronKind::Tesseract8Cell,
        PolychoronKind target = PolychoronKind::Icositetrachoron24,
        float morphProgress = 0.5f,
        float scale = 1.0f,
        float tubeRadius = 0.025f,
        std::size_t radialSegments = 6
    ) const;

    [[nodiscard]] std::vector<world_gpu_abi::WorldInstanceGpu> generateActivePolychoronWorldInstances(
        PolychoronKind source = PolychoronKind::Tesseract8Cell,
        PolychoronKind target = PolychoronKind::Icositetrachoron24,
        float morphProgress = 0.5f,
        float scale = 1.0f
    ) const;

    // -------------------------------------------------------------------------
    // Continuous Implicit Manifold G-Buffer Synthesis
    // -------------------------------------------------------------------------
    [[nodiscard]] SculptureGBuffer renderActiveImplicitGBuffer(
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

    [[nodiscard]] SculptureStereoGBufferPair renderActiveStereoImplicitGBuffer(
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
    // Autonomous Camera Orbit & Synchronized Frame Dispatch
    // -------------------------------------------------------------------------
    [[nodiscard]] SculptureVec3 computeAutoOrbitCamera(
        float radius = 3.8f,
        float elevation = 0.8f,
        float orbitSpeed = 0.25f) const noexcept;

    [[nodiscard]] std::optional<SculpturePipelineFrame> executeCoordinatedTickWithAutoCamera(
        class SculpturePipeline& pipeline,
        float wallDeltaSeconds,
        float cameraRadius = 3.8f,
        float cameraElevation = 0.8f,
        float cameraOrbitSpeed = 0.25f,
        const SculptureSnapshot* externalCpuSnapshot = nullptr,
        const std::vector<float>* externalGpuSpatialBuffer = nullptr
    );

    // Fault-Tolerant Execution Introspection
    [[nodiscard]] std::uint32_t consecutiveExecutionErrors() const noexcept { return consecutiveExecutionErrors_; }
    [[nodiscard]] std::string_view lastExecutionError() const noexcept { return lastExecutionError_; }
    void clearExecutionErrors() noexcept { consecutiveExecutionErrors_ = 0U; lastExecutionError_.clear(); }

    // -------------------------------------------------------------------------
    // Interactive Morph & Transition Control
    // -------------------------------------------------------------------------
    void setTargetParams(
        const SculptureProgramParams& target,
        float durationSeconds = 3.0f,
        MorphEasing easing = MorphEasing::Smoothstep) noexcept;

    void setTargetPreset(
        const SculpturePreset& preset,
        float durationSeconds = 3.0f,
        MorphEasing easing = MorphEasing::Smoothstep) noexcept;

    void setTargetWorldMode(
        const SculptureWorldMode& worldMode,
        float durationSeconds = 3.0f,
        MorphEasing easing = MorphEasing::Smoothstep) noexcept;

    [[nodiscard]] SculptureWorldMode exportActiveWorldMode(
        std::string_view presetId = "active_moment",
        std::string_view revision = "live") const noexcept;

    void snapToTarget() noexcept;

    // Generatively perturbs active parameters smoothly via geodesic morphing
    void mutateParameters(
        float mutationScale = 0.2f,
        float durationSeconds = 2.0f,
        MorphEasing easing = MorphEasing::Smoothstep) noexcept;

    // Advances to the next physical color model and rotates hue
    void randomizePalette(
        float durationSeconds = 1.5f,
        MorphEasing easing = MorphEasing::Smoothstep) noexcept;

    // Cycles through mathematical inner transform maps
    void cycleInnerMap(
        float durationSeconds = 1.0f,
        MorphEasing easing = MorphEasing::Smoothstep) noexcept;

    // Cycles through spatial reduction modes
    void cycleReduction(
        float durationSeconds = 1.0f,
        MorphEasing easing = MorphEasing::Smoothstep) noexcept;

    // Smoothly returns active parameters to an authored baseline preset
    void resetToBaseline(
        const SculptureProgramParams& baseline,
        float durationSeconds = 1.2f,
        MorphEasing easing = MorphEasing::Smoothstep) noexcept;

    // -------------------------------------------------------------------------
    // Curated Tour & Playlist Sequencing
    // -------------------------------------------------------------------------
    bool startCuratedTour(
        std::string_view tourName,
        float durationSeconds = 3.0f,
        float dwellSeconds = 5.0f,
        MorphEasing easing = MorphEasing::Smoothstep);

    bool cycleCuratedTour(int direction = 1);

    // Prepare a sorted directory tour before replacing playback. Unusable input
    // or invalid timing preserves the current tour; success retains its mode.
    bool startDirectoryTour(
        const std::filesystem::path& directoryPath,
        float durationSeconds = 3.0f,
        float dwellSeconds = 5.0f,
        MorphEasing easing = MorphEasing::Smoothstep);

    void stopTour() noexcept;
    void stepTourNext(bool startTransition = true) noexcept;
    void stepTourPrevious(bool startTransition = true) noexcept;
    void toggleTourPause() noexcept;
    void setTourPaused(bool paused) noexcept;
    void setTourPlaybackMode(TourPlaybackMode mode) noexcept { choreographer_.setTourPlaybackMode(mode); }
    [[nodiscard]] TourPlaybackMode tourPlaybackMode() const noexcept { return choreographer_.tourPlaybackMode(); }
    [[nodiscard]] bool isTourActive() const noexcept { return choreographer_.isPlaylistActive(); }
    [[nodiscard]] bool isTourPaused() const noexcept { return choreographer_.isPlaylistPaused(); }
    [[nodiscard]] std::string_view activeTourPresetName() const noexcept { std::string_view tourName = choreographer_.isPlaylistActive() ? choreographer_.currentPlaylistTargetName() : ""; if (!tourName.empty()) return tourName; return activeLoadedPresetName_; }
    [[nodiscard]] float tourDwellProgress() const noexcept { return choreographer_.playlistDwellProgress(); }
    void cycleMorphEasing() noexcept;

    // -------------------------------------------------------------------------
    // Polyphonic Multi-Track Layer Choreography
    // -------------------------------------------------------------------------
    void setMultiTrackEnabled(bool enabled) noexcept { multiTrackEnabled_ = enabled; }
    [[nodiscard]] bool multiTrackEnabled() const noexcept { return multiTrackEnabled_; }
    [[nodiscard]] const SculptureMultiTrackChoreographer& multiTrack() const noexcept { return multiTrack_; }
    [[nodiscard]] SculptureMultiTrackChoreographer& multiTrack() noexcept { return multiTrack_; }

    // -------------------------------------------------------------------------
    // VJ Live Performance Deck & Master Crossfader
    // -------------------------------------------------------------------------
    void setPerformanceDeckEnabled(bool enabled) noexcept;
    [[nodiscard]] bool isPerformanceDeckEnabled() const noexcept { return performanceDeckEnabled_; }
    [[nodiscard]] SculpturePerformanceDeck* performanceDeck() noexcept { return performanceDeck_.get(); }
    [[nodiscard]] const SculpturePerformanceDeck* performanceDeck() const noexcept { return performanceDeck_.get(); }
    [[nodiscard]] SculptureMacroAutomationEngine* macroEngine() noexcept {
        return performanceDeck_ ? &performanceDeck_->macroEngine() : nullptr;
    }
    [[nodiscard]] const SculptureMacroAutomationEngine* macroEngine() const noexcept {
        return performanceDeck_ ? &performanceDeck_->macroEngine() : nullptr;
    }
    void setTourBeatSync(bool enabled, std::uint32_t barsPerPreset = 8, std::uint32_t transitionBars = 2) noexcept;
    [[nodiscard]] bool isTourBeatSyncEnabled() const noexcept;

    // -------------------------------------------------------------------------
    // VJ Live Performance Deck JIT Pre-Warming Handshake
    // -------------------------------------------------------------------------
    using AsyncKernelPrewarmCallback = std::function<void(DeckId deck, const SculptureProgramParams& params)>;
    void setAsyncKernelPrewarmCallback(AsyncKernelPrewarmCallback cb) noexcept;
    [[nodiscard]] bool hasAsyncKernelPrewarmCallback() const noexcept {
        return static_cast<bool>(asyncKernelPrewarmCallback_);
    }
    [[nodiscard]] std::uint32_t deckPrewarmCount() const noexcept { return deckPrewarmCount_; }
    [[nodiscard]] std::optional<DeckId> lastPrewarmedDeck() const noexcept { return lastPrewarmedDeck_; }
    [[nodiscard]] std::uint64_t lastPrewarmedSignature() const noexcept { return lastPrewarmedSignature_; }
    void prewarmDeck(DeckId deck);

    // -------------------------------------------------------------------------
    // Script-Based Autonomous Timeline Sequencer
    // -------------------------------------------------------------------------
    [[nodiscard]] SculptureTimelineSequencer* timelineSequencer() noexcept { return timelineSequencer_.get(); }
    [[nodiscard]] const SculptureTimelineSequencer* timelineSequencer() const noexcept { return timelineSequencer_.get(); }
    bool loadTimelineFromJson(std::string_view jsonStr);
    bool loadTimelineFromFile(const std::filesystem::path& path);
    bool buildTimelineFromTour(std::string_view tourName, float dwellSec = 4.0f, float transSec = 2.0f);
    void playTimeline();
    void pauseTimeline() noexcept;
    void stopTimeline() noexcept;

    // -------------------------------------------------------------------------
    // OpenCL Modular Kernel Specialization
    // -------------------------------------------------------------------------
    [[nodiscard]] std::uint64_t currentKernelSignatureHash() const noexcept;
    [[nodiscard]] std::string currentOpenClBuildOptions() const;

    // -------------------------------------------------------------------------
    // Educational Learn Mode Stage Controls
    // -------------------------------------------------------------------------
    void setStageMask(std::uint32_t mask) noexcept;
    void toggleStage(learn_mode::Stage stage) noexcept;
    void setDiagnosticStage(learn_mode::DiagnosticStage stage) noexcept;
    void cycleDiagnosticStage() noexcept;
    void resetLearnModeStages() noexcept;
    void setLearnStageSolo(learn_mode::Stage stage) noexcept;
    void setLearnStageAccumulative(learn_mode::Stage maxStage) noexcept;
    [[nodiscard]] std::uint32_t stageMask() const noexcept { return stageMask_; }
    [[nodiscard]] learn_mode::DiagnosticStage diagnosticStage() const noexcept { return learn_mode::diagnostic(stageMask_); }
    [[nodiscard]] std::string getLearnModePedagogicalReport() const;
    [[nodiscard]] std::string_view activeStageName(learn_mode::Stage stage) const noexcept;
    [[nodiscard]] std::string_view activeStageFormula(learn_mode::Stage stage) const noexcept;

    // -------------------------------------------------------------------------
    // 4D Foliation Spacetime Slicing & Kuramoto Harmonic Coupling
    // -------------------------------------------------------------------------
    void set4DFoliationWaveEnabled(bool enabled) noexcept { enable4DFoliationWave_ = enabled; }
    [[nodiscard]] bool is4DFoliationWaveEnabled() const noexcept { return enable4DFoliationWave_; }
    void toggle4DFoliationWave() noexcept { enable4DFoliationWave_ = !enable4DFoliationWave_; }
    void setFoliationConfig(const Foliation4DConfig& config) noexcept { foliationConfig_ = config; }
    [[nodiscard]] const Foliation4DConfig& foliationConfig() const noexcept { return foliationConfig_; }
    [[nodiscard]] Foliation4DConfig& foliationConfig() noexcept { return foliationConfig_; }
    void set4DManifoldType(Manifold4DType type) noexcept;
    void cycle4DManifoldType() noexcept;

    // -------------------------------------------------------------------------
    // Variable Resolution Scaling & Adaptive Quality Control
    // -------------------------------------------------------------------------
    // Unknown enum values leave the operator override and resource owners alone.
    void setResolutionScale(StageResolutionScale scale) noexcept;
    void cycleResolutionScale() noexcept;
    [[nodiscard]] StageResolutionScale resolutionScale() const noexcept {
        return effectiveResolutionScale(choreographer_.currentParams().auxiliaryResolutionScale);
    }

    // -------------------------------------------------------------------------
    // Multi-Stage Staged Compute & Adaptive Closed-Loop Resolution Governor
    // -------------------------------------------------------------------------
    void setStagedComputeGovernorEnabled(bool enabled) noexcept { enableStagedGovernor_ = enabled; }
    [[nodiscard]] bool isStagedComputeGovernorEnabled() const noexcept { return enableStagedGovernor_; }
    [[nodiscard]] SculptureStagedComputeGovernor& stagedGovernor() noexcept { return stagedGovernor_; }
    [[nodiscard]] const SculptureStagedComputeGovernor& stagedGovernor() const noexcept { return stagedGovernor_; }

    // -------------------------------------------------------------------------
    // Living Quasi-Periodic Palette Agent & OKLab Color Dynamics
    // -------------------------------------------------------------------------
    void setPaletteAgentEnabled(bool enabled) noexcept { paletteAgentEnabled_ = enabled; }
    [[nodiscard]] bool isPaletteAgentEnabled() const noexcept { return paletteAgentEnabled_; }
    void togglePaletteAgent() noexcept { paletteAgentEnabled_ = !paletteAgentEnabled_; }
    [[nodiscard]] SculpturePaletteAgent& paletteAgent() noexcept { return paletteAgent_; }
    [[nodiscard]] const SculpturePaletteAgent& paletteAgent() const noexcept { return paletteAgent_; }
    [[nodiscard]] SculptureVariableResolutionStage& variableResolutionStage() noexcept { return variableResolutionStage_; }
    [[nodiscard]] const SculptureVariableResolutionStage& variableResolutionStage() const noexcept { return variableResolutionStage_; }
    void setUpsampleFilter(UpsampleFilter filter) noexcept {
        variableResolutionStage_.setFilter(filter);
    }
    [[nodiscard]] UpsampleFilter upsampleFilter() const noexcept {
        return variableResolutionStage_.filter();
    }
    void cycleUpsampleFilter() noexcept;

    // -------------------------------------------------------------------------
    // OpenCL Program Binary Disk Caching & Offline Pre-Warming
    // -------------------------------------------------------------------------
    void startBinaryCachePrewarming(const std::string& kernelPath = "MODULES/3dEpiSculpT/kernels/SculptureGenerate.cl");
    void stopBinaryCachePrewarming() noexcept;
    [[nodiscard]] bool isBinaryCachePrewarming() const noexcept;
    [[nodiscard]] BinaryCacheStats binaryCacheStats() const noexcept {
        return SculptureProgramBinaryCache::stats();
    }

    // -------------------------------------------------------------------------
    // Inspection & State Queries
    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    // HUD Telemetry & Educational Inspection
    // -------------------------------------------------------------------------
    [[nodiscard]] std::string formatHudTelemetry(
        std::string_view pipelineName = "OpenCL->Vulkan",
        std::string_view cameraName = "Default") const;

    [[nodiscard]] std::string getEducationalPipelineSummary() const;

    [[nodiscard]] bool captureSnapshotBundle(
        const std::string& directory,
        std::uint32_t width,
        std::uint32_t height,
        const std::uint8_t* bgraPixels,
        std::string_view presetName = "",
        SnapshotPaths* outPaths = nullptr) const;

    // Zero-Stall Background Live Capture
    // Lazy allocation/thread startup may throw; callers retain error policy.
    [[nodiscard]] SculptureLiveCaptureController& liveCaptureController();
    bool triggerAsyncCapture(
        std::uint32_t width,
        std::uint32_t height,
        std::vector<std::uint8_t> bgraPixels,
        std::uint32_t generation,
        std::string_view presetName = ""
    );

    // Standalone Shader & WebGL2 App Synthesizer & Export
    [[nodiscard]] std::string synthesizeShadertoyShader(const ShaderSynthesisConfig& config = {}) const;
    [[nodiscard]] std::string synthesizeStandaloneHtml(const ShaderSynthesisConfig& config = {}) const;
    bool exportShaderToFile(const std::string& filepath, const ShaderSynthesisConfig& config = {}) const;
    bool exportStandaloneHtmlToFile(const std::string& filepath, const ShaderSynthesisConfig& config = {}) const;

    // -------------------------------------------------------------------------
    // 2.5D Epimorphic Relief Mesh Generator & Non-Euclidean Manifolds
    // -------------------------------------------------------------------------
    [[nodiscard]] field_geometry::ReliefMesh generateActiveReliefMesh(
        std::uint32_t gridRes = 64,
        bool applyPoincare = true
    ) const;

    [[nodiscard]] EpimorphicReliefData generateActiveEpimorphicRelief(
        const EpimorphicReliefConfig& config = {}
    ) const;

    [[nodiscard]] EpimorphicReliefData generateBraidedKnotRelief(
        const KnotHomotopyConfig& knotConfig = {},
        std::size_t numStrands = 3U,
        const EpimorphicReliefConfig& reliefConfig = {}
    ) const;

    bool exportReliefMeshObj(
        const std::string& filepath,
        const EpimorphicReliefConfig& config = {},
        std::string_view objectName = "EpimorphicRelief"
    ) const;

    bool exportReliefMeshStl(
        const std::string& filepath,
        const EpimorphicReliefConfig& config = {},
        bool binary = true,
        std::string_view headerOrSolidName = "EpimorphicRelief"
    ) const;

    [[nodiscard]] const SculptureProgramParams& currentParams() const noexcept;
    [[nodiscard]] SculpturePipelineTelemetry telemetry() const noexcept;
    [[nodiscard]] const SculptureProducerCadence<SculptureProgramParams>& cadence() const noexcept { return cadence_; }
    [[nodiscard]] const SculptureMorphChoreographer& choreographer() const noexcept { return choreographer_; }
    [[nodiscard]] SculptureMorphChoreographer& choreographer() noexcept { return choreographer_; }
    [[nodiscard]] const ImplicitQualityPolicy& qualityPolicy() const noexcept { return qualityPolicy_; }
    [[nodiscard]] const SculptureModulationEngine& modulationEngine() const noexcept { return modulation_; }
    [[nodiscard]] SculptureModulationEngine& modulationEngine() noexcept { return modulation_; }
    [[nodiscard]] std::uint32_t generation() const noexcept { return currentGeneration_; }

    // -------------------------------------------------------------------------
    // Autonomous Sculptural Director & Geodesic Navigation
    // -------------------------------------------------------------------------
    void enableAutoDirector(bool enable, const AutoDirectorConfig& config = {});
    [[nodiscard]] SculptureAutoDirector* autoDirector() noexcept { return autoDirector_.get(); }
    [[nodiscard]] const SculptureAutoDirector* autoDirector() const noexcept { return autoDirector_.get(); }

    // -------------------------------------------------------------------------
    // Deterministic Video Sequence Recording & Frame Archiving
    // -------------------------------------------------------------------------
    [[nodiscard]] SculptureSequenceRecorder* sequenceRecorder() noexcept { return sequenceRecorder_.get(); }
    [[nodiscard]] const SculptureSequenceRecorder* sequenceRecorder() const noexcept { return sequenceRecorder_.get(); }
    bool startSequenceRecording(std::string_view sessionName = "");
    bool stopSequenceRecording(SequenceStopOutcome* outcome = nullptr);
    [[nodiscard]] bool isSequenceRecording() const noexcept;
    bool recordFrame(
        std::uint32_t width,
        std::uint32_t height,
        const std::uint8_t* pixels,
        const SequenceFrameMetadata& metadata = {},
        bool isBgra = true
    );
    bool recordFrameForSession(
        std::uint64_t expectedSession,
        std::uint32_t width,
        std::uint32_t height,
        const std::uint8_t* pixels,
        const SequenceFrameMetadata& metadata = {},
        bool isBgra = true
    );

    // -------------------------------------------------------------------------
    // Fault-Tolerant Parameter Sanitization & Kernel Guard
    // -------------------------------------------------------------------------
    // Eliminates NaNs, infinities, and out-of-bounds divergence before GPU dispatch
    static void sanitizeProgramParams(SculptureProgramParams& params) noexcept;

private:
    friend class SculptureCoordinatorEventRouter;
    friend class SculptureManifoldSynthesisService;

    // -------------------------------------------------------------------------
    // Modular 5-Stage Coordinated Execution Pipeline
    // -------------------------------------------------------------------------
    float stepTimingAndTransport(float wallDeltaSeconds) noexcept;
    SculptureProgramParams composeMasterProgramParams(float effectiveDelta) noexcept;
    void stepDynamicResonance(SculptureProgramParams& params, float deltaSeconds) noexcept;
    void refreshProceduralGeometryControls() noexcept;
    void applyGovernanceAndQuality(SculptureProgramParams& params) noexcept;
    [[nodiscard]] StageResolutionScale effectiveResolutionScale(
        StageResolutionScale authoredScale) const noexcept;
    void stepAudioResonance(SculptureProgramParams& params, float deltaSeconds) noexcept;
    void stepFoliationSpacetime(SculptureProgramParams& params, float deltaSeconds) noexcept;
    void updateTelemetry() noexcept;
    SculpturePipelineFrame buildFrame(const SculptureProgramParams& params, float deltaSeconds) const noexcept;
    
    // Modular Event Bus Bindings (delegated to SculptureCoordinatorEventRouter)
    void bindPlaybackAndTourEvents(SculptureEventBus& eventBus);
    void bindPerformanceDeckEvents(SculptureEventBus& eventBus);
    void bindFileAndSequenceEvents(SculptureEventBus& eventBus);
    void bindModifierEvents(SculptureEventBus& eventBus);

    std::unique_ptr<SculptureSequenceRecorder> sequenceRecorder_{};

    SculpturePipelineCoordinatorConfig config_{};
    const SculpturePresetLibrary* library_ = nullptr;
    SculptureProducerCadence<SculptureProgramParams> cadence_{{}, &equivalentProducerParams};
    SculptureMorphChoreographer choreographer_;
    SculptureMultiTrackChoreographer multiTrack_{};
    bool multiTrackEnabled_ = false;
    ImplicitQualityPolicy qualityPolicy_;
    SculptureModulationEngine modulation_;
    // The modulation engine holds compiled runtime routes; capture needs the
    // accepted text representation so a saved world can replay the same map.
    std::vector<std::string> activeRoutingMap_;
    bool activeRoutingMapInitialized_ = false;
    std::string activeLoadedPresetName_;
    SculptureAudioResonance audioResonance_;
    std::unique_ptr<SculptureAutoDirector> autoDirector_{};
    std::unique_ptr<SculptureLiveCaptureController> liveCapture_{};
    std::unique_ptr<SculpturePerformanceDeck> performanceDeck_{};
    std::unique_ptr<SculptureTimelineSequencer> timelineSequencer_{};
    std::unique_ptr<SculptureProgramBinaryCache::Prewarmer> binaryPrewarmer_{};
    std::vector<std::unique_ptr<ISculptureCoordinatorPlugin>> plugins_;
    bool performanceDeckEnabled_ = false;
    bool enableAudioResonance_ = true;
    bool enable4DFoliationWave_ = true;
    Foliation4DConfig foliationConfig_{};
    SculptureStagedComputeGovernor stagedGovernor_{};
    bool enableStagedGovernor_ = true;
    // A live resolution command is an explicit operator choice until the next
    // authored target is loaded; it never mutates adaptive-governor history.
    std::optional<StageResolutionScale> resolutionScaleOverride_{};
    SculptureVariableResolutionStage variableResolutionStage_{};
    SculpturePaletteAgent paletteAgent_{};
    bool paletteAgentEnabled_ = false;
    std::optional<AudioSignalContext> audioContext_{};
    AudioEnvelopeFollower audioFollower_{};
    void advanceTourOnPerformanceDeck(DeckId targetDeck) noexcept;
    void handleDeckPrewarm(DeckId deck, const SculptureProgramParams& params);

    AsyncKernelPrewarmCallback asyncKernelPrewarmCallback_{};
    std::uint32_t deckPrewarmCount_ = 0U;
    std::optional<DeckId> lastPrewarmedDeck_ = std::nullopt;
    std::uint64_t lastPrewarmedSignature_ = 0ULL;

    std::uint32_t currentGeneration_ = 0U;
    float wallClockAccumulator_ = 0.0f;
    float simulatedTime_ = 0.0f;
    std::uint32_t stageMask_ = learn_mode::kAllStages;
    SculpturePipelineTelemetry telemetry_{};

    std::vector<float> auxiliaryTextureRgba_{};
    std::uint32_t auxiliaryTextureWidth_ = 0U;
    std::uint32_t auxiliaryTextureHeight_ = 0U;
    std::uint64_t auxiliaryTextureRevision_ = 0U;
    std::vector<float> auxiliaryVoxelGrid_{};
    ProceduralGeometrySample proceduralGeometrySample_{};
    std::unique_ptr<SculptureVolumetricMemory> volumetricMemory_{};
    SculptureVolumetricFieldCoupler volumetricCoupler_{};

    SculptureRemoteServer* remoteServer_ = nullptr;
    SculptureEventBus* boundEventBus_ = nullptr;
    std::uint32_t consecutiveExecutionErrors_ = 0U;
    std::string lastExecutionError_{};
};

} // namespace epi_sculpt
