#pragma once

#include "base.hpp"

namespace epi::presets {

// [1A] Rigid isotropic frame beta=1 (Lambda=0, zero echo spreading) + WaveRing sin(20r-5t)e^{-2r} with mix weight a*I => luma ~ a*I^2 confined to r<0.69; tau=3 steps.
/**
 * 4x3 Orthogonal Spatial Wave Matrix -- cell 1A.
 *
 * Row 1 (Isotropic / Radial) x Column A (Sparse Solitary Pulses).
 * - T(z) = z (Identity, zn0 = zn2 = 1): Clamp never activates because the
 *   unit square maps onto itself, so every iterate is exactly isotropic.
 * - WaveRing seed I = clamp(sin(20r - 5t) e^{-2r}, 0, 1) is blended with
 *   weight a = alpha*I, so the rendered value is ~alpha*I^2: the e^{-2r}
 *   envelope squares to e^{-4r} and I^2 >= 0.05 needs r < 0.69 and a
 *   rectified duty < 0.42 => ~90% of the frame stays below luma 0.05.
 * - fade = e^{-1/3} (Law L3 tau = 3 steps): crests travel outward at
 *   v = 5/20 = 0.25 units/s; the 3-step trail is 0.0125 units, i.e. sharp.
 * - OKLab rotation (mode 17) with spiral_a = spiral_r = breath_phase = 0 is
 *   isotropic and adds zero floor lift: black background stays black.
 *
 * Temporal ladder: f_0 = 0.06 * 2^(0/12) = 0.06000 Hz (12-TET; no two
 * cells share a frequency). Seed hue = 0/12 on the matrix hue wheel.
 */
inline Pattern spatial_wave_radial_solitary_target_pulse() {
  Pattern p;
  p.name = "spatial_wave_radial_solitary_target_pulse";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;

  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.7165313f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 0.06f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f; // O9 bounded hue clock
  p.color.hue_velocity_breath = 0.001508f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 0.98f;
  p.seed.pulse_freq = 0.06f;
  p.seed.pulse_amp = 0.0f; // envelope == 1: normalized, non-strobing injection

  p.speed = 1.0f;

  return p;
}

} // namespace epi::presets
