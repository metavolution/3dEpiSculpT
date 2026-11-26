#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern interference_evolved_v2() {
  Pattern p;
  p.name = "interference_evolved_v2";
  p.inner = InnerFunction::Moire;
  p.reduction = ReductionType::Torus;
  
  // The Moire transform requires scaling z up significantly to see the grid interference.
  // We use zn0 to scale the coordinate domain.
  p.zn.zn[0] = { 1.00f, 0.03f }; // Mutated
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.01f, 0.0f};  // Slight expansion
  p.zn.zn[3] = {0.0f, 0.0f};
  
  p.color.color_mode = presets::as_int(presets::ColorMode::MoireInterference); // Moire Interference color mode
  p.color.spiral_angle_coeff = 0.5f; // Hue jump
  p.color.spiral_radius_coeff = 25.0f; // High frequency spatial interference
  p.color.chroma_breath_freq = 15.0f; // Spatial wave frequency
  p.color.hue_velocity_base = 0.05f;   // Temporal phase shift
  p.color.chroma_breath_amp = 0.15f; // Moire hue displacement amplitude
  p.color.luma_shape_depth = 0.5f; // Moire lightness displacement amplitude  
  p.color.hue_rotation = 0.05f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.96f; // Faster fade to prevent mud
  
  p.seed.border_width = 0.1f;
  p.seed.hue_speed = 0.15f;
  p.seed.alpha = 1.0f;
  
  return p;
}

} // namespace epi::presets
