#pragma once

#include "OctonionAlgebra.h"
#include "SculptureProgramSchema.h"
#include "SculptureIsosurface.h"
#include <vector>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <string_view>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: 8D OCTONIONIC EXCEPTIONAL G2 FIBRATION ENGINE
// -----------------------------------------------------------------------------
// The octonionic Hopf fibration maps the 7-sphere in 8D hypercomplex space
// S^7 \subset \mathbb{O} onto the 4-sphere S^4 with fibers isomorphic to S^3:
//
//     \pi_{\mathbb{O}}(X) = (2 A B^*, |A|^2 - |B|^2) \in S^4 \subset \mathbb{H} \times \mathbb{R}
//     where X = (A, B) \in \mathbb{H} \times \mathbb{H} \cong \mathbb{O}
//
// Key Mathematical Invariants:
// 1. Non-Associative Associator: Measures algebraic curvature in 8-space:
//        [X, Y, Z] = (X * Y) * Z - X * (Y * Z) \neq 0
//    Points with maximal associator norm produce non-Euclidean chromatic halos.
// 2. G2 Exceptional Lie Group Automorphism:
//    The 14-dimensional Lie group G2 = Aut(\mathbb{O}) acts transitively on S^6
//    and preserves the octonionic Fano plane multiplication table.
// 3. Villarceau-Octonion Double Projection:
//    Projects S^7 -> S^4 -> \mathbb{R}^3 using nested Fano triad rotations,
//    foliating 3D space with interwoven chiral hyper-torus streamlines.
// -----------------------------------------------------------------------------

struct OctonionFiberPoint {
    SculptureVec3 position{};
    SculptureVec3 tangent{};
    float associatorCurvature = 0.0f;
    float g2InvariantPhase = 0.0f;
    float materialCoord = 0.0f;
};

struct OctonionFibrationConfig {
    std::size_t numHyperShells = 3U;          // Nested 8D hyper-spherical shells
    std::size_t fibersPerShell = 16U;         // Fibers per hyper-shell
    std::size_t pointsPerFiber = 64U;         // Resolution per streamline loop
    float g2RotationRate = 0.25f;             // G2 automorphism spin speed
    float associatorGain = 0.40f;             // Non-associative curvature weighting
    float projectionScale = 2.2f;             // 8D -> 3D stereographic scale
    float tubeRadius = 0.022f;                // Extruded tube radius
    std::uint32_t radialSegments = 6U;        // Tube cross-section polygon vertices
    bool closeLoops = true;
};

class SculptureOctonionFibration {
public:
    // Octonionic 8D -> 4D -> 3D Hopf-like submersion
    [[nodiscard]] static SculptureVec3 projectOctonionHopf(const Octonion& o, float scale = 2.2f) noexcept;

    // Evaluates non-associative curvature norm ||[X, Y, Z]||
    [[nodiscard]] static float computeAssociatorCurvature(
        const Octonion& x, const Octonion& y, const Octonion& z
    ) noexcept;

    // Evaluates a specific 3D fiber point along the S^7 fiber trajectory
    [[nodiscard]] static OctonionFiberPoint evaluateFiberPoint(
        const Octonion& baseOctonion,
        float fiberAngle,
        float g2Angle,
        float associatorGain = 0.40f,
        float scale = 2.2f
    ) noexcept;

    // Generates complete collection of 3D streamlines foliating the 8D G2 manifold
    [[nodiscard]] static std::vector<std::vector<SculptureVec3>> generateFoliationStreamlines(
        const OctonionFibrationConfig& config,
        float simTime = 0.0f
    );

    // Generates a watertight 3D triangle mesh of the octonion foliation streamline tubes
    [[nodiscard]] static IsosurfaceMesh generateFoliationMesh(
        const OctonionFibrationConfig& config,
        float simTime = 0.0f
    );
};

} // namespace epi_sculpt
