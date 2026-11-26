#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimalist 1): Korteweg-de Vries (KdV) Two-Soliton Ribbon.
 *
 * Mathematical foundation:
 * - In the integrable Korteweg-de Vries equation u_t + 6 u u_x + u_xxx = 0,
 *   a single kink-potential profile is proportional to tanh(kappa * (x - c*t)),
 *   whose derivative sech^2(kappa * x) is a localized solitary wave. When two
 *   solitons collide, they emerge with unchanged shape and only a clean
 *   logarithmic phase shift.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Tanh: provides smooth, bounded kink compression without chaotic
 *   high-frequency folding, preserving minimalist planar ribbon geometry.
 * - Pre/Post affine: supercritical product |tanh(1.18)| * |1.26| > 1.04 ensures
 *   border seed energy propagates smoothly across the entire [-1, 1]^2 torus
 *   instead of stagnating at the frame perimeter.
 * - Color = SpatialWave (mode 4): golden-conjugate spatial wave ratio
 *   (0.382 : 0.618) creates clean, non-repeating prismatic interference bands.
 * - ZN animation: gentle 2-lobe translation drift mimics elastic two-soliton
 *   phase-shift breathing.
 */
inline Pattern spatial_wave_minimal_soliton() {
  Pattern p;
  p.name = "spatial_wave_minimal_soliton";
  p.inner = InnerFunction::Tanh;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.180f, 0.045f};
  p.zn.zn[1] = {0.015f, -0.010f};
  p.zn.zn[2] = {1.260f, -0.032f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.11f;
  p.zn.zn0_anim_amp = 0.028f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.14f;
  p.zn.zn1_anim_amp = 0.036f;
  p.zn.zn1_anim_k = 2.0f;

  p.color.hue_rotation = 0.008f;
  p.color.sat_scale = 1.032f;
  p.color.fade = 0.988f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.56f;
  p.color.spiral_radius_coeff = 0.382f;
  p.color.chroma_breath_amp = 0.14f;
  p.color.chroma_breath_freq = 0.618f;
  p.color.chroma_breath_phase = 0.09f;
  p.color.hue_velocity_base = 0.016f;
  p.color.hue_velocity_breath = -0.011f;
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.11f;
  p.color.luma_shape_center = 1.618f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.065f;
  p.seed.hue = 0.52f;
  p.seed.hue_speed = 0.09f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.06f;
  p.seed.pulse_amp = 0.15f;

  p.speed = 0.72f;

  p.motion
      .add("soliton_wave_pitch",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.068f,
                            .amplitude = 0.14f,
                            .offset = 0.56f},
           control::ModTarget::SpiralAngleCoeff)
      .add("kdv_phase_shift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.042f,
                            .amplitude = 0.038f,
                            .offset = -0.032f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Im)
      .add("ribbon_sat_breath",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.055f,
                            .amplitude = 0.045f,
                            .offset = 0.11f},
           control::ModTarget::LumaShapeDepth);

  return p;
}

} // namespace epi::presets
