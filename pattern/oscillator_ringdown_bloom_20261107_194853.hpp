#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Oscillator ringdown bloom preset.
 *
 * PHYSICS: getriebener gedämpfter harmonischer Oszillator — TRANSIENT RESPONSE
 *
 * The complete solution to x'' + 2γx' + ω₀²x = F₀·cos(ωt) has TWO parts:
 *
 *   x(t) = A_transient · e^{-γt} · cos(ω_d·t + φ₀)     ← dies away
 *        + A_steady · cos(ωt - δ)                         ← persists
 *
 * The TRANSIENT part decays exponentially with time constant 1/γ, while
 * the steady-state persists at the driving frequency.  When the driving
 * force STOPS, the system "rings down" — oscillating with exponentially
 * decreasing amplitude until the next drive event re-excites it.
 *
 * This preset visualizes the RINGDOWN:
 *   1. Drive pulse arrives → amplitude BLOOMS (grows rapidly)
 *   2. Drive ends → transient envelope e^{-γt} takes over
 *   3. Oscillation continues but FADES (visible decay per frame)
 *   4. Near silence → next drive pulse → BLOOM again
 *
 * ENGINE MAPPING:
 *
 *   TRANSIENT ENVELOPE = cosh(z) inner function
 *     cosh(z) = (e^z + e^{-z})/2 is the SYMMETRIC exponential envelope:
 *     cosh(0) = 1 (maximum at equilibrium), growing exponentially outward.
 *     This is geometrically similar to the e^{-γt} envelope of the free
 *     oscillator response — under iterated feedback with fade < 1, the
 *     cosh profile decays frame-by-frame, creating the visible RINGDOWN.
 *
 *     The BLOOM happens when a seed pulse arrives: the cosh dome suddenly
 *     inflates with fresh energy, amplitude jumps up, and the pattern
 *     "flowers" outward from the injection region before decaying again.
 *
 *   STRONG DAMPING = fade 0.973 (Q ≈ 115)
 *     -ln(0.973) ≈ 0.0274 = γ (damping coefficient per frame)
 *     At 60fps, the decay time constant is ~36 frames ≈ 0.6 seconds.
 *     You can COUNT the oscillation cycles as they fade: ~6-8 clearly
 *     visible oscillations between drive events.  This is the regime
 *     of a tuning fork or a struck bell — clearly ringing, clearly dying.
 *
 *   PERIODIC RE-EXCITATION = seed pulse (amp=0.44, freq=0.10)
 *     The seed border injects energy at frequency 0.10 Hz (pulse_freq).
 *     Between injections, the oscillator RINGS DOWN visibly.  The drive
 *     frequency is deliberately LOW (0.10) so there is ENOUGH TIME between
 *     pulses to see the full ringdown envelope.  Each re-excitation
 *     creates a visible "bloom" of new structure bursting outward.
 *
 *   WIDE DRIVE BORDER = border_width 0.046
 *     The driving force enters visibly from the edges.  You can SEE the
 *     energy wave propagating INWARD from the borders, exciting the
 *     oscillator back to life.  Then the edges decay first (further from
 *     equilibrium) while the center (near equilibrium) sustains longest.
 *
 *   THREE-TIMESCALE COLOR = Alive mode 3
 *     The alive hue velocity has three components:
 *       v(t) = ω₀ + ω_d·sin(0.08·2πt) + Δω·sin(0.003·2πt + 1.7)
 *     Mapped to the oscillator:
 *       hv_base  = 0.018 → NATURAL FREQUENCY ω₀ (fast hue cycling)
 *       hv_breath = 0.013 → DAMPED FREQUENCY ω_d = √(ω₀²-γ²)
 *       hv_drift  = 0.006 → RINGDOWN ENVELOPE (slow modulation)
 *     Visual result: fast color oscillation (the vibration) wrapped in a
 *     medium-speed modulation (damped natural freq) wrapped in a SLOW
 *     rise-and-fall (the ringdown envelope).  It is a TIME-FREQUENCY
 *     spectrogram made visible.
 *
 * - Torus reduction: smooth resonant cavity (mirrored wrap creates
 *   standing-wave conditions without harsh grid boundaries).
 *
 * - Pre-rotation at π/7 (≈25.7°): creates 7-fold standing modes.
 *   7 is prime, so no accidental degeneracies — each harmonic is distinct.
 *
 * - Post-rotation at -1/φ radians (≈-35.4°): the reciprocal golden ratio
 *   creates quasiperiodic orbit decay — the ringdown path never exactly
 *   repeats, like a damped Lissajous figure with irrational frequency ratio.
 *
 * WHAT TO WATCH (educational sequence):
 *   1. BLOOM: Bright energy bursts inward from the borders (drive pulse)
 *   2. RING: Cosh-shaped standing waves vibrate outward from center
 *   3. DECAY: Each successive oscillation is visibly dimmer (e^{-γt})
 *   4. SILENCE: Pattern nearly fades to black (damping wins)
 *   5. RE-BLOOM: Next drive pulse → cycle repeats from step 1
 *   Count ~6-8 visible oscillation cycles per ringdown — that's Q/π.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern oscillator_ringdown_bloom() {
  // π/7 (≈25.71 deg): 7-fold standing wave modes (prime = no degeneracies)
  constexpr float C7  = 0.9009688679f;   // cos(π/7)
  constexpr float S7  = 0.4338837391f;   // sin(π/7)
  // -1/φ (≈-35.4 deg): reciprocal golden ratio → quasiperiodic decay orbit
  // 1/φ = φ-1 = 0.6180339887... rad
  constexpr float CIG  = 0.8150232f;     // cos(1/φ)
  constexpr float SIGN = -0.5794437f;    // sin(-1/φ)

  constexpr float PRE  = 1.18f;  // drives cosh across the torus boundary
  constexpr float POST = 0.96f;  // sustained ringdown without starving the seed rim

  Pattern p;
  p.name = "oscillator_ringdown_bloom";
  p.inner = InnerFunction::Cosh;         // (e^z+e^-z)/2 IS the transient envelope
  p.reduction = ReductionType::Torus;    // smooth resonant cavity

  p.zn.zn[0] = {C7 * PRE, S7 * PRE};
  p.zn.zn[1] = {0.014f, -0.009f};       // perturbation: excite multiple modes
  p.zn.zn[2] = {CIG * POST, SIGN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.095f;
  p.zn.zn0_anim_amp = 0.048f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.068f;
  p.zn.zn1_anim_amp = 0.026f;
  p.zn.zn1_anim_k = 7.0f;

  // ── Color: MODE 3 (Alive) — three timescales of the decaying oscillator ──
  p.color.hue_rotation = 0.016f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.988f;                // sustained ringdown (~80 frames) so blooms fill the cavity
  p.color.color_mode = as_int(ColorMode::Alive);               // Alive: 3-velocity = 3 oscillator frequencies
  p.color.spiral_angle_coeff = 0.75f;   // angular: standing wave node pattern
  p.color.spiral_radius_coeff = 0.45f;  // radial: distance from equilibrium
  p.color.chroma_breath_amp = 0.28f;    // strong: visible amplitude modulation
  p.color.chroma_breath_freq = 0.14f;   // spatial breathing within cavity
  p.color.chroma_breath_phase = 0.35f;
  // THREE OSCILLATOR FREQUENCIES (spectrogram visible)
  p.color.hue_velocity_base = 0.018f;   // FAST: natural frequency ω₀
  p.color.hue_velocity_breath = 0.013f; // MEDIUM: damped natural freq ω_d
  p.color.hue_velocity_drift = 0.006f;  // SLOW: ringdown envelope
  // Luminance: bright at maximum amplitude, dark at nodes
  p.color.luma_shape_depth = 0.26f;     // strong: make ringdown amplitude visible
  p.color.luma_shape_center = 1.9f;

  // ── Seed: THE DRIVING FORCE (periodic re-excitation) ──
  p.seed.border_width = 0.11f;          // visible drive rim that cosh folds inward
  p.seed.hue_speed = 0.11f;
  p.seed.alpha = 0.95f;
  p.seed.pulse_freq = 0.25f;            // visible bloom/ringdown cycle
  p.seed.pulse_amp = 0.25f;             // modulates drive without extinguishing the field

  p.speed = 0.80f;                      // moderate: educational observation pace
  return p;
}

} // namespace epi::presets
