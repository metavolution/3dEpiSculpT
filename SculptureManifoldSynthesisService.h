/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - SCULPTURE MANIFOLD SYNTHESIS SERVICE
 * ============================================================================
 *
 * [EDUCATIONAL NOTE: Higher-Dimensional Manifold & Relief Synthesis Domain Service]
 * ----------------------------------------------------------------------------
 * Isolates all 4D foliation mesh synthesis, twistor fibration ribbons,
 * 4D polychoron great-circle homotopy meshes & world instances, Kuramoto
 * wave field sampling, 3D Helmholtz acoustic pressure grids, CPU implicit
 * G-buffer rendering, and 2.5D non-Euclidean relief exports from the core
 * `SculpturePipelineCoordinator` frame-tick state machine.
 * ============================================================================
 */

#pragma once

#include "Sculpture4DFoliation.h"
#include "Sculpture4DPolychoronHomotopy.h"
#include "SculptureFieldGeometry.h"
#include "EpimorphicReliefGenerator.h"
#include "SculptureProceduralGeometryRequest.h"
#include "SculptureKnotHomotopy.h"
#include "SculptureRaymarcher.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace epi_sculpt {

class SculpturePipelineCoordinator;

class SculptureManifoldSynthesisService {
public:
    static void rasterizeAcousticPressureGrid3D(
        const SculpturePipelineCoordinator& coordinator,
        std::vector<float>& outVoxelGrid,
        std::uint32_t voxelDim,
        float time = 0.0f
    ) noexcept;

    [[nodiscard]] static IsosurfaceMesh generateActiveManifoldMesh(
        const SculpturePipelineCoordinator& coordinator,
        Manifold4DType type,
        float scale = 1.0f,
        std::size_t uDivisions = 32,
        std::size_t vDivisions = 32
    );

    [[nodiscard]] static IsosurfaceMesh generateActiveTwistorFibrationMesh(
        const SculpturePipelineCoordinator& coordinator,
        std::size_t fiberCount = 8,
        std::size_t pointsPerFiber = 32,
        float ribbonWidth = 0.03f,
        float scale = 1.0f
    );

    [[nodiscard]] static std::vector<float> sampleActiveKuramotoWaveField(
        const SculpturePipelineCoordinator& coordinator,
        const std::vector<SculptureVec3>& samplePoints
    );

    [[nodiscard]] static IsosurfaceMesh generateActivePolychoronHomotopyMesh(
        const SculpturePipelineCoordinator& coordinator,
        PolychoronKind source = PolychoronKind::Tesseract8Cell,
        PolychoronKind target = PolychoronKind::Icositetrachoron24,
        float morphProgress = 0.5f,
        float scale = 1.0f,
        float tubeRadius = 0.025f,
        std::size_t radialSegments = 6
    );

    [[nodiscard]] static std::vector<world_gpu_abi::WorldInstanceGpu> generateActivePolychoronWorldInstances(
        const SculpturePipelineCoordinator& coordinator,
        PolychoronKind source = PolychoronKind::Tesseract8Cell,
        PolychoronKind target = PolychoronKind::Icositetrachoron24,
        float morphProgress = 0.5f,
        float scale = 1.0f
    );

    [[nodiscard]] static SculptureGBuffer renderActiveImplicitGBuffer(
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
    );

    [[nodiscard]] static SculptureStereoGBufferPair renderActiveStereoImplicitGBuffer(
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
    );

    [[nodiscard]] static field_geometry::ReliefMesh generateActiveReliefMesh(
        const SculpturePipelineCoordinator& coordinator,
        std::uint32_t gridRes = 64,
        bool applyPoincare = true
    );

    [[nodiscard]] static EpimorphicReliefData generateActiveEpimorphicRelief(
        const SculpturePipelineCoordinator& coordinator,
        const EpimorphicReliefConfig& config = {}
    );

    [[nodiscard]] static ReliefMeshRequest describeEpimorphicRelief(
        const SculpturePipelineCoordinator& coordinator,
        const EpimorphicReliefConfig& config) noexcept;

    [[nodiscard]] static EpimorphicReliefData generateEpimorphicRelief(
        const SculpturePipelineCoordinator& coordinator,
        const ReliefMeshRequest& request);

    [[nodiscard]] static EpimorphicReliefData generateBraidedKnotRelief(
        const SculpturePipelineCoordinator& coordinator,
        const KnotHomotopyConfig& knotConfig = {},
        std::size_t numStrands = 3U,
        const EpimorphicReliefConfig& reliefConfig = {}
    );

    static bool exportReliefMeshObj(
        const SculpturePipelineCoordinator& coordinator,
        const std::string& filepath,
        const EpimorphicReliefConfig& config = {},
        std::string_view objectName = "EpimorphicRelief"
    );

    static bool exportReliefMeshStl(
        const SculpturePipelineCoordinator& coordinator,
        const std::string& filepath,
        const EpimorphicReliefConfig& config = {},
        bool binary = true,
        std::string_view headerOrSolidName = "EpimorphicRelief"
    );
};

} // namespace epi_sculpt
