#pragma once

#include "QuaternionAlgebra.h"
#include "SculptureProgramSchema.h"
#include "SculptureIsosurface.h"
#include "Sculpture4DFoliation.h"

#include <vector>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <string_view>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: 4D CLIFFORD TORUS KNOT BRAIDING & HOMOTOPY INVARIANTS
// -----------------------------------------------------------------------------
// A (p, q)-torus knot is an embedded closed loop that winds p times longitudinally
// and q times meridionally on a torus. In 4D hypercomplex space, the Clifford torus
// T^2 = S^1 x S^1 is a flat, uncurved minimal sub-manifold of the 3-sphere S^3 \subset H.
//
// The parametric 4D quaternion trajectory is given by:
//     q(t) = 1/sqrt(2) * ( cos(p*t), sin(p*t), cos(q*t + phi), sin(q*t + phi) ) in S^3
//
// Under conformal stereographic projection sigma: S^3 \ { (0,0,0,1) } -> R^3:
//     r(t) = sigma(q(t)) = s / (1 - q_w(t)) * ( q_x(t), q_y(t), q_z(t) )
//
// Key Mathematical Properties & Invariants:
// 1. Frenet-Serret Orthonormal Frame (T, N, B):
//      T(t) = r'(t) / ||r'(t)||
//      B(t) = (r'(t) x r''(t)) / ||r'(t) x r''(t)||
//      N(t) = B(t) x T(t)
// 2. Differential Invariants (Curvature kappa and Torsion tau):
//      kappa(t) = ||r'(t) x r''(t)|| / ||r'(t)||^3
//      tau(t)   = ((r'(t) x r''(t)) . r'''(t)) / ||r'(t) x r''(t)||^2
// 3. Gauss Linking Integral:
//      Lk(gamma_1, gamma_2) = 1/(4*pi) * oint oint ((r1 - r2) / ||r1 - r2||^3) . (dr1 x dr2)
// 4. Continuous Homotopy Deformation:
//      Interpolates (p_1, q_1) -> (p_2, q_2) smoothly, generating intertwining
//      braid structures with dynamic topological self-intersection transitions.
// 5. 4D Geodesic Flow Advection:
//      Advects the Clifford knot trajectory along tangent Killing vector fields of
//      SO(4) and hyper-manifold foliation fields, maintaining the S^3 unit sphere invariant.
// -----------------------------------------------------------------------------

struct KnotFramePoint {
    SculptureVec3 position{};
    SculptureVec3 tangent{};
    SculptureVec3 normal{};
    SculptureVec3 binormal{};
    float curvature = 0.0f;
    float torsion = 0.0f;
    float arcLength = 0.0f;
    float parameterT = 0.0f;
};

struct KnotHomotopyConfig {
    float p = 3.0f;                          // Longitudinal winding number
    float q = 2.0f;                          // Meridional winding number (e.g. 3,2 = Trefoil knot)
    float phaseOffset = 0.0f;                // Meridional phase angle phi
    float stereographicScale = 2.0f;         // S^3 -> R^3 projection scale s
    std::size_t sampleCount = 256U;          // Trajectory sample count per revolution
    float tubeRadius = 0.045f;               // Ribbon / tube cross-section radius
    std::size_t radialSegments = 16U;        // Radial tessellation segments for geometry
    float leftIsoclinicSpeed = 0.4f;         // 4D Left SO(4) isoclinic spin
    float rightIsoclinicSpeed = 0.25f;       // 4D Right SO(4) isoclinic spin
    bool closeLoop = true;                   // Periodic closed loop
    Manifold4DType foliationManifold = Manifold4DType::CliffordTorus; // 4D manifold for vector field flow
    float geodesicFlowSpeed = 0.0f;          // Advection flow velocity magnitude
};

struct KnotTopologicalInvariants {
    float totalTwist = 0.0f;       // Tw = (1/2pi) * \int \tau ds
    float totalCurvature = 0.0f;   // K_tot = \int \kappa ds (>= 2pi by Fenchel's theorem)
    float totalArcLength = 0.0f;   // Total closed trajectory length
    float maxCurvature = 0.0f;
    float maxTorsion = 0.0f;
    bool isFenchelSatisfied = false; // K_tot >= 2*pi
};

