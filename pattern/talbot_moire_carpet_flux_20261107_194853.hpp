#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Talbot/moire interference carpet preset.
 *
 * Scientific inspiration:
 * - Talbot self-imaging and moire beat fields from near-periodic lattice
 *   superposition in wave optics.
 * - Implemented here as wrapped affine recurrence with incommensurate phase
 *   drift under feedback.
 *
 * Canonical pipeline mapping remains unchanged:
 * transform -> reduce -> sample -> color(mode4 spatial wave) -> seed -> blend
 */
inline Pattern talbot_moire_carpet_flux() {
  constexpr float PHI = 1.6180339887f;
  constexpr float SQRT2 = 1.4142135624f;
  constexpr float PRE = 1.3832f;
  constexpr float POST = 1.2770f;

  // 2*pi/10 (36 deg)
  constexpr float C10 = 0.8090169944f;
  constexpr float S10 = 0.5877852523f;
  // 2*pi*(sqrt(2)-1)
  constexpr float CQ = -0.8582161857f;
  constexpr float SQ = 0.5132883972f;

  Pattern p;
  p.name = "talbot_moire_carpet_flux";
  p.inner = InnerFunction::Cos;
  p.reduction = ReductionType::Grid;

  p.zn.zn[0] = {C10 * PRE, S10 * PRE};
  p.zn.zn[1] = {0.009f, -0.006f};
  p.zn.zn[2] = {CQ * POST, SQ * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.15f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.82f;      // hue x-slope
  p.color.spiral_radius_coeff = 0.7071067f; // wave1 spatial freq
  p.color.chroma_breath_amp = 0.20f;       // wave1 amplitude
  p.color.chroma_breath_freq = SQRT2;      // wave2 spatial freq
  p.color.chroma_breath_phase = 0.12f;     // wave2 amplitude
  p.color.hue_velocity_base = 0.022f;      // wave1 time speed
  p.color.hue_velocity_breath = 0.015f;    // wave2 time speed
  p.color.hue_velocity_drift = 0.008f;     // base drift
  p.color.luma_shape_depth = 0.30f;        // sat breathing amplitude
  p.color.luma_shape_center = 2.0f * PHI;  // sat breathing frequency

  p.seed.border_width = 0.083f;
  p.seed.hue_speed = 0.068f;
  p.seed.alpha = 0.92f;
  p.speed = 0.95f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.175f;
  p.zn.zn0_anim_amp = 0.052f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.163f;
  p.zn.zn1_anim_amp = 0.056f;
  p.zn.zn1_anim_k = 8.0f;
  p.seed.pulse_freq = 0.131f;
  p.seed.pulse_amp = 0.19f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.094f,
                            .amplitude = 0.198f,
                            .offset = 0.707f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.094f,
                            .amplitude = 0.110f,
                            .offset = 0.200f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.057f,
                            .amplitude = 0.030f,
                            .offset = 0.068f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
