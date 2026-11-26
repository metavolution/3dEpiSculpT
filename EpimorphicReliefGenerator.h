#pragma once
#include "SculptureProgramSchema.h"
#include <vector>
#include <cstdint>

namespace epi_sculpt {

// ----------------------------------------------------------------------------
// EDUCATIONAL ANNOTATION: Epimorphic Relief Generation
// This module provides a highly decoupled, thread-parallel generator 
// that builds a tessellated 2D manifold (a relief mesh) by sampling an 
// underlying volumetric or 2D feedback field. 
// It utilizes standard std::thread parallelization to map the grid 
// efficiently across hardware cores, producing vertices and indices 
// that can later be ingested by the Vulkan backend for presentation.
// ----------------------------------------------------------------------------

enum class ReliefManifoldProjection : std::uint8_t {
    Planar = 0,                 // Standard Euclidean 2.5D elevation
    PoincareDisk = 1,           // Hyperbolic Poincaré disk conformal metric
    KleinBeltrami = 2,          // Projective Hyperbolic Klein-Beltrami disk
    RiemannSphere = 3,          // Stereographic Riemann Sphere projection
    ToroidalChamber = 4,        // Foliated Toroidal Chamber embedding
    HyperbolicPseudosphere = 5  // Beltrami Pseudosphere (constant negative curvature)
};

inline constexpr std::size_t kReliefManifoldProjectionCount = 6U;

[[nodiscard]] constexpr const char* name(ReliefManifoldProjection proj) noexcept {
    switch (proj) {
    case ReliefManifoldProjection::Planar: return "Planar (Euclidean 2.5D)";
    case ReliefManifoldProjection::PoincareDisk: return "PoincareDisk (Conformal Hyperbolic)";
    case ReliefManifoldProjection::KleinBeltrami: return "KleinBeltrami (Projective Hyperbolic)";
    case ReliefManifoldProjection::RiemannSphere: return "RiemannSphere (Stereographic S2)";
    case ReliefManifoldProjection::ToroidalChamber: return "ToroidalChamber (Foliated Torus)";
    case ReliefManifoldProjection::HyperbolicPseudosphere: return "HyperbolicPseudosphere (Beltrami)";
    default: return "Unknown";
    }
}

[[nodiscard]] constexpr const char* metricDescription(ReliefManifoldProjection proj) noexcept {
    switch (proj) {
    case ReliefManifoldProjection::Planar:
        return "ds^2 = dx^2 + dz^2, K = 0 (Flat Euclidean)";
    case ReliefManifoldProjection::PoincareDisk:
        return "ds^2 = 4(dx^2 + dz^2)/(1 - r^2)^2, K = -1 (Conformal Hyperbolic)";
    case ReliefManifoldProjection::KleinBeltrami:
        return "ds^2 = ((1-r^2)dr^2 + r^2 dtheta^2)/(1 - r^2)^2, K = -1 (Projective Hyperbolic)";
    case ReliefManifoldProjection::RiemannSphere:
        return "ds^2 = 4(dx^2 + dz^2)/(1 + r^2)^2, K = +1 (Stereographic S2)";
    case ReliefManifoldProjection::ToroidalChamber:
        return "T^2 = S^1(R) x S^1(r), K_inner < 0, K_outer > 0 (Foliated Torus)";
    case ReliefManifoldProjection::HyperbolicPseudosphere:
        return "ds^2 = du^2 + e^(2u) dv^2, K = -1 (Constant Negative Curvature)";
    default:
        return "Riemannian Manifold";
    }
}

struct ReliefMeshVertex {
    SculptureVec3 position{};
    SculptureVec3 normal{};
    float u = 0.0f;
    float v = 0.0f;
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

struct EpimorphicReliefData {
    std::vector<ReliefMeshVertex> vertices;
    std::vector<std::uint32_t> indices;
    float minHeight = 0.0f;
    float maxHeight = 0.0f;
    float averageHeight = 0.0f;
    ReliefManifoldProjection projection = ReliefManifoldProjection::Planar;
    bool valid = false;
    bool isSolidWatertight = false;
    float gaussianCurvatureMin = 0.0f;
    float gaussianCurvatureMax = 0.0f;
    float meanCurvatureMin = 0.0f;
    float meanCurvatureMax = 0.0f;

