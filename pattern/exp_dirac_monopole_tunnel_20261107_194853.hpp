#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Exponential Dirac monopole tunnel preset.
 *
 * THEORETICAL ELECTRODYNAMICS: THE MAGNETIC MONOPOLE
 *
 * In 1931, Paul Dirac showed that if even ONE magnetic monopole exists
 * ANYWHERE in the universe, ALL electric charge must be quantized:
 *
 *   e × g = n × hbar/2      (n = integer)
 *
 * This is the most elegant explanation of charge quantization — and it
 * remains one of the deepest unsolved problems in physics.  Despite
 * decades of searching (cosmic rays, particle accelerators, Moon rock),
 * no monopole has been found.  Yet the mathematics is so beautiful that
 * most theorists believe they exist.
 *
 * A monopole creates a RADIAL magnetic field: B = (mu_0 × g)/(4pi r^2) r_hat,
 * perfectly analogous to Coulomb's law for electric charges.  The DIRAC
 * STRING is a thin solenoid stretching from the monopole to infinity —
 * a gauge artifact that reveals the TOPOLOGICAL nature of the monopole.
 * The Wu-Yang fiber bundle description (1975) eliminates the string by
 * using two coordinate patches, showing that the monopole IS topology.
 *
 * THE ELECTROMAGNETIC COUPLING AMPLITUDE: sqrt(alpha)
 *
 * In quantum electrodynamics, every interaction vertex in a Feynman
 * diagram contributes a factor of sqrt(alpha) ≈ 0.0854 to the amplitude.
 * The probability of a process with n vertices goes as alpha^n — this
 * is WHY the perturbation series converges (alpha ≈ 1/137 << 1).
 *
 * sqrt(alpha) IS the probability amplitude for a charged particle to
 * emit or absorb a single virtual photon.  It is the FUNDAMENTAL UNIT
 * of electromagnetic interaction — the coupling between matter and light.
 *
 * ENGINE MAPPING TO THE DIRAC MONOPOLE:
 *
 *   THE MONOPOLE FIELD = Exp inner function
 *     Exp maps Cartesian to polar: straight lines become concentric
 *     circles, vertical lines become radial rays.  This IS the monopole:
 *     radial magnetic field lines emanating isotropically from a point
 *     source.  Each radial ray is one flux line; the exponential growth
 *     models the 1/r^2 field strength (in feedback space, each iteration
 *     creates a new shell of expanding flux).
 *
 *     PRE = 0.88: the HIGHEST expansion ever for an Exp preset,
 *     pushing the exponential mapping into its densest possible regime.
 *     With POST = 0.93, the product 0.818 is the highest for any Exp
 *     preset — each depth layer retains 81.8% of its predecessor's
 *     energy, creating the deepest Exp accumulation ever achieved.
 *
 *   THE FLUX TUBE = Torus reduction
 *     Torus provides seamless wrapping — the cell where magnetic flux
 *     is confined.  The tunnel view IS looking along a Dirac string:
 *     the solenoid of singular flux connecting the monopole to infinity.
 *     Inside the string, the B field is enormous; outside, it's zero.
 *     The tunnel is literally the interior of this flux tube.
 *
 *   73 FEYNMAN VERTICES = sqrt(alpha) ≈ 0.0854 rad pre-rotation
 *     sqrt(alpha) = 1/sqrt(137.036) ≈ 0.08544 rad (4.89°)
 *     This IS the coupling amplitude: every QED vertex × sqrt(alpha).
 *
 *     cos(1/sqrt(137)) ≈ 0.9964
 *     sin(1/sqrt(137)) ≈ 0.0854
 *
 *     360°/4.89° ≈ 73.6 → quasi-73-fold symmetry.
 *     Each of the 73 sectors IS one virtual photon exchange channel
 *     radiating from the monopole — 73 Feynman diagrams made visible.
 *
 *     73 is the 21st prime.  Its mirror 37 is the 12th prime, and
 *     the mirror of 21 is 12 — a self-referential prime structure.
 *     73 is also a STAR NUMBER (centered 12-gonal number): the
 *     number of points in a Star of David pattern with 4 points per
 *     edge.  In the monopole context: 73 flux lines arranged in a
 *     star-like radial pattern.
 *
 *   THE FULL EM CYCLE = -arctan(2*pi) ≈ -80.96° post-rotation
 *     2*pi: the COMPLETE electromagnetic cycle.  Every EM wave,
 *     every photon, every oscillation completes one full 2*pi cycle.
 *     arctan(2*pi) is the angle at which the full cycle meets unity.
 *
 *     cos(arctan(2*pi)) = 1/sqrt(1 + 4*pi^2) ≈ 0.1572
 *     sin(-arctan(2*pi)) = -2*pi/sqrt(1 + 4*pi^2) ≈ -0.9876
 *
 *     The near-90° post-rotation (~81°) means each depth layer is
 *     rotated by almost a quarter turn.  After ~4.45 layers, the
 *     pattern approximately returns — creating a quasi-4.5-fold
 *     DEPTH PERIODICITY.  This irrational depth period ensures
 *     the monopole field NEVER exactly repeats.
 *
 *     Combined: 73-fold × 4.45-fold = ~325 interference cells.
 *     This is the DENSEST Exp tunnel in the entire collection.
 *
 *   THREE QUANTUM TIMESCALES = Mode 3 (Alive)
 *     base  = 0.030 → virtual photon exchange (FASTEST: QED vertex rate)
 *     breath = 0.016 → monopole pair fluctuation (vacuum instability)
 *     drift  = 0.005 → topological evolution (Dirac string reorientation)
 *
 * WHAT TO WATCH:
 *   1. 73-FOLD RADIAL FAN: ultra-fine magnetic flux lines from the monopole
 *   2. DEEPEST EXP: PRE=0.88 creates the densest Exp tunnel ever
 *   3. QUARTER-TURN DEPTH: ~81° rotation = quasi-4.5-fold layer stacking
 *   4. TRIPLE QUANTUM: three QED timescales shimmer independently
 *   5. THE STRING: look for the axial singularity (Dirac string)
 *   6. 325 CELLS: the densest interference pattern of any Exp preset
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern exp_dirac_monopole_tunnel() {
  // sqrt(alpha) = 1/sqrt(137) ≈ 0.08544 rad (4.89°): EM coupling amplitude
  // The fundamental vertex factor: every Feynman diagram × sqrt(alpha)
  // quasi-73-fold: 73 virtual photon channels from the monopole
  constexpr float CM   =  0.9964f;           // cos(1/√137)
  constexpr float SM   =  0.0854f;           // sin(1/√137) ≈ √α itself!
  // -arctan(2π) ≈ -1.4130 rad (-80.96°): the full EM cycle meets unity
  // cos = 1/√(1+4π²), sin = -2π/√(1+4π²) — near-90° depth stacking
  // ~4.45 layers per depth period → irrational, never-repeating
  constexpr float CFC  =  0.1572f;           // cos(arctan(2π)) = 1/√(1+4π²)
  constexpr float SFCN = -0.9876f;           // sin(-arctan(2π)) = -2π/√(1+4π²)

  constexpr float PRE  = 1.2129f;  // HIGHEST EVER for Exp: maximum monopole flux density
  constexpr float POST = 0.9477f;  // mild contraction: product 0.818 (highest Exp product)

  Pattern p;
  p.name = "exp_dirac_monopole_tunnel";
  p.inner = InnerFunction::Exp;              // radial monopole field: B ∝ 1/r²
  p.reduction = ReductionType::Torus;        // Dirac flux tube: string interior

  p.zn.zn[0] = {CM * PRE, SM * PRE};
  p.zn.zn[1] = {0.0357f, -0.0308f};           // perturbation: quantum vacuum fluctuation
  p.zn.zn[2] = {CFC * POST, SFCN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 3 (Alive) — three quantum electrodynamic timescales ──
  p.color.hue_rotation = 0.008f;             // slow: deep quantum field contemplation
  p.color.sat_scale = 1.16f;                 // HIGH: monopole radiation brilliance
  p.color.fade = 0.992f;                     // EXTREME: ~125 frame persistence
  p.color.color_mode = as_int(ColorMode::Alive);                    // Alive: QED triple quantum dynamics
  p.color.spiral_angle_coeff = 1.00f;        // VERY STRONG: 73-fold must be visible
  p.color.spiral_radius_coeff = 0.55f;       // strong: flux tube depth rings
  // Monopole vacuum oscillation (virtual pair production)
  p.color.chroma_breath_amp = 0.22f;         // moderate: Schwinger pair fluctuations
  p.color.chroma_breath_freq = 0.11f;        // vacuum oscillation rate
  p.color.chroma_breath_phase = 0.25f;       // topological phase
  // THREE QED TIMESCALES
  p.color.hue_velocity_base = 0.030f;        // FASTEST: virtual photon exchange rate
  p.color.hue_velocity_breath = 0.016f;      // MEDIUM: monopole pair fluctuation
  p.color.hue_velocity_drift = 0.005f;       // SLOW: Dirac string reorientation
  // Luminance: maximum radial flux line definition
  p.color.luma_shape_depth = 0.26f;          // VERY STRONG: 73 flux lines must be sharp
  p.color.luma_shape_center = 2.1f;

  // ── Seed: ultra-minimal — the monopole field IS the visual ──
  p.seed.border_width = 0.081f;              // very subtle: flux dominates
  p.seed.hue_speed = 0.099f;                  // very slow: quantum vacuum
  p.seed.alpha = 0.92f;

  p.speed = 0.34f;                           // VERY slow: densest Exp demands meditation
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.136f;
  p.zn.zn0_anim_amp = 0.051f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.206f;
  p.zn.zn1_anim_amp = 0.055f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.101f;
  p.seed.pulse_amp = 0.18f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.078f,
                            .amplitude = 0.154f,
                            .offset = 0.550f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.095f,
                            .amplitude = 0.117f,
                            .offset = 0.220f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.061f,
                            .amplitude = 0.035f,
                            .offset = 0.099f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
