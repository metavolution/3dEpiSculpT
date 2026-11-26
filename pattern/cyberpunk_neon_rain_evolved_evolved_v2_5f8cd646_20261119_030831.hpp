#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern cyberpunk_neon_rain_evolved_evolved_v2() {
  Pattern p;
  p.name = "cyberpunk_neon_rain_evolved_evolved_v2";
  
  // High-frequency periodic inner function
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Torus;

  // Scale and translate: squish vertically to create horizontal bands
  // that fall downwards like rain or glowing data streams.
  p.zn.zn[0] = { 0.86f, -0.05f }; // Mutated
  p.zn.zn[1] = {0.0f, -0.2f}; // Constant downward translate
  
  p.zn.zn[2] = {1.2f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  // Color Mode 8: Op-Art Isoluminant Vibration
  // Creates highly saturated, neon-like bands of color
  p.color.color_mode = presets::as_int(presets::ColorMode::IsoluminantVibrate);
  p.color.sat_scale = 1.3f;
  p.color.fade = 0.96f;
  p.color.hue_rotation = 0.02f;
  
  p.color.spiral_angle_coeff = 0.8f;     // Sharp hue jumps
  p.color.spiral_radius_coeff = 3.0f;    // High frequency
  p.color.chroma_breath_freq = 5.0f;     // Rapid vibration
  p.color.hue_velocity_base = 0.5f;      // Downward color flow

  // Seed injection
  p.seed.type = SeedType::Simplex; // Procedural border seed
  p.seed.border_width = 0.02f;
  p.seed.hue_speed = 0.1f;

  // Add motion: dynamic falling speed and pulse
  p.motion.add("rain_speed",
      control::ModLane{.wave         = control::Waveform::Sine,
                       .frequency_hz = 0.2f,
                       .amplitude    = 0.05f,
                       .offset       = -0.2f},
      control::ModTarget::Zn1Im);
      
  p.motion.add("neon_pulse",
      control::ModLane{.wave         = control::Waveform::Triangle,
                       .frequency_hz = 0.8f,
                       .amplitude    = 0.3f,
                       .offset       = 1.3f},
      control::ModTarget::SatScale);
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.176f;
  p.zn.zn0_anim_amp = 0.043f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.133f;
  p.zn.zn1_anim_amp = 0.044f;
  p.zn.zn1_anim_k = 8.0f;
  p.seed.pulse_freq = 0.075f;
  p.seed.pulse_amp = 0.15f;


  return p;
}

} // namespace epi::presets
