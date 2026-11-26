#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: a Tour of the Main Cardioid by the Fixed-Point Multiplier.
 *
 * Mathematics:
 * - P3 is v (v + 1) / 2. With v = 2.8 z - 0.5 and zn3 = 0.5 / 2.8 the map is
 *   v -> lambda v (1 + v), lambda = 1.4 zn2: a quadratic polynomial whose
 *   fixed point v = 0 has multiplier exactly lambda. It is conjugate to
 *   w^2 + c with c = lambda / 2 - lambda^2 / 4, the parametrisation of the
 *   main cardioid of the Mandelbrot set by its multiplier.
 * - Two lanes in quadrature on zn2 make lambda = 0.985 e^(i theta(t)) run
 *   once round, just inside the unit circle: c travels the whole cardioid
 *   just inside its boundary. On the way the fixed point is nearly parabolic
 *   at every rational angle p / q (q petals, the bulbs of the Mandelbrot set
 *   attach there: cauliflower at 0, basilica at 1/2, rabbit at 1/3) and
 *   nearly a Siegel centre in between. The Julia set stays a quasicircle
 *   throughout and deforms continuously, sprouting q arms at each rational.
 *
 * Pipeline: P3, zn0 = 2.8, zn1 = -0.5; Clamp; Simplex border seed, so the
 * exterior is drawn as escape terraces. One tour takes 250 s.
 *
 * Mechanism assertion: escaped class outside, contracting class inside with
 * rate |lambda| = 0.985 at the fixed point z = 0.179 for every theta.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_cardioid_multiplier_tour() {
  Pattern p;
  p.name = "spatial_wave_cardioid_multiplier_tour";
  p.inner = InnerFunction::P3;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.8f, 0.0f};
  p.zn.zn[1] = {-0.5f, 0.0f};
  p.zn.zn[2] = {0.7036f, 0.0f};
  p.zn.zn[3] = {0.17857f, 0.0f};

  p.color.hue_rotation = 0.06f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.6f;
  p.color.spiral_radius_coeff = 0.9f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.06f;
  p.seed.hue = 0.82f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("multiplier_cos",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 0.7036f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Re)
      .add("multiplier_sin",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 0.7036f,
                            .offset = 0.0f},
           control::ModTarget::Zn2Im)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.016f,
                            .amplitude = 0.02f,
                            .offset = 0.06f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