    [[nodiscard]] std::size_t triangleCount() const noexcept {
        return indices.size() / 3U;
    }
};

struct EpimorphicReliefConfig {
    std::uint32_t gridResX = 128U;
    std::uint32_t gridResY = 128U;
    float heightScale = 0.5f;
    float curvaturePower = 1.0f;
    ReliefManifoldProjection projection = ReliefManifoldProjection::Planar;
    float torusMajorRadius = 1.2f;
    float torusMinorRadius = 0.4f;
    float pseudosphereRadius = 0.8f;
    bool computeAnalyticalNormals = true;
    bool closeSolidMesh = false;       // Creates side walls and bottom base for 3D printing
    float solidBaseDepth = 0.2f;       // Base floor depth below minimum relief height
    bool colorizeCurvature = false;    // Colors vertices by Gaussian/Mean curvature

    [[nodiscard]] bool operator==(const EpimorphicReliefConfig&) const noexcept = default;
};

class EpimorphicReliefGenerator {
public:
    // Enhanced multi-manifold non-Euclidean relief generator
    static EpimorphicReliefData generate(
        const std::vector<float>& feedbackField,
        std::uint32_t fieldWidth,
        std::uint32_t fieldHeight,
        const EpimorphicReliefConfig& config
    );

    // Backward-compatible overload
    static EpimorphicReliefData generate(
        const std::vector<float>& feedbackField,
        std::uint32_t fieldWidth,
        std::uint32_t fieldHeight,
        std::uint32_t gridResX,
        std::uint32_t gridResY,
        float heightScale,
        float curvaturePower,
        bool applyPoincareDisk
    );

    // -------------------------------------------------------------------------
    // 3D Printing & Digital Fabrication Mesh Exporters
    // -------------------------------------------------------------------------
    static bool exportObj(
        const std::string& filepath,
        const EpimorphicReliefData& data,
        std::string_view objectName = "EpimorphicRelief"
    );

    static std::string exportObjToString(
        const EpimorphicReliefData& data,
        std::string_view objectName = "EpimorphicRelief"
    );

    static bool exportStlBinary(
        const std::string& filepath,
        const EpimorphicReliefData& data,
        std::string_view headerText = "epiC Epimorphic Relief 3D Solid"
    );

    static bool exportStlAscii(
        const std::string& filepath,
        const EpimorphicReliefData& data,
        std::string_view solidName = "EpimorphicRelief"
    );

    static std::vector<std::uint8_t> exportStlBinaryToBuffer(
        const EpimorphicReliefData& data,
        std::string_view headerText = "epiC Epimorphic Relief 3D Solid"
    );

    // -------------------------------------------------------------------------
    // Topological Invariant & Watertight Slicer Readiness Analysis
    // -------------------------------------------------------------------------
    struct MeshTopologyReport {
        bool isClosedManifold = false;
        int eulerCharacteristic = 0;
        std::size_t vertexCount = 0;
        std::size_t edgeCount = 0;
        std::size_t triangleCount = 0;
        std::size_t boundaryEdgeCount = 0;
        std::size_t nonManifoldEdgeCount = 0;
        std::size_t inconsistentOrientationCount = 0;
        std::size_t degenerateTriangleCount = 0;
        SculptureVec3 boundingBoxMin{};
        SculptureVec3 boundingBoxMax{};
    };

    [[nodiscard]] static MeshTopologyReport analyzeTopology(
        const EpimorphicReliefData& data
    );

    [[nodiscard]] static bool isWatertightClosedSolid(
        const EpimorphicReliefData& data
    );

    // Deduces optimal manifold projection from program mathematics
    [[nodiscard]] static ReliefManifoldProjection resolveReliefProjection(
        const SculptureProgramParams& params
    ) noexcept;
};

} // namespace epi_sculpt
