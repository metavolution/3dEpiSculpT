#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: Whirling Squares, a Zoom with Every Twist.
 *
 * Mathematics:
 * - z -> 1.10 e^(i theta) z enlarges and turns. Reading history through it
 *   shows the window's own border reduced by 1/1.10 and turned by -theta,
 *   then again, and again: a nest of squares converging on the centre, the
 *   Droste picture, each square one generation older.
 * - Corresponding corners of the nested squares lie on logarithmic spirals.
 *   For theta = 2 pi p / q the corners of every q-th square line up, and the
 *   nest shows 4 q / gcd(4, q) spiral arms; a square repeats every quarter
 *   turn, so theta and theta + pi / 2 give the same nest.
 * - Two lanes in quadrature on zn0 with amplitude exactly 1.10 make theta run
 *   uniformly round the circle at fixed zoom, so the arm count changes
 *   continuously through every rational.
 *
 * Pipeline: Identity, zn0 = 1.10 e^(i theta(t)); Clamp; Simplex border seed.
 * Every orbit leaves the window after about ln(1/|z|) / ln 1.10 steps, so the
 * generation number is a logarithmic depth. One revolution takes 400 s.
 *
 * Mechanism assertion: escaped class ~ 1 away from the centre; uniform
 * log-stretch ln 1.10 at every theta.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_whirling_squares_twist_sweep() {
  Pattern p;
  p.name = "spatial_wave_whirling_squares_twist_sweep";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.1f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.045f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.97f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 1.2f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.08f;
  p.seed.hue = 0.92f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("twist_cos",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0025f,
                            .amplitude = 1.1f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn0Re)
      .add("twist_sin",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0025f,
                            .amplitude = 1.1f,
                            .offset = 0.0f},
           control::ModTarget::Zn0Im)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.019f,
                            .amplitude = 0.02f,
                            .offset = 0.045f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
