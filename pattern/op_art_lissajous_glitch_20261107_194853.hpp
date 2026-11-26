#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern op_art_lissajous_glitch() {
  Pattern p;
  p.name = "op_art_lissajous_glitch";
  p.inner = InnerFunction::LissajousGlitch;
  p.reduction = ReductionType::Torus;
  
  // A slight zoom and rotation to drive the lissajous evolution
  p.zn.zn[0] = {1.0f, 0.02f};
  p.zn.zn[2] = {0.98f, 0.01f};
  
  // Use mode 8 (Isoluminant Vibration) for an intense op-art effect
  p.color.color_mode = presets::as_int(presets::ColorMode::IsoluminantVibrate); 
  p.color.hue_rotation = 0.01f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.99f;
  p.color.spiral_angle_coeff = 0.5f;
  p.color.spiral_radius_coeff = 2.0f;
  p.color.chroma_breath_freq = 5.0f;
  p.color.hue_velocity_base = 0.05f;

  p.seed.border_width = 0.1f;
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 0.8f;
  p.speed = 1.0f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.170f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.153f;
  p.zn.zn1_anim_amp = 0.042f;
  p.zn.zn1_anim_k = 5.0f;
  p.seed.pulse_freq = 0.127f;
  p.seed.pulse_amp = 0.19f;

  return p;
}

} // namespace epi::presets
