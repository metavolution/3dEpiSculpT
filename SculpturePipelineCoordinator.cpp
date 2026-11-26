/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - SCULPTURE PIPELINE COORDINATOR IMPLEMENTATION
 * ============================================================================
 */

#include "SculpturePipelineCoordinator.h"
#include "SculptureCoordinatorEventRouter.h"
#include "SculptureManifoldSynthesisService.h"
#include "SculptureSpatialMemoryField.h"
#include "SculptureMultirateEngine.h"
#include "SculpturePaletteAgent.h"
#include "SculpturePipeline.h"
#include "SculptureFieldGeometry.h"
#include "SculptureGpuAbi.h"
#include "SculptureSpatialSonifier.h"
#include "SculpturePerformanceDeck.h"
#include "SculptureSerializer.h"
#include "SculptureRemoteServer.h"
#include "Sculpture4DFoliation.h"
#include "SculptureParameterComposer.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>



namespace epi_sculpt {

namespace {
    struct CaptureProvenance {
        std::string presetId = "captured_moment";
        std::string revision = "live";
        std::vector<std::string> routingMap;
    };

    // Capture records the authored recipe separately from the current animated
    // parameters. A preset lookup is deliberately best-effort: live edits stay
    // reproducible as a truthful "live" moment instead of claiming a preset
    // revision that the coordinator cannot identify.
    [[nodiscard]] CaptureProvenance resolveCaptureProvenance(
        const SculpturePresetLibrary* library,
        std::string_view requestedPreset) {
        if (library != nullptr && !requestedPreset.empty()) {
            if (const SculpturePresetAsset* asset = library->find(requestedPreset)) {
                return {
                    .presetId = asset->id,
                    .revision = asset->revision.empty() ? "unversioned" : asset->revision,
                    .routingMap = asset->routingMap,
                };
            }
        }

        if (!requestedPreset.empty()) {
            return {.presetId = std::string(requestedPreset)};
        }
        return {};
    }

    // Larger divisors are coarser and therefore cheaper. Governance may
    // reduce an authored/operator-requested resolution, never increase cost.
    [[nodiscard]] constexpr StageResolutionScale coarserResolutionScale(
        StageResolutionScale first,
        StageResolutionScale second) noexcept {
        return static_cast<std::uint8_t>(first) >= static_cast<std::uint8_t>(second)
            ? first
            : second;
    }

