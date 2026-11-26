#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern alive_artist_breathing() {
  Pattern p;
  p.name = "alive_artist_breathing";
  p.inner = InnerFunction::Exp;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.02f, 0.03f};
  p.zn.zn[1] = {0.0f, 0.0f};
  
  p.color.color_mode = as_int(ColorMode::ChromaBreathing);
  p.display_post_mode = "palette3d"; // Vector-field color mapping
  
  p.color.spiral_angle_coeff = 0.5f;
  p.color.spiral_radius_coeff = 0.8f;
  p.color.chroma_breath_amp = 0.4f;
  p.color.chroma_breath_freq = 0.2f;
  p.color.hue_velocity_base = 0.01f;
  p.color.hue_velocity_breath = 0.03f;
  p.color.hue_velocity_drift = 0.002f;
  p.color.luma_shape_depth = 0.15f;
  p.color.sat_scale = 1.02f;
  p.color.fade = 0.97f;

  p.seed.border_width = 0.2f;
  p.seed.hue_speed = 0.1f;
  p.seed.alpha = 0.7f;
  p.speed = 1.0f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.164f;
  p.zn.zn0_anim_amp = 0.037f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.205f;
  p.zn.zn1_anim_amp = 0.050f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.130f;
  p.seed.pulse_amp = 0.20f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.053f,
                            .amplitude = 0.224f,
                            .offset = 0.800f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.118f,
                            .amplitude = 0.120f,
                            .offset = 0.400f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.090f,
                            .amplitude = 0.035f,
                            .offset = 0.100f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
