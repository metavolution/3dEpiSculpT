#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern golden_ratio() {
  constexpr float phi = 1.6180339887f;
  constexpr float inv_phi = 0.6180339887f;
  constexpr float inv_phi2 = 0.3819660113f;

  constexpr float scale = 0.96f;
  constexpr float c5 = 0.30901699f * scale;
  constexpr float s5 = 0.95105652f * scale;

  Pattern p;
  p.name = "golden_ratio";
  p.inner = InnerFunction::Cosh;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {c5, s5};
  p.zn.zn[2] = {0.99f, 0.0f};
  p.color.hue_rotation = 0.01f * inv_phi;
  p.color.sat_scale = 1.008f;
  p.color.fade = 0.986f;

  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.25f;
  p.color.spiral_radius_coeff = inv_phi;
  p.color.chroma_breath_amp = 0.09f;
  p.color.chroma_breath_freq = inv_phi2;
  p.color.chroma_breath_phase = 0.065f;
  p.color.hue_velocity_base = 0.01f * inv_phi;
  p.color.hue_velocity_breath = 0.01f * inv_phi2;
  p.color.hue_velocity_drift = 0.003f * inv_phi;
  p.color.luma_shape_depth = 0.20f;
  p.color.luma_shape_center = phi;

  p.seed.border_width = 0.09f;
  p.seed.hue_speed = 0.05f * phi;
  p.seed.alpha = 0.9f;
  p.speed = 0.55f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.114f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.179f;
  p.zn.zn1_anim_amp = 0.042f;
  p.zn.zn1_anim_k = 2.0f;
  p.seed.pulse_freq = 0.079f;
  p.seed.pulse_amp = 0.20f;

  return p;
}

} // namespace epi::presets
