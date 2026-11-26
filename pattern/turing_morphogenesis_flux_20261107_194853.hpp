#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Turing-inspired morphogenesis preset.
 *
 * Practical model mapping:
 * - `tanh` inner acts as a saturating nonlinearity (reaction term analogue).
 * - `grid` reduction creates a discrete repeating lattice (diffusion lattice analogue).
 * - feedback sampling carries temporal memory (activator/inhibitor trail analogue).
 * - alive color mode exposes phase-space motion rather than static coloring.
 *
 * This stays fully within the existing OpenCL canonical pipeline:
 * transform -> reduce -> sample -> color -> seed -> blend.
 */
inline Pattern turing_morphogenesis_flux() {
  Pattern p;
  p.name = "turing_morphogenesis_flux";
  p.inner = InnerFunction::Tanh;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {1.4764f, 0.3010f};
  p.zn.zn[1] = {0.2047f, -0.1864f};
  p.zn.zn[2] = {1.2178f, -0.4652f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.009f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.985f;
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 0.66f;
  p.color.spiral_radius_coeff = 0.42f;
  p.color.chroma_breath_amp = 0.16f;
  p.color.chroma_breath_freq = 0.11f;
  p.color.chroma_breath_phase = 0.38f;
  p.color.hue_velocity_base = 0.015f;
  p.color.hue_velocity_breath = 0.020f;
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.19f;
  p.color.luma_shape_center = 1.7f;

  p.seed.border_width = 0.078f;
  p.seed.hue_speed = 0.07f;
  p.seed.alpha = 0.92f;
  p.speed = 0.72f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.160f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.157f;
  p.zn.zn1_anim_amp = 0.043f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.111f;
  p.seed.pulse_amp = 0.15f;

  return p;
}

} // namespace epi::presets