class SculptureKnotHomotopy {
public:
    // Evaluates 4D Quaternionic knot point q(t) in S^3
    [[nodiscard]] static Quaternion evaluateKnot4D(
        float t, float p, float q, float phi = 0.0f) noexcept;

    // Advects a 4D quaternionic knot coordinate along the 4D manifold's geodesic velocity vector field
    [[nodiscard]] static Quaternion advectKnotPoint4D(
        const Quaternion& q,
        Manifold4DType manifold,
        float time,
        float flowMagnitude = 1.0f
    ) noexcept;

    // Evaluates stereographic projection from S^3 to 3D Euclidean space R^3
    [[nodiscard]] static SculptureVec3 stereographicProject(
        const Quaternion& q, float scale = 1.0f) noexcept;

    // Inverse stereographic projection from R^3 to S^3
    [[nodiscard]] static Quaternion inverseStereographicProject(
        const SculptureVec3& p, float scale = 1.0f) noexcept;

    // Computes complete Frenet-Serret frame, curvature, and torsion at parameter t
    [[nodiscard]] static KnotFramePoint evaluateKnotFrame(
        float t,
        const KnotHomotopyConfig& config,
        float time = 0.0f
    ) noexcept;

    // Generates a discretized trajectory of Frenet frames along the (p, q) knot
    [[nodiscard]] static std::vector<KnotFramePoint> generateTrajectory(
        const KnotHomotopyConfig& config,
        float time = 0.0f
    );

    // Computes the numerical Gauss Linking Number between two closed 3D curves
    [[nodiscard]] static float computeGaussLinkingNumber(
        const std::vector<KnotFramePoint>& curveA,
        const std::vector<KnotFramePoint>& curveB
    ) noexcept;

    // Computes differential and integral topological invariants along the closed knot curve
    [[nodiscard]] static KnotTopologicalInvariants computeTopologicalInvariants(
        const std::vector<KnotFramePoint>& trajectory
    ) noexcept;

    // Generates an N-strand topological braid with equal angular phase spacing
    [[nodiscard]] static std::vector<std::vector<KnotFramePoint>> generateMultiStrandBraid(
        const KnotHomotopyConfig& baseConfig,
        std::size_t numStrands = 3U,
        float time = 0.0f,
        float strandTwistSpeed = 0.0f
    );

    // Generates an immutable SculptureSnapshot populated with particle samples across all N braided strands
    [[nodiscard]] static SculptureSnapshot generateMultiStrandSnapshot(
        const KnotHomotopyConfig& baseConfig,
        std::size_t numStrands = 3U,
        float time = 0.0f,
        float morph = 0.0f,
        float strandTwistSpeed = 0.0f
    );

    // Generates a continuous homotopy blend between knot (p1, q1) and (p2, q2)
    [[nodiscard]] static std::vector<KnotFramePoint> generateHomotopyMorph(
        const KnotHomotopyConfig& startConfig,
        const KnotHomotopyConfig& targetConfig,
        float morphProgress,
        float time = 0.0f
    );

    // Generates an immutable SculptureSnapshot populated with particle samples along the knot
    [[nodiscard]] static SculptureSnapshot generateSnapshot(
        const KnotHomotopyConfig& config,
        float time = 0.0f,
        float morph = 0.0f
    );

    // Generates a watertight 3D tubular mesh around the braided knot
    [[nodiscard]] static IsosurfaceMesh generateTubeMesh(
        const KnotHomotopyConfig& config,
        float time = 0.0f
    );

    // Generates a 3D ribbon / strip mesh using the normal and binormal vectors of the Frenet frame
    [[nodiscard]] static IsosurfaceMesh generateRibbonMesh(
        const KnotHomotopyConfig& config,
        float ribbonWidth = 0.12f,
        float ribbonTwistRate = 0.0f,
        float time = 0.0f
    );

    // Rasterizes multi-strand braided knot trajectories onto a 2D scalar potential field grid
    [[nodiscard]] static std::vector<float> rasterizeBraidTo2DField(
        const KnotHomotopyConfig& config,
        std::size_t numStrands = 3U,
        std::uint32_t width = 256U,
        std::uint32_t height = 256U,
        float time = 0.0f,
        float splatRadius = 0.04f
    );
};

} // namespace epi_sculpt
