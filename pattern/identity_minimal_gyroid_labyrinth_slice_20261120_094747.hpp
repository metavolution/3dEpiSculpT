#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Showcase of the GyroidSlice seed (seed id 7), rendered nearly bare.
 *
 * - The seed lights the nodal set g = 0 of the gyroid
 *   g = sin x cos y + sin y cos z + sin z cos x, two periods across the field,
 *   on the plane z = t/2. Each slice is a labyrinth of thin lines that pinches
 *   and reconnects as the plane drifts (period 4 pi in time).
 * - T(z) = A sin(eps z), eps = pi/4, A = 4/pi: T'(0) = 1, cubic warp < 5% in
 *   |z| < 1, no fold; the labyrinth is shown undistorted.
 * - fade e^(-1/4) (tau = 4) turns each reconnection into a brief afterimage;
 *   alpha = 0.7178 pins the steady line peak at P = 0.92.
 */
inline Pattern identity_minimal_gyroid_labyrinth_slice() {
  Pattern p;
  p.name = "identity_minimal_gyroid_labyrinth_slice";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {0.7853982f, 0.0f}; // eps = pi/4
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.2732395f, 0.0f}; // A = 4/pi
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.7788008f;
  p.color.color_mode = as_int(ColorMode::LumaLockedHueDrift);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.06f;
  p.color.chroma_breath_freq = 0.07f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.1099557f; // +-0.25 turn, bounded
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;

  p.seed.type = SeedType::GyroidSlice;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.50f;                // cyan lines; g shifts hue by at most +-0.15
  p.seed.hue_speed = 0.01f;
  p.seed.alpha = 0.7178163f;         // peak-normalized, P = 0.92
  p.seed.pulse_freq = 0.07f;
  p.seed.pulse_amp = 0.0f;

  p.speed = 0.6f;

  return p;
}

} // namespace epi::presets
