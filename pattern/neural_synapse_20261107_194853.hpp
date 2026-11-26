#pragma once

#include "epi/pattern.hpp"
#include "epi/presets/color_mode.hpp"

namespace epi::presets {

inline Pattern neural_synapse() {
  Pattern p;
  p.name = "neural_synapse";
  
  // Base fractal form that branches out
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Torus;
  
  // Transform
  p.zn.zn[0] = Complexf(0.5f, -0.8f);
  p.zn.zn[1] = Complexf(0.0f, 0.0f);
  p.zn.zn[2] = Complexf(1.01f, 0.0f); // slight expansion
  p.zn.zn[3] = Complexf(-0.1f, -0.1f);
  
  // Color configuration using NeuralSynapse mode
  p.color.color_mode = as_int(ColorMode::NeuralSynapse);
  p.color.hue_rotation = 0.65f; // Blueish baseline tissue
  p.color.hue_velocity_base = 0.1f; // Speed of signals
  
  p.color.spiral_angle_coeff = 0.2f;
  p.color.spiral_radius_coeff = 2.0f; // Spatial frequency of sparks
  
  p.color.chroma_breath_amp = 1.0f; // Saturation burst on spark
  p.color.chroma_breath_freq = 0.5f; // Firing rate
  
  p.color.luma_shape_depth = 0.5f; // Brightness pop on spark
  p.color.sat_scale = 0.6f; // Muted base tissue
  p.color.fade = 0.96f; // Long trails for branches
  
  p.seed = SeedConfig{
      .type = SeedType::Simplex,
      .image_path = "",
      .hue = 0.65f,
      .saturation = 0.0f,
      .alpha = 0.1f
  };
  
  p.speed = 0.8f; // Evolve at a deliberate pace
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.125f;
  p.zn.zn0_anim_amp = 0.044f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.145f;
  p.zn.zn1_anim_amp = 0.041f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.128f;
  p.seed.pulse_amp = 0.17f;

  
  return p;
}

} // namespace epi::presets
