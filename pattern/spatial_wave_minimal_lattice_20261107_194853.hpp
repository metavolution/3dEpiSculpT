#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern spatial_wave_minimal_lattice() {
  Pattern p;
  p.name = "spatial_wave_minimal_lattice";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {0.995f, 0.005f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.010f;
  p.color.sat_scale = 1.11f;
  p.color.fade = 0.989f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.78f;
  p.color.spiral_radius_coeff = 0.21f;
  p.color.chroma_breath_amp = 0.11f;
  p.color.chroma_breath_freq = 0.36f;
  p.color.chroma_breath_phase = 0.07f;
  p.color.hue_velocity_base = 0.011f;
  p.color.hue_velocity_breath = 0.006f;
  p.color.hue_velocity_drift = 0.002f;
  p.color.luma_shape_depth = 0.26f;
  p.color.luma_shape_center = 1.5f;

  p.seed.border_width = 0.018f;
  p.seed.hue_speed = 0.12f;
  p.seed.alpha = 0.9f;
  p.speed = 0.62f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.161f;
  p.zn.zn0_anim_amp = 0.047f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.182f;
  p.zn.zn1_anim_amp = 0.052f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.137f;
  p.seed.pulse_amp = 0.15f;

  return p;
}

} // namespace epi::presets
