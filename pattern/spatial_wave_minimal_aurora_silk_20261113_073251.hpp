#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimalist 2): Alfvenic Color-Field Aurora Silk.
 *
 * Mathematical foundation:
 * - Pure affine similarity dilation + shear (Inner = Identity, z' = zn0 * z + zn1)
 *   with |zn0| = 1.048 > 1 maps interior coordinates outward toward the [-1, 1]^2
 *   Torus mirror boundary, advecting border luminance smoothly inward as nested
 *   logarithmic silk curtains without nonlinear topological tearing.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Identity + Torus reduction: purest possible affine mirror cascade.
 * - Color = SpatialWave (mode 4) + LinearX spatial mode: couples smooth horizontal
 *   phase slope with two slow irrational harmonic waves (1 : sqrt(2)), evoking
 *   James Turrell / magnetospheric Alfven-sheet color gradients.
 */
inline Pattern spatial_wave_minimal_aurora_silk() {
  Pattern p;
  p.name = "spatial_wave_minimal_aurora_silk";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.046f, 0.032f};
  p.zn.zn[1] = {0.012f, -0.008f};
  p.zn.zn[2] = {1.005f, -0.014f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.09f;
  p.zn.zn0_anim_amp = 0.020f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.12f;
  p.zn.zn1_anim_amp = 0.025f;
  p.zn.zn1_anim_k = 3.0f;

  p.color.hue_rotation = 0.007f;
  p.color.sat_scale = 1.036f;
  p.color.fade = 0.991f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spatial_mode = ColorSpatialMode::HorizontalWave;
  p.color.spiral_angle_coeff = 0.44f;
  p.color.spiral_radius_coeff = 0.28f;
  p.color.chroma_breath_amp = 0.15f;
  p.color.chroma_breath_freq = 0.396f; // ~0.28 * sqrt(2)
  p.color.chroma_breath_phase = 0.10f;
  p.color.hue_velocity_base = 0.014f;
  p.color.hue_velocity_breath = 0.009f;
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.10f;
  p.color.luma_shape_center = 1.25f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.062f;
  p.seed.hue = 0.44f;
  p.seed.hue_speed = 0.075f;
  p.seed.alpha = 0.94f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.12f;

  p.speed = 0.64f;

  p.motion
      .add("alfven_curtain_shear",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.054f,
                            .amplitude = 0.024f,
                            .offset = -0.014f},
           control::ModTarget::Zn2Im)
      .add("aurora_band_pitch",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.077f,
                            .amplitude = 0.12f,
                            .offset = 0.44f,
                            .phase = 0.25f},
           control::ModTarget::SpiralAngleCoeff)
      .add("silk_hue_glide",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.038f,
                            .amplitude = 0.005f,
                            .offset = 0.007f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
