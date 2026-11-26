#pragma once

#include "base.hpp"

namespace epi::presets {

// Post-critically finite quartic Basilica lock (F12 0.7*(z^4-1) * zn2=10/7 => w^4-1, zn0=5/4) with 2:3 SpatialWave ratio.
/**
 * Rational Resonance Lock 2: Post-Critically Finite Quartic Basilica Lattice.
 *
 * Analytical derivation:
 * - Inner = F12 computes f(w) = 0.7 * (w^4 - 1). Setting zn2 = 10/7 cancels the
 *   0.7 factor identically, yielding the exact monic polynomial t_z = w^4 - 1.
 * - Pre-scale zn0 = 5/4 = 1.25 places the 4 exact roots w in {+-1, +-i} at
 *   rational clip coordinates z_in in {(+-4/5, 0), (0, +-4/5)}, while the
 *   critical point z_in = 0 maps in 1 step to t_z(0) = -1.0 (exact boundary
 *   seed hit), locking every D4 compass lobe onto a rational preimage tree.
 * - On [-1, 1]^2, |w|_max = (5/4)*sqrt(2) = 1.7678 => |t_z|_max <= 8.77
 *   (at most 4.4 Torus folds per step, strictly bounded away from Nyquist).
 * - Color = SpatialWave (mode 4): exact 2:3 integer spatial harmonic lock
 *   (spiral_r = 2.0, breath_freq = 3.0) with 3:-2 counter-propagating phase
 *   velocities (hv_base = 0.03, hv_breath = -0.02, hv_drift = 0.0) forming
 *   stationary nodal standing-wave beats every 100 s.
 */
inline Pattern rational_quartic_basilica_lock() {
  Pattern p;
  p.name = "rational_quartic_basilica_lock";
  p.inner = InnerFunction::F12;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.25f, 0.0f};         // exact 5:4 pre-scale (roots at +-0.8)
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {10.0f / 7.0f, 0.0f};  // exact 10:7 post-scale => t_z = w^4 - 1
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.05f;
  p.color.fade = 0.9375f;             // 15/16 exact dyadic decay (tau = 15.5 steps)
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.5f;  // 1:2 half-turn horizontal slope
  p.color.spiral_radius_coeff = 2.0f; // wave1 = 2 cycles across u in [0, 1]
  p.color.chroma_breath_amp = 0.15f;
  p.color.chroma_breath_freq = 3.0f;  // wave2 = 3 cycles (exact 2:3 harmonic lock)
  p.color.chroma_breath_phase = 0.10f;
  p.color.hue_velocity_base = 0.03f;    // 3:2 temporal frequency ratio
  p.color.hue_velocity_breath = -0.02f; // counter-propagating standing-wave lock
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.12f;
  p.color.luma_shape_center = 4.0f;   // 4-cycle D4 saturation harmonic

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.0625f;      // 1/16 exact border frame
  p.seed.hue = 0.5f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.9375f;             // 15/16
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;            // unit envelope env(t) == 1.0

  p.speed = 1.0f;

  p.motion
      .add("standing_wave_amp_3",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.06f, // 3:2 lane frequency lock
                            .amplitude = 0.05f,
                            .offset = 0.15f},
           control::ModTarget::ChromaBreathAmp)
      .add("standing_wave_amp_2",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.04f,
                            .amplitude = 0.04f,
                            .offset = 0.10f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathPhase);

  return p;
}

} // namespace epi::presets
