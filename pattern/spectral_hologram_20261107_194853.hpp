#pragma once

#include "epi/pattern.hpp"
#include "epi/presets/color_mode.hpp"

namespace epi::presets {

inline Pattern spectral_hologram() {
  Pattern p;
  p.name = "spectral_hologram";
  
  // A complex composite inner function from the advanced registry
  // f10: "vec2(0.9) * (cmul(z, ctanh(z)) + ccos(cexp(z)))" 
  // or f14: "z + i·cos(i·sin(iz)): Deep nested trig-hyperbolic"
  p.inner = InnerFunction::F14;
  p.reduction = ReductionType::Torus;

  // Zn configuration: simple affine zoom to explore structure
  p.zn.zn[0] = Complexf(1.05f, 0.05f);
  p.zn.zn[1] = Complexf(-0.1f, 0.1f);
  p.zn.zn[2] = Complexf(0.9f, 0.0f);
  p.zn.zn[3] = Complexf(0.0f, 0.0f);

  p.speed = 1.0f;

  // Spectral Prism Color Configuration
  p.color.color_mode = as_int(ColorMode::SpectralPrism);
  p.color.sat_scale = 1.4f;
  p.color.fade = 0.94f;
  
  // Spatial coordinates for prism (creates spiraling gradients)
  p.color.spiral_angle_coeff = 3.0f;
  p.color.spiral_radius_coeff = 2.0f;
  
  // Speeds for R, G, B phase shifts
  p.color.hue_velocity_base = 0.06f;   // R moves
  p.color.hue_velocity_breath = -0.04f; // G moves back
  p.color.hue_velocity_drift = 0.03f;   // B moves slow
  
  // Breathing
  p.color.chroma_breath_amp = 0.4f;
  p.color.chroma_breath_freq = 0.5f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.155f;
  p.zn.zn0_anim_amp = 0.036f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.135f;
  p.zn.zn1_anim_amp = 0.042f;
  p.zn.zn1_anim_k = 5.0f;
  p.seed.pulse_freq = 0.132f;
  p.seed.pulse_amp = 0.20f;


  return p;
}

} // namespace epi::presets
