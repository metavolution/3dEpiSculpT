#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern mandelbrot_ghost() {
  Pattern p;
  p.name = "mandelbrot_ghost";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.3673f, 0.2206f};
  p.zn.zn[1] = {0.2210f, -0.2201f};
  p.zn.zn[2] = {1.2769f, -0.1799f};
  p.zn.zn[3] = {0.245f, 0.0f};
  p.color.hue_rotation = 0.012f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.990f;

  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.15f;
  p.color.spiral_radius_coeff = 0.12f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 0.20f;
  p.color.chroma_breath_phase = 0.04f;
  p.color.hue_velocity_base = 0.008f;
  p.color.hue_velocity_breath = 0.005f;
  p.color.hue_velocity_drift = 0.002f;
  p.color.luma_shape_depth = 0.15f;
  p.color.luma_shape_center = 2.0f;

  p.seed.border_width = 0.090f;
  p.seed.hue_speed = 0.096f;
  p.seed.alpha = 0.92f;
  p.speed = 0.5f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.124f;
  p.zn.zn0_anim_amp = 0.047f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.196f;
  p.zn.zn1_anim_amp = 0.054f;
  p.zn.zn1_anim_k = 5.0f;
  p.seed.pulse_freq = 0.132f;
  p.seed.pulse_amp = 0.15f;

  return p;
}

} // namespace epi::presets
