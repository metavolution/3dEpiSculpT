#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh hyperbolic tessellation abyss preset.
 *
 * HYPERBOLIC GEOMETRY: THE {7,3} TILING & ESCHER'S CIRCLE LIMIT
 *
 * In HYPERBOLIC GEOMETRY, the interior angles of polygons are SMALLER
 * than in flat Euclidean space — and this extra angular room allows
 * infinitely many tiles to fit inside a bounded disk.  The Poincaré
 * disk model maps the infinite hyperbolic plane conformally into the
 * unit disk: objects near the boundary are infinitely compressed, but
 * all angles are preserved.
 *
 * The {7,3} TILING places regular heptagons (7-sided polygons) so
 * that exactly 3 meet at each vertex.  In flat space this is impossible
 * (3 regular heptagons would need 3 × 128.57° = 385.7° > 360°), but
 * in hyperbolic space each vertex angle shrinks to exactly 2π/3 = 120°,
 * and 3 × 120° = 360° works perfectly.  The result is an infinite
 * ocean of heptagons that get smaller and smaller toward the boundary,
 * exactly as in M.C. Escher's "Circle Limit" woodcuts (1958-1960).
 *
 * THE DEFECT AND CURVATURE:
 * In flat space, interior angle = π(p-2)/p.  For p=7: 5π/7 ≈ 128.57°.
 * In the {7,3} tiling, vertex angle = 2π/3 = 120°.
 * The angular defect per polygon = 7 × (5π/7 - 2π/3) = 5π - 14π/3 = π/3.
 * By Gauss-Bonnet: curvature × area = 2π - (sum of angles at a vertex),
 * confirming NEGATIVE CURVATURE — the hallmark of hyperbolic space.
 *
 * ENGINE MAPPING TO THE HYPERBOLIC TESSELLATION:
 *
 *   THE POINCARÉ BOUNDARY = Tanh inner function
 *     In the Poincaré disk, the boundary circle |z| = 1 represents
 *     INFINITY — points near it are infinitely far from the center
 *     in hyperbolic distance: d(0,z) = 2·artanh(|z|).  tanh IS the
 *     inverse of this distance function: |z| = tanh(d/2).  Under
 *     iteration, tanh's ±1 saturation walls ARE the boundary of
 *     the Poincaré disk — the circle at infinity where the infinite
 *     tiling converges to zero-size tiles.
 *
 *     The dense filament web from iterated tanh IS the {7,3} tiling:
 *     each filament traces a geodesic (circular arc perpendicular to
 *     the boundary), and the filament intersections are the vertices
 *     where 3 heptagonal tiles meet.
 *
 *   THE CONFORMAL MAP = Torus reduction
 *     The Poincaré disk preserves ANGLES (conformal) while compressing
 *     distances near the boundary.  Torus reduction gives seamless
 *     wrapping — the visual equivalent of the disk's conformal
 *     self-similarity.  Looking into the tunnel IS descending through
 *     the infinite layers of the hyperbolic tessellation, each layer
 *     containing more tiles than the last (exponential growth of the
 *     number of tiles with depth — a fundamental property of negative
 *     curvature).
 *
 *   THE DUAL ORDER = π/7 ≈ 25.71° pre-rotation
 *     cos(π/7) ≈ 0.9009, sin(π/7) ≈ 0.4339
 *
 *     π/7: the interior half-angle of the regular heptagon.  This is
 *     the angle subtended by each vertex when viewed from the center.
 *     360°/25.71° = 14 → exact 14-fold symmetry.
 *
 *     14 = 2 × 7: the ORDER of the dihedral group D₇, the symmetry
 *     group of the heptagon (7 rotations × 2 reflections).  Each of
 *     the 14 sectors IS one symmetry element of the heptagonal tile.
 *
 *     7 is: the smallest prime that is NOT a Mersenne exponent giving
 *     a Mersenne prime, a Woodall prime, a safe prime (2×3+1), and
 *     the number of FRIEZE GROUPS (all possible 1D symmetry patterns).
 *     In the {7,3} tiling: 7 edges per tile, 3 tiles per vertex,
 *     Euler formula gives genus-3 surface for the quotient.
 *
 *   THE CURVATURE DEPTH = -arctan(√6) ≈ -67.79° post-rotation
 *     cos(arctan(√6)) = 1/√7 ≈ 0.3780 (EXACT: 1² + (√6)² = 7)
 *     sin(-arctan(√6)) = -√6/√7 ≈ -0.9258 (EXACT)
 *
 *     1/√7: the curvature normalization of the {7,3} tiling.  The
 *     Gaussian curvature K of the hyperbolic plane that admits this
 *     tiling scales as K ∝ -1/7.  The denominator √7 connects the
 *     pre-rotation (7 sides) to the depth structure (curvature), and
 *     √6 in the numerator is the number of edges meeting at each
 *     vertex (3 tiles × 2 edges per tile contributing to each vertex).
 *
 *     360°/67.79° ≈ 5.31 → quasi-5.3-fold depth symmetry.
 *     5 is a FIBONACCI NUMBER, and the 5.31-fold irrational depth
 *     ensures the layered tessellation NEVER exactly repeats — each
 *     descent shows a subtly different arrangement of heptagonal tiles.
 *
 *     Combined: 14-fold × 5.31-fold = ~74 interference cells — a
 *     dense hyperbolic lattice of interlocking heptagonal regions.
 *
 *   THREE ISOMETRIES OF H² = Mode 3 (Alive)
 *     The hyperbolic plane has three types of isometry:
 *     base  = 0.020 → ELLIPTIC: rotation around a fixed point
 *                      (spinning one heptagonal tile in place)
 *     breath = 0.012 → HYPERBOLIC: translation along a geodesic
 *                       (sliding between tiles along a straight line)
 *     drift  = 0.004 → PARABOLIC: horocyclic flow toward the boundary
 *                       (the limiting case — spiraling toward infinity)
 *
 *     MAXIMUM CALIBRATION: matching the densest Tanh presets for
 *     brilliance: fade=0.992, sat_scale=1.16, luma_shape_depth=0.26
 *
 * WHAT TO WATCH:
 *   1. 14-FOLD MANDALA: dihedral symmetry of the heptagonal tile
 *   2. INFINITE DESCENT: tiles shrink toward the boundary (hyperbolic compression)
 *   3. GEODESIC WEB: tanh filaments trace circular arcs ⊥ to boundary
 *   4. TRIPLE SHIMMER: elliptic/hyperbolic/parabolic isometry dynamics
 *   5. 1/√7 DEPTH: curvature normalization structures each layer
 *   6. POINCARÉ BOUNDARY: tanh ±1 saturation IS the circle at infinity
 *   7. EXPONENTIAL GROWTH: each depth layer has MORE tiles than the last
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern tanh_hyperbolic_tessellation_abyss() {
  // π/7 ≈ 0.4488 rad (25.71°): heptagonal half-angle
  // D₇ dihedral group: 7 rotations × 2 reflections = 14 symmetry elements
  // Exact 14-fold → the full symmetry group of the {7,3} tile
  constexpr float CH   =  0.9009f;           // cos(π/7)
  constexpr float SH   =  0.4339f;           // sin(π/7)
  // -arctan(√6) ≈ -1.1832 rad (-67.79°): curvature depth angle
  // cos = 1/√7 EXACT: Gaussian curvature normalization K ∝ -1/7
  // sin = -√6/√7 EXACT: 6 = edges meeting at each vertex (3 tiles × 2)
  // 1² + (√6)² = 7 ✓  Pythagorean: geometry + edges = polygon
  constexpr float CKD  =  0.3780f;           // cos(arctan(√6)) = 1/√7
  constexpr float SKDN = -0.9258f;           // sin(-arctan(√6)) = -√6/√7

  constexpr float PRE  = 1.14f;  // MAXIMUM-TIER expansion: densest heptagonal web
  constexpr float POST = 0.84f;  // strong contraction: product 0.958 (near-critical)

  Pattern p;
  p.name = "tanh_hyperbolic_tessellation_abyss";
  p.inner = InnerFunction::Tanh;             // Poincaré boundary: |z| = tanh(d/2)
  p.reduction = ReductionType::Torus;        // conformal wrapping: seamless disk mapping

  p.zn.zn[0] = {CH * PRE, SH * PRE};
  p.zn.zn[1] = {0.003f, -0.004f};           // perturbation: break exact crystallography
  p.zn.zn[2] = {CKD * POST, SKDN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 3 (Alive) — three isometries of the hyperbolic plane ──
  p.color.hue_rotation = 0.006f;             // very slow: geometric contemplation
  p.color.sat_scale = 1.16f;                 // HIGH: iridescent tile brilliance
  p.color.fade = 0.992f;                     // EXTREME: ~125 frame persistence (deepest)
  p.color.color_mode = as_int(ColorMode::Alive);                    // Alive: elliptic/hyperbolic/parabolic dynamics
  p.color.spiral_angle_coeff = 0.90f;        // strong: shows 14 dihedral sectors
  p.color.spiral_radius_coeff = 0.55f;       // STRONG: depth rings = hyperbolic layers
  // Hyperbolic oscillation (geodesic deviation)
  p.color.chroma_breath_amp = 0.20f;         // moderate: tidal curvature breathing
  p.color.chroma_breath_freq = 0.10f;        // geodesic oscillation rate
  p.color.chroma_breath_phase = 0.3f;        // curvature phase
  // THREE ISOMETRIES OF H²
  p.color.hue_velocity_base = 0.020f;        // MEDIUM: elliptic rotation around fixed point
  p.color.hue_velocity_breath = 0.012f;      // MEDIUM: hyperbolic translation along geodesic
  p.color.hue_velocity_drift = 0.004f;       // SLOW: parabolic horocycle flow to boundary
  // Luminance: MAXIMUM tile boundary contrast
  p.color.luma_shape_depth = 0.26f;          // VERY STRONG: crisp heptagonal tile edges
  p.color.luma_shape_center = 2.2f;

  // ── Seed: ultra-minimal — the tessellation IS the visual ──
  p.seed.border_width = 0.007f;              // very subtle: tiles dominate completely
  p.seed.hue_speed = 0.03f;                  // very slow: geometric meditation
  p.seed.alpha = 0.85f;

  p.speed = 0.34f;                           // VERY slow: infinite descent demands patience
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.108f;
  p.zn.zn0_anim_amp = 0.049f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.219f;
  p.zn.zn1_anim_amp = 0.049f;
  p.zn.zn1_anim_k = 2.0f;
  p.seed.pulse_freq = 0.134f;
  p.seed.pulse_amp = 0.18f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.075f,
                            .amplitude = 0.154f,
                            .offset = 0.550f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.121f,
                            .amplitude = 0.110f,
                            .offset = 0.200f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.044f,
                            .amplitude = 0.030f,
                            .offset = 0.030f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
