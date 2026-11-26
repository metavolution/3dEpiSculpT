#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern interference() {
  Pattern p;
  p.name = "interference";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {0.98f, 0.01f};
  p.zn.zn[2] = {1.0f, 0.01f};

  p.color.hue_rotation = 0.008f;
  p.color.sat_scale = 1.01f;
  p.color.fade = 0.99f;
  p.color.distance_modulate = true;

  p.seed.border_width = 0.15f;
  p.seed.alpha = 0.7f;
  p.seed.pulse_amp = 0.5f;
  p.seed.pulse_freq = 0.03f;

  p.speed = 0.8f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.165f;
  p.zn.zn0_anim_amp = 0.050f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.136f;
  p.zn.zn1_anim_amp = 0.047f;
  p.zn.zn1_anim_k = 5.0f;

  return p;
}

} // namespace epi::presets
