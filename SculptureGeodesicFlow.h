/**
 * ============================================================================
 * EPI2 - HYPERBOLIC GEODESIC FLOW & POINCARÉ MANIFOLD ENGINE
 * ============================================================================
 *
 * Educational Annotation: Non-Euclidean Geometry & Geodesic Dynamics in H^3
 * -------------------------------------------------------------------------
 * Extends the 3D epimorphism visual space into Riemannian manifolds of constant
 * negative sectional curvature K = -1 (Hyperbolic 3-Space H^3).
 *
 * Mathematical Foundations:
 * 1. Poincaré Ball Model (B^3):
 *    Metric tensor: ds^2 = 4 * (dx^2 + dy^2 + dz^2) / (1 - ||x||^2)^2
 *    Conformal representation inside the open unit ball ||x|| < 1.
 *
 * 2. Lorentz / Hyperboloid Model (H^3 in R^(3,1)):
 *    Minkowski inner product: <x, y>_M = -x0*y0 + x1*y1 + x2*y2 + x3*y3
 *    Hyperboloid sheet: <x, x>_M = -1, x0 >= 1.
 *    Geodesics are exact hyperbolas: gamma(t) = cosh(t)*p + sinh(t)*v.
 *
 * 3. Regular Hyperbolic Honeycombs & Coxeter Groups:
 *    Tessellations of H^3 by regular hyperbolic polyhedra:
 *    - Order-5 Cubic Honeycomb {4, 3, 5}
 *    - Order-3 Icosahedral Honeycomb {3, 5, 3}
 *    - Order-5 Dodecahedral Honeycomb {5, 3, 5}
 *    Distance is evaluated by iterative Coxeter reflection folding.
 *
 * 4. Horosphere Foliations & Loxodromic Streamline Advection:
 *    Horospheres (limit spheres centered at the ideal boundary S^2_infty) act
 *    as parabolic wavefronts that shape kinetic flow fields.
 * ============================================================================
 */

#pragma once

#include "SculptureProgramSchema.h"
#include "Sculpture4DFoliation.h"
#include <array>
#include <span>
#include <string_view>
#include <cmath>
#include <cstdint>

namespace epi_sculpt {

enum class HyperbolicHoneycombType : std::uint8_t {
    Order5Cubic = 0,       // {4, 3, 5} Cubic cells with 5 cubes sharing each edge
    Order3Icosahedral = 1, // {3, 5, 3} Icosahedral cells with 3 icosahedra per edge
    Order5Dodecahedral = 2,// {5, 3, 5} Dodecahedral cells with 5 dodecahedra per edge
    HorosphereWave = 3,    // Parabolic horospherical foliation
    LoxodromicSpiral = 4   // Loxodromic helical flow along a geodesic axis
};

[[nodiscard]] constexpr std::string_view name(HyperbolicHoneycombType type) noexcept {
    switch (type) {
    case HyperbolicHoneycombType::Order5Cubic: return "Order-5 Cubic {4,3,5}";
    case HyperbolicHoneycombType::Order3Icosahedral: return "Order-3 Icosahedral {3,5,3}";
    case HyperbolicHoneycombType::Order5Dodecahedral: return "Order-5 Dodecahedral {5,3,5}";
    case HyperbolicHoneycombType::HorosphereWave: return "Horosphere Wavefront";
    case HyperbolicHoneycombType::LoxodromicSpiral: return "Loxodromic Spiral";
    }
    return "Order-5 Cubic";
}

struct GeodesicFlowConfig {
    HyperbolicHoneycombType honeycombType = HyperbolicHoneycombType::Order5Cubic;
    float curvature = -1.0f;       // Sectional curvature K (strictly negative)
    float flowSpeed = 0.05f;       // Geodesic advection velocity
    float loxodromicPitch = 0.35f; // Pitch angle for loxodromic rotation
    float horosphereFrequency = 0.25f; // Frequency of nested horospheres
    float foldingIterations = 8.0f;// Number of Coxeter reflection steps
    float boundaryRadius = 0.95f;  // Poincaré ball clipping boundary (< 1.0)
};

class SculptureGeodesicFlow {
public:
    // Conformal & Isometric Coordinate Maps
    [[nodiscard]] static SculptureVec4 poincareToLorentz(const SculptureVec3& p) noexcept;
    [[nodiscard]] static SculptureVec3 lorentzToPoincare(const SculptureVec4& h) noexcept;

    // Hyperbolic Metric & Distance
    [[nodiscard]] static float hyperbolicDistance(const SculptureVec3& a, const SculptureVec3& b) noexcept;
    [[nodiscard]] static float poincareMetricScale(const SculptureVec3& p) noexcept;

    // Geodesic Propagation & Streamline Integration
    [[nodiscard]] static SculptureVec3 advanceGeodesic(
        const SculptureVec3& position,
        const SculptureVec3& velocity,
        float t
    ) noexcept;

    [[nodiscard]] static SculptureVec3 evaluateLoxodromicFlow(
        const SculptureVec3& pos,
        float time,
        float pitch
    ) noexcept;

    // Hyperbolic Honeycomb & Horosphere Distance Estimators
    [[nodiscard]] static float sampleHoneycombSDF(
        const SculptureVec3& p,
        HyperbolicHoneycombType type,
        std::uint32_t maxIterations = 8
    ) noexcept;

    [[nodiscard]] static float sampleHorosphereField(
        const SculptureVec3& p,
        float time,
        float frequency
    ) noexcept;

    // Streamline Advection across Sample Batches
    static void applyGeodesicFlow(
        std::span<SculptureSample> samples,
        float time,
        const GeodesicFlowConfig& config
    ) noexcept;

    // Coxeter Mirror Reflection Primitive in Minkowski Space
    [[nodiscard]] static SculptureVec4 reflectMinkowski(
        const SculptureVec4& p,
        const SculptureVec4& normal
    ) noexcept;
};

} // namespace epi_sculpt
