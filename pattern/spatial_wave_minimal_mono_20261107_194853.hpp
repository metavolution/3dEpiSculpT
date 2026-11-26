#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern spatial_wave_minimal_mono() {
  Pattern p;
  p.name = "spatial_wave_minimal_mono";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {0.985f, 0.015f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.012f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.42f;
  p.color.spiral_radius_coeff = 0.52f;
  p.color.chroma_breath_amp = 0.08f;
  p.color.chroma_breath_freq = 0.64f;
  p.color.chroma_breath_phase = 0.06f;
  p.color.hue_velocity_base = 0.013f;
  p.color.hue_velocity_breath = 0.009f;
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.18f;
  p.color.luma_shape_center = 2.8f;

  p.seed.border_width = 0.03f;
  p.seed.hue_speed = 0.18f;
  p.seed.alpha = 0.8f;
  p.speed = 0.85f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.113f;
  p.zn.zn0_anim_amp = 0.035f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.125f;
  p.zn.zn1_anim_amp = 0.055f;
  p.zn.zn1_anim_k = 6.0f;
  p.seed.pulse_freq = 0.117f;
  p.seed.pulse_amp = 0.15f;

  return p;
}

} // namespace epi::presets
