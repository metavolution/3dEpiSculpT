#pragma once

#include "SculptureProgramSchema.h"
#include "SculptureIsosurface.h"
#include "SculptureGeometryExporter.h"
#include "SculptureSpatialGestureEngine.h"
#include <vector>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <string_view>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: 4D HOPF FIBRATION & CLIFFORD GEODESIC ENGINE
// -----------------------------------------------------------------------------
// The quaternionic Hopf fibration maps points on the 3-sphere in 4D space
// S^3 \subset \mathbb{H} to the 2-sphere S^2 \subset \mathbb{R}^3 via the
// submersion:
//
//     \pi(q) = (2(q_x q_z + q_w q_y), 2(q_y q_z - q_w q_x), q_w^2 + q_z^2 - q_x^2 - q_y^2)
//
// Key Geometrical Invariants:
// 1. Fiber Invariant: Every fiber \pi^{-1}(p) for p \in S^2 is a great circle in S^3.
// 2. Villarceau Foliation: Under stereographic projection \sigma: S^3 \to \mathbb{R}^3,
//    the preimage fibers project onto linked Villarceau circles that foliate
//    nested Clifford tori in 3D Euclidean space.
// 3. Geodesic Tangent Field: Evaluates the smooth 4D tangent vector field of
//    geodesic flow, allowing particles to advect through intertwining Clifford
//    torus orbits without intersection.
// 4. Conformal Deformation: Maps 4D isoclinic rotations (Left/Right SO(4) actions)
//    into dynamic 3D knot transformations and vortex twists.
// -----------------------------------------------------------------------------

struct HopfFiberPoint {
    SculptureVec3 position{};
    SculptureVec3 tangent{};
    float torusRadius = 0.0f;
    float phaseAngle = 0.0f;
    float curvature = 0.0f;
};

struct HopfFoliationConfig {
    std::size_t numTori = 4U;               // Nested Clifford tori count
    std::size_t fibersPerTorus = 12U;        // Fibers per torus
    std::size_t pointsPerFiber = 64U;        // Resolution per Villarceau circle
    float stereographicScale = 2.0f;        // 4D -> 3D stereographic projection radius
    float isoclinicRotationRateLeft = 0.5f;  // Left SO(4) isoclinic spin
    float isoclinicRotationRateRight = 0.3f; // Right SO(4) isoclinic spin
    float tubeRadius = 0.025f;              // Radius of extruded streamline tubes
    bool closeLoops = true;
};

class SculptureHopfEngine {
public:
    // 4D Quaternionic Hopf Map: S^3 -> S^2
    [[nodiscard]] static SculptureVec3 projectHopf(const Quaternion4D& q) noexcept;

    // Evaluates stereographic projection: S^3 \setminus \{(0,0,0,1)\} -> \mathbb{R}^3
    [[nodiscard]] static SculptureVec3 stereographicProject(const Quaternion4D& q, float scale = 1.0f) noexcept;

    // Inverse stereographic projection: \mathbb{R}^3 -> S^3
    [[nodiscard]] static Quaternion4D inverseStereographicProject(const SculptureVec3& p, float scale = 1.0f) noexcept;

    // Evaluates a specific 3D Villarceau circle point for a given base point on S^2 and fiber angle \xi \in [0, 2\pi)
    [[nodiscard]] static HopfFiberPoint evaluateFiberPoint(
        const SculptureVec3& basePointS2,
        float fiberAngle,
        float stereographicScale = 2.0f,
        float thetaL = 0.0f,
        float thetaR = 0.0f
    ) noexcept;

    // Evaluates the 3D velocity vector induced by 4D Hopf geodesic flow at position p
    [[nodiscard]] static SculptureVec3 evaluateHopfGeodesicVelocity(
        const SculptureVec3& p,
        float stereographicScale = 2.0f,
        float twistRate = 1.0f
    ) noexcept;

    // Generates complete collection of 3D streamlines foliating the 4D Hopf bundle
    [[nodiscard]] static std::vector<std::vector<SculptureVec3>> generateFoliationStreamlines(
        const HopfFoliationConfig& config,
        float simTime = 0.0f
    );

    // Generates a watertight 3D triangle mesh of the Hopf foliation tubes
    [[nodiscard]] static IsosurfaceMesh generateFoliationMesh(
        const HopfFoliationConfig& config,
        float simTime = 0.0f
    );
};

} // namespace epi_sculpt
