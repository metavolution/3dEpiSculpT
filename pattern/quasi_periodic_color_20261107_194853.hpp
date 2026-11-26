#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern quasi_periodic_color() {
  Pattern p;
  p.name = "quasi_periodic_color";

  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Torus;

  // Pentagonal (36 deg) pre-twist + silver-ratio counter-twist so toroidal
  // sine sheets weave aperiodic standing waves across the whole domain.
  p.zn.zn[0] = {1.116f, 0.811f};
  p.zn.zn[1] = {0.028f, -0.017f};
  p.zn.zn[2] = {0.965f, -0.445f};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.162f;
  p.zn.zn0_anim_amp = 0.048f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.262f;
  p.zn.zn1_anim_amp = 0.042f;
  p.zn.zn1_anim_k = 5.0f;

  // Quasi-Periodic Wave Color (phi radial + sqrt(2) angular interference)
  p.color.color_mode = as_int(ColorMode::QuasiPeriodicWave);
  p.color.hue_rotation = 0.012f;
  p.color.spiral_angle_coeff = 2.5f;
  p.color.spiral_radius_coeff = 0.85f;
  p.color.chroma_breath_amp = 0.28f;
  p.color.hue_velocity_base = 0.045f;
  p.color.hue_velocity_breath = 0.028f;
  p.color.hue_velocity_drift = 0.017f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.985f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.085f;
  p.seed.hue_speed = 0.06f;
  p.seed.alpha = 0.68f;
  p.speed = 0.82f;

  return p;
}

} // namespace epi::presets
