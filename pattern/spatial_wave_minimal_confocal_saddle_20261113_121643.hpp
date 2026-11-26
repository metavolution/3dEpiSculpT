#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimalist 6): Confocal Elliptic-Coordinate Saddle.
 *
 * Mathematical foundation:
 * - w = cosh(z) is the conformal map behind elliptic coordinates: lines
 *   Re z = const become confocal ellipses and Im z = const become confocal
 *   hyperbolas with foci at +/-1. The same map solves potential flow through
 *   a slit and the field of a charged strip.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Cosh with real pre-scale k = 1.22 and real post-gain c = 0.98:
 *   Re T = c*cosh(kx)*cos(ky), Im T = c*sinh(kx)*sin(ky). The geometry keeps
 *   full D2 mirror symmetry, so no ZN animation is authored.
 * - The critical point z = 0 has critical value T(0) = c = 0.98, placed on
 *   purpose inside the seed border (|x| > 1 - 0.09). Near the centre
 *   T - c ~ c*k^2*(x^2 - y^2)/2, so the seed ramp paints a luminous
 *   hyperbolic saddle (an X of light along the diagonals) instead of a black
 *   super-attracting hole.
 * - Color = SpatialWave with golden-conjugate wave pair (0.618 : 1.236).
 * - Motion: Zn2Re moves the critical value across the border ramp, which
 *   opens and closes the saddle like a breathing focal slit.
 */
inline Pattern spatial_wave_minimal_confocal_saddle() {
  Pattern p;
  p.name = "spatial_wave_minimal_confocal_saddle";
  p.inner = InnerFunction::Cosh;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.22f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.98f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.006f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.989f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.40f;
  p.color.spiral_radius_coeff = 0.618f;
  p.color.chroma_breath_amp = 0.14f;
  p.color.chroma_breath_freq = 1.236f;
  p.color.chroma_breath_phase = 0.07f;
  p.color.hue_velocity_base = 0.013f;
  p.color.hue_velocity_breath = 0.009f;
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.15f;
  p.color.luma_shape_center = 1.5f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.09f;
  p.seed.hue = 0.86f;
  p.seed.hue_speed = 0.075f;
  p.seed.alpha = 0.93f;
  p.seed.pulse_freq = 0.055f;
  p.seed.pulse_amp = 0.13f;
  p.speed = 0.6f;

  p.motion
      .add("focal_slit_breath",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.043f,
                            .amplitude = 0.045f,
                            .offset = 0.98f},
           control::ModTarget::Zn2Re)
      .add("hyperbola_hue_slope",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.061f,
                            .amplitude = 0.10f,
                            .offset = 0.40f},
           control::ModTarget::SpiralAngleCoeff);

  return p;
}

} // namespace epi::presets
