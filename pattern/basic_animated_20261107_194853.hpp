#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern basic_animated() {
  Pattern p;
  p.name = "basic_animated";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.06f, 0.08f};
  p.zn.zn[2] = {0.98f, 0.03f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.085f;
  p.zn.zn0_anim_amp = 0.050f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.062f;
  p.zn.zn1_anim_amp = 0.038f;
  p.zn.zn1_anim_k = 3.0f;

  p.color.hue_rotation = 0.018f;
  p.color.sat_scale = 1.02f;
  p.color.fade = 0.98f;

  p.seed.border_width = 0.1f;
  p.seed.hue_speed = 0.12f;
  p.seed.pulse_freq = 0.025f;
  p.seed.pulse_amp = 0.4f;

  return p;
}

} // namespace epi::presets
