#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern spatial_wave_advanced_quasiphase_matrix() {
  constexpr float PRE = 0.91f;
  constexpr float POST = 0.87f;
  constexpr float C7 = 0.6234898019f;  // cos(2*pi/7)
  constexpr float S7 = 0.7818314825f;  // sin(2*pi/7)
  constexpr float CQ = -0.8582161857f; // cos(2*pi*(sqrt(2)-1))
  constexpr float SQ = 0.5132883972f;  // sin(2*pi*(sqrt(2)-1))

  Pattern p;
  p.name = "spatial_wave_advanced_quasiphase_matrix";
  p.inner = InnerFunction::Cosh;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {C7 * PRE, S7 * PRE};
  p.zn.zn[1] = {0.014f, -0.011f};
  p.zn.zn[2] = {CQ * POST, SQ * POST};
  p.zn.zn[3] = {0.0f, 0.377f};

  p.color.hue_rotation = 0.016f;
  p.color.sat_scale = 1.15f;
  p.color.fade = 0.974f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.91f;
  p.color.spiral_radius_coeff = 1.272f;
  p.color.chroma_breath_amp = 0.18f;
  p.color.chroma_breath_freq = 1.414f;
  p.color.chroma_breath_phase = 0.13f;
  p.color.hue_velocity_base = 0.024f;
  p.color.hue_velocity_breath = 0.017f;
  p.color.hue_velocity_drift = 0.009f;
  p.color.luma_shape_depth = 0.31f;
  p.color.luma_shape_center = 4.6f;

  p.seed.border_width = 0.034f;
  p.seed.hue_speed = 0.19f;
  p.seed.alpha = 0.97f;
  p.speed = 1.08f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.168f;
  p.zn.zn0_anim_amp = 0.046f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.173f;
  p.zn.zn1_anim_amp = 0.036f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.082f;
  p.seed.pulse_amp = 0.19f;

  return p;
}

} // namespace epi::presets
