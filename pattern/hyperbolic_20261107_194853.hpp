#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern hyperbolic() {
  Pattern p;
  p.name = "hyperbolic";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {0.98f, 0.02f};
  p.zn.zn[2] = {0.5f, 0.0f};

  p.color.hue_rotation = 0.01f;
  p.color.fade = 0.97f;
  p.color.distance_modulate = true;

  p.seed.border_width = 0.2f;
  p.seed.alpha = 0.6f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.166f;
  p.zn.zn0_anim_amp = 0.044f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.147f;
  p.zn.zn1_anim_amp = 0.044f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.127f;
  p.seed.pulse_amp = 0.20f;


  return p;
}

} // namespace epi::presets
