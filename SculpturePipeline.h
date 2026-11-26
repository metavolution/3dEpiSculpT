#pragma once

#include "SculptureProgram.h"
#include "SculpturePresentationTypes.h"
#include "SculptureGeometryCadence.h"
#include "SculptureAudio.h"
#include "SculptureMorphChoreographer.h"
#include "SculptureLodEngine.h"
#include "SculptureModulationEngine.h"
#include "SculptureTelemetry.h"
#include "SculptureAudioResonance.h"

#include <string_view>
#include <memory>
#include <cstdint>
#include <cstddef>
#include <chrono>

namespace epi_sculpt {

class SculptureRaymarcher;
class SculptureSpatialSonifier;
class SculptureSpatialLBVH;
class SculptureSpatialGestureEngine;
class SculptureMultirateEngine;
class SculptureVolumetricAtmosphere;
class SculptureSpatialMemoryField;

struct SculptureRaymarchConfig;
struct SculptureGBuffer;
struct SculptureStereoGBufferPair;
struct SculptureStereoRig;
struct VolumetricLightTransportConfig;
enum class MediumProfile : std::uint8_t;
struct RayHitResult;

// -----------------------------------------------------------------------------
// Educational Annotation: UNIFIED 3D EPIMORPHIC SCULPTURE PIPELINE
// -----------------------------------------------------------------------------
// This pipeline preserves the control-plane vocabulary of the 6-stage
// mathematical lift. The active production data path is OpenCL computation
// followed by Vulkan presentation; legacy CPU/OpenMP descriptions below are
// historical tooling concepts, not parallel live math backends.
//
//   [ epimorphism parameters ] -> [ OpenCL field/instance/world producer ]
//                                  -> [ immutable delivery ]
//                                  -> [ Vulkan presentation strategy ]
// -----------------------------------------------------------------------------

struct PipelineStats {
    float frameTimeMs = 0.0f;
    float computeTimeMs = 0.0f;
    float renderPrepTimeMs = 0.0f;
    std::size_t activeInstances = 0;
    std::size_t activeTriangles = 0;
    std::uint64_t currentGeneration = 0;
    RenderStrategy activeStrategy = RenderStrategy::PointInstances;
    SculptureLodStats lodStats{};
    SculptureStageTelemetry stageTelemetry{};
    bool openClActive = false;
    bool fallbackCpuUsed = false;
};
// -----------------------------------------------------------------------------
// Educational Annotation: PURE EXECUTION PIPELINE (STATE SEPARATION)
// -----------------------------------------------------------------------------
// The application/coordinator owns authored animation, choreography and palette
// evolution, then supplies its composed recipe through updateParams(). This host
// derives a working recipe using the existing explicit hand-gesture overlays and
// executes it. It also retains local CPU history, field tools and derived analysis;
// those services must not silently create a second palette/animation authority.
// -----------------------------------------------------------------------------
class SculpturePipeline {
public:
    SculpturePipeline();
    // Start authored/base state, the initial working recipe and CPU evaluator
    // from this value. First-frame modulation still derives from the base recipe.
    explicit SculpturePipeline(const SculptureProgramParams& initialParams);
    virtual ~SculpturePipeline();

    SculpturePipeline(const SculpturePipeline&) = delete;
    SculpturePipeline& operator=(const SculpturePipeline&) = delete;
    SculpturePipeline(SculpturePipeline&&) noexcept;
    SculpturePipeline& operator=(SculpturePipeline&&) noexcept;

    // Strategy & Parameter Configuration
    void setRenderStrategy(RenderStrategy strategy) noexcept;
    [[nodiscard]] RenderStrategy renderStrategy() const noexcept;
    // Cycles only modes with a complete healthy OpenCL/Vulkan implementation.
    // CPU-recovery-only strategies remain selectable through explicit tooling
    // while their GPU extraction counterparts are developed.
    void cycleRenderStrategy() noexcept;
    // CPU-side extraction is available only to present an explicit producer
    // failure-recovery frame. Normal OpenCL deliveries never enter this lane.
    [[nodiscard]] bool requiresCpuRecoveryFrame() const noexcept;

    void updateParams(const SculptureProgramParams& params);
    [[nodiscard]] const SculptureProgramParams& params() const noexcept;

    // (Morph and multi-track choreography have been moved to SculpturePipelineCoordinator
    // to strictly enforce the boundary between application state and rendering execution)
    // LOD Configuration
    void setLodConfig(const SculptureLodConfig& config) noexcept;
    [[nodiscard]] const SculptureLodConfig& lodConfig() const noexcept;

    // Controls CPU mesh preparation independently of presentation framerate.

    // 4D Foliation Configuration

    // 4D Hopf Fibration Configuration

    // 4D Clifford Torus Knot Homotopy Configuration

    // 8D Octonionic Exceptional G2 Fibration Configuration

    // Penrose Twistor & Kerr-Newman Spacetime Ergosphere Configuration

    // Multi-Strand Quaternionic Ribbon Braiding Configuration

    // Hyperbolic Geodesic Flow & Poincaré Manifold Configuration

