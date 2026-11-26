#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern quasi_periodic_wave() {
  Pattern p;
  p.name = "quasi_periodic_wave";
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.3239f, 0.2136f};
  p.zn.zn[2] = {1.1530f, -0.1625f};
  p.color.hue_rotation = 0.003f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.986f;

  p.color.color_mode = as_int(ColorMode::QuasiPeriodicWave);
  p.color.spiral_angle_coeff = 0.15f;
  p.color.spiral_radius_coeff = 0.25f;
  p.color.chroma_breath_amp = 0.10f;
  p.color.hue_velocity_base = 0.011f;
  p.color.hue_velocity_breath = 0.007f;
  p.color.hue_velocity_drift = 0.005f;

  p.seed.border_width = 0.085f;
  p.seed.hue_speed = 0.121f;
  p.seed.alpha = 0.92f;
  p.speed = 0.6f;
  p.zn.zn[1] = {0.0396f, -0.0432f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.162f;
  p.zn.zn0_anim_amp = 0.037f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.208f;
  p.zn.zn1_anim_amp = 0.044f;
  p.zn.zn1_anim_k = 8.0f;
  p.seed.pulse_freq = 0.094f;
  p.seed.pulse_amp = 0.17f;

  return p;
}

} // namespace epi::presets
