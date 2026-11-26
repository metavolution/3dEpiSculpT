#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Driven damped harmonic oscillator — resonance wave.
 *
 * PHYSICS: getriebener gedaempfter harmonischer Oszillator
 *
 * The equation of motion:
 *   x'' + 2*gamma*x' + omega_0^2 * x = F_0 * cos(omega*t)
 *
 * Solution: x(t) = A_transient * e^(-gamma*t) * cos(omega_d*t + phi_0)
 *                 + A_steady * cos(omega*t - delta)
 *
 * The transient decays exponentially while the steady-state response
 * persists at the driving frequency.  At RESONANCE (omega = omega_0),
 * the amplitude reaches its maximum: A_max = F_0 / (2*gamma*omega_0).
 *
 * ENGINE MAPPING — each mechanism in the driven oscillator maps EXACTLY
 * to an engine parameter:
 *
 *   OSCILLATION = sin(z) inner function
 *     sin(z) IS the wave equation solution.  sin(x)cosh(y) along the
 *     real axis is pure oscillation; the cosh(y) growth along imaginary
 *     axis creates amplitude modulation.  Under iterated feedback, this
 *     produces PROPAGATING WAVE FRONTS.
 *
 *   DAMPING = fade parameter (0.978)
 *     Each frame: amplitude *= fade = e^(-gamma)
 *     Quality factor Q = pi / (-ln(fade)) = pi / 0.0222 ~ 141
 *     This is LIGHTLY DAMPED: ~45 frames of visible oscillation before
 *     amplitude drops to 1/e.  At 60fps, the decay is visible over
 *     about 0.75 seconds — slow enough to SEE, fast enough to be dynamic.
 *
 *   DRIVING = seed injection (border_width=0.050, pulse_amp=0.45)
 *     The seed border acts as the external driving force F_0*cos(omega*t).
 *     The pulse_amp=0.45 modulates the seed alpha between 0.05 and 0.95,
 *     creating a VISIBLE on/off cycle of energy injection from the edges.
 *     You can SEE the "push" arriving from the borders, driving the
 *     wave structure deeper into the frame.
 *
 *   RESONANCE = breathing color mode (mode 2) near-matched to seed pulse
 *     The breathing envelope (breath_freq=0.12) and seed pulse
 *     (pulse_freq=0.15) are CLOSE but not equal.  This creates a
 *     BEAT PATTERN: the saturation modulation slowly builds up and
 *     decays as the driving and response drift in and out of phase.
 *     Beat period = 1/|0.15 - 0.12| = 1/0.03 ~ 33 time units.
 *     This IS the resonance envelope: you see amplitude building
 *     when drive and response are in phase, and decaying when they're
 *     out of phase.
 *
 * - Torus reduction = periodic boundary (like a vibrating string with
 *   fixed endpoints reflected into a torus).  The mirrored wrapping
 *   creates standing wave nodes — exactly what you see in a resonant
 *   cavity.
 *
 * - Pre-rotation at pi/6 (30 deg, dodecagonal) creates 12 standing
 *   wave nodes around the cavity — like the 12th harmonic of a drum.
 *
 * - Counter-rotation in post (-pi/8 = -22.5 deg) makes successive
 *   wave fronts propagate in alternating directions, creating visible
 *   STANDING WAVE interference.  This is how resonance appears:
 *   forward and reflected waves superpose.
 *
 * WHAT TO WATCH:
 *   1. Energy pulses IN from the borders (driving force)
 *   2. Wave structure BUILDS UP (resonant amplification)
 *   3. Saturation BREATHES (amplitude modulation of the oscillation)
 *   4. Between drive peaks, structure FADES (damping)
 *   5. The cycle repeats — driven, damped, driven, damped...
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern driven_resonance_wave() {
  // pi/6 (30 deg): dodecagonal standing wave nodes
  constexpr float C6  = 0.8660254038f;   // cos(pi/6) = sqrt(3)/2
  constexpr float S6  = 0.5f;            // sin(pi/6) = 1/2
  // -pi/8 (-22.5 deg): counter-rotation for standing wave interference
  constexpr float C8  = 0.9238795325f;   // cos(pi/8)
  constexpr float S8N = -0.3826834324f;  // sin(-pi/8)

  constexpr float PRE  = 1.4305f;  // moderate kick: clear wave structure (not chaotic)
  constexpr float POST = 1.2580f;  // mild contraction: Q ~ 141 lightly damped

  Pattern p;
  p.name = "driven_resonance_wave";
  p.inner = InnerFunction::Sin;          // IS the oscillation function
  p.reduction = ReductionType::Torus;    // resonant cavity (periodic boundaries)

  p.zn.zn[0] = {C6 * PRE, S6 * PRE};
  p.zn.zn[1] = {0.0544f, -0.0309f};       // small perturbation (excites multiple modes)
  p.zn.zn[2] = {C8 * POST, S8N * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 2 (Breathing) — the oscillation amplitude IS the breathing ──
  p.color.hue_rotation = 0.016f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.986f;                // DAMPING: gamma = -ln(0.978) ~ 0.022
  p.color.color_mode = as_int(ColorMode::Breathing);               // Breathing: visible amplitude modulation
  p.color.spiral_angle_coeff = 0.70f;   // angular variation shows wave nodes
  p.color.spiral_radius_coeff = 0.40f;  // radial depth: distance from resonator center
  // Breathing envelope = RESPONSE AMPLITUDE of the oscillator
  p.color.chroma_breath_amp = 0.35f;    // STRONG: make oscillation very visible
  p.color.chroma_breath_freq = 0.12f;   // near the driving frequency (beat!)
  p.color.chroma_breath_phase = 2.5f;   // SPATIAL: breathing rotates with the wave
  // Mode 2 uses velocity params for spiral hue (not velocity-dependent)
  p.color.hue_velocity_base = 0.015f;
  p.color.hue_velocity_breath = 0.012f;
  p.color.hue_velocity_drift = 0.003f;
  // Luminance shaping: wave peaks (bright) vs nodes (dark)
  p.color.luma_shape_depth = 0.26f;     // strong contrast between peaks and nodes
  p.color.luma_shape_center = 1.8f;

  // ── Seed: THE DRIVING FORCE ──
  p.seed.border_width = 0.091f;         // WIDE: driving force visibly enters from edges
  p.seed.hue_speed = 0.12f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.15f;            // DRIVING FREQUENCY: near breath_freq for resonance
  p.seed.pulse_amp = 0.45f;             // STRONG DRIVE: visible on/off energy injection

  p.speed = 0.85f;                      // moderate-brisk: observable oscillation tempo

  // ── Motion layer: slow damping drift (Q-factor modulation) ───────────────
  //
  // PHYSICS RATIONALE:
  //   The fade parameter maps to the oscillator's damping coefficient γ:
  //     fade = e^{-γ}  →  γ = -ln(fade)
  //   At fade=0.978, γ ≈ 0.022, Q = π/γ ≈ 141 (lightly damped).
  //   At fade=0.974, γ ≈ 0.026, Q ≈ 120 — more damped, faster decay.
  //   At fade=0.982, γ ≈ 0.018, Q ≈ 175 — less damped, wave structure
  //     lingers longer before each drive cycle refreshes it.
  //
  //   A 40-second sine on fade drives the system between these two extremes.
  //   The MACRO PERIOD creates a second-order beat structure atop the
  //   already-present 33-unit micro-beat (|seed_pulse - chroma_breath|).
  //   The result is a three-level resonance hierarchy:
  //     • Frame level   — the per-frame oscillation (inner Sin transform)
  //     • Beat level    — 33-unit beat between drive (0.15) and breath (0.12)
  //     • Macro level   — 40-second Q-factor drift (this motion lane)
  //
  //   Two slow hue lanes add spectral contrast to each Q phase:
  //     During high-Q phases (wave structure lingers) → warmer hue rotation
  //     During low-Q  phases (fast decay)             → cooler hue rotation
  p.motion
      .add("q_drift",
           control::ModLane{.wave         = control::Waveform::Sine,
                            .frequency_hz = 1.0f / 40.0f,   // 40-second macro period
                            .amplitude    = 0.004f,          // fade: 0.974 .. 0.982
                            .offset       = 0.978f},         // resting = preset fade
           control::ModTarget::Fade)
      .add("hue_sweep",
           control::ModLane{.wave         = control::Waveform::Sine,
                            .frequency_hz = 1.0f / 38.0f,   // slightly incommensurate
                            .amplitude    = 0.007f,          // hue_rot: 0.009 .. 0.023
                            .offset       = 0.016f},         // resting = preset hue_rotation
           control::ModTarget::HueRot);
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.099f;
  p.zn.zn0_anim_amp = 0.035f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.143f;
  p.zn.zn1_anim_amp = 0.051f;
  p.zn.zn1_anim_k = 7.0f;


  return p;
}

} // namespace epi::presets
