#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern spatial_wave_minimal_duality() {
  Pattern p;
  p.name = "spatial_wave_minimal_duality";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.008f;
  p.color.sat_scale = 1.16f;
  p.color.fade = 0.993f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.68f;
  p.color.spiral_radius_coeff = 0.24f;
  p.color.chroma_breath_amp = 0.10f;
  p.color.chroma_breath_freq = 0.41f;
  p.color.chroma_breath_phase = 0.05f;
  p.color.hue_velocity_base = 0.009f;
  p.color.hue_velocity_breath = 0.005f;
  p.color.hue_velocity_drift = 0.0018f;
  p.color.luma_shape_depth = 0.22f;
  p.color.luma_shape_center = 1.4f;

  p.seed.border_width = 0.015f;
  p.seed.hue_speed = 0.09f;
  p.seed.alpha = 0.88f;
  p.speed = 0.46f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.101f;
  p.zn.zn0_anim_amp = 0.037f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.176f;
  p.zn.zn1_anim_amp = 0.047f;
  p.zn.zn1_anim_k = 8.0f;
  p.seed.pulse_freq = 0.113f;
  p.seed.pulse_amp = 0.18f;

  return p;
}

} // namespace epi::presets
