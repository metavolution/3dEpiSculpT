#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Exponential driven resonance tunnel preset.
 *
 * PHYSICS: GETRIEBENER GEDÄMPFTER HARMONISCHER OSZILLATOR (Resonanztunnel)
 *
 * The equation of motion of the driven damped harmonic oscillator:
 *
 *   ẍ + 2γẋ + ω₀²x = F₀ cos(ωt)
 *
 * This preset makes EVERY TERM of the equation directly visible as a
 * component of an exponential cathedral tunnel.  Where the original
 * oscillator presets (driven_resonance_wave, damped_harmonic_spring)
 * use Sin/Cos inner functions (which ARE the oscillation), this preset
 * uses Exp — which maps the oscillator's PHASE SPACE into architectural
 * form.  The result: a tunnel of resonant arches where you can SEE the
 * energy flowing in from the borders (driving), building up in the
 * arches (resonance), and fading between pulses (damping).
 *
 * ENGINE MAPPING TO THE DDHO EQUATION:
 *
 *   THE OSCILLATION = Exp(z) inner function
 *     The complex exponential e^z = e^x(cos y + i sin y) decomposes
 *     into AMPLITUDE (e^x along real axis) and PHASE (y = angle).
 *     This IS the polar form of the oscillator's state: x(t) = A(t)cos(φ(t)).
 *     Under iterated feedback, the Exp creates ARCHES (from the e^x
 *     radial mapping) at angular positions determined by the rotation —
 *     a direct visualization of the oscillator's amplitude-phase evolution.
 *
 *   THE RESONANT CAVITY = Grid reduction
 *     Grid tiles the plane into discrete cells.  Each cell IS a resonant
 *     cavity: the tile boundaries are REFLECTIVE WALLS where the oscillation
 *     reverses.  With Exp mapping, Grid boundaries become visible RIBS —
 *     the structural skeleton of the cathedral.  Standing waves form between
 *     the ribs, exactly as in a resonant organ pipe or laser cavity.
 *
 *   THE NATURAL FREQUENCY ω₀ = 1 radian pre-rotation (57.30°)
 *     1 radian is the NATURAL UNIT of angle — the angle subtended when
 *     arc length equals radius.  Using ω₀ = 1 (in natural units) is the
 *     canonical normalization of the oscillator equation.  This creates
 *     2π/1 ≈ 6.28 arch-pairs per revolution: each pair is one complete
 *     oscillation cycle made visible as architecture.
 *
 *   THE RESONANCE PHASE SHIFT δ = π/2 post-rotation (−90°)
 *     At RESONANCE (ω = ω₀), the response lags the driving force by
 *     EXACTLY π/2 (90 degrees).  This is the most famous result in
 *     oscillator physics:
 *
 *       δ(ω) = arctan(2γω/(ω₀² − ω²))
 *       At ω = ω₀: δ = arctan(∞) = π/2
 *
 *     The post-rotation of −π/2 makes this LITERALLY VISIBLE: each depth
 *     layer of the tunnel is rotated exactly 90° from the previous.
 *     This perpendicular stacking IS the resonance phase shift made
 *     architectural — you can SEE that each "response" layer is a
 *     quarter-turn behind the "driving" layer above it.
 *
 *   THE DRIVING FORCE F₀cos(ωt) = seed pulse
 *     The seed border (width=0.040) injects energy from the frame edges
 *     at frequency pulse_freq=0.13.  You can SEE the energy entering:
 *     a bright pulse appears at the borders and propagates inward through
 *     the tunnel arches.  This IS the external force F₀cos(ωt) made visible.
 *
 *   THE DAMPING γ = fade (0.982)
 *     Each frame: amplitude *= 0.982 = e^(−γ).  So γ = −ln(0.982) ≈ 0.0182.
 *     Quality factor Q = π/(−ln(fade)) = π/0.0182 ≈ 173.
 *     This is MODERATELY DAMPED: ~55 frames (≈0.9s at 60fps) before
 *     amplitude drops to 1/e.  Between driving pulses, you can SEE
 *     the arches dimming — that's the dissipation term 2γẋ in action.
 *
 *   THE RESONANCE ENVELOPE = beat between pulse and breathing
 *     pulse_freq = 0.13 (driving ω)
 *     breath_freq = 0.10 (response near ω₀)
 *     Beat frequency = |0.13 − 0.10| = 0.03
 *     Beat period ≈ 33 time units: the saturation/brightness slowly
 *     BUILDS UP (constructive: drive and response in phase) then
 *     DECAYS (destructive: drive and response out of phase).
 *     This IS the resonance envelope A(t) = F₀/√((ω₀²−ω²)² + 4γ²ω²)
 *     made visible as pulsating tunnel brightness.
 *
 *   THREE TIMESCALES = hue velocity components
 *     v_base  = 0.015 → ω₀ (natural frequency: fast hue cycling)
 *     v_breath = 0.010 → ω  (driving frequency: medium modulation)
 *     v_drift  = 0.004 → Δω (beat frequency: slow envelope)
 *     The hue evolution IS a spectrogram of the oscillator: you see
 *     fast oscillation modulated by driving, modulated by resonance.
 *
 *   PHASE SHIFT VISUALIZATION = breath_phase (3.0)
 *     The breathing varies angularly: breath(θ) = 1 + A·sin(ωt + 3θ).
 *     At any instant, different angular sectors are at DIFFERENT PHASES
 *     of the oscillation.  This IS the phase portrait — you can see
 *     which parts of the tunnel are at maximum amplitude (constructive)
 *     and which are at minimum (destructive), with a 3-lobe spatial
 *     pattern showing the phase relationship δ between drive and response.
 *
 * WHAT TO WATCH (the DDHO in action):
 *   1. DRIVING: bright pulses enter from borders → F₀cos(ωt)
 *   2. BUILDUP: arch structure grows brighter → resonant amplification
 *   3. DAMPING: between pulses, arches dim → dissipation 2γẋ
 *   4. RE-DRIVE: next pulse arrives → amplitude jumps up again
 *   5. BEAT ENVELOPE: overall brightness slowly waxes and wanes
 *   6. PHASE SHIFT: brightness peak is 90° BEHIND the driving peak
 *   7. THREE SPEEDS: fast hue = ω₀, medium pulse = ω, slow beat = Δω
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern exp_driven_resonance_tunnel() {
  // ω₀ = 1 radian (57.30°): the natural frequency in canonical units
  constexpr float CW0  =  0.5403f;           // cos(1)
  constexpr float SW0  =  0.8415f;           // sin(1)
  // δ = −π/2 (−90°): THE resonance phase shift
  // At resonance: response lags drive by exactly 90°
  constexpr float CPI2 =  0.0f;              // cos(π/2)  = 0 exactly
  constexpr float SPI2 = -1.0f;              // sin(−π/2) = −1 exactly

  constexpr float PRE  = 1.1594f;  // moderate: clear arch structure (not chaotic)
  constexpr float POST = 0.9136f;  // moderate contraction: product 0.738

  Pattern p;
  p.name = "exp_driven_resonance_tunnel";
  p.inner = InnerFunction::Exp;              // amplitude × phase decomposition
  p.reduction = ReductionType::Grid;         // resonant cavity walls = arch ribs

  p.zn.zn[0] = {CW0 * PRE, SW0 * PRE};
  p.zn.zn[1] = {0.0355f, -0.0399f};           // perturbation: excites multiple modes
  p.zn.zn[2] = {CPI2 * POST, SPI2 * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 2 (Breathing) — the oscillation amplitude envelope ──
  p.color.hue_rotation = 0.014f;             // moderate: oscillation visible in color
  p.color.sat_scale = 1.10f;
  p.color.fade = 0.982f;                     // DAMPING: Q ≈ 173, visible decay
  p.color.color_mode = as_int(ColorMode::Breathing);                    // Breathing: amplitude modulation
  p.color.spiral_angle_coeff = 0.85f;        // angular: wave node sectors
  p.color.spiral_radius_coeff = 0.40f;       // radial: distance from oscillation center
  // RESPONSE AMPLITUDE OSCILLATION
  p.color.chroma_breath_amp = 0.38f;         // STRONG: visible oscillation envelope
  p.color.chroma_breath_freq = 0.10f;        // response frequency (near ω₀)
  p.color.chroma_breath_phase = 3.0f;        // SPATIAL PHASE: see phase shift across tunnel
  // THREE TIMESCALES: ω₀ (natural), ω (driving), Δω (beat)
  p.color.hue_velocity_base = 0.015f;        // FAST: natural spring frequency ω₀
  p.color.hue_velocity_breath = 0.010f;      // MEDIUM: driving frequency ω
  p.color.hue_velocity_drift = 0.004f;       // SLOW: beat/resonance envelope Δω
  // Luminance: oscillation amplitude as brightness contrast
  p.color.luma_shape_depth = 0.28f;          // STRONG: amplitude directly visible
  p.color.luma_shape_center = 1.9f;

  // ── Seed: THE DRIVING FORCE F₀cos(ωt) ──
  p.seed.border_width = 0.094f;              // WIDE: energy visibly enters from edges
  p.seed.hue_speed = 0.10f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.13f;                 // DRIVING FREQUENCY ω (detuned → beats!)
  p.seed.pulse_amp = 0.40f;                  // STRONG DRIVE: visible energy injection

  p.speed = 0.60f;                           // moderate: educational observation tempo
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.102f;
  p.zn.zn0_anim_amp = 0.047f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.119f;
  p.zn.zn1_anim_amp = 0.055f;
  p.zn.zn1_anim_k = 5.0f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.063f,
                            .amplitude = 0.120f,
                            .offset = 0.400f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.093f,
                            .amplitude = 0.120f,
                            .offset = 0.380f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.060f,
                            .amplitude = 0.035f,
                            .offset = 0.100f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
