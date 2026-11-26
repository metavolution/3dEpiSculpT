#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern luma_locked_hue_drift() {
  Pattern p;
  p.name = "luma_locked_hue_drift";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.12f, 0.16f};
  p.zn.zn[1] = {0.015f, -0.012f};
  p.zn.zn[2] = {0.98f, 0.07f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.18f;
  p.zn.zn0_anim_amp = 0.07f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.12f;
  p.zn.zn1_anim_amp = 0.018f;
  p.zn.zn1_anim_k = 3.0f;

  p.color.color_mode = as_int(ColorMode::LumaLockedHueDrift);
  p.color.hue_rotation = 0.018f;
  p.color.hue_velocity_base = 0.045f;
  p.color.spiral_angle_coeff = 1.8f;
  p.color.spiral_radius_coeff = 0.9f;
  p.color.sat_scale = 0.92f;
  p.color.fade = 0.988f;

  p.seed.border_width = 0.075f;
  p.seed.hue_speed = 0.12f;
  p.seed.alpha = 0.94f;
  p.speed = 0.90f;
  return p;
}

} // namespace epi::presets
