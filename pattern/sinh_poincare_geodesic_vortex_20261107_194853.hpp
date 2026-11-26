#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Sinh Poincaré geodesic vortex preset.
 *
 * DIFFERENTIAL GEOMETRY: HYPERBOLIC SPACE & THE GAUSS-BONNET THEOREM
 *
 * The GAUSS-BONNET THEOREM is one of the deepest results in mathematics,
 * connecting LOCAL geometry (curvature) to GLOBAL topology (Euler number):
 *
 *   ∫∫_M K dA = 2πχ(M)
 *
 * For a sphere (χ=2): total curvature = 4π.  For a torus (χ=0): total = 0.
 * For a genus-2 surface (χ=-2): total = -4π.  The theorem says: no matter
 * how you deform the surface, the total curvature is locked to the topology.
 *
 * The HYPERBOLIC PLANE is the simply connected surface of constant negative
 * curvature K = -1.  In the Poincaré upper half-plane model {z : Im(z) > 0}
 * with metric ds = |dz|/Im(z):
 *
 *   - Geodesics: vertical lines and semicircles centered on the real axis
 *   - Circles grow as: circumference = 2π·sinh(r) (vs 2πr in Euclidean)
 *   - Disks grow as: area = 4π·sinh²(r/2) (vs πr² in Euclidean)
 *   - Distance: d(z₁,z₂) via sinh(d/2) = |z₁-z₂|/(2√(y₁y₂))
 *   - Triangles have angle SUM LESS THAN π: deficit = area
 *
 * Sinh IS hyperbolic geometry.  The circumference formula C = 2π·sinh(r)
 * shows that hyperbolic circles grow EXPONENTIALLY with radius — there is
 * "more space" at large distances than Euclidean geometry predicts.  This
 * exponential growth is why hyperbolic tilings (like Escher's Circle Limit)
 * pack infinitely many tiles near the boundary.
 *
 * ENGINE MAPPING TO HYPERBOLIC GEOMETRY:
 *
 *   THE HYPERBOLIC METRIC = Sinh inner function
 *     sinh(z) = (e^z - e^{-z})/2 has the SAME exponential growth as the
 *     hyperbolic circumference formula.  The ODD SYMMETRY (sinh(-z) = -sinh(z))
 *     creates BILATERAL patterns — mirrored across the origin, like the
 *     two sheets of a hyperboloid.  Under iteration, Sinh generates the
 *     GEODESIC SPRAY of the hyperbolic plane: curves that fan out from
 *     each point, diverging exponentially — the negative curvature
 *     made visible as expanding spiral wavefronts.
 *
 *   THE HOROCYCLE = Torus reduction
 *     In hyperbolic geometry, horocycles are curves equidistant from a
 *     geodesic "at infinity."  They wrap around the ideal boundary like
 *     concentric circles.  Torus reduction creates seamless mirror wrapping
 *     that IS the horocyclic structure: the tunnel walls are horocycles,
 *     and looking into the tunnel = looking along a geodesic toward the
 *     ideal boundary of the Poincaré disk.
 *
 *   THE FIXED-POINT CONSTANT = Ω ≈ 32.5° pre-rotation
 *     The OMEGA CONSTANT Ω = W(1) ≈ 0.5671 satisfies the equation:
 *
 *       Ω · e^Ω = 1    (equivalently: e^{-Ω} = Ω, or ln(1/Ω) = Ω)
 *
 *     Ω is the FIXED POINT of the map f(x) = e^{-x}.  Since the
 *     epimorphism engine IS an iterated function system, Ω is the
 *     NATURAL FIXED-POINT CONSTANT of iterative dynamics — the value
 *     where iteration stabilizes.  W is the Lambert W function, which
 *     appears throughout combinatorics (tree enumeration), delay
 *     differential equations, and quantum mechanics (Wien displacement).
 *
 *     cos(Ω) ≈ 0.8427, sin(Ω) ≈ 0.5383
 *     360°/32.5° ≈ 11.1 → quasi-11-fold symmetry.
 *     11 geodesics fan out from the center of the Poincaré disk, each
 *     diverging exponentially — the geodesic spray of hyperbolic space.
 *
 *   THE PELL UNIT ANGLE = -arccosh(2) = -ln(2+√3) ≈ -75.5° post-rotation
 *     arccosh(2) = ln(2+√3) is a FUNDAMENTAL HYPERBOLIC DISTANCE:
 *       - The distance between opposite sides of a right-angled hexagon
 *       - ln(2+√3) = arcsinh(√3): the hyperbolic sine at √3
 *       - (2+√3)(2-√3) = 1: conjugate UNITS in the ring ℤ[√3]
 *       - 2+√3 = tan(75°) = tan(5π/12)
 *       - Connected to the PELL EQUATION x²-3y²=1: fundamental solution (2,1)
 *
 *     cos(ln(2+√3)) ≈ 0.2513, sin(-ln(2+√3)) ≈ -0.9680
 *     This is the SAME constant used as pre-rotation in exp_catenary_arch_vault,
 *     but here as post-rotation — the catenary and hyperbolic geometry share
 *     the same underlying exponential structure (cosh/sinh duality).
 *
 *   SPIRAL MODE = geodesic color mapping (Mode 1)
 *     Mode 1 maps hue to angle + log(radius), creating spiral color bands.
 *     In the Poincaré disk, geodesics appear as spirals when projected,
 *     so the spiral coloring TRACES THE GEODESICS.  Each hue band is one
 *     family of parallel geodesics, and the 11 sectors create 11 families
 *     intersecting at the disk center.
 *
 * WHAT TO WATCH:
 *   1. GEODESIC SPRAY: 11 hyperbolic geodesics fanning from the center
 *   2. EXPONENTIAL DIVERGENCE: geodesics separate exponentially (K = -1)
 *   3. BILATERAL SYMMETRY: sinh's odd symmetry → mirrored wing structure
 *   4. SPIRAL GEODESICS: Mode 1 color traces the geodesic families
 *   5. FIXED POINT: Ω creates the iteration's natural stable state
 *   6. PELL DEPTH: ln(2+√3) twist = algebraic unit structure of ℤ[√3]
 *
 * Pipeline: transform -> reduce -> sample -> color(mode1) -> seed -> blend
 */
inline Pattern sinh_poincare_geodesic_vortex() {
  // Ω (omega constant) ≈ 0.5671 rad (32.5°): the Lambert W fixed point
  // Ω·e^Ω = 1 → the natural fixed-point constant of iterated maps
  // 360°/32.5° ≈ 11.1 → quasi-11-fold geodesic spray
  constexpr float CO   =  0.8427f;           // cos(Ω)
  constexpr float SO   =  0.5383f;           // sin(Ω)
  // -arccosh(2) = -ln(2+√3) ≈ -1.3170 rad (-75.5°): Pell equation unit
  // 2+√3 = tan(75°); (2+√3)(2-√3) = 1 → fundamental units in ℤ[√3]
  // Also: arcsinh(√3) = the hyperbolic distance at √3
  constexpr float CPC  =  0.2513f;           // cos(ln(2+√3))
  constexpr float SPCN = -0.9680f;           // sin(-ln(2+√3))

  constexpr float PRE  = 0.72f;  // moderate: safe for sinh exponential growth
  constexpr float POST = 0.90f;  // gentle contraction: product 0.648

  Pattern p;
  p.name = "sinh_poincare_geodesic_vortex";
  p.inner = InnerFunction::Sinh;             // hyperbolic metric: exponential divergence
  p.reduction = ReductionType::Torus;        // horocyclic wrapping: seamless ideal boundary

  p.zn.zn[0] = {CO * PRE, SO * PRE};
  p.zn.zn[1] = {0.005f, -0.004f};           // perturbation: break bilateral symmetry
  p.zn.zn[2] = {CPC * POST, SPCN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 1 (Spiral) — geodesic family color mapping ──
  p.color.hue_rotation = 0.008f;             // moderate: geodesic drift
  p.color.sat_scale = 1.10f;                 // moderate vibrancy
  p.color.fade = 0.987f;                     // deep: ~77 frame persistence
  p.color.color_mode = as_int(ColorMode::Spiral);                    // Spiral: hue tracks geodesic angle
  p.color.spiral_angle_coeff = 1.20f;        // STRONG: shows 11 geodesic families
  p.color.spiral_radius_coeff = 0.60f;       // strong: hyperbolic depth rings
  // Mode 1 does not use breathing or hue velocity params
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: strong geodesic wavefront contrast
  p.color.luma_shape_depth = 0.25f;          // strong: crisp geodesic arcs
  p.color.luma_shape_center = 2.0f;

  // ── Seed: moderate — injects curvature perturbation ──
  p.seed.border_width = 0.012f;              // moderate: boundary geodesics visible
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 0.86f;

  p.speed = 0.42f;                           // moderate: watch geodesic divergence
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.105f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.146f;
  p.zn.zn1_anim_amp = 0.045f;
  p.zn.zn1_anim_k = 6.0f;
  p.seed.pulse_freq = 0.125f;
  p.seed.pulse_amp = 0.19f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.067f,
                            .amplitude = 0.168f,
                            .offset = 0.600f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.091f,
                            .amplitude = 0.050f,
                            .offset = 0.000f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.086f,
                            .amplitude = 0.030f,
                            .offset = 0.050f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
