#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Cube Potts critical bloom preset.
 *
 * STATISTICAL MECHANICS: THE 3-STATE POTTS MODEL AT CRITICALITY
 *
 * The q-state POTTS MODEL generalizes the Ising model: each lattice spin
 * takes one of q values, and neighbors interact via:
 *
 *   H = -J * SUM_<ij> delta(sigma_i, sigma_j)
 *
 * For q=3, the model describes systems with THREE coexisting phases —
 * three-domain grain boundaries in polycrystals, three-color percolation,
 * and adsorbed monolayer ordering on triangular substrates.
 *
 * THE CRITICAL POINT:
 * On the 2D square lattice, the EXACT critical coupling is:
 *
 *   beta_c * J = ln(1 + sqrt(q))
 *
 * For q=3: beta_c * J = ln(1+sqrt(3)) ≈ 1.0051
 *
 * This was proven by Baxter (1973) using the connection to the six-vertex
 * model.  At T_c, the Z_3 symmetry (cyclic permutation of 3 states) is
 * spontaneously broken — the system chooses one of 3 degenerate ground
 * states.  The transition for q=3 is SECOND ORDER (continuous), with
 * exact critical exponents from conformal field theory:
 *
 *   CENTRAL CHARGE c = 4/5  (minimal model M(6,5))
 *   nu = 5/6       (correlation length exponent)
 *   eta = 4/15     (anomalous dimension)
 *   beta = 1/9     (order parameter exponent)
 *
 * CRITICAL SLOWING DOWN: Near T_c, the correlation time diverges as
 * tau ~ |T - T_c|^{-z*nu}, where z ≈ 2 is the dynamic exponent.
 * Fluctuations become infinitely slow — the system can't decide which
 * of the 3 phases to settle into.
 *
 * ENGINE MAPPING TO THE 3-STATE POTTS CRITICAL POINT:
 *
 *   THE Z_3 SYMMETRY = Cube (z^3) inner function
 *     z^3 has EXACT 3-fold rotational symmetry: z^3 maps the complex
 *     plane onto itself with 120-degree periodicity.  Each of the three
 *     sectors IS one Potts state (sigma = 0, 1, 2).  Under iteration,
 *     z^3 creates domains of 3-fold symmetry separated by sharp walls —
 *     the DOMAIN BOUNDARIES where adjacent Potts spins disagree.
 *
 *     z^3 TRIPLES the pre-rotation angle: our 57.58 degrees becomes
 *     172.7 degrees effective — almost a full inversion (180 degrees).
 *     This near-inversion means the dynamics hover at the EDGE of
 *     bilateral symmetry — the system nearly but not quite cancels,
 *     creating slow evolution.  This 7.3-degree residual IS the critical
 *     fluctuation: the system balanced between order and disorder.
 *
 *   THE LATTICE = Grid reduction
 *     The Potts model IS a lattice model.  Grid tiles are LATTICE SITES,
 *     each containing one spin.  Grid boundaries are the NEAREST-NEIGHBOR
 *     BONDS through which the interaction J couples adjacent spins.
 *     At T_c, domain walls span ALL scales (fractal percolation).
 *
 *   THE CRITICAL COUPLING = ln(1+sqrt(3)) ≈ 57.58 degrees pre-rotation
 *     The EXACT critical temperature of the 3-state Potts model:
 *     beta_c * J = ln(1+sqrt(3)) ≈ 1.0051 rad.
 *
 *     cos(ln(1+sqrt(3))) ≈ 0.5377
 *     sin(ln(1+sqrt(3))) ≈ 0.8432
 *
 *     1+sqrt(3) ≈ 2.7321 — sometimes called the BRONZE MEAN (by analogy
 *     with the golden mean 1+sqrt(5)/2 and silver mean 1+sqrt(2)).
 *     z^3 triples: 3 x 57.58 = 172.7 degrees — near-bilateral, creating
 *     the characteristic CRITICAL SLOWING DOWN visible in the visual.
 *
 *     360 deg / 57.58 deg ≈ 6.25 -> quasi-6-fold pre-symmetry.
 *     With z^3 three-fold: 6 x 3 = 18 features per ring.
 *
 *   THE CONFORMAL CHARGE = -arctan(4/5) ≈ -38.66 degrees post-rotation
 *     c = 4/5: the central charge of the conformal field theory that
 *     describes the 3-state Potts universality class (minimal model M(6,5)).
 *
 *     cos(arctan(4/5)) = 5/sqrt(41) ≈ 0.7809  (EXACT: 4^2+5^2 = 41)
 *     sin(-arctan(4/5)) = -4/sqrt(41) ≈ -0.6247  (EXACT)
 *
 *     41 is PRIME, a centered square number, and a Newman-Shanks-Williams
 *     prime.  The 4-5-sqrt(41) right triangle IS the geometry of conformal
 *     symmetry at the Potts critical point.
 *
 *     360 deg / 38.66 deg ≈ 9.31 -> quasi-9-fold depth.
 *     9 = 3^2: the square of the number of Potts states!
 *     Combined: 6 x 9 ≈ 56 interference cells.
 *
 *   THERMAL FLUCTUATION = Mode 2 (Breathing)
 *     At T_c, thermodynamic quantities oscillate as the system fluctuates
 *     between the 3 degenerate phases.  Mode 2 breathing creates visible
 *     pulsation — the energy density oscillating between ordered (low
 *     energy, domains aligned) and disordered (high energy, domain walls
 *     everywhere).  The SLOW breath frequency models CRITICAL SLOWING
 *     DOWN: near T_c, the dynamics become infinitely sluggish.
 *
 * WHAT TO WATCH:
 *   1. THREE DOMAINS: z^3 three-fold creates 3 Potts-state regions
 *   2. DOMAIN WALLS: Grid boundaries + z^3 edges = spin disagreement
 *   3. CRITICAL BREATHING: slow pulsation = critical slowing down
 *   4. NEAR-INVERSION: 172.7-degree effective angle = edge of symmetry
 *   5. FRACTAL BOUNDARIES: domain walls span all scales at T_c
 *   6. PHASE COEXISTENCE: 3 colors compete in the breathing cycle
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern cube_potts_critical_bloom() {
  // ln(1+sqrt(3)) ≈ 1.0051 rad (57.58 deg): EXACT critical coupling beta_c*J
  // 1+sqrt(3) ≈ 2.7321: the "bronze mean" — Baxter's exact solution (1973)
  // z^3 triples: 172.7 deg effective — near-bilateral critical fluctuation
  constexpr float CPC  =  0.5377f;           // cos(ln(1+sqrt(3)))
  constexpr float SPC  =  0.8432f;           // sin(ln(1+sqrt(3)))
  // -arctan(4/5) ≈ -0.6747 rad (-38.66 deg): conformal charge c = 4/5
  // Minimal model M(6,5): the CFT universality class of 3-state Potts
  // cos = 5/sqrt(41), sin = -4/sqrt(41) — EXACT (4^2 + 5^2 = 41, prime)
  constexpr float CCC  =  0.7809f;           // cos(arctan(4/5)) = 5/sqrt(41)
  constexpr float SCCN = -0.6247f;           // sin(-arctan(4/5)) = -4/sqrt(41)

  constexpr float PRE  = 0.88f;  // moderate: Cube z^3 amplifies structure fast
  constexpr float POST = 0.90f;  // moderate contraction: product 0.792

  Pattern p;
  p.name = "cube_potts_critical_bloom";
  p.inner = InnerFunction::Cube;             // Z_3 symmetry: 3 degenerate Potts states
  p.reduction = ReductionType::Grid;         // lattice: nearest-neighbor interaction bonds

  p.zn.zn[0] = {CPC * PRE, SPC * PRE};
  p.zn.zn[1] = {0.004f, -0.003f};           // perturbation: Z_3 symmetry breaking seed
  p.zn.zn[2] = {CCC * POST, SCCN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // -- Color: MODE 2 (Breathing) -- thermal fluctuation at T_c --
  p.color.hue_rotation = 0.012f;             // moderate: thermal drift
  p.color.sat_scale = 1.10f;                 // moderate: warm lattice glow
  p.color.fade = 0.986f;                     // deep: ~71 frame persistence (critical memory)
  p.color.color_mode = as_int(ColorMode::Breathing);                    // Breathing: thermal oscillation near T_c
  p.color.spiral_angle_coeff = 0.95f;        // STRONG: shows 3-fold domain structure
  p.color.spiral_radius_coeff = 0.45f;       // moderate: correlation depth rings
  // CRITICAL FLUCTUATION: thermal pulsation at the phase transition
  p.color.chroma_breath_amp = 0.35f;         // strong: dramatic critical oscillation
  p.color.chroma_breath_freq = 0.06f;        // VERY SLOW: critical slowing down!
  p.color.chroma_breath_phase = 2.5f;        // strong spatial: correlated domain pulsation
  // Mode 2 does not use hue velocity params
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: sharp domain boundary contrast
  p.color.luma_shape_depth = 0.22f;          // moderate-strong: domain wall visibility
  p.color.luma_shape_center = 2.0f;

  // -- Seed: moderate -- injects thermal noise to drive fluctuations --
  p.seed.border_width = 0.015f;              // moderate: thermal excitation boundary
  p.seed.hue_speed = 0.06f;
  p.seed.alpha = 0.88f;

  p.speed = 0.58f;                           // slow: watch critical dynamics unfold
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.105f;
  p.zn.zn0_anim_amp = 0.043f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.191f;
  p.zn.zn1_anim_amp = 0.043f;
  p.zn.zn1_anim_k = 2.0f;
  p.seed.pulse_freq = 0.123f;
  p.seed.pulse_amp = 0.14f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.085f,
                            .amplitude = 0.126f,
                            .offset = 0.450f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.079f,
                            .amplitude = 0.120f,
                            .offset = 0.350f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.049f,
                            .amplitude = 0.030f,
                            .offset = 0.060f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