    class LambdaPlugin final : public ISculptureCoordinatorPlugin {
        std::function<void(SculptureProgramParams&, float, float)> func_;
    public:
        explicit LambdaPlugin(std::function<void(SculptureProgramParams&, float, float)> f) : func_(std::move(f)) {}
        void step(SculptureProgramParams& p, float t, float d) override { func_(p, t, d); }
    };
}

SculpturePipelineCoordinator::SculpturePipelineCoordinator(
    SculpturePipelineCoordinatorConfig config,
    const SculpturePresetLibrary* library)
    : config_(config),
      library_(library),
      cadence_(config.cadence, &equivalentProducerParams),
      choreographer_(),
      qualityPolicy_(config.quality),
      enableAudioResonance_(config.enableAudioResonance) {
    if (config_.enableAutoDirector) {
        autoDirector_ = std::make_unique<SculptureAutoDirector>(config_.autoDirectorConfig);
    }
    if (config_.autoStartTour && library_ != nullptr) {
        (void)startCuratedTour(
            config_.defaultTourName,
            config_.defaultTransitionSeconds,
            config_.defaultDwellSeconds);
    }
    
    // 3D Volumetric Epimorphism Feedback Field (Phase 3 Recursive World Memory)
    volumetricMemory_ = std::make_unique<SculptureVolumetricMemory>();

    // Plugins prepare control parameters. Volume production remains explicit in
    // update(), where acceptance decides whether producer cadence is consumed.
    plugins_.push_back(std::make_unique<LambdaPlugin>([this](SculptureProgramParams& p, float t, float d) {
        stepAudioResonance(p, d);
    }));
    plugins_.push_back(std::make_unique<LambdaPlugin>([this](SculptureProgramParams& p, float t, float d) {
        stepFoliationSpacetime(p, d);
    }));
    proceduralGeometrySample_ = {currentParams(), simulatedTime_, choreographer_.progress()};
}

void SculpturePipelineCoordinator::enableAutoDirector(bool enable, const AutoDirectorConfig& config) {
    if (enable) {
        autoDirector_ = std::make_unique<SculptureAutoDirector>(config);
    } else {
        autoDirector_.reset();
    }
}

void SculpturePipelineCoordinator::bindEventBus(SculptureEventBus& eventBus) {
    if (boundEventBus_ == &eventBus) return;
    boundEventBus_ = &eventBus;
    SculptureCoordinatorEventRouter::bindAll(*this, eventBus);
}

void SculpturePipelineCoordinator::bindPlaybackAndTourEvents(SculptureEventBus& eventBus) {
    SculptureCoordinatorEventRouter::bindPlaybackAndTourEvents(*this, eventBus);
}

void SculpturePipelineCoordinator::bindPerformanceDeckEvents(SculptureEventBus& eventBus) {
    SculptureCoordinatorEventRouter::bindPerformanceDeckEvents(*this, eventBus);
}

void SculpturePipelineCoordinator::bindFileAndSequenceEvents(SculptureEventBus& eventBus) {
    SculptureCoordinatorEventRouter::bindFileAndSequenceEvents(*this, eventBus);
}

void SculpturePipelineCoordinator::bindModifierEvents(SculptureEventBus& eventBus) {
    SculptureCoordinatorEventRouter::bindModifierEvents(*this, eventBus);
}

void SculpturePipelineCoordinator::setAuxiliary2DTexture(
    const std::vector<float>& textureRgba,
    std::uint32_t width,
    std::uint32_t height) {
    if (width == 0U || height == 0U || textureRgba.empty()) {
        clearAuxiliary2DTexture();
        return;
    }
    const std::uint64_t pixels = static_cast<std::uint64_t>(width) * height;
    if (pixels > textureRgba.size() / 4U) {
        throw std::invalid_argument("Auxiliary texture requires a complete RGBA field");
    }
    // Prepare both projections before publishing this source's content revision.
    auto texture = textureRgba;
    std::vector<float> voxels;
    field_geometry::sample2DTextureTo3DVoxelGrid(
        texture, width, height, voxels, gpu_abi::kVolumeGridSize);
    variableResolutionStage_.configure(width, height, resolutionScale());
    auxiliaryTextureRgba_.swap(texture);
    auxiliaryVoxelGrid_.swap(voxels);
    auxiliaryTextureWidth_ = width;
    auxiliaryTextureHeight_ = height;
    ++auxiliaryTextureRevision_;
}

void SculpturePipelineCoordinator::clearAuxiliary2DTexture() noexcept {
    auxiliaryTextureRgba_.clear();
    auxiliaryTextureWidth_ = 0U;
    auxiliaryTextureHeight_ = 0U;
    auxiliaryVoxelGrid_.clear();
    ++auxiliaryTextureRevision_;
}

void SculpturePipelineCoordinator::rasterizeAcousticPressureGrid3D(
    std::vector<float>& outVoxelGrid,
    std::uint32_t voxelDim,
    float time
) const noexcept {
    SculptureManifoldSynthesisService::rasterizeAcousticPressureGrid3D(*this, outVoxelGrid, voxelDim, time);
}

IsosurfaceMesh SculpturePipelineCoordinator::generateActiveManifoldMesh(
    Manifold4DType type,
    float scale,
    std::size_t uDivisions,
    std::size_t vDivisions
) const {
    return SculptureManifoldSynthesisService::generateActiveManifoldMesh(*this, type, scale, uDivisions, vDivisions);
}

IsosurfaceMesh SculpturePipelineCoordinator::generateActiveTwistorFibrationMesh(
    std::size_t fiberCount,
    std::size_t pointsPerFiber,
    float ribbonWidth,
    float scale
) const {
    return SculptureManifoldSynthesisService::generateActiveTwistorFibrationMesh(*this, fiberCount, pointsPerFiber, ribbonWidth, scale);
}

std::vector<float> SculpturePipelineCoordinator::sampleActiveKuramotoWaveField(
    const std::vector<SculptureVec3>& samplePoints
) const {
    return SculptureManifoldSynthesisService::sampleActiveKuramotoWaveField(*this, samplePoints);
}

IsosurfaceMesh SculpturePipelineCoordinator::generateActivePolychoronHomotopyMesh(
    PolychoronKind source,
    PolychoronKind target,
    float morphProgress,
    float scale,
    float tubeRadius,
    std::size_t radialSegments
) const {
    return SculptureManifoldSynthesisService::generateActivePolychoronHomotopyMesh(
        *this, source, target, morphProgress, scale, tubeRadius, radialSegments);
}

std::vector<world_gpu_abi::WorldInstanceGpu> SculpturePipelineCoordinator::generateActivePolychoronWorldInstances(
    PolychoronKind source,
    PolychoronKind target,
    float morphProgress,
    float scale
) const {
    return SculptureManifoldSynthesisService::generateActivePolychoronWorldInstances(
        *this, source, target, morphProgress, scale);
}

SculptureGBuffer SculpturePipelineCoordinator::renderActiveImplicitGBuffer(
    std::uint32_t width,
    std::uint32_t height,
    const SculptureVec3& cameraPos,
    const SculptureVec3& cameraTarget,
    const SculptureVec3& cameraUp,
    float fovRadians,
    std::uint32_t threadCount,
    const SculptureVec3* prevCameraPos,
    const SculptureVec3* prevCameraTarget
) const {
    return SculptureManifoldSynthesisService::renderActiveImplicitGBuffer(
        *this, width, height, cameraPos, cameraTarget, cameraUp, fovRadians, threadCount, prevCameraPos, prevCameraTarget);
}

SculptureStereoGBufferPair SculpturePipelineCoordinator::renderActiveStereoImplicitGBuffer(
    std::uint32_t eyeWidth,
    std::uint32_t eyeHeight,
    const SculptureStereoRig& rig,
    const SculptureVec3& cameraPos,
    const SculptureVec3& cameraTarget,
    const SculptureVec3& cameraUp,
    float fovRadians,
    std::uint32_t threadCount,
    const SculptureVec3* prevCameraPos,
    const SculptureVec3* prevCameraTarget
) const {
    return SculptureManifoldSynthesisService::renderActiveStereoImplicitGBuffer(
        *this, eyeWidth, eyeHeight, rig, cameraPos, cameraTarget, cameraUp, fovRadians, threadCount, prevCameraPos, prevCameraTarget);
}

float SculpturePipelineCoordinator::stepTimingAndTransport(float wallDeltaSeconds) noexcept {
    if (!std::isfinite(wallDeltaSeconds) || wallDeltaSeconds < 0.0f) {
        wallDeltaSeconds = 0.0f;
    }

    ++telemetry_.totalFramesCoordinated;
    wallClockAccumulator_ += wallDeltaSeconds;

    const float effectiveDelta = cadence_.paused() ? 0.0f : (wallDeltaSeconds * cadence_.speedMultiplier());
    simulatedTime_ += effectiveDelta;

    if (!cadence_.paused()) {
        choreographer_.update(effectiveDelta);
        if (multiTrackEnabled_) {
            multiTrack_.update(effectiveDelta);
        }
        if (performanceDeckEnabled_ && performanceDeck_) {
            if (timelineSequencer_ && timelineSequencer_->isPlaying()) {
                timelineSequencer_->update(effectiveDelta, *performanceDeck_, library_);
            }
            performanceDeck_->update(effectiveDelta);
        }
    }
    return effectiveDelta;
}

SculptureProgramParams SculpturePipelineCoordinator::composeMasterProgramParams(float /*effectiveDelta*/) noexcept {
    return SculptureParameterComposer::composeMasterProgramParams(
        choreographer_,
        &multiTrack_,
        multiTrackEnabled_,
        performanceDeck_.get(),
        performanceDeckEnabled_,
        stageMask_);
}

void SculpturePipelineCoordinator::stepDynamicResonance(SculptureProgramParams& params, float deltaSeconds) noexcept {
    if (paletteAgentEnabled_) {
        LivingPaletteDynamicsMetrics dynMetrics{};
        if (autoDirector_) {
            const auto t = autoDirector_->telemetry();
            dynMetrics.kineticEnergy = t.currentKineticEnergy;
            dynMetrics.lyapunovIndex = t.lyapunovIndex;
            dynMetrics.topologicalEntropy = t.currentEntropy;
        }
        if (audioContext_.has_value()) {
            dynMetrics.transientIntensity = audioFollower_.smoothed().spectralFlux;
        }
        paletteAgent_.updateWithDynamics(params, deltaSeconds, dynMetrics);
    }
}

void SculpturePipelineCoordinator::applyGovernanceAndQuality(SculptureProgramParams& params) noexcept {
    params.auxiliaryResolutionScale = effectiveResolutionScale(params.auxiliaryResolutionScale);
    if (volumetricMemory_) {
        volumetricMemory_->applyResolutionScale(params.auxiliaryResolutionScale);
    }
}

StageResolutionScale SculpturePipelineCoordinator::effectiveResolutionScale(
    StageResolutionScale authoredScale) const noexcept {
    const StageResolutionScale requested = resolutionScaleOverride_.value_or(authoredScale);
    const StageResolutionScale governed = enableStagedGovernor_
        ? stagedGovernor_.resolutionScale()
        : qualityPolicy_.resolutionScale();
    return coarserResolutionScale(requested, governed);
}

void SculpturePipelineCoordinator::sanitizeProgramParams(SculptureProgramParams& params) noexcept {
    SculptureParameterComposer::sanitizeProgramParams(params);
}

std::optional<SculpturePipelineFrame> SculpturePipelineCoordinator::update(
    float wallDeltaSeconds) noexcept {
    // -------------------------------------------------------------------------
    // [EDUCATIONAL PIPELINE FLOW: MODULAR 5-STAGE COORDINATED DISPATCH]
    // Governed by docu/project-vision.txt lines 43-60:
    //  Stage 1: Chronos & Transport Cadence (Time-dilation, speed mult, pause gating)
    //  Stage 2: Master Composition (Deck A/B, automation curves, multi-track, tour)
    //  Stage 3: Dynamic Agency & Resonance (Auto-Director, Living Palette, Kuramoto)
    //  Stage 4: Closed-Loop Governance & Quality (Adaptive resolution, sanitization)
    //  Stage 5: Frame Synthesis & Archival Capture
    // -------------------------------------------------------------------------
    const float effectiveDelta = stepTimingAndTransport(wallDeltaSeconds);
    SculptureProgramParams params = composeMasterProgramParams(effectiveDelta);
    stepDynamicResonance(params, effectiveDelta);
    applyGovernanceAndQuality(params);

    const auto dispatch = cadence_.prepare(simulatedTime_, params);
    if (!dispatch.submit) {
        cadence_.defer(dispatch);
        return std::nullopt;
    }

    // Apply dynamic modulation matrix (LFOs, chaos, audio spectrum, routing rules)
    if (config_.enableModulation) {
        modulation_.updateAndApply(
            params,
            audioContext_.has_value() ? &(*audioContext_) : nullptr,
            simulatedTime_,
            dispatch.simulationDeltaSeconds);
    }

    // Control preparation may follow offered animation time. It is not a
    // transaction over the retained mathematical field or its accepted cadence.
    for (auto& plugin : plugins_) {
        plugin->step(params, simulatedTime_, dispatch.simulationDeltaSeconds);
    }

    try {
        if (volumetricMemory_) {
            volumetricMemory_->step(params, simulatedTime_, dispatch.simulationDeltaSeconds);
        }
    } catch (...) {
        // The volume rejects before accepting mathematical work. Retain the
        // previous emitted sample/generation and leave this interval due.
        // Fixed facts keep this noexcept rejection path allocation-free.
        cadence_.defer(dispatch);
        ++telemetry_.rejectedVolumeSteps;
        telemetry_.volumeStepRejected = true;
        return std::nullopt;
    }
    cadence_.commit(dispatch);
    telemetry_.volumeStepRejected = false;

    // Final fault-tolerant parameter sanitization to eliminate NaNs before GPU dispatch
    sanitizeProgramParams(params);
    // CPU geometry observes the emitted sample, rather than resampling live time
    // on every presentation/eye. This does not certify native math acceptance.
    proceduralGeometrySample_ = {params, simulatedTime_, choreographer_.progress()};

    ++currentGeneration_;
    ++telemetry_.dispatchesEmitted;

    // Update internal metrics dynamically
    updateTelemetry();

    // Package the final computed frame state for OpenCL/Vulkan dispatch
    return buildFrame(params, dispatch.simulationDeltaSeconds);
}

void SculpturePipelineCoordinator::stepAudioResonance(SculptureProgramParams& params, float deltaSeconds) noexcept {
    if (audioContext_.has_value() && enableAudioResonance_) {
        // [EDUCATIONAL ANNOTATION]
        // This abstracts the Kuramoto oscillator synchronization math into a separate physical step.
        // It couples abstract spectral audio data with the topological "surface curvature" and 
        // "Lyapunov index" of the procedural world, causing visual oscillators to phase-lock 
        // with ambient audio beats. This modularizes audio-reactive physics from the main routing.
        GeometricAudioFeatures geom{};
        geom.surfaceCurvature = 0.5f + 0.5f * std::sin(params.rotorAlpha);
        geom.lyapunovIndex = 0.2f + 0.1f * params.seedPulseAmplitude;

        // Advance fluid acoustic physical simulation with ballistic-smoothed audio
        const AudioSignalContext smoothedAudio = audioFollower_.update(*audioContext_, deltaSeconds);
        
        // Ensure the OpenCL backend receives the realtime audio characteristics for the spatial sonifier
        params.audioSpectrum = {
            smoothedAudio.bass,
            smoothedAudio.mid,
            smoothedAudio.treble,
            smoothedAudio.spectralFlux
        };

        SculptureBoundingVolume bounds{};
        bounds.valid = true;
        const float wScale = std::max(params.worldScale, 0.1f);
        bounds.minBounds = {-wScale, -wScale, -wScale};
        bounds.maxBounds = {wScale, wScale, wScale};
        bounds.center = {0.0f, 0.0f, 0.0f};

        audioResonance_.step(smoothedAudio, geom, bounds, simulatedTime_, deltaSeconds);
        // Feed computed tension back into fractal parameters
        audioResonance_.applyResonanceModulation(params, simulatedTime_, deltaSeconds);
    }
}

void SculpturePipelineCoordinator::stepFoliationSpacetime(SculptureProgramParams& params, float deltaSeconds) noexcept {
    if (!enable4DFoliationWave_ || params.enable4D == 0U) {
        return;
    }

    const float k = (audioContext_.has_value() && enableAudioResonance_)
        ? (audioResonance_.metrics().resonanceQFactor * 0.1f)
        : 1.5f;
    float audioDrive = 1.0f;
    if (audioContext_.has_value() && enableAudioResonance_) {
        const auto& metrics = audioResonance_.metrics();
        audioDrive = 1.0f + metrics.kuramotoOrderR * 1.5f + metrics.spectralEntrainment * 0.8f;
    }

    SculptureParameterComposer::stepKuramotoFoliationWave4D(
        params,
        foliationConfig_,
        simulatedTime_,
        deltaSeconds,
        k,
        audioDrive);
}

void SculpturePipelineCoordinator::set4DManifoldType(Manifold4DType type) noexcept {
    foliationConfig_.manifoldType = type;
    auto p = choreographer_.currentParams();
    p.enable4D = static_cast<std::uint32_t>(type) + 1U;
    setTargetParams(p, 0.4f, MorphEasing::Smoothstep);
}

void SculpturePipelineCoordinator::cycle4DManifoldType() noexcept {
    const auto current = static_cast<std::uint8_t>(foliationConfig_.manifoldType);
    const auto next = static_cast<Manifold4DType>((current + 1U) % 16U);
    set4DManifoldType(next);
}

void SculpturePipelineCoordinator::setPerformanceDeckEnabled(bool enabled) noexcept {
    performanceDeckEnabled_ = enabled;
    if (enabled && !performanceDeck_) {
        performanceDeck_ = std::make_unique<SculpturePerformanceDeck>();
        performanceDeck_->setPrewarmCallback([this](DeckId deck, const SculptureProgramParams& params) {
            handleDeckPrewarm(deck, params);
        });
        performanceDeck_->loadDeck(DeckId::DeckA, choreographer_.currentParams(), "", "Live Deck A");
        performanceDeck_->loadDeck(DeckId::DeckB, choreographer_.currentParams(), "", "Live Deck B");
        performanceDeck_->setTourAdvanceCallback([this](DeckId targetDeck) {
            advanceTourOnPerformanceDeck(targetDeck);
        });
    }
}

void SculpturePipelineCoordinator::setAsyncKernelPrewarmCallback(AsyncKernelPrewarmCallback cb) noexcept {
    asyncKernelPrewarmCallback_ = std::move(cb);
}

void SculpturePipelineCoordinator::prewarmDeck(DeckId deck) {
    if (!performanceDeck_) return;
    const auto& slot = performanceDeck_->deck(deck);
    handleDeckPrewarm(deck, slot.params);
}

void SculpturePipelineCoordinator::handleDeckPrewarm(DeckId deck, const SculptureProgramParams& params) {
    ++deckPrewarmCount_;
    lastPrewarmedDeck_ = deck;

    // Compute deterministic 64-bit kernel specialization signature
    const std::uint64_t signature = SculptureKernelSpecializer::computeSignatureHash(params);
    lastPrewarmedSignature_ = signature;

    // Proactively verify / query disk cache presence
    ProgramBinaryKey key{
        .deviceSignature = "OpenCL",
        .buildOptions = SculptureKernelSpecializer::generateBuildOptions(params),
        .kernelTag = "SculptureGenerate",
        .sourceHash = signature
    };
    (void)SculptureProgramBinaryCache::hasBinaryOnDisk(
        SculptureProgramBinaryCache::kDefaultCacheDir, key);

    // Forward to asynchronous background compiler worker if registered
    if (asyncKernelPrewarmCallback_) {
        asyncKernelPrewarmCallback_(deck, params);
    }
}

void SculpturePipelineCoordinator::setTourBeatSync(
    bool enabled,
    std::uint32_t barsPerPreset,
    std::uint32_t transitionBars) noexcept {
    if (enabled && !performanceDeckEnabled_) {
        setPerformanceDeckEnabled(true);
    }
    if (performanceDeck_) {
        SculpturePerformanceDeck::TourBeatSyncConfig cfg{
            .enabled = enabled,
            .barsPerPreset = barsPerPreset,
            .transitionBars = transitionBars
        };
        performanceDeck_->setTourBeatSyncConfig(cfg);
        choreographer_.setPlaylistPaused(enabled);
    }
}

bool SculpturePipelineCoordinator::isTourBeatSyncEnabled() const noexcept {
    return performanceDeckEnabled_ && performanceDeck_ && performanceDeck_->isTourBeatSyncEnabled();
}

void SculpturePipelineCoordinator::advanceTourOnPerformanceDeck(DeckId targetDeck) noexcept {
    if (!performanceDeck_ || !choreographer_.isPlaylistActive() || choreographer_.playlistSize() == 0) {
        return;
    }
    choreographer_.stepPlaylistNext(false);
    const auto* target = choreographer_.playlistTarget(choreographer_.playlistIndex());
    if (target) {
        performanceDeck_->loadDeck(targetDeck, target->params, "", target->name);
    }
}

bool SculpturePipelineCoordinator::loadTimelineFromJson(std::string_view jsonStr) {
    if (!timelineSequencer_) {
        timelineSequencer_ = std::make_unique<SculptureTimelineSequencer>();
    }
    return timelineSequencer_->loadTimelineFromJson(jsonStr);
}

bool SculpturePipelineCoordinator::loadTimelineFromFile(const std::filesystem::path& path) {
    if (!timelineSequencer_) {
        timelineSequencer_ = std::make_unique<SculptureTimelineSequencer>();
    }
    return timelineSequencer_->loadTimelineFromFile(path);
}

bool SculpturePipelineCoordinator::buildTimelineFromTour(
    std::string_view tourName,
    float dwellSec,
    float transSec
) {
    if (!library_) return false;
    if (!timelineSequencer_) {
        timelineSequencer_ = std::make_unique<SculptureTimelineSequencer>();
    }
    return timelineSequencer_->buildFromTour(*library_, tourName, dwellSec, transSec);
}

void SculpturePipelineCoordinator::playTimeline() {
    if (!performanceDeckEnabled_) {
        setPerformanceDeckEnabled(true);
    }
    if (timelineSequencer_) {
        timelineSequencer_->play();
    }
}

void SculpturePipelineCoordinator::pauseTimeline() noexcept {
    if (timelineSequencer_) {
        timelineSequencer_->pause();
    }
}

void SculpturePipelineCoordinator::stopTimeline() noexcept {
    if (timelineSequencer_) {
        timelineSequencer_->stop();
    }
}

bool SculpturePipelineCoordinator::startSequenceRecording(std::string_view sessionName) {
    if (!sequenceRecorder_) {
        sequenceRecorder_ = std::make_unique<SculptureSequenceRecorder>();
    }
    return sequenceRecorder_->startRecording(sessionName);
}

bool SculpturePipelineCoordinator::stopSequenceRecording(SequenceStopOutcome* outcome) {
    if (!sequenceRecorder_) { if (outcome) *outcome = {}; return false; }
    return sequenceRecorder_->stopRecording(outcome);
}

bool SculpturePipelineCoordinator::isSequenceRecording() const noexcept {
    return sequenceRecorder_ && sequenceRecorder_->isRecording();
}

bool SculpturePipelineCoordinator::recordFrame(
    std::uint32_t width,
    std::uint32_t height,
    const std::uint8_t* pixels,
    const SequenceFrameMetadata& metadata,
    bool isBgra
) {
    if (!sequenceRecorder_ || !sequenceRecorder_->isRecording()) return false;
    return sequenceRecorder_->recordFrame(width, height, pixels, metadata, isBgra);
}

bool SculpturePipelineCoordinator::recordFrameForSession(
    std::uint64_t expectedSession,
    std::uint32_t width,
    std::uint32_t height,
    const std::uint8_t* pixels,
    const SequenceFrameMetadata& metadata,
    bool isBgra
) {
    if (!sequenceRecorder_) return false;
    return sequenceRecorder_->recordFrameForSession(expectedSession, width, height,
                                                     pixels, metadata, isBgra);
}

std::uint64_t SculpturePipelineCoordinator::currentKernelSignatureHash() const noexcept {
    const auto p = performanceDeckEnabled_ && performanceDeck_
        ? performanceDeck_->composeMasterParams()
        : choreographer_.currentParams();
    return SculptureKernelSpecializer::computeSignatureHash(p);
}

std::string SculpturePipelineCoordinator::currentOpenClBuildOptions() const {
    const auto p = performanceDeckEnabled_ && performanceDeck_
        ? performanceDeck_->composeMasterParams()
        : choreographer_.currentParams();
    return SculptureKernelSpecializer::generateBuildOptions(p);
}

void SculpturePipelineCoordinator::updateTelemetry() noexcept {
    telemetry_.currentGeneration = currentGeneration_;
    telemetry_.effectiveCadenceHz = cadence_.effectiveUpdateHz();
    telemetry_.activeRaySteps = qualityPolicy_.raySteps();
    telemetry_.activeResolutionScale = resolutionScale();
    telemetry_.isMorphActive = choreographer_.isActive();
    telemetry_.morphProgress = choreographer_.progress();
    telemetry_.isTourActive = choreographer_.isPlaylistActive();
    telemetry_.tourIndex = choreographer_.playlistIndex();
    telemetry_.tourSize = choreographer_.playlistSize();
    telemetry_.activeStageMask = stageMask_;
    telemetry_.activeDiagnosticStage = learn_mode::diagnostic(stageMask_);
    telemetry_.activeModulationRoutes = modulation_.routes().size();
    telemetry_.hasAudioSignal = audioContext_.has_value();
    telemetry_.kuramotoOrderR = audioResonance_.metrics().kuramotoOrderR;
    telemetry_.instantaneousPressure = audioResonance_.metrics().instantaneousPressure;
    telemetry_.resonanceQFactor = audioResonance_.metrics().resonanceQFactor;
    telemetry_.spectralEntrainment = audioResonance_.metrics().spectralEntrainment;
    telemetry_.speedMultiplier = cadence_.speedMultiplier();
    telemetry_.isPaused = cadence_.paused();

    // -------------------------------------------------------------------------
    // Bidirectional OSC Telemetry Broadcast
    // Transmits real-time engine vitals to TouchOSC / remote monitors at ~15Hz
    // -------------------------------------------------------------------------
    if (remoteServer_ && (currentGeneration_ % 4 == 0)) {
        std::string activeCue{};
        if (timelineSequencer_ && timelineSequencer_->isPlaying()) {
            const auto idx = timelineSequencer_->playheadState().activeCueIndex;
            const auto& cues = timelineSequencer_->sequence().cues;
            if (idx < cues.size()) {
                activeCue = cues[idx].title;
            }
        }
        const float crossfadePos = performanceDeck_ ? performanceDeck_->crossfadePosition() : 0.0f;
        const float bpm = performanceDeck_ ? performanceDeck_->bpm() : 120.0f;
        const bool isRec = sequenceRecorder_ && sequenceRecorder_->isRecording();

        remoteServer_->broadcastTelemetry(
            telemetry_.effectiveCadenceHz,
            choreographer_.progress(),
            simulatedTime_,
            crossfadePos,
            bpm,
            isRec,
            activeCue
        );
    }
}

SculpturePipelineFrame SculpturePipelineCoordinator::buildFrame(const SculptureProgramParams& params, float deltaSeconds) const noexcept {
    SculpturePipelineFrame frame{};
    frame.params = params;
    frame.time = simulatedTime_;
    frame.morph = choreographer_.progress();
    frame.deltaSeconds = deltaSeconds;
    frame.generation = currentGeneration_;
    frame.raySteps = qualityPolicy_.raySteps();
    frame.resolutionScale = params.auxiliaryResolutionScale;
    frame.stageMask = stageMask_;
    frame.diagnosticStage = learn_mode::diagnostic(stageMask_);
    frame.speedMultiplier = cadence_.speedMultiplier();
    frame.isPaused = cadence_.paused();
    frame.isTourActive = choreographer_.isPlaylistActive();
    frame.tourIndex = choreographer_.playlistIndex();
    frame.tourSize = choreographer_.playlistSize();
    return frame;
}


// -----------------------------------------------------------------------------
// [EDUCATIONAL ANNOTATION: FAULT-TOLERANT COORDINATED TICK DISPATCH]
// -----------------------------------------------------------------------------
// This method serves as the unified heartbeat of the 3dEpiSculpTMorph engine.
// It coordinates simulation timing, auxiliary holographic memory ingestion,
// autonomous camera navigation, and GPU pipeline dispatch in a strictly
// fault-tolerant manner. If GPU memory pressure, device loss, or an OpenCL
// runtime glitch throws an exception:
//  1. The error is intercepted and recorded in `lastExecutionError_`.
//  2. Closed-loop GPU latency throttling is notified (simulating severe latency),
//     which automatically downscales resolution (Full -> Half -> Quarter).
//  3. If failures accumulate (e.g. 5 consecutive errors), temporal history is
//     flushed to clear corrupt feedback buffers.
//  4. Frame dispatch gracefully returns std::nullopt without crashing the application.
// -----------------------------------------------------------------------------
std::optional<SculpturePipelineFrame> SculpturePipelineCoordinator::executeCoordinatedTick(
    SculpturePipeline& pipeline,
    float wallDeltaSeconds,
    const SculptureVec3& cameraPos,
    const SculptureSnapshot* externalCpuSnapshot,
    const std::vector<float>* externalGpuSpatialBuffer
) {
    try {
        auto maybeFrame = update(wallDeltaSeconds);
        if (!maybeFrame.has_value()) {
            return std::nullopt;
        }

        const auto& frame = *maybeFrame;
        pipeline.updateParams(frame.params);

        if (audioContext_.has_value()) {
            pipeline.setAudioContext(*audioContext_);
        }

        // Stage A: Auxiliary Spatial Memory Ingestion (2D -> 3D Voxel Field)
        const bool evalAuxiliary = !enableStagedGovernor_ || stagedGovernor_.shouldEvaluateAuxiliaryStage(currentGeneration_);
        if (evalAuxiliary) {
            const StageResolutionScale auxiliaryScale = frame.params.auxiliaryResolutionScale;
            variableResolutionStage_.setResolutionScale(auxiliaryScale);
            if (!auxiliaryTextureRgba_.empty() && auxiliaryTextureWidth_ > 0 && auxiliaryTextureHeight_ > 0) {
                const auto& processedSlice = variableResolutionStage_.processAndUpsample(
                    auxiliaryTextureRgba_,
                    auxiliaryTextureWidth_,
                    auxiliaryTextureHeight_,
                    auxiliaryTextureWidth_,
                    auxiliaryTextureHeight_,
                    auxiliaryScale
                );
                pipeline.spatialMemoryField().injectTextureSlice(
                    processedSlice,
                    auxiliaryTextureWidth_,
                    auxiliaryTextureHeight_,
                    0.0f,
                    0.5f
                );
            }
        }

        // Stage B: Autonomous Geodesic Camera Navigation
        SculptureVec3 effectiveCameraPos = cameraPos;
        if (autoDirector_ != nullptr) {
            // [EDUCATIONAL ANNOTATION: GPU-First Zero-Copy Execution Flow]
            // OpenCL / Vulkan is the authoritative mathematical producer.
            // We pass externalCpuSnapshot directly if available; we deliberately avoid
            // forcing a redundant synchronous 16K-point CPU fractal evaluation inside 
            // the 60+ FPS coordinator loop. When running purely on GPU, AutoDirector
            // computes analytical motion metrics directly from program state.
            autoDirector_->update(
                wallDeltaSeconds,
                *this,
                externalCpuSnapshot,
                audioContext_.has_value() ? &*audioContext_ : nullptr
            );
            if (cameraPos.x == 0.0f && cameraPos.y == 0.0f && cameraPos.z == 0.0f) {
                effectiveCameraPos = autoDirector_->cameraPosition();
            }
        }

        const std::vector<float>* effectiveSpatialBuffer = externalGpuSpatialBuffer;
        if (effectiveSpatialBuffer == nullptr && !auxiliaryVoxelGrid_.empty()) {
            effectiveSpatialBuffer = &auxiliaryVoxelGrid_;
        }

        // Stage C: Primary Raymarch & Volumetric Execution
        pipeline.executeFrameWithView(
            frame.time,
            frame.morph,
            frame.deltaSeconds,
            effectiveCameraPos,
            externalCpuSnapshot,
            effectiveSpatialBuffer
        );

        // Stage D: Closed-Loop Latency Feedback & Quality Governor Throttling
        observeProducerMetrics(pipeline.stats().computeTimeMs, pipeline.stats().frameTimeMs);

        consecutiveExecutionErrors_ = 0U;
        return maybeFrame;
    } catch (const std::exception& error) {
        ++consecutiveExecutionErrors_;
        lastExecutionError_ = error.what();

        // Progressive Fault Recovery & Emergency Hysteresis Cooldown
        if (enableStagedGovernor_) {
            stagedGovernor_.observeGpuLatency(999.0f);
        }
        if (consecutiveExecutionErrors_ == 5U) {
            resetSpeed();
        }

        return std::nullopt;
    } catch (...) {
        ++consecutiveExecutionErrors_;
        lastExecutionError_ = "Unknown pipeline exception";
        if (enableStagedGovernor_) {
            stagedGovernor_.observeGpuLatency(999.0f);
        }
        return std::nullopt;
    }
}

SculptureVec3 SculpturePipelineCoordinator::computeAutoOrbitCamera(
    float radius,
    float elevation,
    float orbitSpeed) const noexcept {
    const float angle = simulatedTime_ * orbitSpeed;
    return {
        radius * std::sin(angle),
        elevation + 0.35f * std::sin(angle * 0.5f),
        radius * std::cos(angle)
    };
}

std::optional<SculpturePipelineFrame> SculpturePipelineCoordinator::executeCoordinatedTickWithAutoCamera(
    SculpturePipeline& pipeline,
    float wallDeltaSeconds,
    float cameraRadius,
    float cameraElevation,
    float cameraOrbitSpeed,
    const SculptureSnapshot* externalCpuSnapshot,
    const std::vector<float>* externalGpuSpatialBuffer
) {
    const SculptureVec3 autoCam = computeAutoOrbitCamera(cameraRadius, cameraElevation, cameraOrbitSpeed);
    return executeCoordinatedTick(
        pipeline,
        wallDeltaSeconds,
        autoCam,
        externalCpuSnapshot,
        externalGpuSpatialBuffer
    );
}

void SculpturePipelineCoordinator::observeProducerMetrics(
    float kernelMilliseconds,
    float totalProducerLatencyMilliseconds) noexcept {
    cadence_.observeProducerLatency(totalProducerLatencyMilliseconds);
    (void)qualityPolicy_.observeKernelMilliseconds(kernelMilliseconds);
    if (enableStagedGovernor_) {
        (void)stagedGovernor_.observeGpuLatency(kernelMilliseconds);
    }
}

void SculpturePipelineCoordinator::setAudioContext(const AudioSignalContext& audio) noexcept {
    audioContext_ = audio;
}

void SculpturePipelineCoordinator::setSynestheticAudioFeedback(const SculptureSpatialSonifier& sonifier) noexcept {
    const auto bands = sonifier.currentSpectralBands();
    AudioSignalContext ctx{};
    ctx.subBass = bands[0];
    ctx.bass = bands[1];
    ctx.mid = bands[2];
    ctx.treble = bands[3];
    ctx.spectralFlux = (bands[0] + bands[1] + bands[2] + bands[3]) * 0.25f;
    setAudioContext(ctx);
}

void SculpturePipelineCoordinator::clearAudioContext() noexcept {
    audioContext_.reset();
    audioResonance_.reset();
}

void SculpturePipelineCoordinator::addModulationRoute(const CompiledModulationRoute& route) {
    modulation_.addRoute(route);
}

bool SculpturePipelineCoordinator::parseAndAddModulationRoute(std::string_view routeString) {
    if (!modulation_.parseAndAddRoute(routeString)) return false;
    activeRoutingMap_.emplace_back(routeString);
    activeRoutingMapInitialized_ = true;
    return true;
}

void SculpturePipelineCoordinator::loadPresetModulationRoutes(const std::vector<std::string>& routingMap) {
    modulation_.clearRoutes();
    activeRoutingMap_.clear();
    activeRoutingMapInitialized_ = true;
    for (const std::string& route : routingMap) {
        // Persist only routes the runtime accepted, so a capture cannot claim
        // to replay malformed authored text that was never active.
        if (modulation_.parseAndAddRoute(route)) activeRoutingMap_.push_back(route);
    }
}

void SculpturePipelineCoordinator::clearModulationRoutes() noexcept {
    modulation_.clearRoutes();
    activeRoutingMap_.clear();
    activeRoutingMapInitialized_ = true;
}

void SculpturePipelineCoordinator::setTargetParams(
    const SculptureProgramParams& target,
    float durationSeconds,
    MorphEasing easing) noexcept {
    resolutionScaleOverride_.reset();
    choreographer_.setTarget(target, durationSeconds, easing);
    if (durationSeconds <= 0.0f) {
        refreshProceduralGeometryControls();
    }
}

void SculpturePipelineCoordinator::setTargetPreset(
    const SculpturePreset& preset,
    float durationSeconds,
    MorphEasing easing) noexcept {
    resolutionScaleOverride_.reset();
    choreographer_.setTargetPreset(preset, durationSeconds, easing);
    if (durationSeconds <= 0.0f) {
        refreshProceduralGeometryControls();
    }
    if (config_.enableModulation) loadPresetModulationRoutes(preset.routingMap);
}

void SculpturePipelineCoordinator::setTargetWorldMode(
    const SculptureWorldMode& worldMode,
    float durationSeconds,
    MorphEasing easing) noexcept {
    resolutionScaleOverride_.reset();
    choreographer_.setTarget(worldMode.params, durationSeconds, easing);
    if (durationSeconds <= 0.0f) {
        refreshProceduralGeometryControls();
    }
    if (config_.enableModulation) loadPresetModulationRoutes(worldMode.routingMap);
}

SculptureWorldMode SculpturePipelineCoordinator::exportActiveWorldMode(
    std::string_view presetId,
    std::string_view revision) const noexcept {
    SculptureWorldMode mode{};
    mode.presetName = std::string(presetId);
    mode.presetRevision = std::string(revision);
    mode.params = (performanceDeckEnabled_ && performanceDeck_)
        ? performanceDeck_->composeMasterParams()
        : choreographer_.currentParams();
    mode.params.auxiliaryResolutionScale = effectiveResolutionScale(
        mode.params.auxiliaryResolutionScale);
    mode.state.time = simulatedTime_;
    mode.state.morph = choreographer_.progress();
    if (performanceDeckEnabled_ && performanceDeck_) {
        mode.macroAutomationJson = performanceDeck_->macroEngine().serializeJson();
    }
    return mode;
}

void SculpturePipelineCoordinator::snapToTarget() noexcept {
    choreographer_.snapToTarget();
    refreshProceduralGeometryControls();
}

void SculpturePipelineCoordinator::refreshProceduralGeometryControls() noexcept {
    // Immediate edits remain responsive even while paused. Hold the last sampled
    // time: repeated control/snap calls cannot become a second animation clock.
    proceduralGeometrySample_.params = currentParams();
    proceduralGeometrySample_.morph = choreographer_.progress();
}

void SculpturePipelineCoordinator::mutateParameters(
    float mutationScale,
    float durationSeconds,
    MorphEasing easing) noexcept {
    SculptureProgramParams params = choreographer_.currentParams();
    mutationScale = std::clamp(mutationScale, 0.01f, 1.0f);

    const float seed = simulatedTime_ + static_cast<float>(currentGeneration_);
    auto perturb = [seed](float base, float scale, float offset) {
        float wave = std::sin(seed * 12.9898f + offset * 78.233f);
        return base + wave * scale;
    };

    params.pre.scaleReal = std::clamp(perturb(params.pre.scaleReal, 0.25f * mutationScale, 1.0f), 0.2f, 3.0f);
    params.pre.scaleImag = std::clamp(perturb(params.pre.scaleImag, 0.15f * mutationScale, 2.0f), -1.5f, 1.5f);
    params.post.scaleReal = std::clamp(perturb(params.post.scaleReal, 0.20f * mutationScale, 3.0f), 0.2f, 2.5f);
    params.post.scaleImag = std::clamp(perturb(params.post.scaleImag, 0.15f * mutationScale, 4.0f), -1.5f, 1.5f);
    params.latentW = std::clamp(perturb(params.latentW, 0.35f * mutationScale, 5.0f), -1.0f, 1.0f);
    params.seedAlpha = std::clamp(perturb(params.seedAlpha, 0.20f * mutationScale, 6.0f), 0.1f, 0.95f);
    params.seedPulseAmplitude = std::clamp(perturb(params.seedPulseAmplitude, 0.25f * mutationScale, 7.0f), 0.0f, 1.0f);
    params.volumetricDensityScale = std::clamp(perturb(params.volumetricDensityScale, 0.30f * mutationScale, 8.0f), 0.5f, 3.0f);
    params.curlStrength = std::clamp(perturb(params.curlStrength, 0.15f * mutationScale, 9.0f), 0.0f, 0.8f);

    // 4D Clifford Rotors & Hyper-dimensional Foliation Perturbations
    params.rotorAlpha = std::fmod(params.rotorAlpha + perturb(0.0f, 0.75f * mutationScale, 10.0f), 6.2831853f);
    params.rotorBeta = std::fmod(params.rotorBeta + perturb(0.0f, 0.65f * mutationScale, 11.0f), 6.2831853f);
    params.rotorTheta4D = std::fmod(params.rotorTheta4D + perturb(0.0f, 0.55f * mutationScale, 12.0f), 6.2831853f);
    params.topologyBlend = std::clamp(perturb(params.topologyBlend, 0.25f * mutationScale, 13.0f), 0.15f, 0.95f);
    params.patternSpeed = std::clamp(perturb(params.patternSpeed, 0.40f * mutationScale, 14.0f), 0.01f, 1.0f);
    params.seedWidth = std::clamp(perturb(params.seedWidth, 0.35f * mutationScale, 15.0f), 0.1f, 2.0f);

    choreographer_.setTarget(params, durationSeconds, easing);
}

void SculpturePipelineCoordinator::randomizePalette(
    float durationSeconds,
    MorphEasing easing) noexcept {
    SculptureProgramParams params = choreographer_.currentParams();
    const std::uint32_t effectCount = static_cast<std::uint32_t>(ColorEffectType::Bioluminescence) + 1U;
    const std::uint32_t nextEffect = (static_cast<std::uint32_t>(params.colorEffect) + 1U) % effectCount;
    params.colorEffect = static_cast<ColorEffectType>(nextEffect);
    params.hueRotation = std::fmod(params.hueRotation + 0.381966f, 1.0f); // Golden ratio hue step
    choreographer_.setTarget(params, durationSeconds, easing);
}

void SculpturePipelineCoordinator::cycleInnerMap(
    float durationSeconds,
    MorphEasing easing) noexcept {
    SculptureProgramParams params = choreographer_.currentParams();
    const std::uint32_t current = static_cast<std::uint32_t>(params.inner);
    const std::uint32_t count = static_cast<std::uint32_t>(InnerMap::KaliSetOrbit) + 1U;
    params.inner = static_cast<InnerMap>((current + 1U) % count);
    choreographer_.setTarget(params, durationSeconds, easing);
}

void SculpturePipelineCoordinator::cycleReduction(
    float durationSeconds,
    MorphEasing easing) noexcept {
    SculptureProgramParams params = choreographer_.currentParams();
    const std::uint32_t current = static_cast<std::uint32_t>(params.reduction);
    const std::uint32_t count = static_cast<std::uint32_t>(ReductionMode::CliffordFoliation) + 1U;
    params.reduction = static_cast<ReductionMode>((current + 1U) % count);
    choreographer_.setTarget(params, durationSeconds, easing);
}

void SculpturePipelineCoordinator::resetToBaseline(
    const SculptureProgramParams& baseline,
    float durationSeconds,
    MorphEasing easing) noexcept {
    resolutionScaleOverride_.reset();
    choreographer_.setTarget(baseline, durationSeconds, easing);
    if (durationSeconds <= 0.0f) {
        choreographer_.snapToTarget();
    }
}

bool SculpturePipelineCoordinator::startCuratedTour(
    std::string_view tourName,
    float durationSeconds,
    float dwellSeconds,
    MorphEasing easing) {
    if (library_ == nullptr) return false;
    const bool ok = library_->buildCuratedTour(
        choreographer_, tourName, durationSeconds, dwellSeconds, easing);
    if (ok) {
        choreographer_.startPlaylist(true);
        if (performanceDeckEnabled_ && performanceDeck_ && choreographer_.playlistSize() > 0) {
            const auto* targetA = choreographer_.playlistTarget(0);
            if (targetA) {
                performanceDeck_->loadDeck(DeckId::DeckA, targetA->params, "", targetA->name);
                performanceDeck_->setCrossfadePosition(0.0f);
            }
            if (choreographer_.playlistSize() > 1) {
                const auto* targetB = choreographer_.playlistTarget(1);
                if (targetB) {
                    performanceDeck_->loadDeck(DeckId::DeckB, targetB->params, "", targetB->name);
                }
            }
        }
    }
    return ok;
}

bool SculpturePipelineCoordinator::cycleCuratedTour(int direction) {
    if (library_ == nullptr) return false;
    const auto tours = library_->availableTours();
    if (tours.empty()) return false;

    static int currentTourIdx = 0;
    if (direction > 0) {
        currentTourIdx = (currentTourIdx + 1) % static_cast<int>(tours.size());
    } else if (direction < 0) {
        currentTourIdx = (currentTourIdx - 1 + static_cast<int>(tours.size())) % static_cast<int>(tours.size());
    }
    const std::string& nextTour = tours[static_cast<std::size_t>(currentTourIdx)];
    std::fprintf(stderr, "[Tour Coordinator] Switching to Curated Tour [%d/%zu]: %s\n",
                 currentTourIdx + 1, tours.size(), nextTour.c_str());
    return startCuratedTour(nextTour, config_.defaultTransitionSeconds, config_.defaultDwellSeconds);
}

bool SculpturePipelineCoordinator::startDirectoryTour(
    const std::filesystem::path& directoryPath,
    float durationSeconds,
    float dwellSeconds,
    MorphEasing easing) {
    if (!std::isfinite(durationSeconds) || durationSeconds < 0.0f
        || !std::isfinite(dwellSeconds) || dwellSeconds < 0.0f) return false;

    // IO and decoding may fail after finding some targets. Keep the live tour
    // intact until a complete candidate is ready, including its playback mode.
    SculptureMorphChoreographer prepared(choreographer_.currentParams());
    prepared.setTourPlaybackMode(choreographer_.tourPlaybackMode());
    const std::size_t count = prepared.addPlaylistDirectory(
        directoryPath, durationSeconds, dwellSeconds, easing);
    if (count == 0 || !std::isfinite(prepared.splineTotalDuration())) return false;

    if (!prepared.tryStartPlaylistFrom(0, true)) return false;
    static_assert(std::is_nothrow_move_assignable_v<SculptureMorphChoreographer>);
    choreographer_ = std::move(prepared);
    std::fprintf(stderr, "[Tour Coordinator] Started Directory Tour (%zu presets) from: %s\n",
                 count, directoryPath.c_str());
    return true;
}

void SculpturePipelineCoordinator::stopTour() noexcept {
    choreographer_.stopPlaylist();
    if (performanceDeckEnabled_ && performanceDeck_) {
        performanceDeck_->cancelAutoCrossfade();
        auto cfg = performanceDeck_->tourBeatSyncConfig();
        cfg.enabled = false;
        performanceDeck_->setTourBeatSyncConfig(cfg);
    }
}

void SculpturePipelineCoordinator::stepTourNext(bool startTransition) noexcept {
    if (performanceDeckEnabled_ && performanceDeck_ && choreographer_.isPlaylistActive() && choreographer_.playlistSize() > 0) {
        choreographer_.stepPlaylistNext(false);
        const auto* target = choreographer_.playlistTarget(choreographer_.playlistIndex());
        if (target) {
            const DeckId targetDeck = (performanceDeck_->crossfadePosition() <= 0.5f) ? DeckId::DeckB : DeckId::DeckA;
            performanceDeck_->loadDeck(targetDeck, target->params, "", target->name);
            if (startTransition) {
                if (performanceDeck_->isTourBeatSyncEnabled()) {
                    performanceDeck_->triggerBeatSyncedCrossfade(
                        performanceDeck_->tourBeatSyncConfig().transitionBars, targetDeck, true);
                } else {
                    performanceDeck_->triggerAutoCrossfade(
                        target->durationSeconds > 0.0f ? target->durationSeconds : 3.0f, targetDeck);
                }
            }
        }
        return;
    }
    choreographer_.stepPlaylistNext(startTransition);
}

void SculpturePipelineCoordinator::stepTourPrevious(bool startTransition) noexcept {
    if (performanceDeckEnabled_ && performanceDeck_ && choreographer_.isPlaylistActive() && choreographer_.playlistSize() > 0) {
        choreographer_.stepPlaylistPrevious(false);
        const auto* target = choreographer_.playlistTarget(choreographer_.playlistIndex());
        if (target) {
            const DeckId targetDeck = (performanceDeck_->crossfadePosition() <= 0.5f) ? DeckId::DeckB : DeckId::DeckA;
            performanceDeck_->loadDeck(targetDeck, target->params, "", target->name);
            if (startTransition) {
                if (performanceDeck_->isTourBeatSyncEnabled()) {
                    performanceDeck_->triggerBeatSyncedCrossfade(
                        performanceDeck_->tourBeatSyncConfig().transitionBars, targetDeck, true);
                } else {
                    performanceDeck_->triggerAutoCrossfade(
                        target->durationSeconds > 0.0f ? target->durationSeconds : 3.0f, targetDeck);
                }
            }
        }
        return;
    }
    choreographer_.stepPlaylistPrevious(startTransition);
}

void SculpturePipelineCoordinator::toggleTourPause() noexcept {
    choreographer_.togglePlaylistPause();
}

void SculpturePipelineCoordinator::setTourPaused(bool paused) noexcept {
    choreographer_.setPlaylistPaused(paused);
}

void SculpturePipelineCoordinator::cycleMorphEasing() noexcept {
    auto env = choreographer_.envelopeConfig();
    auto nextEasing = static_cast<MorphEasing>((static_cast<std::uint8_t>(env.easing) + 1U) % kMorphEasingCount);
    env.easing = nextEasing;
    choreographer_.setEnvelopeConfig(env);
}

void SculpturePipelineCoordinator::setStageMask(std::uint32_t mask) noexcept {
    stageMask_ = learn_mode::normalize(mask);
}

void SculpturePipelineCoordinator::toggleStage(learn_mode::Stage stage) noexcept {
    stageMask_ ^= learn_mode::bit(stage);
}

void SculpturePipelineCoordinator::setDiagnosticStage(
    learn_mode::DiagnosticStage stage) noexcept {
    stageMask_ = learn_mode::withDiagnostic(stageMask_, stage);
}

void SculpturePipelineCoordinator::cycleDiagnosticStage() noexcept {
    stageMask_ = learn_mode::cycleDiagnostic(stageMask_);
}

void SculpturePipelineCoordinator::resetLearnModeStages() noexcept {
    stageMask_ = learn_mode::withAllStages(0U);
}

void SculpturePipelineCoordinator::setLearnStageSolo(learn_mode::Stage stage) noexcept {
    stageMask_ = learn_mode::withDiagnostic(learn_mode::maskForSoloStage(stage), learn_mode::DiagnosticStage::FullPipeline);
}

void SculpturePipelineCoordinator::setLearnStageAccumulative(learn_mode::Stage maxStage) noexcept {
    stageMask_ = learn_mode::withDiagnostic(learn_mode::maskForAccumulativeStage(maxStage), learn_mode::DiagnosticStage::FullPipeline);
}

void SculpturePipelineCoordinator::setResolutionScale(StageResolutionScale scale) noexcept {
    if (!isValidStageResolutionScale(scale)) return;
    resolutionScaleOverride_ = scale;
    // A live command is an operator override, not a mutation of adaptive
    // governor history.  Keeping those concerns separate prevents an old
    // manual tier from leaking into the next loaded preset/world.
    const StageResolutionScale effective = effectiveResolutionScale(
        choreographer_.currentParams().auxiliaryResolutionScale);
    variableResolutionStage_.setResolutionScale(effective);
    if (volumetricMemory_) {
        volumetricMemory_->applyResolutionScale(effective);
    }
}

void SculpturePipelineCoordinator::cycleResolutionScale() noexcept {
    switch (resolutionScale()) {
    case StageResolutionScale::Full:
        setResolutionScale(StageResolutionScale::Half);
        break;
    case StageResolutionScale::Half:
        setResolutionScale(StageResolutionScale::Quarter);
        break;
    case StageResolutionScale::Quarter:
        setResolutionScale(StageResolutionScale::Eighth);
        break;
    case StageResolutionScale::Eighth:
        setResolutionScale(StageResolutionScale::Full);
        break;
    }
}

void SculpturePipelineCoordinator::cycleUpsampleFilter() noexcept {
    const auto next = static_cast<UpsampleFilter>(
        (static_cast<std::uint8_t>(variableResolutionStage_.filter()) + 1U) % 3U);
    variableResolutionStage_.setFilter(next);
}

std::string SculpturePipelineCoordinator::getLearnModePedagogicalReport() const {
    return learn_mode::formatFullPedagogicalReport(stageMask_);
}

std::string_view SculpturePipelineCoordinator::activeStageName(learn_mode::Stage stage) const noexcept {
    return learn_mode::stageName(stage);
}

std::string_view SculpturePipelineCoordinator::activeStageFormula(learn_mode::Stage stage) const noexcept {
    return learn_mode::stageMathematicalFormula(stage);
}

const SculptureProgramParams& SculpturePipelineCoordinator::currentParams() const noexcept {
    return choreographer_.currentParams();
}

SculpturePipelineTelemetry SculpturePipelineCoordinator::telemetry() const noexcept {
    SculpturePipelineTelemetry t = telemetry_;
    t.currentGeneration = currentGeneration_;
    t.effectiveCadenceHz = cadence_.effectiveUpdateHz();
    t.activeRaySteps = qualityPolicy_.raySteps();
    t.activeResolutionScale = resolutionScale();
    t.isMorphActive = choreographer_.isActive();
    t.morphProgress = choreographer_.progress();
    t.isTourActive = choreographer_.isPlaylistActive();
    t.tourIndex = choreographer_.playlistIndex();
    t.tourSize = choreographer_.playlistSize();
    t.activeStageMask = stageMask_;
    t.activeDiagnosticStage = learn_mode::diagnostic(stageMask_);
    t.activeModulationRoutes = modulation_.routes().size();
    t.hasAudioSignal = audioContext_.has_value();
    t.kuramotoOrderR = audioResonance_.metrics().kuramotoOrderR;
    t.instantaneousPressure = audioResonance_.metrics().instantaneousPressure;
    t.resonanceQFactor = audioResonance_.metrics().resonanceQFactor;
    t.spectralEntrainment = audioResonance_.metrics().spectralEntrainment;
    t.speedMultiplier = cadence_.speedMultiplier();
    t.isPaused = cadence_.paused();
    return t;
}

std::string SculpturePipelineCoordinator::formatHudTelemetry(
    std::string_view pipelineName,
    std::string_view cameraName) const {
    char baseStatus[256];
    std::snprintf(baseStatus, sizeof(baseStatus),
                  "[%s]  |  CAM: %.*s  |  %s%.2fx",
                  pipelineName.data(),
                  static_cast<int>(cameraName.size()), cameraName.data(),
                  cadence_.paused() ? "PAUSED  |  " : "", cadence_.speedMultiplier());

    std::string result(baseStatus);

    const auto resScale = resolutionScale();
    if (resScale != StageResolutionScale::Full) {
        char resStr[96];
        std::snprintf(resStr, sizeof(resStr), "  |  RES: %s (%s)", name(resScale).data(),
                      upsampleFilter() == UpsampleFilter::CatmullRomBicubic ? "CatmullRom" :
                      (upsampleFilter() == UpsampleFilter::SmoothstepBilinear ? "Smoothstep" : "Bilinear"));
        result += resStr;
    }

    if (choreographer_.isPlaylistActive()) {
        char tourStr[256];
        const auto targetName = choreographer_.currentPlaylistTargetName();
        std::snprintf(tourStr, sizeof(tourStr), "  |  TOUR [%zu/%zu%s%.*s] %s(morph %.0f%% | dwell %.0f%%)",
                      choreographer_.playlistIndex() + 1U,
                      choreographer_.playlistSize(),
                      targetName.empty() ? "" : ": ",
                      static_cast<int>(targetName.size()), targetName.data(),
                      choreographer_.isPlaylistPaused() ? "PAUSED " : "",
                      choreographer_.progress() * 100.0f,
                      choreographer_.playlistDwellProgress() * 100.0f);
        result += tourStr;
    }

    if (audioContext_.has_value() && enableAudioResonance_) {
        char audioResStr[128];
        std::snprintf(audioResStr, sizeof(audioResStr),
                      "  |  ACOUSTIC RES [Q:%.1f, R:%.2f, Entrain:%.2f]",
                      audioResonance_.metrics().resonanceQFactor,
                      audioResonance_.metrics().kuramotoOrderR,
                      audioResonance_.metrics().spectralEntrainment);
        result += audioResStr;
    }

    const auto& cp = choreographer_.currentParams();
    if (cp.enable4D > 0U) {
        char foliationStr[160];
        std::snprintf(foliationStr, sizeof(foliationStr),
                      "  |  4D FOLIATION [%s: %s (a:%.2f, b:%.2f, t:%.2f)]",
                      enable4DFoliationWave_ ? "DYNAMIC" : "STATIC",
                      name(foliationConfig_.manifoldType).data(),
                      cp.rotorAlpha, cp.rotorBeta, cp.rotorTheta4D);
        result += foliationStr;
    }

    // 6-stage learn mode decomposition
    const auto activeMask = stageMask_ & learn_mode::kAllStages;
    if (activeMask != learn_mode::kAllStages || learn_mode::diagnostic(stageMask_) != learn_mode::DiagnosticStage::FullPipeline) {
        char learnStr[512];
        const auto diag = learn_mode::diagnostic(stageMask_);
        std::string formulaAddendum;
        for (std::size_t s = 0; s < learn_mode::kStageCount; ++s) {
            auto stg = static_cast<learn_mode::Stage>(s);
            if (activeMask == learn_mode::bit(stg)) {
                formulaAddendum = std::string("  |  Formula: ") + std::string(learn_mode::stageMathematicalFormula(stg));
                break;
            }
        }
        std::snprintf(learnStr, sizeof(learnStr),
                      "\n[LEARN MODE] 1:Xform(%c) 2:Reduce(%c) 3:Smpl(%c) 4:Color(%c) 5:Seed(%c) 6:Blend(%c)  |  Diag: %s%s",
                      learn_mode::enabled(stageMask_, learn_mode::Stage::Transform) ? '+' : '-',
                      learn_mode::enabled(stageMask_, learn_mode::Stage::Reduce) ? '+' : '-',
                      learn_mode::enabled(stageMask_, learn_mode::Stage::FeedbackSample) ? '+' : '-',
                      learn_mode::enabled(stageMask_, learn_mode::Stage::Color) ? '+' : '-',
                      learn_mode::enabled(stageMask_, learn_mode::Stage::Seed) ? '+' : '-',
                      learn_mode::enabled(stageMask_, learn_mode::Stage::Blend) ? '+' : '-',
                      learn_mode::diagnosticDescription(diag),
                      formulaAddendum.c_str());
        result += learnStr;
    }

    if (performanceDeckEnabled_ && performanceDeck_) {
        result += "\n[VJ DECK] " + performanceDeck_->formatDeckStatus();
        if (deckPrewarmCount_ > 0U) {
            char prewarmStr[96];
            const char* dName = (lastPrewarmedDeck_ == DeckId::DeckA) ? "A" : (lastPrewarmedDeck_ == DeckId::DeckB ? "B" : "?");
            std::snprintf(prewarmStr, sizeof(prewarmStr),
                          " | JIT-Prewarm:%u[Deck %s]",
                          deckPrewarmCount_, dName);
            result += prewarmStr;
        }
        if (timelineSequencer_ && timelineSequencer_->isPlaying()) {
            result += " | " + timelineSequencer_->formatHudString();
        }
    }

    if (enableStagedGovernor_) {
        result += "\n[STAGED COMPUTE] " + stagedGovernor_.formatHudStatus();
    }

    if (paletteAgentEnabled_) {
        result += "\n" + paletteAgent_.formatHudStatus();
    }

    if (sequenceRecorder_ && sequenceRecorder_->isRecording()) {
        result += "\n" + sequenceRecorder_->formatHudIndicator();
    }

    return result;
}

std::string SculpturePipelineCoordinator::getEducationalPipelineSummary() const {
    const auto& p = currentParams();
    std::ostringstream ss;
    ss << "╔═══════════════════════════════════════════════════════════════════════╗\n";
    ss << "║     EPIMORPHIC 3D MATHEMATICAL PIPELINE & ARCHITECTURAL SUMMARY       ║\n";
    ss << "╠═══════════════════════════════════════════════════════════════════════╣\n";
    ss << "║ 1. PRE-AFFINE TRANSFORM:   (" << p.pre.scaleReal << " + " << p.pre.scaleImag << "i) * z + (" << p.pre.offsetReal << " + " << p.pre.offsetImag << "i)\n";
    ss << "║ 2. INNER NONLINEAR MAPS:   A: " << name(p.inner) << "  |  B: " << name(p.innerB) << " (Blend: " << p.topologyBlend << ")\n";
    ss << "║ 3. POST-AFFINE TRANSFORM:  (" << p.post.scaleReal << " + " << p.post.scaleImag << "i) * z + (" << p.post.offsetReal << " + " << p.post.offsetImag << "i)\n";
    ss << "║ 4. DOMAIN REDUCTION:       " << name(p.reduction) << "\n";
    ss << "║ 5. SEED STAGES:            A: " << name(p.seedType) << "  |  B: " << name(p.seedTypeB) << " (Width: " << p.seedWidth << ", Alpha: " << p.seedAlpha << ")\n";
    ss << "║ 6. OPTICAL COLOR MODEL:    " << name(p.colorEffect) << " (HueSpeed: " << p.hueSpeed << ", ChromaBreath: " << p.chromaBreath << ")\n";
    ss << "╟───────────────────────────────────────────────────────────────────────╢\n";
    ss << "║ 4D HYPER-COORDINATES:      W: " << p.latentW << "  |  4D Manifold Mode: " << p.enable4D << "  |  Rotors (a:" << p.rotorAlpha << ", b:" << p.rotorBeta << ", t4D:" << p.rotorTheta4D << ")\n";
    if (p.enable4D > 0U) {
        ss << "║ 4D FOLIATION WAVE:         " << (enable4DFoliationWave_ ? "DYNAMIC (KURAMOTO SYNC)" : "STATIC") << "  |  Manifold: " << name(foliationConfig_.manifoldType) << "\n";
    }
    ss << "║ PARTICIPATING ATMOSPHERE:  DensityScale: " << p.volumetricDensityScale << "  |  Anisotropy (g): " << p.fogAnisotropy << "\n";
    ss << "║ FLUID & ADVECTION:         Advection: " << p.advectionSpeed << "  |  Curl: " << p.curlStrength << "  |  Viscosity: " << p.kineticViscosity << "\n";
    ss << "║ QUALITY & CADENCE:         RaySteps: " << qualityPolicy_.raySteps() << "  |  ResScale: " << name(resolutionScale()) << "  |  Speed: " << cadence_.speedMultiplier() << "x\n";
    if (enableStagedGovernor_) {
        ss << "║ STAGED COMPUTE GOVERNOR:   " << stagedGovernor_.formatHudStatus() << "\n";
    }
    if (audioContext_.has_value()) {
        ss << "║ ACOUSTIC ENTRAINMENT:      Q: " << audioResonance_.metrics().resonanceQFactor << "  |  Kuramoto R: " << audioResonance_.metrics().kuramotoOrderR << "  |  Entrainment: " << audioResonance_.metrics().spectralEntrainment << "\n";
    }
    ss << "╚═══════════════════════════════════════════════════════════════════════╝";
    return ss.str();
}

bool SculpturePipelineCoordinator::captureSnapshotBundle(
    const std::string& directory,
    std::uint32_t width,
    std::uint32_t height,
    const std::uint8_t* bgraPixels,
    std::string_view presetName,
    SnapshotPaths* outPaths) const {
    const std::string_view capturePreset = presetName.empty()
        ? activeTourPresetName() : presetName;
    CaptureProvenance provenance = resolveCaptureProvenance(library_, capturePreset);
    if (activeRoutingMapInitialized_) provenance.routingMap = activeRoutingMap_;
    const auto paths = SculptureStateRecorder::generatePaths(directory, provenance.presetId);
    if (outPaths != nullptr) {
        *outPaths = paths;
    }

    SculptureWorldState state{
        .version = 1,
        .time = simulatedTime_,
        .morph = choreographer_.progress(),
        .cameraAngle = 0.0f
    };

    SculptureProgramParams captureParams = (performanceDeckEnabled_ && performanceDeck_)
        ? performanceDeck_->composeMasterParams()
        : currentParams();
    captureParams.auxiliaryResolutionScale = effectiveResolutionScale(
        captureParams.auxiliaryResolutionScale);

    SculptureWorldMode worldMode = SculptureStateRecorder::makeCaptureWorldMode(
        state,
        provenance.presetId,
        provenance.revision,
        captureParams,
        provenance.routingMap
    );

    if (performanceDeckEnabled_ && performanceDeck_) {
        worldMode.macroAutomationJson = performanceDeck_->macroEngine().serializeJson();
    }

    return SculptureStateRecorder::writeSnapshot(
        paths.pngPath,
        paths.worldPath,
        paths.epimodePath,
        paths.epiPath,
        width,
        height,
        bgraPixels,
        worldMode
    );
}

SculptureLiveCaptureController& SculpturePipelineCoordinator::liveCaptureController() {
    if (!liveCapture_) {
        liveCapture_ = std::make_unique<SculptureLiveCaptureController>("snapshots");
    }
    return *liveCapture_;
}

bool SculpturePipelineCoordinator::triggerAsyncCapture(
    std::uint32_t width,
    std::uint32_t height,
    std::vector<std::uint8_t> bgraPixels,
    std::uint32_t generation,
    std::string_view presetName
) {
    SculptureWorldState state{
        .version = 1,
        .time = simulatedTime_,
        .morph = choreographer_.progress(),
        .cameraAngle = 0.0f
    };

    const std::string_view capturePreset = presetName.empty()
        ? activeTourPresetName() : presetName;
    CaptureProvenance provenance = resolveCaptureProvenance(library_, capturePreset);
    if (activeRoutingMapInitialized_) provenance.routingMap = activeRoutingMap_;
    auto req = SculptureLiveCaptureController::makeSpacebarRequest(
        state,
        provenance.presetId,
        provenance.revision,
        (performanceDeckEnabled_ && performanceDeck_) ? performanceDeck_->composeMasterParams() : currentParams(),
        provenance.routingMap,
        width,
        height,
        std::move(bgraPixels),
        "snapshots"
    );
    req.worldMode.canonicalFeedbackGeneration = generation;

    if (performanceDeckEnabled_ && performanceDeck_) {
        req.worldMode.macroAutomationJson = performanceDeck_->macroEngine().serializeJson();
    }

    return liveCaptureController().enqueueCapture(std::move(req));
}

field_geometry::ReliefMesh SculpturePipelineCoordinator::generateActiveReliefMesh(
    std::uint32_t gridRes,
    bool applyPoincare
) const {
    return SculptureManifoldSynthesisService::generateActiveReliefMesh(*this, gridRes, applyPoincare);
}

EpimorphicReliefData SculpturePipelineCoordinator::generateActiveEpimorphicRelief(
    const EpimorphicReliefConfig& config
) const {
    return SculptureManifoldSynthesisService::generateActiveEpimorphicRelief(*this, config);
}

ReliefMeshRequest SculpturePipelineCoordinator::describeEpimorphicRelief(
    const EpimorphicReliefConfig& config) const noexcept {
    return SculptureManifoldSynthesisService::describeEpimorphicRelief(*this, config);
}

EpimorphicReliefData SculpturePipelineCoordinator::generateEpimorphicRelief(
    const ReliefMeshRequest& request) const {
    return SculptureManifoldSynthesisService::generateEpimorphicRelief(*this, request);
}

EpimorphicReliefData SculpturePipelineCoordinator::generateBraidedKnotRelief(
    const KnotHomotopyConfig& knotConfig,
    std::size_t numStrands,
    const EpimorphicReliefConfig& reliefConfig
) const {
    return SculptureManifoldSynthesisService::generateBraidedKnotRelief(*this, knotConfig, numStrands, reliefConfig);
}

std::string SculpturePipelineCoordinator::synthesizeShadertoyShader(const ShaderSynthesisConfig& config) const {
    return SculptureShaderSynthesizer::synthesizeShadertoyGLSL(currentParams(), config);
}

std::string SculpturePipelineCoordinator::synthesizeStandaloneHtml(const ShaderSynthesisConfig& config) const {
    return SculptureShaderSynthesizer::synthesizeStandaloneHTML5(currentParams(), config);
}

bool SculpturePipelineCoordinator::exportShaderToFile(const std::string& filepath, const ShaderSynthesisConfig& config) const {
    return SculptureShaderSynthesizer::exportToFile(filepath, currentParams(), config);
}

bool SculpturePipelineCoordinator::exportStandaloneHtmlToFile(const std::string& filepath, const ShaderSynthesisConfig& config) const {
    return SculptureShaderSynthesizer::exportHtmlToFile(filepath, currentParams(), config);
}

bool SculpturePipelineCoordinator::exportReliefMeshObj(
    const std::string& filepath,
    const EpimorphicReliefConfig& config,
    std::string_view objectName
) const {
    return SculptureManifoldSynthesisService::exportReliefMeshObj(*this, filepath, config, objectName);
}

bool SculpturePipelineCoordinator::exportReliefMeshStl(
    const std::string& filepath,
    const EpimorphicReliefConfig& config,
    bool binary,
    std::string_view headerOrSolidName
) const {
    return SculptureManifoldSynthesisService::exportReliefMeshStl(*this, filepath, config, binary, headerOrSolidName);
}

void SculpturePipelineCoordinator::startBinaryCachePrewarming(const std::string& kernelPath) {
    if (!library_) return;
    if (!binaryPrewarmer_) {
        binaryPrewarmer_ = std::make_unique<SculptureProgramBinaryCache::Prewarmer>();
    }
    binaryPrewarmer_->start(*library_, kernelPath);
}

void SculpturePipelineCoordinator::stopBinaryCachePrewarming() noexcept {
    if (binaryPrewarmer_) {
        binaryPrewarmer_->stop();
    }
}

bool SculpturePipelineCoordinator::isBinaryCachePrewarming() const noexcept {
    return binaryPrewarmer_ && binaryPrewarmer_->isRunning();
}

} // namespace epi_sculpt
