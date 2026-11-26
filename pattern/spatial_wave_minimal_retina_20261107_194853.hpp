#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern spatial_wave_minimal_retina() {
  Pattern p;
  p.name = "spatial_wave_minimal_retina";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {0.992f, 0.012f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.011f;
  p.color.sat_scale = 1.18f;
  p.color.fade = 0.989f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.48f;
  p.color.spiral_radius_coeff = 0.58f;
  p.color.chroma_breath_amp = 0.09f;
  p.color.chroma_breath_freq = 0.89f;
  p.color.chroma_breath_phase = 0.04f;
  p.color.hue_velocity_base = 0.012f;
  p.color.hue_velocity_breath = 0.007f;
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.20f;
  p.color.luma_shape_center = 3.1f;

  p.seed.border_width = 0.02f;
  p.seed.hue_speed = 0.15f;
  p.seed.alpha = 0.82f;
  p.speed = 0.78f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.134f;
  p.zn.zn0_anim_amp = 0.050f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.164f;
  p.zn.zn1_anim_amp = 0.039f;
  p.zn.zn1_anim_k = 8.0f;
  p.seed.pulse_freq = 0.118f;
  p.seed.pulse_amp = 0.14f;

  return p;
}

} // namespace epi::presets
