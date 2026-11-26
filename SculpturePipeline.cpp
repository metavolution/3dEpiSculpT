#include "SculpturePipeline.h"
#include "SculptureRaymarcher.h"
#include "SculptureSpatialSonifier.h"
#include "SculptureSpatialLBVH.h"
#include "SculptureSpatialGestureEngine.h"
#include "SculptureMultirateEngine.h"
#include "SculptureVolumetricAtmosphere.h"
#include "SculptureSpatialMemoryField.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace epi_sculpt {

namespace {

[[nodiscard]] constexpr bool strategyRequiresCpuFrame(RenderStrategy strategy) noexcept {
    return !isGpuNative(strategy);
}

} // namespace

SculpturePipeline::SculpturePipeline() 
    : SculpturePipeline(SculptureProgramParams{}) {}

SculpturePipeline::SculpturePipeline(const SculptureProgramParams& initialParams)
    : baseParams_(initialParams),
      workingParams_(initialParams),
      cpuProgram_(std::make_unique<SculptureProgram>(initialParams)) {
    spatialSonifier_ = std::make_unique<SculptureSpatialSonifier>();
    spatialLBVH_ = std::make_unique<SculptureSpatialLBVH>();
    raymarcher_ = std::make_unique<SculptureRaymarcher>();
    gestureEngine_ = std::make_unique<SculptureSpatialGestureEngine>();
    multirateEngine_ = std::make_unique<SculptureMultirateEngine>();
    atmosphereEngine_ = std::make_unique<SculptureVolumetricAtmosphere>();
    spatialMemoryField_ = std::make_unique<SculptureSpatialMemoryField>();
    currentSnapshot_ = std::make_unique<SculptureSnapshot>();
}


void SculpturePipeline::updateParams(const SculptureProgramParams& params) {
    baseParams_ = params;
    if (cpuProgram_) {
        cpuProgram_->setParams(params);
    }
}

void SculpturePipeline::executeFrame(float simTime, float morph, float deltaSeconds) {
    executeFrameWithView(simTime, morph, deltaSeconds, SculptureVec3{0.0f, 0.0f, 0.0f});
}

