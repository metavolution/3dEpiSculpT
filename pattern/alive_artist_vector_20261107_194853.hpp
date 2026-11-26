#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern alive_artist_vector() {
  Pattern p;
  p.name = "alive_artist_vector";
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.01f, 0.02f};
  p.zn.zn[1] = {0.0f, 0.0f};
  
  p.color.color_mode = as_int(ColorMode::AtmosphericDrift);
  p.display_post_mode = "palette3d"; // Vector-field color mapping
  
  p.color.spiral_angle_coeff = 0.2f;
  p.color.spiral_radius_coeff = 0.15f;
  p.color.chroma_breath_amp = 0.25f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.hue_velocity_base = 0.02f;
  p.color.hue_velocity_breath = 0.015f;
  p.color.hue_velocity_drift = 0.005f;
  p.color.luma_shape_depth = 0.2f;
  p.color.luma_shape_center = 2.0f;
  p.color.sat_scale = 1.05f;
  p.color.fade = 0.98f;

  p.seed.border_width = 0.15f;
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 0.8f;
  p.speed = 0.8f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.165f;
  p.zn.zn0_anim_amp = 0.041f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.202f;
  p.zn.zn1_anim_amp = 0.042f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.134f;
  p.seed.pulse_amp = 0.20f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.094f,
                            .amplitude = 0.120f,
                            .offset = 0.150f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.095f,
                            .amplitude = 0.120f,
                            .offset = 0.250f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.056f,
                            .amplitude = 0.030f,
                            .offset = 0.050f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
