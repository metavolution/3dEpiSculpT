#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * TP1 Interference Veil — showcase for GPU inner ID 14 (cos(z) − tanh(z)).
 *
 * MATHEMATICS: TRANSCENDENTAL PAIR INTERFERENCE
 *
 * The map f(z) = cos(z) − tanh(z) combines two functions with singularities
 * at the same imaginary positions (poles at ±iπ/2 + nπi) but with opposed
 * convergence directions: cos drives z toward its real zeroes, tanh drives z
 * toward 0.  Their difference creates a web of competing basins:
 *
 *   Near z = 0:   f(z) ≈ 1 − z − z²/2 + z³/3 + ...  → offset attractor
 *   Near Im(z) ≈ π/2: f(z) amplifies → large-scale sweeping motion
 *   Near Re(z) = ±π/2: cos zeroes create fold lines
 *
 * The result is a veil-like interference texture — overlapping bands from
 * the cosine's periodicity modulated by the sigmoid smoothing of tanh.
 *
 * COLOR MODE 1 — SPIRAL:
 *
 *   hue_shift = hue_time + hv_drift × (spiral_a × angle + spiral_r × log_r)
 *
 * With spiral_angle_coeff = 1.8 the hue sweeps fast over the interference
 * bands, turning each fold-line into a color boundary.  The moderate
 * chroma_breath_amp gives gentle luminance pulse at the fold density peaks.
 *
 * AFFINE GEOMETRY:
 *
 * zn0 = (0.82, 0.15): scale-down with ~10.4° rotation.  The contraction
 * keeps iterates away from the pole strip |Im(z)| ≈ π/2, preventing
 * divergence.  The tilt breaks real-axis symmetry → asymmetric veil.
 *
 * zn2 = (0.90, −0.06): mild post-affine contraction with −3.8° counter-
 * rotation balances the pre-rotation, producing a gentle helical drift
 * without net angular accumulation.
 *
 * Pipeline: transform → reduce(torus) → sample → color(mode1) → seed → blend
 */
inline Pattern tp1_interference_veil() {
  Pattern p;
  p.name = "tp1_interference_veil";
  // Named-enum path: InnerFunction::TP1 ↔ GPU id 14 (cos(z)−tanh(z))
  p.inner = InnerFunction::TP1;
  p.extended_inner_id = -1;
  p.reduction = ReductionType::Torus;

  // Scale+rotation keeps iterates clear of tanh/cos pole strip
  p.zn.zn[0] = {0.82f,  0.15f};
  // Slight translate: breaks reflection symmetry → asymmetric interference
  p.zn.zn[1] = {0.012f, 0.006f};
  // Mild counter-rotation post-affine: dampens angular accumulation
  p.zn.zn[2] = {0.90f, -0.06f};
  p.zn.zn[3] = {0.0f,   0.0f};

  // ── Color: MODE 1 (Spiral) — fold lines → color boundaries ──
  p.color.hue_rotation      = 0.004f;   // very slow base hue drift
  p.color.sat_scale         = 1.08f;
  p.color.fade              = 0.993f;   // moderate persistence
  p.color.color_mode        = as_int(ColorMode::Spiral);        // Spiral
  p.color.spiral_angle_coeff  = 1.80f;  // fast angular hue sweep over folds
  p.color.spiral_radius_coeff = 0.60f;  // log_r rings mark basin boundaries
  p.color.chroma_breath_amp   = 0.30f;  // gentle luminance pulse at fold peaks
  p.color.chroma_breath_freq  = 0.80f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base   = 0.28f;
  p.color.hue_velocity_breath = 0.12f;
  p.color.hue_velocity_drift  = 0.55f;  // 55% spatial hue / 45% base
  p.color.luma_shape_depth    = 0.18f;
  p.color.luma_shape_center   = 1.6f;

  // ── Seed: fine border injection — refreshes veil edges ──
  p.seed.border_width = 0.014f;
  p.seed.hue          = 0.68f;
  p.seed.hue_speed    = 0.014f;
  p.seed.alpha        = 0.80f;

  p.speed = 0.32f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.101f;
  p.zn.zn0_anim_amp = 0.049f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.160f;
  p.zn.zn1_anim_amp = 0.054f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.093f;
  p.seed.pulse_amp = 0.14f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.059f,
                            .amplitude = 0.168f,
                            .offset = 0.600f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.102f,
                            .amplitude = 0.120f,
                            .offset = 0.300f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.089f,
                            .amplitude = 0.030f,
                            .offset = 0.014f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