void SculpturePipeline::executeFrameWithView(
    float simTime,
    float morph,
    float deltaSeconds,
    const SculptureVec3& cameraPos,
    const SculptureSnapshot* externalCpuSnapshot,
    const std::vector<float>* externalGpuSpatialBuffer
) {
    auto startTotal = std::chrono::high_resolution_clock::now();

    // The tree borrows currentSnapshot_. A failed copy/evaluation can change
    // samples before throwing, so invalidate before any frame mutation. Rebuild
    // remains demand-driven; accepted mathematical history is not rolled back.
    spatialIndexDirty_ = true;

    // 0. Derive working parameters from the caller's already composed recipe.
    evaluateModulationStage(deltaSeconds);

    // Publish the complete recipe, including removal of a previous gesture
    // overlay. Configuration assignment does not consume CPU simulation cadence.
    if (cpuProgram_) {
        cpuProgram_->setParams(workingParams_);
    }

    // 1. Evaluate Continuous Snapshot (with stateful cadence governance)
    auto startCompute = std::chrono::high_resolution_clock::now();
    if (externalCpuSnapshot) {
        *currentSnapshot_ = *externalCpuSnapshot; // Reuse the dense CPU field from producer fallback
    } else {
        evaluateSnapshotStage(simTime, morph, deltaSeconds);
        // If we computed it on CPU but OpenCL or an external coordinator provided the spatial grid, ingest it
        // This prevents freezing/falling out of sync while keeping CPU state intact
        if (externalGpuSpatialBuffer && !externalGpuSpatialBuffer->empty()) {
            currentSnapshot_->spatialBuffer = *externalGpuSpatialBuffer;
        }
    }
    auto endCompute = std::chrono::high_resolution_clock::now();

    // 2. Count active instance points and update kinematics.
    evaluateKinematicsStage();

    // 3. Closed-Loop LOD & Dynamic Performance Governance
    evaluateLodStage(cameraPos);

    // 4. Optional snapshot-derived audio analysis.
    evaluateSpatialAudioStage();

    // 5. Strategy-Specific Meshing & Extraction (LOD-Adaptive)
    auto startPrep = std::chrono::high_resolution_clock::now();
    std::size_t totalTriangles = 0;
    auto endTotal = std::chrono::high_resolution_clock::now();

    // 6. Update Pipeline Telemetry Stats
    SculptureLodStats lodStats{};
    if (lodConfig_.enableDynamicLod) {
        switch (currentLod_.tier) {
        case SculptureLodTier::LOD0_Full: lodStats.lod0Count = motionKinematics_.activeCount; break;
        case SculptureLodTier::LOD1_Medium: lodStats.lod1Count = motionKinematics_.activeCount / 2; break;
        case SculptureLodTier::LOD2_Low: lodStats.lod2Count = motionKinematics_.activeCount / 4; break;
        case SculptureLodTier::LOD_Culled: lodStats.lodCulledCount = 0; break;
        }
        stats_.activeInstances = (currentLod_.tier == SculptureLodTier::LOD_Culled)
            ? 0 : (motionKinematics_.activeCount / std::max(currentLod_.sampleStride, std::size_t{1}));
    } else {
        lodStats.lod0Count = motionKinematics_.activeCount;
        stats_.activeInstances = motionKinematics_.activeCount;
    }

    stats_.frameTimeMs = std::chrono::duration<float, std::milli>(endTotal - startTotal).count();
    stats_.computeTimeMs = std::chrono::duration<float, std::milli>(endCompute - startCompute).count();
    stats_.renderPrepTimeMs = std::chrono::duration<float, std::milli>(endTotal - startPrep).count();
    stats_.activeTriangles = totalTriangles;
    stats_.currentGeneration++;
    stats_.activeStrategy = activeStrategy_;
    stats_.lodStats = lodStats;
    stats_.fallbackCpuUsed = (externalCpuSnapshot == nullptr);
    stats_.openClActive = (externalCpuSnapshot != nullptr || externalGpuSpatialBuffer != nullptr);

    stats_.stageTelemetry = SculptureStageTelemetry{};
    stats_.stageTelemetry.probeExecutionTimeUs = stats_.computeTimeMs * 1000.0f;
}

// -----------------------------------------------------------------------------
// Educational Annotation: MODULAR PIPELINE EVALUATION STAGES
// The unified pipeline executes in discrete, decoupled phases to maximize 
// cache locality, improve temporal cadence stability, and provide clean 
// extension points for future mathematical producers.
// -----------------------------------------------------------------------------

void SculpturePipeline::evaluateModulationStage(float deltaSeconds) {
    workingParams_ = baseParams_;

    // Palette evolution belongs to the coordinator's opt-in palette agent.
    // Preserve that complete authored/composed color recipe here.
    // Process purely spatial interactions tied directly to the pipeline's voxel grid
    if (gestureEngine_->leftHand().valid || gestureEngine_->rightHand().valid) {
        gestureEngine_->processGestures(workingParams_, currentSnapshot_->spatialBuffer, deltaSeconds);
        gestureEngine_->applySpatialGestureTo4DLatent(workingParams_, deltaSeconds);
    }
}

