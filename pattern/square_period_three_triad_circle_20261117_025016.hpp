#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Square (Advanced 3.5D): the Period-Three Triad; One Circle through the
 * Airplane, the Rabbit and the Co-rabbit.
 *
 * Mathematics:
 * - The centres of the period-3 components of the Mandelbrot set are the
 *   roots of c^3 + 2c^2 + c + 1 = 0: the airplane c = -1.7548777 and the
 *   rabbit and co-rabbit c = -0.1225612 +- 0.7448618i. Three points fix a
 *   circle; by the real symmetry its centre is real: x0 = -0.7687711,
 *   R = 0.9861066. Its right end, c = 0.2173356, lies in the main cardioid
 *   (fixed-point multiplier 0.64).
 * - T(z) = 2.2 z^2 + c/2.2 (w -> w^2 + c, w = 2.2 z), with c walked exactly
 *   round that circle by two quadrature lanes on zn3. Starting at the
 *   airplane, the walk passes the co-rabbit at 364 s, the cardioid at
 *   500 s, the rabbit at 636 s and returns at 1000 s. At the three centres
 *   the critical point is periodic (superattracting 3-cycle): three
 *   Julia sets with the same period and different combinatorics (the
 *   airplane's real kneading, the rabbits' rotation numbers 1/3 and
 *   2/3), mirror images or not. Between them the walk crosses the
 *   boundary of M many times: dust, then connected sets again.
 * - Exactness: |c| <= 1.755, so K lies in |w| <= 1.916, |z| <= 0.871, inside
 *   the Simplex band; the border has |w| >= 2.2 and stays clamped.
 *
 * Pipeline: Square, zn0 = 2.2, zn1 = 0, zn2 = 1/2.2; zn3 = (x0 + R e^(i psi))
 * / 2.2 on two lanes at 0.001 Hz; Clamp; Simplex border; fade 0.95; OKLab
 * step 1/3, so passes 3 apart share a hue and a 3-cycle's basins share a
 * band colour.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Square / Clamp / Simplex / OKLabColorRotation
 * Nearest:    square_period_three_window_intermittency (the airplane's
 *             window on the real axis); this walk visits all three period-3
 *             centres.
 * Height:     age = passes before exit.
 * Hue clock:  bounded; step 1/3 +- 0.006.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; centres, circle and
 *             visiting times evaluated arithmetically. Which other
 *             components the circle crosses is not computed. Not built,
 *             rendered or measured.
 * @assert sing[0].escapes == 0 && sing[0].period == 3
 */
inline Pattern square_period_three_triad_circle() {
  constexpr float WALK_HZ = 0.001f;
  constexpr float RADIUS = 0.44823028f; // R / 2.2
  Pattern p;
  p.name = "square_period_three_triad_circle";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.2f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.45454545f, 0.0f};  // 1/2.2
  p.zn.zn[3] = {-0.79767167f, 0.0f}; // airplane c / 2.2

  p.color.hue_rotation = 0.33333333f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.06f;
  p.seed.hue = 0.6f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("triad_walk_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = WALK_HZ,
                            .amplitude = RADIUS,
                            .offset = -0.34944139f, // x0 / 2.2
                            .phase = 0.75f},        // psi = pi: airplane
           control::ModTarget::Zn3Re)
      .add("triad_walk_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = WALK_HZ,
                            .amplitude = RADIUS,
                            .offset = 0.0f,
                            .phase = 0.5f},
           control::ModTarget::Zn3Im)
      .add("triad_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
                            .amplitude = 0.006f,
                            .offset = 0.33333333f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.025f,
                            .offset = 0.05f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
