#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * PAREIDOLIA ORGANIC VISION
 * 
 * Pareidolie erzwingen (Vom V1 zum IT-Cortex):
 * Uses an organic texture (flesh/clouds/liquid) as a base seed instead of
 * geometric lines. Overlaid with poly-hyperbolic recursion, it produces 
 * mysterious evolving shapes that trigger the brain's face/object recognition
 * systems without ever resolving into concrete objects.
 */
inline Pattern pareidolia_organic_vision() {
  Pattern p;
  p.name = "pareidolia_organic_vision";
  p.inner = InnerFunction::F8; // poly-hyperbolic mix for organic branching
  p.reduction = ReductionType::Torus;
  
  // Moderate scaling and rotation to create layered structures
  p.zn.zn[0] = {1.01f, 0.04f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.97f, -0.02f};
  p.zn.zn[3] = {0.0f, 0.0f};
  
  p.color.color_mode = presets::as_int(presets::ColorMode::Alive); // Alive color mode
  p.color.hue_velocity_base = 0.005f;
  p.color.hue_velocity_breath = 0.002f;
  p.color.hue_velocity_drift = 0.001f;
  p.color.spiral_angle_coeff = 0.3f;
  
  p.color.hue_rotation = 0.0f; 
  p.color.sat_scale = 0.98f;     // Keep saturation slightly constrained for organic feel
  p.color.fade = 0.97f;          // Longer memory for deeper structures
  
  p.seed.type = SeedType::OrganicNoise;
  p.seed.border_width = 1.0f; // Full frame injection
  p.seed.alpha = 0.5f;        // Subtle injection, let the feedback do the work
  p.seed.hue_speed = 0.05f;
  
  // Slow gentle motion to let the brain 'search'
  p.motion = control::PatternMotion::translation_wander(0.0f, 0.0f, 0.3f, 40.0f);
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.173f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.163f;
  p.zn.zn1_anim_amp = 0.047f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.120f;
  p.seed.pulse_amp = 0.14f;

  
  return p;
}

} // namespace epi::presets
