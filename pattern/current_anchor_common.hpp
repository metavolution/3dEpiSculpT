#pragma once

#include "base.hpp"

namespace epi::presets {

// Freeze the canonical "current" contract values explicitly.
// Do not rely on Pattern default member initializers here: contract visuals
// must remain stable even if Pattern defaults evolve in the future.
inline void apply_frozen_current_contract_core(Pattern &p) {
  // Transform
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {0.97f, 0.03f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  // Color
  p.color.hue_rotation = 0.02f;
  p.color.sat_scale = 1.02f;
  p.color.fade = 0.98f;
  p.color.use_hsls = false;
  p.color.distance_modulate = true;
  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.spiral_angle_coeff = 1.0f;
  p.color.spiral_radius_coeff = 0.5f;
  p.color.chroma_breath_amp = 0.15f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.02f;
  p.color.hue_velocity_breath = 0.01f;
  p.color.hue_velocity_drift = 0.002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;

  // Seed
  p.seed.type = SeedType::Simplex;
  p.seed.image_path.clear();
  p.seed.border_width = 0.1f;
  p.seed.hue = 0.0f;
  p.seed.saturation = 1.0f;
  p.seed.alpha = 1.0f;
  p.seed.smooth_mask = 50.0f;
  p.seed.hue_speed = 0.1f;
  p.seed.pulse_freq = 0.02f;
  p.seed.pulse_amp = 0.3f;

  // Runtime-affecting fields carried in Pattern
  p.speed = 1.0f;
  p.learn_mode = false;
  p.stage_mask = 0x7FFFFFFF;
}

} // namespace epi::presets
