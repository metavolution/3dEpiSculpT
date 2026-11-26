#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern op_art_symmetry_glitch() {
  Pattern p;
  p.name = "op_art_symmetry_glitch";
  p.inner = InnerFunction::SymmetryBreak;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.01f, 0.05f};
  p.zn.zn[2] = {0.99f, -0.05f};
  
  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.hue_rotation = 0.01f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.98f;

  p.seed.border_width = 0.05f;
  p.seed.hue_speed = 0.1f;
  p.seed.alpha = 0.9f;
  p.speed = 0.8f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.103f;
  p.zn.zn0_anim_amp = 0.046f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.126f;
  p.zn.zn1_anim_amp = 0.056f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.114f;
  p.seed.pulse_amp = 0.19f;

  return p;
}

} // namespace epi::presets
