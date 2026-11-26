#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern spatial_wave_quantum_tide() {
  Pattern p;
  p.name = "spatial_wave_quantum_tide";
  p.inner = InnerFunction::Cosh;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {0.84f, 0.16f};
  p.zn.zn[1] = {-0.03f, 0.02f};
  p.zn.zn[2] = {0.93f, -0.11f};
  p.zn.zn[3] = {0.0f, 0.72f};

  p.color.hue_rotation = 0.019f;
  p.color.sat_scale = 1.14f;
  p.color.fade = 0.971f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.97f;
  p.color.spiral_radius_coeff = 1.19f;
  p.color.chroma_breath_amp = 0.20f;
  p.color.chroma_breath_freq = 1.43f;
  p.color.chroma_breath_phase = 0.18f;
  p.color.hue_velocity_base = 0.032f;
  p.color.hue_velocity_breath = 0.022f;
  p.color.hue_velocity_drift = 0.012f;
  p.color.luma_shape_depth = 0.34f;
  p.color.luma_shape_center = 5.0f;

  p.seed.border_width = 0.045f;
  p.seed.hue_speed = 0.27f;
  p.seed.alpha = 1.0f;
  p.speed = 1.45f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.133f;
  p.zn.zn0_anim_amp = 0.040f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.210f;
  p.zn.zn1_anim_amp = 0.053f;
  p.zn.zn1_anim_k = 2.0f;
  p.seed.pulse_freq = 0.139f;
  p.seed.pulse_amp = 0.17f;

  return p;
}

} // namespace epi::presets
