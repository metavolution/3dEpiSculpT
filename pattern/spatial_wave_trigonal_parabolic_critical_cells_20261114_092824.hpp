#pragma once

#include "base.hpp"

namespace epi::presets {

// [3B] D3 cubic cusp z^3+kappa at kappa=2/(3sqrt3): parabolic fixed point z=1/sqrt3 with multiplier 3z^2=1 -- smooth basin vs Torus-folded Cantor-dust cells.
/**
 * 4x3 Orthogonal Spatial Wave Matrix -- cell 3B.
 *
 * Row 3 (Crystallographic, 3-fold) x Column B (Critical Bifurcation).
 * - T(z) = z^3 + kappa (Cube, zn3 = kappa real). Fixed point z^3 + kappa = z
 *   with multiplier 3z^2 = 1 gives z_p = 1/sqrt3, kappa_c = z_p - z_p^3 =
 *   2/(3 sqrt3) = 0.3849002: the parabolic cusp of the cubic family.
 * - kappa < kappa_c: attracting fixed point, connected smooth basin;
 *   kappa > kappa_c: Julia set shatters into Cantor dust (cells). At
 *   kappa_c the basin is the 3-fold 'cauliflower' with critical slowing.
 * - T(omega z) = T(z) for omega^3 = 1 and kappa breaks z -> -z: exact D3.
 * - Torus folds escaping orbits back chaotically: they keep hitting the
 *   Simplex border band (w = 0.10) => bright fractured cells outside, dark
 *   smooth parabolic basin inside (orbits creep to z_p where the seed is 0).
 * - Lane sweeps kappa = kappa_c +- 0.012 (~3%) at f_7 across the cusp.
 * - fade = e^{-1/32}; ChromaBreathing (mode 9) with spiral_a = 3 and
 *   breath_phase = 6 pi colours the three lobes with a 3-fold hue wheel.
 *
 * Temporal ladder: f_7 = 0.06 * 2^(7/12) = 0.08990 Hz (12-TET; no two
 * cells share a frequency). Seed hue = 7/12 on the matrix hue wheel.
 */
inline Pattern spatial_wave_trigonal_parabolic_critical_cells() {
  Pattern p;
  p.name = "spatial_wave_trigonal_parabolic_critical_cells";
  p.inner = InnerFunction::Cube;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.3849002f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.9692332f;
  p.color.color_mode = as_int(ColorMode::ChromaBreathing);
  p.color.spiral_angle_coeff = 3.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.2f;
  p.color.chroma_breath_freq = 0.0898984f;
  p.color.chroma_breath_phase = 18.8495559f;
  p.color.hue_velocity_base = 0.0f; // O9 bounded hue clock
  p.color.hue_velocity_breath = 0.1412121f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.1f;
  p.seed.hue = 0.5833333f;
  p.seed.hue_speed = 0.014f;
  p.seed.alpha = 0.9f;
  p.seed.pulse_freq = 0.0898984f;
  p.seed.pulse_amp = 0.0f; // envelope == 1: normalized, non-strobing injection

  p.speed = 0.5f;

  p.motion
      .add("cubic_cusp_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0898984f,
                            .amplitude = 0.012f,
                            .offset = 0.3849002f},
           control::ModTarget::Zn3Re);

  return p;
}

} // namespace epi::presets
