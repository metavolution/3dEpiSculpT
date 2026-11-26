#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern palette_morph_journey() {
  Pattern p;
  p.name = "palette_morph_journey";
  
  // Use a balanced flow inner function to see the palette journey over a nice topology
  p.inner = InnerFunction::Cos;
  p.reduction = ReductionType::Grid;
  
  // Create beautiful Op-Art geometries using Cos + Grid
  p.zn.zn[0] = {1.3814f, 0.2229f}; 
  p.zn.zn[2] = {1.2353f, 0.0686f}; // Slight spiral inward
  
  p.color.color_mode = presets::as_int(presets::ColorMode::PaletteMorph); // Palette-Morph Mode
  
  // Morph parameters
  p.color.spiral_angle_coeff = 1.0f; 
  p.color.spiral_radius_coeff = 0.5f; 
  
  // Speed of morphing
  p.color.hue_rotation = 0.0f; // Base offset
  p.color.hue_velocity_base = 0.01f;     // Drift through palette
  p.color.hue_velocity_breath = 0.0f;    
  p.color.hue_velocity_drift = 0.0f;     
  
  // Breath morphing envelope (swaps between warm and cool palettes)
  p.color.chroma_breath_amp = 1.0f;  
  p.color.chroma_breath_freq = 0.05f; 
  p.color.chroma_breath_phase = 1.0f; 
  
  p.color.sat_scale = 1.86f;
  p.color.fade = 0.989f; // Nice long trails
  
  p.seed.border_width = 0.080f;
  p.seed.alpha = 0.92f;
  p.speed = 0.6f;
  
  // Adding some slow translational drift to reveal the palette over space
  p.motion = epi::control::PatternMotion::slow_color_drift(0.005f, 0.01f, 30.0f);
  p.motion.add("wander_re",
               epi::control::ModLane{.wave         = epi::control::Waveform::NoiseHash,
                                     .frequency_hz = 0.1f,
                                     .amplitude    = 0.02f,
                                     .offset       = 0.0f},
               epi::control::ModTarget::Zn1Re);
  p.zn.zn[1] = {0.0344f, -0.0351f};
  p.seed.hue_speed = 0.090f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.118f;
  p.zn.zn0_anim_amp = 0.040f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.205f;
  p.zn.zn1_anim_amp = 0.042f;
  p.zn.zn1_anim_k = 6.0f;
  p.seed.pulse_freq = 0.104f;
  p.seed.pulse_amp = 0.16f;


  return p;
}

} // namespace epi::presets