void SculpturePipeline::evaluateSnapshotStage(float simTime, float effectiveMorph, float deltaSeconds) {
    // EDUCATIONAL: Consolidate mathematical strategy evaluation.
    // We evaluate the active strategy, then apply universal post-processing
    // (like spatial memory evolution) consistently at the end.
    cpuProgram_->evaluateWithCadence(simTime, effectiveMorph, deltaSeconds, *currentSnapshot_);
    if (currentSnapshot_->spatialBuffer.empty() && workingParams_.auxiliaryResolutionScale != StageResolutionScale::Full) {
        multirateEngine_->executeMultirateSeedToBuffer(
            workingParams_,
            simTime,
            stats_.currentGeneration,
            currentSnapshot_->spatialBuffer
        );
    }
    
    // Universal spatial memory field evolution for all strategies.
    spatialMemoryField_->evolve(deltaSeconds, workingParams_, simTime);

    // Inject true 3D spatial field (Gray-Scott CPU) into spatialBuffer so renderers can consume it.
    if (!currentSnapshot_->spatialBuffer.empty()) {
        const std::uint32_t gridN = SculptureProgram::getAuxiliaryVoxelGridDimension(workingParams_.auxiliaryResolutionScale);
        if (currentSnapshot_->spatialBuffer.size() == static_cast<std::size_t>(gridN * gridN * gridN)) {
            for (std::uint32_t z = 0; z < gridN; ++z) {
                const float pz = (static_cast<float>(z) / std::max(1.0f, static_cast<float>(gridN - 1))) * 2.0f - 1.0f;
                for (std::uint32_t y = 0; y < gridN; ++y) {
                    const float py = (static_cast<float>(y) / std::max(1.0f, static_cast<float>(gridN - 1))) * 2.0f - 1.0f;
                    for (std::uint32_t x = 0; x < gridN; ++x) {
                        const float px = (static_cast<float>(x) / std::max(1.0f, static_cast<float>(gridN - 1))) * 2.0f - 1.0f;
                        
                        const SpatialFieldSample sample = spatialMemoryField_->sampleTrilinear(px, py, pz);
                        // The original fluid simulation density is overwritten by the superior Turing fluid
                        currentSnapshot_->spatialBuffer[(z * gridN + y) * gridN + x] = sample.density;
                    }
                }
            }
        }
    }
}

void SculpturePipeline::evaluateKinematicsStage() {
    std::size_t activeCount = 0;
    float minX = 1.0e30f, minY = 1.0e30f, minZ = 1.0e30f;
    float maxX = -1.0e30f, maxY = -1.0e30f, maxZ = -1.0e30f;
    float totalVx = 0.0f, totalVy = 0.0f, totalVz = 0.0f;
    float maxSpd = 0.0f;
    float totalKinetic = 0.0f;

    for (const auto& s : currentSnapshot_->samples) {
        if (s.radius <= 0.0f) continue;
        activeCount++;
        const float r = s.radius;
        minX = std::min(minX, s.x - r);
        minY = std::min(minY, s.y - r);
        minZ = std::min(minZ, s.z - r);
        maxX = std::max(maxX, s.x + r);
        maxY = std::max(maxY, s.y + r);
        maxZ = std::max(maxZ, s.z + r);

        const float spd = std::sqrt(s.vx * s.vx + s.vy * s.vy + s.vz * s.vz);
        totalVx += s.vx;
        totalVy += s.vy;
        totalVz += s.vz;
        maxSpd = std::max(maxSpd, spd);
        totalKinetic += 0.5f * spd * spd;
    }

    if (activeCount > 0) {
        boundingVolume_.minBounds = SculptureVec3{minX, minY, minZ};
        boundingVolume_.maxBounds = SculptureVec3{maxX, maxY, maxZ};
        boundingVolume_.center = SculptureVec3{
            0.5f * (minX + maxX),
            0.5f * (minY + maxY),
            0.5f * (minZ + maxZ)
        };
        const float dx = maxX - boundingVolume_.center.x;
        const float dy = maxY - boundingVolume_.center.y;
        const float dz = maxZ - boundingVolume_.center.z;
        boundingVolume_.boundingRadius = std::sqrt(dx * dx + dy * dy + dz * dz);
        boundingVolume_.valid = true;

        motionKinematics_.averageVelocity = SculptureVec3{
            totalVx / static_cast<float>(activeCount),
            totalVy / static_cast<float>(activeCount),
            totalVz / static_cast<float>(activeCount)
        };
        motionKinematics_.maxSpeed = maxSpd;
        motionKinematics_.kineticEnergy = totalKinetic;
        motionKinematics_.activeCount = activeCount;
        motionKinematics_.valid = true;
    } else {
        boundingVolume_ = {};
        motionKinematics_ = {};
    }
}

