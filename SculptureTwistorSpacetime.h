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
#include <complex>

namespace epi_sculpt {

// -----------------------------------------------------------------------------
// Educational Annotation: PENROSE TWISTORS & KERR-NEWMAN SPACETIME ERGOSPHERE
// -----------------------------------------------------------------------------
// In Penrose Twistor Theory, 4D Minkowski spacetime points x^\\mu = (t, x, y, z)
// are represented as projective 2-planes in 4-dimensional complex Twistor space
// T \\cong C^4 via the incidence relation:
//     \\omega^A = i x^{AA\x27} \\pi_{A\x27}
// where x^{AA\x27} = 1/sqrt(2) * [[t + z, x - i*y], [x + i*y, t - z]].
//
// A Robinson Congruence is the spacetime manifestation of a single twistor Z^\\alpha,
// forming a space-filling family of shear-free, twisting null geodesics (optical rays)
// whose linking number is invariant (Lk = 1) and whose stereographic slice
// is isomorphic to the Hopf fibration.
//
// In General Relativity, the Kerr-Newman metric describes a spinning, charged black hole:
// 1. Event Horizon:
//      r_+ = M + \\sqrt{M^2 - a^2 - Q^2}
// 2. Static Limit Surface (Ergosphere boundary):
//      r_E(\\theta) = M + \\sqrt{M^2 - a^2 \\cos^2\\theta - Q^2}
// 3. Lense-Thirring Frame Dragging Angular Velocity:
//      \\Omega(r, \\theta) = \\frac{(2 M r - Q^2) a}{(r^2 + a^2)^2 - a^2 \\Delta \\sin^2\\theta}
//      where \\Delta = r^2 - 2 M r + a^2 + Q^2.
// -----------------------------------------------------------------------------

struct TwistorSpinor {
    std::complex<float> omega0{0.0f, 0.0f};
    std::complex<float> omega1{0.0f, 0.0f};
    std::complex<float> pi0{0.0f, 0.0f};
    std::complex<float> pi1{0.0f, 0.0f};

    [[nodiscard]] float normSq() const noexcept {
        return (std::conj(omega0) * pi0 + std::conj(omega1) * pi1 +
                omega0 * std::conj(pi0) + omega1 * std::conj(pi1)).real();
    }
};

struct KerrNewmanConfig {
    float mass = 1.0f;               // Mass parameter M
    float spin = 0.85f;              // Angular momentum per unit mass a = J/M (|a| <= M)
    float charge = 0.30f;            // Electric charge Q (Q^2 + a^2 <= M^2)
    float horizonScale = 1.2f;       // Spatial visualization scale
    float diskInnerRadius = 1.8f;    // Accretion disk inner boundary
    float diskOuterRadius = 4.5f;    // Accretion disk outer boundary
    float frameDragStrength = 1.0f;  // Lense-Thirring angular velocity multiplier
    std::size_t orbitSampleCount = 256U; // Number of trajectory samples per orbit
};

struct TwistorCongruenceConfig {
    float helicity = 0.5f;           // Twistor conformal helicity s = 1/2 * \\Sigma(Z)
    float twistRate = 1.25f;         // Robinson congruence shear-free twist rate
    float rayLength = 3.5f;          // Length of null geodesic rays
    std::size_t rayCount = 64U;      // Number of congruence null rays
    std::size_t samplesPerRay = 32U; // Discretization resolution per ray
    float timeEvolutionSpeed = 0.4f; // Conformal time evolution
};

class SculptureTwistorSpacetime {
public:
    // Computes Penrose Incidence Twistor Z = (\\omega^A, \\pi_{A\x27}) for spacetime point (t, x, y, z)
    [[nodiscard]] static TwistorSpinor evaluateIncidenceTwistor(
        float t, float x, float y, float z,
        const std::complex<float>& pi0 = {1.0f, 0.0f},
        const std::complex<float>& pi1 = {0.0f, 1.0f}
    ) noexcept;

    // Evaluates Robinson Shear-free Null Congruence spatial tangent vector at (x, y, z)
    [[nodiscard]] static SculptureVec3 evaluateRobinsonVelocity(
        const SculptureVec3& pos,
        float time,
        float twistRate = 1.0f
    ) noexcept;

    // Evaluates Kerr-Newman Outer Event Horizon radius r_+
    [[nodiscard]] static float evaluateOuterHorizonRadius(
        float mass, float spin, float charge
    ) noexcept;

    // Evaluates Kerr-Newman Ergosphere boundary radius r_E(\\theta)
    [[nodiscard]] static float evaluateErgosphereRadius(
        float theta, float mass, float spin, float charge
    ) noexcept;

    // Evaluates Lense-Thirring Frame Dragging Angular Velocity \\Omega(r, \\theta)
    [[nodiscard]] static float evaluateFrameDraggingOmega(
        float r, float theta, float mass, float spin, float charge
    ) noexcept;

    // Converts Boyer-Lindquist coordinates (r, \\theta, \\phi) to Kerr-Schild Cartesian (x, y, z)
    [[nodiscard]] static SculptureVec3 boyerLindquistToCartesian(
        float r, float theta, float phi, float spin
    ) noexcept;

    // Generates multi-scale particle orbits in Kerr-Newman ergosphere and accretion disk
    [[nodiscard]] static std::vector<SculptureSample> generateErgosphereOrbits(
        const KerrNewmanConfig& config,
        float time
    );

    // Generates Robinson Twistor Congruence null ray trajectories
    [[nodiscard]] static std::vector<SculptureSample> generateRobinsonCongruenceRays(
        const TwistorCongruenceConfig& config,
        float time
    );

    // Generates an immutable SculptureSnapshot combining Ergosphere and Twistor fields
    [[nodiscard]] static SculptureSnapshot generateSnapshot(
        const KerrNewmanConfig& kerrConfig,
        const TwistorCongruenceConfig& twistorConfig,
        float time,
        float morph = 0.0f
    );

    // Generates a 3D watertight triangulated mesh for the Kerr-Newman Ergosphere
    [[nodiscard]] static IsosurfaceMesh generateErgosphereMesh(
        const KerrNewmanConfig& config,
        std::size_t uSegments = 48U,
        std::size_t vSegments = 24U
    );

    // Generates a 3D watertight triangulated mesh for the Event Horizon
    [[nodiscard]] static IsosurfaceMesh generateEventHorizonMesh(
        const KerrNewmanConfig& config,
        std::size_t uSegments = 48U,
        std::size_t vSegments = 24U
    );
};

} // namespace epi_sculpt
