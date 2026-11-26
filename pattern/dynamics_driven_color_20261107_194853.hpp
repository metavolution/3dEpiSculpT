#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern dynamics_driven_color() {
  Pattern p;
  p.name = "dynamics_driven_color";
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.15f, 0.16f};
  p.zn.zn[1] = {-0.015f, 0.011f};
  p.zn.zn[2] = {0.98f, -0.07f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.19f;
  p.zn.zn0_anim_amp = 0.075f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.13f;
  p.zn.zn1_anim_amp = 0.018f;
  p.zn.zn1_anim_k = 4.0f;

  p.color.color_mode = as_int(ColorMode::DynamicsDriven);
  p.color.hue_rotation = 0.008f;
  p.color.hue_velocity_base = 0.018f;
  p.color.spiral_angle_coeff = 0.65f;  // u-dependence
  p.color.spiral_radius_coeff = 14.0f; // len = 0.01 * |t_z| sensitivity
  p.color.chroma_breath_amp = 0.35f;
  p.color.chroma_breath_freq = 75.0f;  // scales (len - luma_r0) across [-1, 1]
  p.color.luma_shape_center = 0.012f;  // midpoint of len = 0.01 * |t_z|
  p.color.sat_scale = 1.04f;
  p.color.fade = 0.986f;

  p.seed.border_width = 0.080f;
  p.seed.hue_speed = 0.09f;
  p.seed.alpha = 0.93f;
  p.speed = 0.80f;
  return p;
}

} // namespace epi::presets
