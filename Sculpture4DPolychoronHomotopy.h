/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - SCULPTURE 4D POLYCHORON HOMOTOPY ENGINE
 * ============================================================================
 *
 * Educational Annotation: HIGHER-DIMENSIONAL COXETER GEOMETRY & S^3 HOMOTOPY
 * ----------------------------------------------------------------------------
 * Fulfills Phase 6 of the 3D and Higher-Dimensional Procedural World Vision:
 * Continuous 4D Manifold Slicing, Homotopy Blending, and SO(4) Lie Rotations.
 *
 * In 4-dimensional Euclidean space R^4, the regular polychora (4-polytopes)
 * represent the highest degree of spatial symmetry in geometry. Unlike 3D space
 * which has exactly 5 Platonic solids, 4D Euclidean space hosts 6 convex regular
 * polychora governed by Coxeter-Dynkin reflection groups:
 *   1. 5-Cell (Hyper-tetrahedron, {3,3,3}, A_4 symmetry, 5 vertices)
 *   2. 8-Cell (Tesseract / Hypercube, {4,3,3}, B_4/C_4 symmetry, 16 vertices)
 *   3. 16-Cell (Cross-polytope / Orthoplex, {3,3,4}, B_4/C_4 symmetry, 8 vertices)
 *   4. 24-Cell (Icositetrachoron, {3,4,3}, F_4 exceptional symmetry, 24 vertices)
 *   5. 120-Cell (Hecatonicosachoron, {5,3,3}, H_4 symmetry, 600 vertices)
 *   6. 600-Cell (Hexacosichoron, {3,3,5}, H_4 symmetry, 120 vertices)
 *
 * Key Mathematical Formulations in this Engine:
 * ---------------------------------------------
 * 1. Geodesic Homotopy on the 3-Sphere S^3:
 *    A naive linear blend (1 - t)*v_A + t*v_B collapses polytope vertices through
 *    the singular origin (0,0,0,0) in R^4.
 *    Instead, we evaluate the geodesic Spherical Linear Interpolation (Slerp):
 *        v(t) = [sin((1 - t)*theta) / sin(theta)] * v_A + [sin(t*theta) / sin(theta)] * v_B
 *    where theta = arccos(clamp(v_A . v_B, -1, 1)).
 *    This guarantees that every interpolated vertex glides along the Riemannian
 *    great-circle geodesic on S^3, preserving unit radius and metric curvature.
 *
 * 2. Double-Isoclinic SO(4) Rotations:
 *    The Lie group SO(4) is locally isomorphic to SU(2) x SU(2) / Z_2. Any 4D
 *    rotation decomposes into a pair of commuting left and right isoclinic rotations:
 *        R_SO(4)(alpha, beta) = R_Left(alpha) * R_Right(beta)
 *    which eliminates gimbal lock in 4-space and generates double-toroidal flow.
 *
 * 3. Conformal Stereographic Projection pi: S^3 -> R^3:
 *    Projects the 4D hypersphere onto 3D visual space from projection pole (0,0,0,R_pole):
 *        (X, Y, Z) = (x, y, z) / (1 - w / R_pole)
 *    Conformal mappings preserve infinitesimal angles and circles, transforming
 *    4D circular fibers and polytope edges into smooth 3D orbits and Villarceau circles.
 * ============================================================================
 */

#pragma once

#include "Sculpture4DFoliation.h"
#include "SculptureProgramSchema.h"
#include "SculptureIsosurface.h"
#include "SculptureGeometryExporter.h"
#include "SculptureWorldGpuAbi.h"
#include <array>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <string_view>