void SculpturePipeline::evaluateLodStage(const SculptureVec3& cameraPos) {
    currentLodDecision_ = lodGovernor_.update(
        boundingVolume_, cameraPos, stats_.frameTimeMs, stats_.currentGeneration
    );
    currentLod_.tier = currentLodDecision_.tier;
    currentLod_.sampleStride = currentLodDecision_.sampleStride;
    currentLod_.radiusCompScale = currentLodDecision_.radiusCompensation;
    currentLod_.isVisible = (currentLodDecision_.tier != SculptureLodTier::LOD_Culled);

    if (!lodConfig_.enableDynamicLod) {
        currentLod_.tier = SculptureLodTier::LOD0_Full;
        currentLod_.sampleStride = 1U;
        currentLod_.radiusCompScale = 1.0f;
        currentLod_.isVisible = true;
    }
}

void SculpturePipeline::setSpatialAudioAnalysisEnabled(bool enabled) noexcept {
    if (spatialAudioAnalysisEnabled_ == enabled) return;
    spatialAudioAnalysisEnabled_ = enabled;
    if (!enabled) {
        // Withdraw the previous observation immediately; disabled consumers must
        // not interpret old geometry as current. No synthesis/history reset.
        currentAudioFeatures_ = {};
        spatialSonifier_->updateFeatures(currentAudioFeatures_);
    }
}

void SculpturePipeline::evaluateSpatialAudioStage() {
    if (!spatialAudioAnalysisEnabled_ || !currentSnapshot_) return;
    currentAudioFeatures_ = SculptureSpatialSonifier::extractFeatures(*currentSnapshot_);
    spatialSonifier_->updateFeatures(currentAudioFeatures_);
}



void SculpturePipeline::renderVolumetricView(
    std::uint32_t width,
    std::uint32_t height,
    const SculptureVec3& cameraPos,
    const SculptureVec3& cameraTarget,
    const SculptureVec3& cameraUp,
    float fovRadians,
    float time,
    float morph,
    std::vector<float>& outRgbBuffer,
    std::uint32_t threadCount
) const {
    if (!cpuProgram_) return;
    raymarcher_->renderImage(
        width, height, cameraPos, cameraTarget, cameraUp, fovRadians,
        *cpuProgram_, time, morph, outRgbBuffer, currentSnapshot_->spatialBuffer, threadCount
    );
}

void SculpturePipeline::renderStereoVRView(
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
    std::uint32_t threadCount
) const {
    if (!cpuProgram_) return;
    raymarcher_->renderStereoImagePair(
        eyeWidth, eyeHeight, rig, cameraPos, cameraTarget, cameraUp, fovRadians,
        *cpuProgram_, time, morph, outLeftRgb, outRightRgb, currentSnapshot_->spatialBuffer, threadCount
    );
}

void SculpturePipeline::renderVolumetricGBuffer(
    std::uint32_t width,
    std::uint32_t height,
    const SculptureVec3& cameraPos,
    const SculptureVec3& cameraTarget,
    const SculptureVec3& cameraUp,
    float fovRadians,
    float time,
    float morph,
    SculptureGBuffer& outGBuffer,
    std::uint32_t threadCount,
    const SculptureVec3* prevCameraPos,
    const SculptureVec3* prevCameraTarget
) const {
    if (!cpuProgram_) return;
    raymarcher_->renderGBuffer(
        width, height, cameraPos, cameraTarget, cameraUp, fovRadians,
        *cpuProgram_, time, morph, outGBuffer, currentSnapshot_->spatialBuffer, threadCount,
        nullptr, prevCameraPos, prevCameraTarget
    );
}

