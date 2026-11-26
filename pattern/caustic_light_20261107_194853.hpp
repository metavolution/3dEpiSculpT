#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Caustic light — complex tangent feedback with grid reduction.
 *
 * tan(z) has poles at z = (n+½)π, creating sharp discontinuity lines
 * where the image tears — visually identical to light caustics on the
 * bottom of a swimming pool. The pre-affine scale is kept tight (0.28)
 * so only the first ring of poles is active; this prevents the feedback
 * from diverging while preserving the razor-sharp caustic geometry.
 *
 * Grid reduction (mirror-fold) creates kaleidoscopic repetition of the
 * caustic lines. Spatial wave color mode adds slow color drift across
 * the caustic field.
 */
inline Pattern caustic_light() {
  Pattern p;
  p.name = "caustic_light";
  p.inner = InnerFunction::Tan;
  p.reduction = ReductionType::Grid;
  // Tight pre-affine: keep |z| within the first pole ring (|z| < π/2)
  p.zn.zn[0] = {1.1839f, 0.0846f};
  p.zn.zn[1] = {0.0463f, -0.0327f};
  // Post-affine: slight expansion to fill domain
  p.zn.zn[2] = {1.1056f, -0.1558f};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.color.hue_rotation = 0.018f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.986f;
  // Mode 4: spatial wave
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff  = 0.22f;
  p.color.spiral_radius_coeff = 0.16f;
  p.color.chroma_breath_amp   = 0.07f;
  p.color.chroma_breath_freq  = 0.25f;
  p.color.chroma_breath_phase = 0.05f;
  p.color.hue_velocity_base   = 0.011f;
  p.color.hue_velocity_breath = 0.007f;
  p.color.hue_velocity_drift  = 0.004f;
  p.color.luma_shape_depth    = 0.16f;
  p.color.luma_shape_center   = 2.2f;
  p.seed.border_width = 0.093f;
  p.seed.hue_speed = 0.12f;
  p.seed.alpha = 0.92f;
  p.speed = 0.8f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.112f;
  p.zn.zn0_anim_amp = 0.044f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.178f;
  p.zn.zn1_anim_amp = 0.039f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.112f;
  p.seed.pulse_amp = 0.15f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.079f,
                            .amplitude = 0.120f,
                            .offset = 0.160f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.078f,
                            .amplitude = 0.065f,
                            .offset = 0.070f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.059f,
                            .amplitude = 0.042f,
                            .offset = 0.120f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
