#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Cube triple point bloom preset.
 *
 * THERMODYNAMICS: THE GIBBS TRIPLE POINT & PHASE COEXISTENCE
 *
 * At exactly 273.16 K and 611.657 Pa, water exists in THREE PHASES
 * simultaneously — solid, liquid, and gas.  This is the TRIPLE POINT,
 * the unique state where all three phase boundaries meet.  It is so
 * precisely defined that it was used as the fundamental calibration
 * point for the Kelvin temperature scale until 2019.
 *
 * The GIBBS PHASE RULE determines the degrees of freedom:
 *
 *   F = C - P + 2
 *
 * For pure water (C=1) at the triple point (P=3): F = 1 - 3 + 2 = 0.
 * ZERO degrees of freedom — the triple point is an INVARIANT of nature.
 * Change ANY variable (temperature, pressure) and one phase disappears.
 * This makes the triple point the most thermodynamically constrained
 * state possible for a single-component system.
 *
 * Near the triple point, CRITICAL OPALESCENCE occurs: density
 * fluctuations at all length scales cause the fluid to scatter light
 * strongly, creating a pearlescent shimmer as the system hovers between
 * phases.  These fluctuations are described by the ORNSTEIN-ZERNIKE
 * correlation function, which diverges at the critical point.
 *
 * THE BOLTZMANN ENTROPY at the triple point:
 *   S = k_B × ln(W)  where W = 3 (accessible macrostates)
 *   S_triple = k_B × ln(3) ≈ 1.099 × k_B
 *
 * ln(3) is the entropy per particle of a three-state system in natural
 * units (nats).  In information theory, it equals log₂(3) ≈ 1.585 bits
 * — the information content of one TRIT (ternary digit).
 *
 * ENGINE MAPPING TO TRIPLE POINT THERMODYNAMICS:
 *
 *   THREE-PHASE SYMMETRY = Cube inner function (z³)
 *     z³ creates EXACT 3-fold rotational symmetry — the mathematical
 *     group C₃.  Each of the three lobes IS one thermodynamic phase:
 *     solid (crystalline order), liquid (local order), gas (disorder).
 *     The lobe boundaries are the PHASE COEXISTENCE CURVES — the lines
 *     along which two phases share equal chemical potential.  Where all
 *     three boundaries meet IS the triple point.
 *
 *     ROTATION NOTE: z³ TRIPLES the pre-rotation angle.  At π/6 = 30°:
 *     3 × 30° = 90° — the internal rotation is a QUARTER TURN.  This
 *     means each z³ iteration adds an orthogonal component, creating
 *     a spiral that visits all four quadrants before nearly repeating.
 *     The 4-quadrant cycle × 3-fold z³ = 12 effective sectors.
 *     12 = the coordination number of FCC close-packing — the number
 *     of nearest neighbors in the densest crystal structure (relevant
 *     to the solid phase at the triple point).
 *
 *   PHASE SPACE CONTINUITY = Torus reduction
 *     The thermodynamic phase diagram is a CONTINUOUS SURFACE — phases
 *     don't have edges or seams.  Torus gives seamless mirror wrapping:
 *     the Gibbs surface folds back on itself without boundaries.  This
 *     creates smooth, flowing trefoil-like patterns rather than the
 *     sharp crystalline tiles that Grid would impose.
 *
 *   THE HEXAGONAL LATTICE = π/6 = 30° pre-rotation
 *     cos(π/6) = √3/2 ≈ 0.8660  (EXACT)
 *     sin(π/6) = 1/2 = 0.5000   (EXACT)
 *
 *     π/6 is the angle of the HEXAGONAL LATTICE — the most efficient
 *     2D packing.  Ice Ih (ordinary ice) IS a hexagonal crystal:
 *     each water molecule forms 4 hydrogen bonds arranged in a
 *     tetrahedral geometry that tiles as hexagonal layers.  The angle
 *     π/6 directly encodes this crystallographic symmetry.
 *
 *     360°/30° = 12 → 12-fold base symmetry.
 *     But z³ acts as a 3-fold cover: effective visual = 12/gcd(12,3) = 4
 *     quadrants, each containing a 3-fold trefoil motif.
 *
 *   BOLTZMANN ENTROPY DEPTH = -ln(3) ≈ -62.94° post-rotation
 *     ln(3) ≈ 1.0986 rad: the ENTROPY OF THREE COEXISTING STATES.
 *     S = k_B ln(3) is the thermodynamic entropy at the triple point,
 *     measuring the three-way degeneracy of the equilibrium.
 *
 *     cos(ln(3)) ≈ 0.4533     sin(-ln(3)) ≈ -0.8914
 *     Check: 0.4533² + 0.8914² ≈ 0.2055 + 0.7946 = 1.000 ✓
 *
 *     ln(3) is TRANSCENDENTAL (Lindemann-Weierstrass theorem):
 *     no finite polynomial with integer coefficients has ln(3) as root.
 *     This guarantees the depth stacking NEVER exactly repeats.
 *
 *     360°/62.94° ≈ 5.72 → quasi-5.7-fold depth symmetry.
 *     Combined: 12 × 5.7 ≈ 68 interference cells.
 *
 *   CRITICAL OPALESCENCE = Mode 2 (Breathing)
 *     Near the triple point, the system fluctuates between phases.
 *     The breathing IS these density fluctuations: saturation peaks
 *     when one phase dominates (ordered), dips during the chaotic
 *     transition between phases.  The spatial phase term makes the
 *     breathing ROTATE — different sectors of the pattern are in
 *     different phases simultaneously, exactly as in the coexistence
 *     region of a Van der Waals fluid.
 *
 * WHAT TO WATCH:
 *   1. THREE LOBES: z³ creates the three coexisting phases
 *   2. TREFOIL FLOW: Torus wrapping creates smooth lobe interweaving
 *   3. BREATHING: critical opalescence — density fluctuation shimmer
 *   4. HEXAGONAL BASE: π/6 = the ice crystal lattice angle
 *   5. TRANSCENDENTAL DEPTH: ln(3) = 3-state entropy, never repeating
 *   6. QUARTER-TURN: 3×30°=90° spiral visits all quadrants
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern cube_triple_point_bloom() {
  // π/6 = 30° (0.5236 rad): hexagonal lattice — the crystal structure of ice
  // cos = √3/2, sin = 1/2 — EXACT algebraic (the most elegant trig pair)
  // z³ triples to 90°: quarter-turn spiral through phase space
  // 12-fold base symmetry → FCC coordination number
  constexpr float CH   =  0.8660f;           // cos(π/6) = √3/2
  constexpr float SH   =  0.5000f;           // sin(π/6) = 1/2
  // -ln(3) ≈ -1.0986 rad (-62.94°): Boltzmann entropy of 3-state system
  // S = k_B ln(3): the thermodynamic weight of triple-point coexistence
  // TRANSCENDENTAL: Lindemann-Weierstrass → never-repeating depth layers
  // quasi-5.7-fold: irrational depth stacking (max visual complexity)
  constexpr float CBE  =  0.4533f;           // cos(ln(3))
  constexpr float SBEN = -0.8914f;           // sin(-ln(3))

  constexpr float PRE  = 1.3435f;  // moderate: z³ is unbounded but weaker than exp
  constexpr float POST = 1.3044f;  // moderate contraction: product 0.801

  Pattern p;
  p.name = "cube_triple_point_bloom";
  p.inner = InnerFunction::Cube;             // z³: 3-fold phase symmetry (C₃ group)
  p.reduction = ReductionType::Torus;        // continuous Gibbs surface, no tile edges

  p.zn.zn[0] = {CH * PRE, SH * PRE};
  p.zn.zn[1] = {0.2657f, -0.1922f};           // perturbation: thermal noise at the triple point
  p.zn.zn[2] = {CBE * POST, SBEN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 2 (Breathing) — critical opalescence ──
  p.color.hue_rotation = 0.012f;             // moderate: phase boundary drift
  p.color.sat_scale = 1.10f;                 // moderate: opalescent shimmer
  p.color.fade = 0.984f;                     // moderate-deep: ~62 frame persistence
  p.color.color_mode = as_int(ColorMode::Breathing);                    // Breathing: density fluctuation near criticality
  p.color.spiral_angle_coeff = 0.95f;        // strong: shows trefoil lobe sectors
  p.color.spiral_radius_coeff = 0.45f;       // moderate: depth ring banding
  // CRITICAL FLUCTUATION: phase-transition thermal oscillation
  p.color.chroma_breath_amp = 0.28f;         // moderate: visible opalescence
  p.color.chroma_breath_freq = 0.07f;        // slow: thermal equilibrium timescale
  p.color.chroma_breath_phase = 2.2f;        // spatial gradient: rotating phase fronts
  // Mode 2 does not use hue velocity params
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: moderate trefoil contrast
  p.color.luma_shape_depth = 0.18f;          // moderate: soft phase boundary definition
  p.color.luma_shape_center = 1.8f;

  // ── Seed: moderate — injects thermal perturbation ──
  p.seed.border_width = 0.091f;              // moderate: phase boundary injection
  p.seed.hue_speed = 0.08f;                  // moderate: thermal color cycling
  p.seed.alpha = 0.92f;

  p.speed = 0.65f;                           // moderate: watch phases coalesce
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.173f;
  p.zn.zn0_anim_amp = 0.051f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.181f;
  p.zn.zn1_anim_amp = 0.046f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.093f;
  p.seed.pulse_amp = 0.19f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.059f,
                            .amplitude = 0.126f,
                            .offset = 0.450f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.102f,
                            .amplitude = 0.120f,
                            .offset = 0.280f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.069f,
                            .amplitude = 0.030f,
                            .offset = 0.080f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
