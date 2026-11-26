#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern op_art_pareidolia() {
  Pattern p;
  p.name = "op_art_pareidolia";
  p.inner = InnerFunction::Exp;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.01f, 0.01f};
  p.zn.zn[2] = {0.99f, -0.01f};
  
  p.color.color_mode = as_int(ColorMode::DynamicsDriven);
  p.color.hue_rotation = 0.05f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.99f;

  p.seed.type = SeedType::OrganicNoise;
  p.seed.border_width = 0.05f;
  p.seed.hue_speed = 0.15f;
  p.seed.alpha = 0.9f;
  p.speed = 0.8f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.150f;
  p.zn.zn0_anim_amp = 0.043f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.223f;
  p.zn.zn1_anim_amp = 0.039f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.113f;
  p.seed.pulse_amp = 0.16f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.085f,
                            .amplitude = 0.154f,
                            .offset = 0.550f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.086f,
                            .amplitude = 0.103f,
                            .offset = 0.180f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.084f,
                            .amplitude = 0.052f,
                            .offset = 0.150f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
