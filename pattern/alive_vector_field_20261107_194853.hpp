#pragma once

#include "../../pattern.hpp"

namespace epi::presets {

inline Pattern alive_vector_field() {
  Pattern p;
  p.name = "alive_vector_field";
  
  // A swirling structure to show off vector-field dynamics
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Grid;
  p.display_post_mode = "palette3d";
  
  p.zn.zn[0] = {1.01f, 0.05f};
  p.zn.zn[2] = {0.95f, 0.05f}; 
  
  // Luma-locked hue drift
  p.color.color_mode = presets::as_int(presets::ColorMode::LumaLockedHueDrift); 
  p.color.spiral_angle_coeff = 2.0f; 
  p.color.spiral_radius_coeff = 1.0f;
  
  p.color.hue_rotation = 0.0f;
  p.color.hue_velocity_base = 0.05f; 
  
  p.color.sat_scale = 1.05f;
  p.color.fade = 0.98f;
  
  p.seed.border_width = 0.1f;
  p.seed.alpha = 0.8f;
  p.speed = 1.0f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.118f;
  p.zn.zn0_anim_amp = 0.051f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.215f;
  p.zn.zn1_anim_amp = 0.053f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.133f;
  p.seed.pulse_amp = 0.15f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.078f,
                            .amplitude = 0.280f,
                            .offset = 1.000f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.121f,
                            .amplitude = 0.103f,
                            .offset = 0.180f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.055f,
                            .amplitude = 0.032f,
                            .offset = 0.090f},
           control::ModTarget::SeedHueSpeed);


  
  return p;
}

} // namespace epi::presets
