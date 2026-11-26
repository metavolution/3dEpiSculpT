/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - SCULPTURE MANIFOLD SYNTHESIS SERVICE IMPLEMENTATION
 * ============================================================================
 */

#include "SculptureManifoldSynthesisService.h"
#include "SculpturePipelineCoordinator.h"
#include "SculptureProgram.h"

#include <algorithm>
#include <memory>

namespace epi_sculpt {

void SculptureManifoldSynthesisService::rasterizeAcousticPressureGrid3D(
    const SculpturePipelineCoordinator& coordinator,
    std::vector<float>& outVoxelGrid,
    std::uint32_t voxelDim,
    float time
) noexcept {
    SculptureBoundingVolume bounds{};
    bounds.minBounds = {-1.5f, -1.5f, -1.5f};
    bounds.maxBounds = { 1.5f,  1.5f,  1.5f};
    bounds.valid = true;
    coordinator.audioResonance_.rasterizePressureField3D(
        outVoxelGrid,
        voxelDim,
        bounds,
        time != 0.0f ? time : coordinator.simulatedTime_
    );
}

IsosurfaceMesh SculptureManifoldSynthesisService::generateActiveManifoldMesh(
    const SculpturePipelineCoordinator& coordinator,
    Manifold4DType type,
    float scale,
    std::size_t uDivisions,
    std::size_t vDivisions
) {
    const float thetaL = coordinator.simulatedTime_ * 0.35f;
    const float thetaR = coordinator.simulatedTime_ * 0.25f;
    return field_geometry::generateFoliationSurfaceMesh(
        type, thetaL, thetaR, scale, uDivisions, vDivisions);
}

IsosurfaceMesh SculptureManifoldSynthesisService::generateActiveTwistorFibrationMesh(
    const SculpturePipelineCoordinator& coordinator,
    std::size_t fiberCount,
    std::size_t pointsPerFiber,
    float ribbonWidth,
    float scale
) {
    const auto& p = coordinator.currentParams();
    return Sculpture4DFoliation::generateTwistorFibrationRibbonMesh(
        fiberCount,
        pointsPerFiber,
        ribbonWidth,
        p.rotorAlpha != 0.0f ? p.rotorAlpha : (coordinator.simulatedTime_ * 0.35f),
        p.rotorBeta != 0.0f ? p.rotorBeta : (coordinator.simulatedTime_ * 0.25f),
        coordinator.simulatedTime_,
        scale * p.worldScale
    );
}

std::vector<float> SculptureManifoldSynthesisService::sampleActiveKuramotoWaveField(
    const SculpturePipelineCoordinator& coordinator,
    const std::vector<SculptureVec3>& samplePoints
) {
    const float k = (coordinator.audioContext_.has_value() && coordinator.enableAudioResonance_)
        ? (coordinator.audioResonance_.metrics().resonanceQFactor * 0.1f)
        : 1.5f;
    return Sculpture4DFoliation::evaluateKuramotoFoliationWaveField(
        samplePoints,
        coordinator.simulatedTime_,
        k,
        2.0f
    );
}

IsosurfaceMesh SculptureManifoldSynthesisService::generateActivePolychoronHomotopyMesh(
    const SculpturePipelineCoordinator& coordinator,
    PolychoronKind source,
    PolychoronKind target,
    float morphProgress,
    float scale,
    float tubeRadius,
    std::size_t radialSegments
) {
    const auto& p = coordinator.currentParams();
    PolychoronHomotopyConfig cfg{};
    cfg.sourcePolytope = source;
    cfg.targetPolytope = target;
    cfg.morphProgress = morphProgress;
    cfg.rotorAlpha = p.rotorAlpha != 0.0f ? p.rotorAlpha : (coordinator.simulatedTime_ * 0.25f);
    cfg.rotorBeta = p.rotorBeta != 0.0f ? p.rotorBeta : (coordinator.simulatedTime_ * 0.18f);
    cfg.rotorTheta4D = p.rotorTheta4D;
    cfg.latentW = p.latentW;
    cfg.scale = scale * p.worldScale;

    StreamlineTubeConfig tubeCfg{};
    tubeCfg.radius = tubeRadius * p.worldScale;
    tubeCfg.radialSegments = static_cast<std::uint32_t>(radialSegments);
    tubeCfg.smoothNormals = true;
    tubeCfg.generateCaps = true;

    Sculpture4DPolychoronHomotopy homotopy(cfg);
    return homotopy.generateHomotopyMesh(coordinator.simulatedTime_, tubeCfg);
}

std::vector<world_gpu_abi::WorldInstanceGpu> SculptureManifoldSynthesisService::generateActivePolychoronWorldInstances(
    const SculpturePipelineCoordinator& coordinator,
    PolychoronKind source,
    PolychoronKind target,
    float morphProgress,
    float scale
) {
    const auto& p = coordinator.currentParams();
    PolychoronHomotopyConfig cfg{};
    cfg.sourcePolytope = source;
    cfg.targetPolytope = target;
    cfg.morphProgress = morphProgress;
    cfg.rotorAlpha = p.rotorAlpha != 0.0f ? p.rotorAlpha : (coordinator.simulatedTime_ * 0.25f);
    cfg.rotorBeta = p.rotorBeta != 0.0f ? p.rotorBeta : (coordinator.simulatedTime_ * 0.18f);
    cfg.rotorTheta4D = p.rotorTheta4D;
    cfg.latentW = p.latentW;
    cfg.scale = scale * p.worldScale;

    Sculpture4DPolychoronHomotopy homotopy(cfg);
    return homotopy.generateWorldInstances(coordinator.simulatedTime_);
}

SculptureGBuffer SculptureManifoldSynthesisService::renderActiveImplicitGBuffer(
    const SculpturePipelineCoordinator& coordinator,
    std::uint32_t width,
    std::uint32_t height,
    const SculptureVec3& cameraPos,
    const SculptureVec3& cameraTarget,
    const SculptureVec3& cameraUp,
    float fovRadians,
    std::uint32_t threadCount,
    const SculptureVec3* prevCameraPos,
    const SculptureVec3* prevCameraTarget
) {
    SculptureGBuffer gBuffer{};
    if (width == 0U || height == 0U) {
        return gBuffer;
    }

    const auto& params = coordinator.currentParams();
    SculptureProgram program(params);

    SculptureRaymarchConfig cfg{};
    cfg.maxSteps = coordinator.qualityPolicy_.raySteps();
    cfg.chamberMode = (params.enable4D > 0U) ? SdfChamberMode::Foliation4DChamber : SdfChamberMode::EpimorphicChamber;
    cfg.boundingRadius = 4.5f * std::max(params.worldScale, 0.1f);
    cfg.enableVolumetricFog = true;

    SculptureRaymarcher raymarcher(cfg);
    raymarcher.renderGBuffer(
        width, height, cameraPos, cameraTarget, cameraUp, fovRadians,
        program, coordinator.simulatedTime_, coordinator.choreographer_.progress(), gBuffer,
        coordinator.auxiliaryVoxelGrid_, threadCount, coordinator.volumetricMemory_.get(),
        prevCameraPos, prevCameraTarget
    );

    return gBuffer;
}

SculptureStereoGBufferPair SculptureManifoldSynthesisService::renderActiveStereoImplicitGBuffer(
    const SculpturePipelineCoordinator& coordinator,
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
) {
    SculptureStereoGBufferPair stereoPair{};
    if (eyeWidth == 0U || eyeHeight == 0U) {
        return stereoPair;
    }

    const auto& params = coordinator.currentParams();
    SculptureProgram program(params);

    SculptureRaymarchConfig cfg{};
    cfg.maxSteps = coordinator.qualityPolicy_.raySteps();
    cfg.chamberMode = (params.enable4D > 0U) ? SdfChamberMode::Foliation4DChamber : SdfChamberMode::EpimorphicChamber;
    cfg.boundingRadius = 4.5f * std::max(params.worldScale, 0.1f);
    cfg.enableStereoVr = true;
    cfg.eyeSeparation = rig.interpupillaryDistance;
    cfg.enableVolumetricFog = true;

    SculptureRaymarcher raymarcher(cfg);
    raymarcher.renderStereoGBufferPair(
        eyeWidth, eyeHeight, rig, cameraPos, cameraTarget, cameraUp, fovRadians,
        program, coordinator.simulatedTime_, coordinator.choreographer_.progress(), stereoPair,
        coordinator.auxiliaryVoxelGrid_, threadCount, coordinator.volumetricMemory_.get(),
        prevCameraPos, prevCameraTarget
    );

    return stereoPair;
}

field_geometry::ReliefMesh SculptureManifoldSynthesisService::generateActiveReliefMesh(
    const SculpturePipelineCoordinator& coordinator,
    std::uint32_t gridRes,
    bool applyPoincare
) {
    const auto& p = coordinator.currentParams();
    std::vector<float> field;
    if (!coordinator.auxiliaryVoxelGrid_.empty()) {
        field = coordinator.auxiliaryVoxelGrid_;
    } else {
        field.resize(256 * 256, 0.0f);
        SculptureProgram prog(p);
        auto snap = std::make_unique<SculptureSnapshot>();
        prog.evaluate(coordinator.simulatedTime_, coordinator.choreographer_.progress(), *snap);
        field_geometry::rasterizeSampleDensityTo2DSeed(*snap, field, 256, 256);
    }
    return field_geometry::generateEpimorphicReliefMesh(
        field,
        256, 256,
        gridRes, gridRes,
        p.radiusScale * 0.35f,
        1.2f,
        applyPoincare
    );
}

EpimorphicReliefData SculptureManifoldSynthesisService::generateActiveEpimorphicRelief(
    const SculpturePipelineCoordinator& coordinator,
    const EpimorphicReliefConfig& config
) {
    return generateEpimorphicRelief(coordinator, describeEpimorphicRelief(coordinator, config));
}

ReliefMeshRequest SculptureManifoldSynthesisService::describeEpimorphicRelief(
    const SculpturePipelineCoordinator& coordinator,
    const EpimorphicReliefConfig& config) noexcept {
    ReliefMeshRequest request;
    request.config = config;
    if (coordinator.hasAuxiliary2DTexture()) {
        request.source.kind = ReliefSourceKind::AuxiliaryTexture;
        request.source.contentRevision = coordinator.auxiliaryTextureRevision_;
    } else if (coordinator.volumetricMemory_ &&
        coordinator.volumetricMemory_->telemetry().meanDensity > 0.001f) {
        request.source.kind = ReliefSourceKind::VolumetricMemory;
        request.source.contentRevision = coordinator.volumetricMemory_->contentRevision();
        request.source.volumeSliceRadius = coordinator.volumetricCoupler_.config().worldBoundingRadius;
    } else {
        request.source.programSample = coordinator.proceduralGeometrySample_;
    }
    return request;
}

EpimorphicReliefData SculptureManifoldSynthesisService::generateEpimorphicRelief(
    const SculpturePipelineCoordinator& coordinator,
    const ReliefMeshRequest& request) {
    // External revisions describe borrowed owner state, not an owned checkpoint.
    // Refuse a stale request rather than certifying geometry from another source.
    if (request.source != describeEpimorphicRelief(coordinator, request.config).source) {
        return {};
    }
    const auto& config = request.config;
    if (request.source.kind == ReliefSourceKind::AuxiliaryTexture) {
        return EpimorphicReliefGenerator::generate(
            coordinator.auxiliaryTextureRgba_,
            coordinator.auxiliaryTextureWidth_,
            coordinator.auxiliaryTextureHeight_,
            config
        );
    }
    const std::uint32_t width = config.gridResX > 0 ? config.gridResX : 256U;
    const std::uint32_t height = config.gridResY > 0 ? config.gridResY : 256U;

    // Direct Volumetric Field Slicing: Elevates 3D persistent memory into the relief manifold
    if (request.source.kind == ReliefSourceKind::VolumetricMemory) {
        const auto field = coordinator.volumetricCoupler_.extractReliefSlice(
            *coordinator.volumetricMemory_, width, height, 0.0f, 0.20f);
        // Empty means acquisition was unavailable, not a valid zero-density
        // slice. Refuse geometry before the navigator considers publication.
        if (field.empty()) {
            return {};
        }
        return EpimorphicReliefGenerator::generate(field, width, height, config);
    }

    std::vector<float> field(static_cast<std::size_t>(width) * height, 0.0f);
    const auto& sample = request.source.programSample;
    SculptureProgram prog(sample.params);
    auto snap = std::make_unique<SculptureSnapshot>();
    prog.evaluate(sample.time, sample.morph, *snap);
    field_geometry::rasterizeSampleDensityTo2DSeed(*snap, field, width, height);
    return EpimorphicReliefGenerator::generate(field, width, height, config);
}

EpimorphicReliefData SculptureManifoldSynthesisService::generateBraidedKnotRelief(
    const SculpturePipelineCoordinator& coordinator,
    const KnotHomotopyConfig& knotConfig,
    std::size_t numStrands,
    const EpimorphicReliefConfig& reliefConfig
) {
    const std::uint32_t width = reliefConfig.gridResX > 0 ? reliefConfig.gridResX : 256U;
    const std::uint32_t height = reliefConfig.gridResY > 0 ? reliefConfig.gridResY : 256U;

    std::vector<float> field = SculptureKnotHomotopy::rasterizeBraidTo2DField(
        knotConfig,
        numStrands,
        width,
        height,
        coordinator.simulatedTime_,
        0.045f
    );

    return EpimorphicReliefGenerator::generate(field, width, height, reliefConfig);
}

bool SculptureManifoldSynthesisService::exportReliefMeshObj(
    const SculpturePipelineCoordinator& coordinator,
    const std::string& filepath,
    const EpimorphicReliefConfig& config,
    std::string_view objectName
) {
    const auto relief = generateActiveEpimorphicRelief(coordinator, config);
    return EpimorphicReliefGenerator::exportObj(filepath, relief, objectName);
}

bool SculptureManifoldSynthesisService::exportReliefMeshStl(
    const SculpturePipelineCoordinator& coordinator,
    const std::string& filepath,
    const EpimorphicReliefConfig& config,
    bool binary,
    std::string_view headerOrSolidName
) {
    const auto relief = generateActiveEpimorphicRelief(coordinator, config);
    if (binary) {
        return EpimorphicReliefGenerator::exportStlBinary(filepath, relief, headerOrSolidName);
    }
    return EpimorphicReliefGenerator::exportStlAscii(filepath, relief, headerOrSolidName);
}

} // namespace epi_sculpt
