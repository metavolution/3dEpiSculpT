#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern magnetic_dipole_resonance() {
  Pattern p;
  p.name = "magnetic_dipole_resonance";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::Torus;
  
  // A gentle zoom and rotation
  p.zn.zn[0] = {1.0f, 0.05f};
  p.zn.zn[2] = {0.99f, 0.02f};
  
  // Use mode 14 (QuasiPeriodicWave) for smooth interference
  p.color.color_mode = presets::as_int(presets::ColorMode::QuasiPeriodicWave); 
  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.05f;
  p.color.fade = 0.98f;
  p.color.spiral_angle_coeff = 1.2f;
  p.color.spiral_radius_coeff = 0.8f;
  p.color.chroma_breath_amp = 0.3f;
  p.color.hue_velocity_base = 0.02f;
  p.color.hue_velocity_breath = 0.015f;
  p.color.hue_velocity_drift = 0.005f;

  p.seed.border_width = 0.15f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.7f;
  p.speed = 1.0f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.182f;
  p.zn.zn0_anim_amp = 0.037f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.191f;
  p.zn.zn1_anim_amp = 0.043f;
  p.zn.zn1_anim_k = 2.0f;
  p.seed.pulse_freq = 0.131f;
  p.seed.pulse_amp = 0.18f;

  return p;
}

} // namespace epi::presets
