#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern glitch_symmetry_break() {
  Pattern p;
  p.name = "glitch_symmetry_break";
  p.inner = InnerFunction::Glitch;
  p.reduction = ReductionType::Torus;
  
  // Base transform: slight rotation and scaling
  p.zn.zn[0] = {1.2851f, 0.2074f};
  p.zn.zn[1] = {0.0372f, -0.0284f};
  p.zn.zn[2] = {1.1777f, -0.0620f};
  p.zn.zn[3] = {0.0f, 0.0f};
  
  // Use alive color system: Mode 3 (Alive - 3-velocity hue model)
  p.color.color_mode = presets::as_int(presets::ColorMode::Alive);
  p.color.hue_velocity_base = 0.03f;
  p.color.hue_velocity_breath = 0.02f;
  p.color.hue_velocity_drift = 0.01f;
  p.color.spiral_angle_coeff = 2.0f;
  p.color.spiral_radius_coeff = -1.0f;
  
  p.color.hue_rotation = 0.0f; // Handled by alive color
  p.color.sat_scale = 1.12f;   // Boost color a bit since it's glitchy
  p.color.fade = 0.986f;
  
  p.seed.border_width = 0.15f;
  p.seed.hue_speed = 0.2f;
  
  // Dynamic motion: slowly modulate the glitch parameter via pre-affine shift
  p.motion = control::PatternMotion::translation_wander(0.0f, 0.0f, 0.15f, 20.0f);
  p.seed.alpha = 0.92f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.132f;
  p.zn.zn0_anim_amp = 0.035f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.173f;
  p.zn.zn1_anim_amp = 0.049f;
  p.zn.zn1_anim_k = 8.0f;
  p.seed.pulse_freq = 0.113f;
  p.seed.pulse_amp = 0.18f;

  
  return p;
}

} // namespace epi::presets
