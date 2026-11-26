#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern tron() {
  Pattern p;
  p.name = "tron";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.12f, 0.14f};
  p.zn.zn[1] = {0.02f, -0.015f};
  p.zn.zn[2] = {0.96f, 0.04f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.090f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.065f;
  p.zn.zn1_anim_amp = 0.030f;
  p.zn.zn1_anim_k = 4.0f;

  p.color.hue_rotation = 0.03f;
  p.color.sat_scale = 1.25f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.72f;
  p.color.spiral_radius_coeff = 1.20f;
  p.color.chroma_breath_amp = 0.22f;
  p.color.chroma_breath_freq = 2.10f;
  p.color.chroma_breath_phase = 0.16f;
  p.color.hue_velocity_base = 0.024f;
  p.color.hue_velocity_breath = 0.014f;
  p.color.hue_velocity_drift = 0.006f;

  p.seed.border_width = 0.09f;
  p.seed.saturation = 1.0f;
  p.seed.alpha = 1.0f;
  p.seed.hue_speed = 0.2f;

  p.speed = 1.2f;
  return p;
}

} // namespace epi::presets
