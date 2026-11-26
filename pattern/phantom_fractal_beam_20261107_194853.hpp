#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern phantom_fractal_beam() {
  Pattern p;
  p.name = "phantom_fractal_beam";
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.05f, 0.05f};
  p.zn.zn[2] = {0.95f, -0.05f};
  p.color.hue_rotation = 0.01f;
  p.color.sat_scale = 1.02f;
  p.color.fade = 0.99f;

  p.color.color_mode = as_int(ColorMode::PhantomFractal);
  p.color.spiral_angle_coeff = 2.0f;
  p.color.spiral_radius_coeff = 1.5f;
  p.color.chroma_breath_amp = 0.15f;
  p.color.hue_velocity_base = 0.05f;
  p.color.luma_shape_depth = 0.3f;

  p.seed.border_width = 0.1f;
  p.seed.hue_speed = 0.2f;
  p.seed.alpha = 1.0f;
  p.speed = 0.5f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.154f;
  p.zn.zn0_anim_amp = 0.043f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.182f;
  p.zn.zn1_anim_amp = 0.048f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.120f;
  p.seed.pulse_amp = 0.20f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.085f,
                            .amplitude = 0.420f,
                            .offset = 1.500f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.081f,
                            .amplitude = 0.092f,
                            .offset = 0.150f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.045f,
                            .amplitude = 0.070f,
                            .offset = 0.200f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
