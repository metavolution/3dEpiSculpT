#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Coupled pendulum energy exchange preset.
 *
 * PHYSICS: getriebener gedämpfter harmonischer Oszillator — COUPLED SYSTEM
 *
 * Two identical pendulums connected by a weak coupling spring:
 *
 *   x₁'' + 2γx₁' + ω₀²x₁ + κ(x₁-x₂) = F₁(t)
 *   x₂'' + 2γx₂' + ω₀²x₂ + κ(x₂-x₁) = F₂(t)
 *
 * The normal modes are:
 *   SYMMETRIC:      both swing together, freq ω₊ = ω₀
 *   ANTISYMMETRIC:  swing in opposition, freq ω₋ = √(ω₀² + 2κ)
 *
 * When only ONE pendulum is displaced, the solution shows BEATING:
 *   x₁(t) ∝ cos(Δω·t/2) · cos(ω_avg·t)     ← amplitude waxes & wanes
 *   x₂(t) ∝ sin(Δω·t/2) · sin(ω_avg·t)     ← opposite phase envelope
 *
 * Energy continuously TRANSFERS from pendulum 1 → 2 → 1 → 2 with
 * beat period T_beat = 2π/Δω.  At t=0, pendulum 1 has all energy;
 * at t=T_beat/2, pendulum 2 has all energy; then it reverses.
 *
 * ENGINE MAPPING:
 *
 *   TWO COUPLED OSCILLATORS = sinh(z) inner function
 *     sinh(z) = (e^z - e^{-z})/2 is the ANTISYMMETRIC hyperbolic function:
 *     sinh(-z) = -sinh(z).  This means:
 *       - The +Re half-plane gets PUSHED outward (e^z dominates)
 *       - The -Re half-plane gets PULLED inward (e^{-z} dominates)
 *     Under iterated feedback, this creates an oscillating push-pull
 *     dynamic between the two halves of each grid tile — EXACTLY the
 *     behavior of coupled pendulums exchanging energy.
 *
 *     The ODD symmetry is key: when the left half has high amplitude
 *     (pendulum 1 active), the right half has low amplitude (pendulum 2
 *     at rest).  The feedback loop gradually transfers structure from one
 *     half to the other, creating the energy exchange.
 *
 *   COUPLING SPRINGS = Grid reduction
 *     Grid reduction tiles the plane with visible boundaries.  Each tile
 *     boundary IS a coupling spring: structure that flows past the
 *     boundary enters the adjacent tile, transferring energy.  The Grid
 *     sawtooth wrap creates sharp boundaries that act as mechanical
 *     coupling points — like the springs connecting adjacent pendulums.
 *
 *   BEAT ENVELOPE = Breathing color (mode 2)
 *     Mode 2: breath = 1 + amp · sin(freq · 2πt + phase · angle)
 *     The breathing IS the beat envelope of the coupled system:
 *       breath_freq = 0.08 → SLOW beat (weak coupling: Δω small)
 *       breath_amp = 0.32 → STRONG modulation (full energy exchange)
 *       breath_phase = 3.0 → SPATIAL variation (each tile pair has its
 *         own beat phase → different pendulum pairs exchange at different
 *         times, creating a cascading wave of energy transfer)
 *
 *   DRIVING FORCE = seed pulse (amp=0.40, freq=0.13)
 *     Periodic re-excitation that KICKS the coupled system.  After each
 *     kick, the energy exchange pattern restarts: one pendulum gets all
 *     the energy, then slowly transfers it to its partner.  The drive
 *     period is chosen to be INCOMMENSURATE with the beat period
 *     (0.13 vs 0.08), so the driving sometimes reinforces and sometimes
 *     opposes the exchange — creating varying amplitude responses.
 *
 *   DAMPING = fade 0.976 (Q ≈ 130)
 *     The coupling spring dissipates some energy on each transfer.
 *     Without re-excitation, both pendulums would eventually stop.
 *     The damping makes the exchange ASYMMETRIC: energy that transfers
 *     from pendulum 1→2 arrives slightly diminished, and the return
 *     2→1 is diminished further.  This creates a slowly DECAYING
 *     beat envelope — exactly as observed in real coupled oscillators.
 *
 * - Pre-rotation at 3π/8 (67.5°): creates 8 pendulum pairs from the
 *   combined 8-fold symmetry (4-fold Grid × 2-fold sinh antisymmetry).
 *
 * - Post-rotation at -π/√3 (irrational): the exchange pattern between
 *   depth levels never repeats.  This is analogous to a chain of coupled
 *   pendulums with incommensurate natural frequencies — the beat pattern
 *   is quasiperiodic, like energy waves in a disordered lattice.
 *
 * WHAT TO WATCH (educational sequence):
 *   1. Drive pulse arrives → ONE half of each tile lights up (pendulum 1)
 *   2. Energy gradually MIGRATES to the other half (coupling transfer)
 *   3. Pendulum 2 reaches maximum → pendulum 1 is nearly dark
 *   4. Exchange REVERSES: energy flows back (the beat pattern)
 *   5. Damping makes each successive transfer slightly weaker
 *   6. Next drive pulse → system re-energized, exchange restarts
 *   The BREATHING modulation of saturation makes this exchange visible
 *   as a slow pulsation, distinct from the fast oscillation structure.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern coupled_pendulum_exchange() {
  // 3π/8 (67.5 deg): 8 pendulum pairs (4-fold Grid × 2-fold antisymmetry)
  constexpr float C38  = 0.3826834324f;  // cos(3π/8) = sin(π/8)
  constexpr float S38  = 0.9238795325f;  // sin(3π/8) = cos(π/8)
  // -π/√3 (≈-1.814 rad): irrational → quasiperiodic exchange pattern
  constexpr float CPS3  = -0.2407f;      // cos(π/√3)
  constexpr float SPS3N = -0.9706f;      // sin(-π/√3)

  constexpr float PRE  = 1.3698f;  // moderate: sinh grows, needs conservative scale
  constexpr float POST = 1.1489f;  // mild contraction for exchange stability

  Pattern p;
  p.name = "coupled_pendulum_exchange";
  p.inner = InnerFunction::Sinh;         // ANTISYMMETRIC: push-pull = two pendulums
  p.reduction = ReductionType::Grid;     // tile boundaries = coupling springs

  p.zn.zn[0] = {C38 * PRE, S38 * PRE};
  p.zn.zn[1] = {0.009f, -0.006f};       // perturbation: break perfect antisymmetry
  p.zn.zn[2] = {CPS3 * POST, SPS3N * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 2 (Breathing) — the BEAT ENVELOPE of the coupled system ──
  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 1.10f;
  p.color.fade = 0.986f;                // DAMPING: coupling dissipation
  p.color.color_mode = as_int(ColorMode::Breathing);               // Breathing: beat envelope visible as saturation
  p.color.spiral_angle_coeff = 0.72f;   // angular: pendulum pair sectors
  p.color.spiral_radius_coeff = 0.44f;  // radial: distance from coupling point
  // BEAT ENVELOPE (slow, strong, spatially varied)
  p.color.chroma_breath_amp = 0.32f;    // STRONG: full energy exchange visible
  p.color.chroma_breath_freq = 0.08f;   // SLOW beat: weak coupling (Δω small)
  p.color.chroma_breath_phase = 3.0f;   // SPATIAL: cascading exchange wave
  p.color.hue_velocity_base = 0.016f;
  p.color.hue_velocity_breath = 0.011f;
  p.color.hue_velocity_drift = 0.004f;
  // Luminance: bright = high energy (active pendulum), dark = low (resting)
  p.color.luma_shape_depth = 0.25f;     // strong: energy density contrast
  p.color.luma_shape_center = 1.8f;

  // ── Seed: DRIVING FORCE (kicks the coupled system) ──
  p.seed.border_width = 0.089f;         // WIDE: drive enters visibly
  p.seed.hue_speed = 0.10f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.13f;            // INCOMMENSURATE with beat freq (0.08)
  p.seed.pulse_amp = 0.40f;             // STRONG: re-excitation visible

  p.speed = 0.78f;                      // moderate: educational pace
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.120f;
  p.zn.zn0_anim_amp = 0.047f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.119f;
  p.zn.zn1_anim_amp = 0.045f;
  p.zn.zn1_anim_k = 7.0f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.082f,
                            .amplitude = 0.123f,
                            .offset = 0.440f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.121f,
                            .amplitude = 0.120f,
                            .offset = 0.320f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.056f,
                            .amplitude = 0.035f,
                            .offset = 0.100f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
