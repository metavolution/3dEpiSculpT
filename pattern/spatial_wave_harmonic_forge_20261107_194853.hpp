#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern spatial_wave_harmonic_forge() {
  Pattern p;
  p.name = "spatial_wave_harmonic_forge";
  p.inner = InnerFunction::Exp;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {0.89f, 0.11f};
  p.zn.zn[1] = {0.02f, -0.01f};
  p.zn.zn[2] = {0.96f, 0.08f};
  p.zn.zn[3] = {0.0f, 1.0472f};

  p.color.hue_rotation = 0.017f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.972f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.76f;
  p.color.spiral_radius_coeff = 0.83f;
  p.color.chroma_breath_amp = 0.17f;
  p.color.chroma_breath_freq = 1.13f;
  p.color.chroma_breath_phase = 0.14f;
  p.color.hue_velocity_base = 0.025f;
  p.color.hue_velocity_breath = 0.018f;
  p.color.hue_velocity_drift = 0.010f;
  p.color.luma_shape_depth = 0.30f;
  p.color.luma_shape_center = 4.2f;

  p.seed.border_width = 0.05f;
  p.seed.hue_speed = 0.22f;
  p.seed.alpha = 1.0f;
  p.speed = 1.25f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.143f;
  p.zn.zn0_anim_amp = 0.049f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.193f;
  p.zn.zn1_anim_amp = 0.040f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.101f;
  p.seed.pulse_amp = 0.19f;

  return p;
}

} // namespace epi::presets
