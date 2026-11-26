#pragma once

#include "../../pattern.hpp"

namespace epi::presets {

inline Pattern chroma_breathing_palette() {
  Pattern p;
  p.name = "chroma_breathing_palette";
  
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Torus;
  p.display_post_mode = "palette3d";
  
  p.zn.zn[0] = {1.0f, 0.0f}; 
  p.zn.zn[2] = {0.99f, 0.02f};
  
  // Chroma Breathing
  p.color.color_mode = presets::as_int(presets::ColorMode::ChromaBreathing); 
  
  p.color.spiral_angle_coeff = 1.0f; 
  p.color.spiral_radius_coeff = 0.5f; 
  
  p.color.chroma_breath_amp = 0.6f;  
  p.color.chroma_breath_freq = 0.1f; 
  p.color.chroma_breath_phase = 0.5f;
  
  p.color.hue_rotation = 0.0f;
  
  p.color.sat_scale = 1.02f;
  p.color.fade = 0.97f;
  
  p.seed.border_width = 0.05f;
  p.seed.alpha = 0.6f;
  p.speed = 0.5f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.154f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.212f;
  p.zn.zn1_anim_amp = 0.037f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.076f;
  p.seed.pulse_amp = 0.14f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.069f,
                            .amplitude = 0.140f,
                            .offset = 0.500f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.089f,
                            .amplitude = 0.120f,
                            .offset = 0.600f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.056f,
                            .amplitude = 0.032f,
                            .offset = 0.090f},
           control::ModTarget::SeedHueSpeed);


  
  return p;
}

} // namespace epi::presets
