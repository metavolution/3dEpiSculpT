#pragma once
#include <epi/pattern.hpp>
#include <epi/presets/color_mode.hpp>

namespace epi::presets {

inline Pattern spectral_prism() {
  Pattern p;
  p.name = "spectral_prism";
  p.inner = InnerFunction::F10;

  // Zn configuration: simple affine zoom to explore structure
  p.zn.zn[0] = {1.1914f, 0.7101f};
  p.zn.zn[1] = {0.0416f, -0.0254f};
  p.zn.zn[2] = {1.1375f, -0.4813f};
  p.zn.zn[3] = Complexf(0.0f, 0.0f);

  p.speed = 1.0f;

  // Spectral Prism Color Configuration
  p.color.color_mode = as_int(ColorMode::SpectralPrism);
  p.color.sat_scale = 1.84f;
  
  // Spatial coordinates for prism (creates spiraling gradients)
  p.color.spiral_angle_coeff = 2.0f;
  p.color.spiral_radius_coeff = 1.5f;
  
  // Speeds for R, G, B phase shifts
  p.color.hue_velocity_base = 0.00f;   // R moves
  p.color.hue_velocity_breath = 0.33f; // G moves back
  p.color.hue_velocity_drift = 0.67f;   // B moves slow
  p.seed.border_width = 0.091f;
  p.seed.alpha = 0.92f;
  p.seed.hue_speed = 0.088f;
  p.color.fade = 0.989f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.172f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.198f;
  p.zn.zn1_anim_amp = 0.048f;
  p.zn.zn1_anim_k = 8.0f;
  p.seed.pulse_freq = 0.136f;
  p.seed.pulse_amp = 0.15f;

  
  return p;
}

} // namespace epi::presets