    // Execution & Evaluation
    void executeFrame(float simTime, float morph, float deltaSeconds);
    virtual void executeFrameWithView(float simTime, float morph, float deltaSeconds, const SculptureVec3& cameraPos, const SculptureSnapshot* externalCpuSnapshot = nullptr, const std::vector<float>* externalGpuSpatialBuffer = nullptr);

    // Retrieve generated outputs
    [[nodiscard]] const SculptureSnapshot& currentSnapshot() const noexcept;
    [[nodiscard]] const PipelineStats& stats() const noexcept;
    [[nodiscard]] const SculptureStageTelemetry& telemetry() const noexcept { return stats_.stageTelemetry; }
    [[nodiscard]] LodEvaluationResult currentLod() const noexcept { return currentLod_; }

    // Audio Context & Dynamic Spectrum Modulation
    void setAudioContext(const AudioSignalContext& audio) noexcept {
        audioContext_ = audio;
        baseParams_.audioSpectrum = {audio.bass, audio.mid, audio.treble, audio.spectralFlux};
        workingParams_.audioSpectrum = {audio.bass, audio.mid, audio.treble, audio.spectralFlux};
    }
    [[nodiscard]] const AudioSignalContext& audioContext() const noexcept { return audioContext_; }
    void setAudioSpectrum(float subBass, float bass, float mid, float treble, float spectralFlux) noexcept {
        audioContext_ = {subBass, bass, mid, treble, spectralFlux};
        baseParams_.audioSpectrum = {bass, mid, treble, spectralFlux};
        workingParams_.audioSpectrum = {bass, mid, treble, spectralFlux};
    }

    // Optional snapshot analysis; independent of audio input and output policy.
    // Enabling observes the next executed frame. Disabling clears cached features
    // without resetting visual history or the sonifier's synthesis buffers.
    void setSpatialAudioAnalysisEnabled(bool enabled) noexcept;
    [[nodiscard]] bool spatialAudioAnalysisEnabled() const noexcept {
        return spatialAudioAnalysisEnabled_;
    }

    // Snapshot-derived features remain neutral until analysis is enabled and a
    // frame executes. Direct sonifier use remains an explicit caller operation.
    [[nodiscard]] const GeometricAudioFeatures& audioFeatures() const noexcept;
    [[nodiscard]] SculptureSpatialSonifier& spatialSonifier() noexcept { return *spatialSonifier_; }
    [[nodiscard]] const SculptureSpatialSonifier& spatialSonifier() const noexcept { return *spatialSonifier_; }
    void generateAudioBlock(float* outStereo, std::size_t frameCount, std::uint32_t sampleRate = 48000) noexcept;

    // Spatial Indexing, Fast Picking & Proximity Neighborhood
    // Derived, lazily rebuilt view: reacquire after every frame attempt, even
    // one that throws. A previously borrowed reference cannot trigger rebuilding.
    // Queries index current stored samples; they do not certify frame completion.
    [[nodiscard]] const SculptureSpatialLBVH& spatialLBVH() const;
    [[nodiscard]] RayHitResult raycast(const SculptureVec3& origin, const SculptureVec3& direction, float maxDist = 100.0f) const;
    [[nodiscard]] std::vector<std::uint32_t> querySpatialNeighborhood(const SculptureVec3& center, float radius) const;

    // Dynamic Spatial Bounding & Motion Kinematics
    [[nodiscard]] const SculptureBoundingVolume& boundingVolume() const noexcept { return boundingVolume_; }
    [[nodiscard]] const SculptureMotionKinematics& motionKinematics() const noexcept { return motionKinematics_; }

    // Volumetric & Stereoscopic VR Raymarching
    [[nodiscard]] SculptureRaymarcher& raymarcher() noexcept { return *raymarcher_; }
    [[nodiscard]] const SculptureRaymarcher& raymarcher() const noexcept { return *raymarcher_; }

    // Recursive 3D Spatial Memory & Fluid Field
    [[nodiscard]] SculptureSpatialMemoryField& spatialMemoryField() noexcept { return *spatialMemoryField_; }
    [[nodiscard]] const SculptureSpatialMemoryField& spatialMemoryField() const noexcept { return *spatialMemoryField_; }
    // Admission belongs to the raymarcher; rejection preserves its configuration.
    [[nodiscard]] bool trySetRaymarchConfig(const SculptureRaymarchConfig& config) noexcept;
    // Compatibility setter; use trySetRaymarchConfig to observe rejection.
    void setRaymarchConfig(const SculptureRaymarchConfig& config) noexcept;

    void renderVolumetricView(
        std::uint32_t width,
        std::uint32_t height,
        const SculptureVec3& cameraPos,
        const SculptureVec3& cameraTarget,
        const SculptureVec3& cameraUp,
        float fovRadians,
        float time,
        float morph,
        std::vector<float>& outRgbBuffer,
        std::uint32_t threadCount = 4
    ) const;

    void renderStereoVRView(
        std::uint32_t eyeWidth,
        std::uint32_t eyeHeight,
        const SculptureStereoRig& rig,
        const SculptureVec3& cameraPos,
        const SculptureVec3& cameraTarget,
        const SculptureVec3& cameraUp,
        float fovRadians,
        float time,
        float morph,
        std::vector<float>& outLeftRgb,
        std::vector<float>& outRightRgb,
        std::uint32_t threadCount = 4
    ) const;

