#pragma once

#include "SculptureProgramSchema.h"
#include "SculptureIsosurface.h"
#include "SculptureKnotHomotopy.h"
#include "QuaternionAlgebra.h"

#include <vector>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <string_view>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: Multi-Strand Quaternionic Ribbon Braiding Engine
// -----------------------------------------------------------------------------
// Generates N-strand intertwining braided ribbons in 4D hypercomplex space S^3
// projected conformally to R^3 via stereographic submersion.
//
// Key Mathematical Features:
// 1. N-Strand S^3 Braid Invariant: Strands maintain constant hyper-spherical distance
//    via distributed phase angles phi_k = 2*pi*k / N.
// 2. Parallel Transport Frame (Bishop Frame): Evaluates rotation-minimizing
//    orthonormal frames (T, u, v) along the space curve to prevent artificial twist.
// 3. Extruded Dual-Sided Ribbon Geometry: Generates clean, indexed triangle meshes
//    with analytic normals, tangents, and (u, v) parameter coordinates for OKLCH palettes.
// 4. Gauss Linking Number Verification: Evaluates topological entanglement Lk(gamma_i, gamma_j).
// -----------------------------------------------------------------------------

struct RibbonStrandConfig {
    std::size_t numStrands = 3U;              // Number of braided strands (e.g. 3 = Borromean/Trefoil braid)
    float ribbonWidth = 0.075f;               // Transverse ribbon width
    float ribbonThickness = 0.008f;           // Normal ribbon thickness
    float p = 3.0f;                           // Longitudinal winding number
    float q = 2.0f;                           // Meridional winding number
    float braidRadius = 0.35f;                // Radial separation between strands
    float twistFrequency = 4.0f;              // Surface twist oscillation frequency
    float phaseOffset = 0.0f;                 // Base phase offset
    float stereographicScale = 2.0f;          // S^3 -> R^3 stereographic scale factor
    std::size_t samplesPerStrand = 256U;      // Longitudinal sample resolution
    float leftIsoclinicSpeed = 0.35f;         // 4D Left SO(4) isoclinic spin
    float rightIsoclinicSpeed = 0.20f;        // 4D Right SO(4) isoclinic spin
    bool closeLoop = true;                    // Periodic closed loop
};

struct BraidTopologicalMetrics {
    std::vector<float> selfWrithe{};          // Self-writhe per strand
    std::vector<float> totalCurvature{};      // Total curvature per strand
    std::vector<std::vector<float>> linkingMatrix{}; // Pairwise Gauss linking numbers
    bool allFenchelSatisfied = false;         // Total curvature >= 2*pi
    std::size_t totalVertices = 0U;
    std::size_t totalTriangles = 0U;
};

class SculptureRibbonBraider {
public:
    // Evaluates multi-strand 4D trajectory and computes Rotation-Minimizing Orthonormal Frames
    [[nodiscard]] static std::vector<std::vector<KnotFramePoint>> generateBraidFrames(
        const RibbonStrandConfig& config,
        float time = 0.0f
    );

    // Generates an indexed dual-sided 3D triangle mesh of the braided ribbons
    [[nodiscard]] static IsosurfaceMesh generateRibbonMesh(
        const RibbonStrandConfig& config,
        float time = 0.0f
    );

    // Evaluates the Gauss linking number between two closed 3D space curves
    [[nodiscard]] static float computeGaussLinkingNumber(
        const std::vector<KnotFramePoint>& strandA,
        const std::vector<KnotFramePoint>& strandB
    ) noexcept;

    // Computes complete topological invariants and metrics for the braided system
    [[nodiscard]] static BraidTopologicalMetrics evaluateTopologicalMetrics(
        const RibbonStrandConfig& config,
        float time = 0.0f
    );

    // Generates a compatible SculptureProgramParams bundle for immediate rendering
    [[nodiscard]] static SculptureProgramParams makeRibbonProgramParams(
        const RibbonStrandConfig& config
    ) noexcept;
};

} // namespace epi_sculpt
