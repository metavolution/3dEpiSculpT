#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern spatial_wave_minimal_prism() {
  Pattern p;
  p.name = "spatial_wave_minimal_prism";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.006f;
  p.color.sat_scale = 1.10f;
  p.color.fade = 0.992f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.62f;
  p.color.spiral_radius_coeff = 0.18f;
  p.color.chroma_breath_amp = 0.12f;
  p.color.chroma_breath_freq = 0.31f;
  p.color.chroma_breath_phase = 0.09f;
  p.color.hue_velocity_base = 0.010f;
  p.color.hue_velocity_breath = 0.006f;
  p.color.hue_velocity_drift = 0.002f;
  p.color.luma_shape_depth = 0.24f;
  p.color.luma_shape_center = 1.2f;

  p.seed.border_width = 0.02f;
  p.seed.hue_speed = 0.08f;
  p.seed.alpha = 0.95f;
  p.speed = 0.5f;
  return p;
}

} // namespace epi::presets
