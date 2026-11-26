#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Quantum wave interference preset.
 *
 * PHYSICS: Fraunhofer diffraction / Kronig-Penney quantum lattice
 *
 * The time-independent Schrödinger equation for a free particle:
 *   -ℏ²/(2m) · d²ψ/dx² = E·ψ
 * has plane-wave solutions ψ(x) = A·sin(kx) + B·cos(kx), where
 * k = √(2mE)/ℏ is the wavenumber.  Between periodic potential barriers
 * (the Kronig-Penney model), the wavefunction IS a sinusoidal standing
 * wave modulated by the barrier geometry.
 *
 * ENGINE MAPPING:
 *
 *   WAVEFUNCTION = sin(z) inner function
 *     sin(z) = sin(x)·cosh(y) + i·cos(x)·sinh(y).
 *     Along the real axis this IS the probability amplitude ψ(x).
 *     Along the imaginary axis, cosh(y) creates evanescent wave
 *     amplification — the quantum tunneling regime where |ψ|² decays
 *     exponentially through classically forbidden regions.
 *
 *   POTENTIAL BARRIERS = Grid reduction
 *     Grid reduction folds coordinates via sawtooth wrap: the sharp
 *     tile boundaries ARE the periodic potential barriers of the
 *     Kronig-Penney model.  Between barriers, ψ oscillates (sin);
 *     AT barriers, the wavefunction must satisfy continuity conditions
 *     creating visible NODES at the boundaries.
 *
 *   DECOHERENCE = fade (0.981)
 *     In quantum mechanics, decoherence destroys phase information as
 *     the system interacts with its environment.  Fade < 1 causes
 *     older interference patterns to lose coherence (brightness) over
 *     time, making the most recently interfering wave fronts dominant.
 *
 *   MEASUREMENT / INJECTION = seed (border_width=0.020)
 *     The seed border injects new "wavefunction amplitude" from the
 *     domain edges — analogous to a source continuously emitting
 *     particles that create the evolving interference pattern.
 *
 *   PROBABILITY OSCILLATION = Breathing color (mode 2)
 *     Mode 2 modulates saturation via:
 *       breath = 1 + amp · sin(freq · 2π · time + phase · angle)
 *     This IS the time-dependent probability density |ψ(x,t)|²
 *     oscillation: the probability "breathes" as the wavefunction
 *     evolves in time.  The spatial phase term (phase · angle) makes
 *     different regions of the interference pattern breathe at different
 *     phases — exactly like the phase differences between adjacent
 *     interference maxima.
 *
 * - Pre-rotation at 2π/7 (≈51.4°, heptagonal): 7 interference maxima.
 *   Prime-order symmetry avoids trivial degeneracies: in a 7-slit
 *   diffraction grating, the secondary maxima form a distinctive
 *   7-pointed pattern with no accidental cancellations.
 *
 * - Post-rotation at -π/φ² (≈ -68.8°, golden-ratio-squared): the most
 *   irrational angle in terms of continued-fraction convergence (after
 *   the golden angle itself).  This ensures the diffraction pattern
 *   never exactly repeats — like incommensurate momentum states in a
 *   quasicrystal potential.
 *
 * WHAT TO WATCH:
 *   1. Standing wave nodes at grid boundaries (potential barriers)
 *   2. Interference fringes between barriers (the sinusoidal ψ)
 *   3. Saturation breathing = probability density |ψ|² evolution
 *   4. Fade reveals decoherence: older fringes lose phase contrast
 *   5. Seed pulses = new particle injection → re-interference
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern quantum_wave_interference() {
  // 2π/7 (≈51.43 deg): heptagonal — 7 diffraction maxima (prime order)
  constexpr float C7  = 0.6234898019f;   // cos(2π/7)
  constexpr float S7  = 0.7818314825f;   // sin(2π/7)
  // -π/φ² (≈-68.75 deg): golden-ratio-squared irrational
  // φ² = φ+1 = 2.618..., π/φ² ≈ 1.200 rad
  constexpr float CG2  = 0.3623577545f;  // cos(π/φ²)
  constexpr float SG2N = -0.9320390860f; // sin(-π/φ²)

  constexpr float PRE  = 1.38f;  // drives sin past ±1 so interference fringes wrap across Grid barriers
  constexpr float POST = 0.97f;  // preserves amplitude near the seed boundary

  Pattern p;
  p.name = "quantum_wave_interference";
  p.inner = InnerFunction::Sin;          // ψ(x) = A·sin(kx) IS the wavefunction
  p.reduction = ReductionType::Grid;     // potential barriers (Kronig-Penney)

  p.zn.zn[0] = {C7 * PRE, S7 * PRE};
  p.zn.zn[1] = {0.005f, -0.003f};       // asymmetric perturbation (broken symmetry)
  p.zn.zn[2] = {CG2 * POST, SG2N * POST};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.075f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.053f;
  p.zn.zn1_anim_amp = 0.024f;
  p.zn.zn1_anim_k = 7.0f;

  // ── Color: MODE 2 (Breathing) — probability density oscillation ──
  p.color.hue_rotation = 0.013f;
  p.color.sat_scale = 1.09f;
  p.color.fade = 0.988f;                // sustained interference contrast
  p.color.color_mode = as_int(ColorMode::Breathing);               // Breathing: |ψ|² probability oscillation
  p.color.spiral_angle_coeff = 0.65f;   // angular: reveals interference pattern sectors
  p.color.spiral_radius_coeff = 0.38f;  // radial: wavefront distance rings
  // Breathing envelope = PROBABILITY DENSITY oscillation
  p.color.chroma_breath_amp = 0.30f;    // STRONG: visible |ψ|² modulation
  p.color.chroma_breath_freq = 0.18f;   // de Broglie frequency
  p.color.chroma_breath_phase = 0.35f;  // spatial: adjacent maxima breathe offset
  p.color.hue_velocity_base = 0.013f;
  p.color.hue_velocity_breath = 0.009f;
  p.color.hue_velocity_drift = 0.003f;
  // Luminance: bright at constructive interference, dark at destructive
  p.color.luma_shape_depth = 0.20f;
  p.color.luma_shape_center = 1.7f;

  // ── Seed: PARTICLE SOURCE ──
  p.seed.border_width = 0.085f;         // visible emission rim for standing-wave fringes
  p.seed.hue_speed = 0.09f;
  p.seed.alpha = 0.93f;
  p.speed = 0.70f;                      // moderate: quantum observation timescale
  return p;
}

} // namespace epi::presets