void SculpturePipeline::renderStereoVRGBuffer(
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
    std::uint32_t threadCount,
    const SculptureVec3* prevCameraPos,
    const SculptureVec3* prevCameraTarget
) const {
    if (!cpuProgram_) return;
    raymarcher_->renderStereoGBufferPair(
        eyeWidth, eyeHeight, rig, cameraPos, cameraTarget, cameraUp, fovRadians,
        *cpuProgram_, time, morph, outStereoGBuffer, currentSnapshot_->spatialBuffer, threadCount,
        nullptr, prevCameraPos, prevCameraTarget
    );
}


const PipelineStats& SculpturePipeline::stats() const noexcept {
    return stats_;
}

const SculptureSnapshot& SculpturePipeline::currentSnapshot() const noexcept {
    return *currentSnapshot_;
}

void SculpturePipeline::rebuildSpatialIndexIfNeeded() const {
    if (!spatialIndexDirty_ || !currentSnapshot_) return;
    spatialLBVH_->build(*currentSnapshot_);
    // A failed build must leave the cache dirty so the next query can retry.
    spatialIndexDirty_ = false;
}

const SculptureSpatialLBVH& SculpturePipeline::spatialLBVH() const {
    rebuildSpatialIndexIfNeeded();
    return *spatialLBVH_;
}

RayHitResult SculpturePipeline::raycast(
    const SculptureVec3& origin,
    const SculptureVec3& direction,
    float maxDist
) const {
    return spatialLBVH().intersectRay(origin, direction, maxDist);
}

std::vector<std::uint32_t> SculpturePipeline::querySpatialNeighborhood(
    const SculptureVec3& center,
    float radius
) const {
    return spatialLBVH().queryRadius(center, radius);
}

const GeometricAudioFeatures& SculpturePipeline::audioFeatures() const noexcept {
    return currentAudioFeatures_;
}

void SculpturePipeline::generateAudioBlock(float* outStereo, std::size_t frameCount, std::uint32_t sampleRate) noexcept {
    if (outStereo) {
        for(std::size_t i = 0; i < frameCount * 2; ++i) { outStereo[i] = 0.0f; }
    }
}



SculpturePipeline::~SculpturePipeline() = default;


void SculpturePipeline::setRenderStrategy(RenderStrategy strategy) noexcept {
    activeStrategy_ = strategy;
}

RenderStrategy SculpturePipeline::renderStrategy() const noexcept {
    return activeStrategy_;
}

void SculpturePipeline::cycleRenderStrategy() noexcept {
    activeStrategy_ = nextRenderStrategy(activeStrategy_);
}

const SculptureProgramParams& SculpturePipeline::params() const noexcept {
    return baseParams_;
}


bool SculpturePipeline::requiresCpuRecoveryFrame() const noexcept {
    return !isGpuNative(activeStrategy_);
}

void SculpturePipeline::setLodConfig(const SculptureLodConfig& config) noexcept {
    lodConfig_ = config;
}

const SculptureLodConfig& SculpturePipeline::lodConfig() const noexcept {
    return lodConfig_;
}

bool SculpturePipeline::trySetRaymarchConfig(const SculptureRaymarchConfig& config) noexcept {
    return raymarcher_->trySetConfig(config);
}

void SculpturePipeline::setRaymarchConfig(const SculptureRaymarchConfig& config) noexcept {
    (void)trySetRaymarchConfig(config);
}

void SculpturePipeline::setAtmosphereConfig(const VolumetricLightTransportConfig& config) noexcept {
    atmosphereEngine_->setConfig(config);
}

void SculpturePipeline::setAtmosphereProfile(MediumProfile profile) noexcept {
    atmosphereEngine_->setMediumProfile(profile);
}

} // namespace epi_sculpt
