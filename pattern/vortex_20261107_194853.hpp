#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern vortex() {
  Pattern p;
  p.name = "vortex";
  p.inner = InnerFunction::Exp;
  p.reduction = ReductionType::Grid;

  p.zn.zn[0] = {0.92f, 0.08f};
  p.zn.zn[1] = {0.0f, 0.1f};
  p.zn.zn[2] = {1.0f, 0.0f};

  p.color.hue_rotation = 0.025f;
  p.color.fade = 0.95f;

  p.seed.border_width = 0.12f;
  p.seed.alpha = 0.8f;

  p.speed = 1.5f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.172f;
  p.zn.zn0_anim_amp = 0.038f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.183f;
  p.zn.zn1_anim_amp = 0.055f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.125f;
  p.seed.pulse_amp = 0.16f;

  return p;
}

} // namespace epi::presets