    void renderVolumetricGBuffer(
        std::uint32_t width,
        std::uint32_t height,
        const SculptureVec3& cameraPos,
        const SculptureVec3& cameraTarget,
        const SculptureVec3& cameraUp,
        float fovRadians,
        float time,
        float morph,
        SculptureGBuffer& outGBuffer,
        std::uint32_t threadCount = 4,
        const SculptureVec3* prevCameraPos = nullptr,
        const SculptureVec3* prevCameraTarget = nullptr
    ) const;

    void renderStereoVRGBuffer(
        std::uint32_t eyeWidth,
        std::uint32_t eyeHeight,
        const SculptureStereoRig& rig,
        const SculptureVec3& cameraPos,
        const SculptureVec3& cameraTarget,
        const SculptureVec3& cameraUp,
        float fovRadians,
        float time,
        float morph,
        SculptureStereoGBufferPair& outStereoGBuffer,
        std::uint32_t threadCount = 4,
        const SculptureVec3* prevCameraPos = nullptr,
        const SculptureVec3* prevCameraTarget = nullptr
    ) const;

    // 6-DOF Spatial Gesture & Tactile Field Manipulation
    [[nodiscard]] SculptureSpatialGestureEngine& gestureEngine() noexcept { return *gestureEngine_; }
    [[nodiscard]] const SculptureSpatialGestureEngine& gestureEngine() const noexcept { return *gestureEngine_; }

    // Multi-Source Modulation Matrix Coordinator

    // Variable-Rate Multirate Staged Execution Engine
    [[nodiscard]] SculptureMultirateEngine& multirateEngine() noexcept { return *multirateEngine_; }
    [[nodiscard]] const SculptureMultirateEngine& multirateEngine() const noexcept { return *multirateEngine_; }

    // Atmospheric Participating Media & Spectral Light Transport
    [[nodiscard]] SculptureVolumetricAtmosphere& atmosphereEngine() noexcept { return *atmosphereEngine_; }
    [[nodiscard]] const SculptureVolumetricAtmosphere& atmosphereEngine() const noexcept { return *atmosphereEngine_; }
    void setAtmosphereConfig(const VolumetricLightTransportConfig& config) noexcept;
    void setAtmosphereProfile(MediumProfile profile) noexcept;

    // First-Class Scene Contributor Integration
    // Closed-Loop Dynamic LOD & Performance Governor
    void setLodGovernorConfig(const LodGovernorConfig& config) noexcept { lodGovernor_.setConfig(config); }
    [[nodiscard]] const LodGovernorConfig& lodGovernorConfig() const noexcept { return lodGovernor_.config(); }
    [[nodiscard]] const SculptureLodGovernor& lodGovernor() const noexcept { return lodGovernor_; }
    [[nodiscard]] SculptureLodGovernor& lodGovernor() noexcept { return lodGovernor_; }
    // Closed-Loop 3D Acoustic Modal Resonance & Harmonic Tensor

private:
    // Modular execution pipeline stages
    void evaluateModulationStage(float deltaSeconds);
    void evaluateSnapshotStage(float simTime, float effectiveMorph, float deltaSeconds);
    void evaluateKinematicsStage();
    void evaluateLodStage(const SculptureVec3& cameraPos);
    void evaluateSpatialAudioStage();
    void rebuildSpatialIndexIfNeeded() const;
    
    SculptureProgramParams baseParams_{};
    SculptureProgramParams workingParams_{};
    RenderStrategy activeStrategy_ = RenderStrategy::PointInstances;
    SculptureLodConfig lodConfig_{};

    std::unique_ptr<SculptureProgram> cpuProgram_;
    std::unique_ptr<SculptureSpatialSonifier> spatialSonifier_;
    mutable std::unique_ptr<SculptureSpatialLBVH> spatialLBVH_;
    std::unique_ptr<SculptureRaymarcher> raymarcher_;
    std::unique_ptr<SculptureSpatialGestureEngine> gestureEngine_;
    std::unique_ptr<SculptureMultirateEngine> multirateEngine_;
    SculptureLodGovernor lodGovernor_{};
    std::unique_ptr<SculptureVolumetricAtmosphere> atmosphereEngine_;
    std::unique_ptr<SculptureSpatialMemoryField> spatialMemoryField_;

    std::unique_ptr<SculptureSnapshot> currentSnapshot_;
    AudioSignalContext audioContext_{};
    bool spatialAudioAnalysisEnabled_ = false;
    // Invalidate before frame mutation; clear only after a successful lazy build.
    mutable bool spatialIndexDirty_ = true;
    GeometricAudioFeatures currentAudioFeatures_{};
    PipelineStats stats_{};
    SculptureBoundingVolume boundingVolume_{};
    SculptureMotionKinematics motionKinematics_{};
    LodEvaluationResult currentLod_{};
    LodGovernorDecision currentLodDecision_{};
};

} // namespace epi_sculpt
