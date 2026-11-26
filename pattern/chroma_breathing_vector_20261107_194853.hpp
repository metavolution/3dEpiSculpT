#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern chroma_breathing_vector() {
  Pattern p;
  p.name = "chroma_breathing_vector";
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Torus;

  // Hyperbolic rotational vector flow with 3-lobe trajectory breathing.
  p.zn.zn[0] = {1.240f, 0.380f};
  p.zn.zn[1] = {0.036f, -0.028f};
  p.zn.zn[2] = {0.960f, 0.320f};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.22f;
  p.zn.zn0_anim_amp = 0.054f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.31f;
  p.zn.zn1_anim_amp = 0.048f;
  p.zn.zn1_anim_k = 3.0f;

  // Geometric-mean-neutral saturation envelope: sat_scale * sqrt(1 - A^2) ~ 1.0
  // so 3-lobe breathing alternates pastel exhale and vivid inhale without washout.
  p.color.color_mode = as_int(ColorMode::ChromaBreathing);
  p.color.hue_rotation = 0.012f;
  p.color.spiral_angle_coeff = 2.00f;
  p.color.spiral_radius_coeff = 0.48f;
  p.color.chroma_breath_amp = 0.22f;
  p.color.chroma_breath_freq = 0.24f;
  p.color.chroma_breath_phase = 3.0f;
  p.color.sat_scale = 1.024f;
  p.color.fade = 0.985f;

  p.seed.border_width = 0.092f;
  p.seed.hue_speed = 0.085f;
  p.seed.alpha = 0.90f;
  p.speed = 0.92f;
  return p;
}

} // namespace epi::presets
