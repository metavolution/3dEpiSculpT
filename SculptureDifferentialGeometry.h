/**
 * ============================================================================
 * EPI2 / 3dEpiSculpTMorph - SCULPTURE DIFFERENTIAL GEOMETRY & FRAME EXTRUDER
 * ============================================================================
 *
 * [EDUCATIONAL NOTE: UNIFIED HIGHER-DIMENSIONAL DIFFERENTIAL GEOMETRY CORE]
 * ----------------------------------------------------------------------------
 * Across the 3D/4D/8D mathematical sculpture engines (Sculpture4DFoliation,
 * Sculpture4DPolychoronHomotopy, SculptureHopfEngine, SculptureKnotHomotopy,
 * SculptureOctonionFibration, SculptureRibbonBraider), space curves and
 * higher-dimensional manifolds share four foundational geometric operations:
 *
 * 1. Double-Isoclinic SO(4) Lie Group Rotations:
 *    Since SO(4) ~= (SU(2)_L x SU(2)_R) / Z_2, any 4D orthogonal rotation
 *    decomposes into commuting left and right isoclinic rotations acting on
 *    (x, y, z, w) in R^4 ~= H.
 *
 * 2. Conformal Stereographic Submersion (S^3 <-> R^3):
 *    Projects points on the 3-sphere S^3 \ {(0, 0, 0, R_pole)} conformally
 *    into 3D Euclidean space while preserving infinitesimal angles and mapping
 *    great-circle Hopf fibers to Villarceau circles. A C^0/C^2 pole guard
 *    prevents coordinate blowup as w -> R_pole.
 *
 * 3. Riemannian S^3 Geodesic Homotopy (Slerp):
 *    Interpolates between 4D vertices along the shortest great-circle arc on
 *    S^3 without collapsing through the singular origin (0, 0, 0, 0).
 *
 * 4. Bishop Rotation-Minimizing Frame (Parallel Transport) & Mesh Extrusion:
 *    Unlike the Frenet-Serret frame—whose normal flips discontinuously at
 *    inflection points where curvature kappa -> 0—the Bishop frame transports
 *    an initial normal (N_0, B_0) along the curve via minimal-rotation
 *    reflections (nabla_T N = 0), yielding twist-free tubular and ribbon
 *    extrensions into indexed IsosurfaceMesh geometry.
 * ============================================================================
 */

#pragma once

#include "Sculpture4DFoliation.h"
#include "SculptureIsosurface.h"
#include "SculptureProgramSchema.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <cmath>

namespace epi_sculpt::diff_geom {

struct OrthonormalFrame3D {
    SculptureVec3 position{};
    SculptureVec3 tangent{0.0f, 0.0f, 1.0f};
    SculptureVec3 normal{1.0f, 0.0f, 0.0f};
    SculptureVec3 binormal{0.0f, 1.0f, 0.0f};
    float curvature = 0.0f;
    float torsion = 0.0f;
    float arcLength = 0.0f;
    float parameterU = 0.0f;
};

struct TubeExtrusionSpec {
    float radius = 0.03f;
    std::uint32_t radialSegments = 6U;
    bool closedLoop = true;
};

struct FlatRibbonExtrusionSpec {
    float width = 0.08f;
    float twistFrequency = 0.0f;
    float twistPhase = 0.0f;
    bool closedLoop = false;
    bool useBinormalSpan = true;
};

struct VolumetricRibbonExtrusionSpec {
    float width = 0.075f;
    float thickness = 0.008f;
    bool closedLoop = true;
};

// -----------------------------------------------------------------------------
// 1. 3D & 4D Vector Algebra Helpers
// -----------------------------------------------------------------------------
[[nodiscard]] SculptureVec3 normalize3(
    const SculptureVec3& v,
    const SculptureVec3& fallback = {0.0f, 1.0f, 0.0f}) noexcept;

[[nodiscard]] constexpr float dot3(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

[[nodiscard]] inline float length3(const SculptureVec3& v) noexcept {
    return std::sqrt(dot3(v, v));
}

[[nodiscard]] constexpr SculptureVec3 cross3(const SculptureVec3& a, const SculptureVec3& b) noexcept {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

[[nodiscard]] SculptureVec4 normalize4(const SculptureVec4& v) noexcept;

[[nodiscard]] constexpr float dot4(const SculptureVec4& a, const SculptureVec4& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

// -----------------------------------------------------------------------------
// 2. Canonical SO(4) Isoclinic Rotation & S^3 <-> R^3 Stereographic Projections
// -----------------------------------------------------------------------------
[[nodiscard]] SculptureVec4 rotateSO4Isoclinic(
    const SculptureVec4& p,
    float thetaL,
    float thetaR,
    float phiL = 0.0f,
    float phiR = 0.0f) noexcept;

[[nodiscard]] SculptureVec3 projectStereographicS3(
    const SculptureVec4& p4,
    float scale = 1.0f,
    float poleDistance = 1.0001f,
    float minDenominator = 0.001f) noexcept;

[[nodiscard]] SculptureVec4 inverseStereographicS3(
    const SculptureVec3& p3,
    float scale = 1.0f) noexcept;

// -----------------------------------------------------------------------------
// 3. Riemannian S^3 Great-Circle Geodesic Slerp
// -----------------------------------------------------------------------------
[[nodiscard]] SculptureVec4 slerpS3(
    const SculptureVec4& a,
    const SculptureVec4& b,
    float t) noexcept;

// -----------------------------------------------------------------------------
// 4. Parallel-Transport Bishop Rotation-Minimizing Frame (RMF)
// -----------------------------------------------------------------------------
[[nodiscard]] std::vector<OrthonormalFrame3D> computeBishopFrames(
    std::span<const SculptureVec3> points,
    bool closedLoop = true,
    float twistFrequency = 0.0f,
    float twistPhase = 0.0f);

// -----------------------------------------------------------------------------
// 5. Universal Tube & Ribbon Mesh Extruders into IsosurfaceMesh
// -----------------------------------------------------------------------------
[[nodiscard]] IsosurfaceMesh extrudeTubeFromPoints(
    std::span<const SculptureVec3> points,
    const TubeExtrusionSpec& spec);

[[nodiscard]] IsosurfaceMesh extrudeFlatRibbonFromPoints(
    std::span<const SculptureVec3> points,
    const FlatRibbonExtrusionSpec& spec);

void appendVolumetricRibbonStrand(
    IsosurfaceMesh& mesh,
    std::span<const OrthonormalFrame3D> frames,
    const VolumetricRibbonExtrusionSpec& spec);

// -----------------------------------------------------------------------------
// 6. Topological Gauss Linking Invariant
// -----------------------------------------------------------------------------
[[nodiscard]] float computeGaussLinkingNumber(
    std::span<const SculptureVec3> curveA,
    std::span<const SculptureVec3> curveB) noexcept;

} // namespace epi_sculpt::diff_geom
