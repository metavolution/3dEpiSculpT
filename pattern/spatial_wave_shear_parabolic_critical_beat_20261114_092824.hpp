#pragma once

#include "base.hpp"

namespace epi::presets {

// [4B] Parabolic sine map lambda*sin z at lambda=1 (multiplier exactly 1): cubic contraction along x, cubic repulsion along iy -- smooth/fractured shear split at threshold.
/**
 * 4x3 Orthogonal Spatial Wave Matrix -- cell 4B.
 *
 * Row 4 (Anisotropic Shear / Beat) x Column B (Critical Bifurcation).
 * - T(z) = lambda sin z with lambda = 1 (Sin, zn0 = zn2 = 1): the fixed point
 *   0 has multiplier exactly 1. sin x = x - x^3/6 contracts along the real
 *   axis while sin(iy) = i(y + y^3/6) repels along the imaginary axis: a
 *   two-petal parabolic flower, i.e. an anisotropic critical point.
 * - lambda < 1: global sink (smooth); lambda > 1: source whose Grid-folded
 *   escape fractures the frame into cells. Lane sweeps lambda = 1 +- 0.03
 *   at f_10, beating the smooth band against the fractured band.
 * - The Cross seed axes coincide with the attracting (x) and repelling (y)
 *   eigen-directions of the parabolic point.
 * - fade = e^{-1/16} (tau = 16 steps). SpatialWave (mode 4, accumulating
 *   hue) uses spiral_a = 1/tau: one x-gradient hue turn per memory time.
 *
 * Temporal ladder: f_10 = 0.06 * 2^(10/12) = 0.10691 Hz (12-TET; no two
 * cells share a frequency). Seed hue = 10/12 on the matrix hue wheel.
 */
inline Pattern spatial_wave_shear_parabolic_critical_beat() {
  Pattern p;
  p.name = "spatial_wave_shear_parabolic_critical_beat";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Grid;

  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.9394131f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0625f;
  p.color.spiral_radius_coeff = 2.0f;
  p.color.chroma_breath_amp = 0.01f;
  p.color.chroma_breath_freq = 0.1069078f;
  p.color.chroma_breath_phase = 0.005f;
  p.color.hue_velocity_base = 0.0f; // O9 bounded hue clock
  p.color.hue_velocity_breath = 0.0026869f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.1f;
  p.color.luma_shape_center = 2.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.8333333f;
  p.seed.hue_speed = 0.016f;
  p.seed.alpha = 0.9f;
  p.seed.pulse_freq = 0.1069078f;
  p.seed.pulse_amp = 0.0f; // envelope == 1: normalized, non-strobing injection

  p.speed = 0.5f;

  p.motion
      .add("parabolic_multiplier_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.1069078f,
                            .amplitude = 0.03f,
                            .offset = 1.0f},
           control::ModTarget::Zn2Re);

  return p;
}

} // namespace epi::presets
