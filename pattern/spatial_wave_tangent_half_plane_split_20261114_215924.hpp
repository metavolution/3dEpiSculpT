#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: the Tangent Family at L = 1, One Basin Becomes Two.
 *
 * Mathematics:
 * - w -> L tan w with real L. tan'(0) = 1, so 0 is a fixed point with
 *   multiplier L. For L < 1 it attracts and its basin is the whole Fatou set.
 *   For L > 1 it repels, and two new attracting fixed points appear on the
 *   imaginary axis at w = +-i y with L tanh y = y. The Julia set becomes the
 *   real line: the upper and the lower half-plane are the two basins
 *   (Devaney and Keen). At L = 1 the single basin splits along a straight
 *   line.
 * - The poles of tan at +-pi/2 are inside the window; near them the image
 *   leaves the square and is clamped, which marks them.
 *
 * Pipeline: Tan, zn0 = 1.7, zn2 = L / 1.7; Clamp; OrganicNoise seed at low
 * alpha, which each basin gathers towards its own focus. The lane sweeps L
 * from 0.7 to 1.5 and back; a small imaginary part tilts the dividing line.
 *
 * Mechanism assertion: contracting class dominant; one attracting fixed
 * point for L < 1, two (on the imaginary axis) for L > 1.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_tangent_half_plane_split() {
  Pattern p;
  p.name = "spatial_wave_tangent_half_plane_split";
  p.inner = InnerFunction::Tan;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.7f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.647f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.05f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.5f;
  p.color.spiral_radius_coeff = 0.8f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::OrganicNoise;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.7f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 0.3f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("gain_L_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.011f,
                            .amplitude = 0.235f,
                            .offset = 0.647f},
           control::ModTarget::Zn2Re)
      .add("divide_tilt",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.017f,
                            .amplitude = 0.02f,
                            .offset = 0.0f},
           control::ModTarget::Zn2Im)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.015f,
                            .amplitude = 0.02f,
                            .offset = 0.05f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
