#pragma once

#include "base.hpp"

namespace epi::presets {

// [4A] Anisotropic sine shear w=sin((pi/2)e^{i atan(1/2)} z): periodic along the (2,1) axis, cosh-chirped across it; Grid seams of thin Simplex (w=0.012) give sparse filaments, tau=1.5.
/**
 * 4x3 Orthogonal Spatial Wave Matrix -- cell 4A.
 *
 * Row 4 (Anisotropic Shear) x Column A (Sparse Solitary Pulses).
 * - T(z) = sin(k z), k = (pi/2) e^{i atan(1/2)} (Sin, zn0 = k): with
 *   X + iY = k z, sin = sin X cosh Y + i cos X sinh Y is 2pi-periodic along
 *   the rational (2,1) direction and exponentially chirped across it.
 * - Grid (sawtooth) folds at odd levels sinX coshY = +-1, cosX sinhY = +-1;
 *   the Simplex band w = 0.012 lights only those seams => sparse oblique
 *   filaments (the cosh chirp bends them into sheared hyperbolae).
 * - fade = e^{-1/1.5} (tau = 1.5 steps): echoes die within two steps.
 * - Zn1Re lane shifts X by +-pi/8 at f_9: filaments slide like solitary
 *   shear pulses. AtmosphericDrift (mode 13) is luma-locked with an
 *   x-gradient hue (spiral_a = 1): anisotropic colour, no luma lift.
 *
 * Temporal ladder: f_9 = 0.06 * 2^(9/12) = 0.10091 Hz (12-TET; no two
 * cells share a frequency). Seed hue = 9/12 on the matrix hue wheel.
 */
inline Pattern spatial_wave_shear_solitary_filaments() {
  Pattern p;
  p.name = "spatial_wave_shear_solitary_filaments";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Grid;

  p.zn.zn[0] = {1.4049629f, 0.7024815f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.5134171f;
  p.color.color_mode = as_int(ColorMode::AtmosphericDrift);
  p.color.spiral_angle_coeff = 1.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 0.1009076f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f; // O9 bounded hue clock
  p.color.hue_velocity_breath = 0.1585052f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.012f;
  p.seed.hue = 0.75f;
  p.seed.hue_speed = 0.028f;
  p.seed.alpha = 0.98f;
  p.seed.pulse_freq = 0.1009076f;
  p.seed.pulse_amp = 0.0f; // envelope == 1: normalized, non-strobing injection

  p.speed = 1.0f;

  p.motion
      .add("shear_filament_slide",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.1009076f,
                            .amplitude = 0.3926991f,
                            .offset = 0.0f},
           control::ModTarget::Zn1Re);

  return p;
}

} // namespace epi::presets
