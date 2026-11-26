#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern atmospheric_drift() {
  Pattern p;
  p.name = "atmospheric_drift";
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.12f, 0.14f};
  p.zn.zn[1] = {0.014f, -0.010f};
  p.zn.zn[2] = {0.98f, 0.04f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.15f;
  p.zn.zn0_anim_amp = 0.065f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.10f;
  p.zn.zn1_anim_amp = 0.016f;
  p.zn.zn1_anim_k = 3.0f;
  p.color.hue_rotation = 0.006f;
  p.color.sat_scale = 1.03f;
  p.color.fade = 0.987f;

  p.color.color_mode = as_int(ColorMode::AtmosphericDrift);
  p.color.spiral_angle_coeff = 0.42f;
  p.color.spiral_radius_coeff = 0.24f;
  p.color.chroma_breath_amp = 0.14f;
  p.color.chroma_breath_freq = 0.23f;
  p.color.chroma_breath_phase = 0.04f;
  p.color.hue_velocity_base = 0.016f;
  p.color.hue_velocity_breath = 0.008f;
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.border_width = 0.080f;
  p.seed.hue_speed = 0.08f;
  p.seed.alpha = 0.92f;
  p.speed = 0.78f;
  return p;
}

} // namespace epi::presets