namespace epi_sculpt {

enum class PolychoronKind : std::uint8_t {
    Tesseract8Cell = 0,     // 4D Hypercube ({4,3,3}, 16 vertices, 32 edges)
    Hexadecachoron16 = 1,   // 4D Orthoplex ({3,3,4}, 8 vertices, 24 edges)
    Icositetrachoron24 = 2, // 4D 24-Cell ({3,4,3}, 24 vertices, 96 edges)
    Hexacosichoron600 = 3,  // 4D 600-Cell ({3,3,5}, 120 vertices, 720 edges)
    Hecatonicosachoron120 = 4, // 4D 120-Cell ({5,3,3}, 600 vertices, 1200 edges)
    CliffordTorus = 5       // Flat Clifford Torus (S^1 x S^1 subset S^3, 64 sample nodes)
};

inline constexpr std::size_t kPolychoronKindCount = 6U;

[[nodiscard]] constexpr std::string_view polychoronName(PolychoronKind kind) noexcept {
    switch (kind) {
    case PolychoronKind::Tesseract8Cell:     return "Tesseract 8-Cell {4,3,3}";
    case PolychoronKind::Hexadecachoron16:   return "Orthoplex 16-Cell {3,3,4}";
    case PolychoronKind::Icositetrachoron24: return "24-Cell Icositetrachoron {3,4,3}";
    case PolychoronKind::Hexacosichoron600:  return "600-Cell Hexacosichoron {3,3,5}";
    case PolychoronKind::Hecatonicosachoron120: return "120-Cell Hecatonicosachoron {5,3,3}";
    case PolychoronKind::CliffordTorus:      return "Clifford Flat Torus (S^1 x S^1)";
    default:                                 return "Unknown Polychoron";
    }
}

// Single vertex in 4D space with semantic metadata
struct PolychoronVertex4D {
    SculptureVec4 position{0.0f, 0.0f, 0.0f, 1.0f};
    std::uint32_t semanticBand = 0U; // 0 = Core/Form, 1 = Lattice/Structure, 2 = Apex/Accent
    float energyPhase = 0.0f;        // Phase along Coxeter orbit
};

// Canonical edge connecting two vertices in 4-space
struct PolychoronEdge {
    std::uint32_t v0 = 0U;
    std::uint32_t v1 = 0U;
    std::uint32_t semanticBand = 0U;
};

// Projected 3D instance ready for instanced drawing or raymarching
struct ProjectedVertex3D {
    SculptureVec3 position{0.0f, 0.0f, 0.0f};
    SculptureVec3 axisScale{1.0f, 1.0f, 1.0f};
    float uniformScale = 0.15f;
    float wCoordinate = 0.0f;
    float opacity = 1.0f;
    std::uint32_t semanticBand = 0U;
    float hueShift = 0.0f;
};

struct PolychoronHomotopyConfig {
    PolychoronKind sourcePolytope = PolychoronKind::Tesseract8Cell;
    PolychoronKind targetPolytope = PolychoronKind::Icositetrachoron24;
    float morphProgress = 0.0f;       // Homotopy blend factor t in [0.0, 1.0]
    float rotorAlpha = 0.0f;          // Left isoclinic rotor angle (radians)
    float rotorBeta = 0.0f;           // Right isoclinic rotor angle (radians)
    float rotorTheta4D = 0.0f;        // Z-W planar rotor angle (radians)
    float latentW = 0.0f;             // Hyperplane slicing coordinate [-1.5, 1.5]
    float sliceThickness = 0.45f;     // Hyperplane slicing depth window
    float projectionPoleR = 1.85f;    // Conformal stereographic projection pole distance
    bool useStereographic = true;     // True = Stereographic S^3 -> R^3; False = Slicing (W = latentW)
    float scale = 1.0f;               // Overall spatial scale

    [[nodiscard]] bool operator==(const PolychoronHomotopyConfig&) const noexcept = default;
};

class Sculpture4DPolychoronHomotopy {
public:
    static constexpr std::size_t kMaxHomotopyVertices = 120U;

    explicit Sculpture4DPolychoronHomotopy(const PolychoronHomotopyConfig& config = PolychoronHomotopyConfig{});

    void setConfig(const PolychoronHomotopyConfig& config) noexcept { config_ = config; }
    [[nodiscard]] const PolychoronHomotopyConfig& config() const noexcept { return config_; }

    // Evaluates the continuous homotopy blend and returns projected 3D vertex positions
    [[nodiscard]] std::vector<ProjectedVertex3D> evaluateHomotopy(float simulatedTime) const;

    // Evaluates a single vertex along the S^3 geodesic homotopy arc
    [[nodiscard]] static SculptureVec4 slerpS3(
        const SculptureVec4& a,
        const SculptureVec4& b,
        float t
    ) noexcept;

    // Conformal stereographic projection: S^3 -> R^3
    [[nodiscard]] static SculptureVec3 projectStereographic(
        const SculptureVec4& p,
        float poleDistance = 1.85f
    ) noexcept;

    // Generates the canonical vertex basis for a specified regular polychoron
    static std::vector<PolychoronVertex4D> generateCanonicalVertices(PolychoronKind kind);

    // Generates the canonical Coxeter edge connectivity for a specified regular polychoron
    static std::vector<PolychoronEdge> generateCanonicalEdges(PolychoronKind kind);

    // Generates curvilinear 3D streamlines along projected S^3 great-circle edge arcs
    [[nodiscard]] std::vector<std::vector<SculptureVec3>> generateHomotopyEdgeCurves(
        float simulatedTime,
        std::size_t samplesPerEdge = 4U
    ) const;

    // Generates a complete 3D polygon tube mesh (IsosurfaceMesh) for the 4D polychoron lattice
    [[nodiscard]] IsosurfaceMesh generateHomotopyMesh(
        float simulatedTime,
        const StreamlineTubeConfig& tubeConfig = {}
    ) const;

    // Emits GPU-compatible world instances for all vertices and struts
    [[nodiscard]] std::vector<world_gpu_abi::WorldInstanceGpu> generateWorldInstances(
        float simulatedTime
    ) const;

    // Evaluates the Coxeter dynamic order parameter for telemetry & HUD presentation
    [[nodiscard]] float evaluateCoxeterOrderParameter(float t) const noexcept;

private:
    PolychoronHomotopyConfig config_{};
};

} // namespace epi_sculpt
