#pragma once
#include <epi/pattern.hpp>

namespace epi::presets {

/**
 * BIOLUMINESCENT PULSE
 * 
 * Deep-sea / biological vibe using the BioluminescentPulse color mode (18).
 * High-complexity structural areas glow with intense cyan/green light
 * against a dark background, mimicking bio-luminescence.
 */
inline Pattern bioluminescent_pulse() {
  Pattern p;
  p.name = "bioluminescent_pulse";
  p.inner = InnerFunction::Square;

  p.zn.zn[0] = {0.95f, 0.0f};  // Near-critical fractal stability
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.85f, 0.05f}; // Gentle contraction + twist
  p.zn.zn[3] = {0.0f, 0.0f};

  p.reduction = ReductionType::Grid;

  // BioluminescentPulse color mode
  p.color.color_mode = presets::as_int(presets::ColorMode::BioluminescentPulse); 
  p.color.hue_rotation = 0.0f; 
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.98f;

  p.seed.type = SeedType::OrganicNoise; // Organic Noise (Pareidolia-like)
  p.seed.alpha = 0.05f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.099f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.137f;
  p.zn.zn1_anim_amp = 0.053f;
  p.zn.zn1_anim_k = 5.0f;
  p.seed.pulse_freq = 0.116f;
  p.seed.pulse_amp = 0.17f;


  return p;
}

} // namespace epi::presets
