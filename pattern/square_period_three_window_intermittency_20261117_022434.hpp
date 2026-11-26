#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Square (Advanced 3.5D): the Period-Three Window, Intermittency, and the
 * Airplane.
 *
 * Mathematics:
 * - T(z) = 2.2 z^2 + c/2.2 is w -> w^2 + c in w = 2.2 z. A Sine lane moves
 *   c over [-1.80, -1.74] on the real axis, across the period-3 window of
 *   the logistic family, where "period three implies chaos" (Li-Yorke;
 *   Sharkovskii's order puts 3 first).
 * - Landmarks: c = -7/4 exactly is the saddle-node where the 3-cycle is
 *   born; c = -1.7548777 (root of c^3 + 2c^2 + c + 1) is the
 *   superattracting "airplane", the rest state; at c = -1.7685177 the
 *   3-cycle doubles (period 6), and the window's own cascade follows;
 *   near -1.7903 a crisis merges the three chaotic bands and the window
 *   closes.
 * - Just right of -7/4 the 3-cycle is a ghost: orbits linger near it for
 *   a number of passes of order 1 / sqrt(eps) (Pomeau-Manneville
 *   intermittency), so the exit-time bands crowd into the three lobes
 *   where the airplane's basins were. Moving left, the basins appear
 *   (dark lakes in three lobes), pinch at each doubling and dissolve into
 *   the interior-free chaotic sets.
 * - Exactness: for |c| <= 1.8, K lies in |w| <= 1.932, |z| <= 0.878, inside
 *   the Simplex band; the border has |w| >= 2.2, which escapes
 *   monotonically, so clamped points stay clamped.
 *
 * Pipeline: Square, zn0 = 2.2, zn1 = 0, zn2 = 1/2.2, zn3 = c/2.2 on the lane
 * (offset -1.77/2.2, amplitude 0.03/2.2, 0.0015 Hz, starting on the
 * airplane); Clamp; Simplex border; fade 0.96; OKLab step 1/3, so a
 * triple of passes returns to the same hue and the three lobes share a
 * band colour.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Square / Clamp / Simplex / OKLabColorRotation
 * Nearest:    square_parabolic_implosion_cauliflower_gate (the period-1
 *             saddle-node at the cusp); spatial_wave_feigenbaum_julia_cascade
 *             (the period-1 cascade). This is the same pair of events one
 *             period up, inside chaos.
 * Height:     age = passes before exit; near the ghost, the lingering time.
 * Hue clock:  bounded; step 1/3 +- 0.006.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; landmarks evaluated
 *             arithmetically (doubling point by bisection on the 3-cycle
 *             multiplier). The crisis value is quoted, not computed. Band
 *             births at the ghost change a region's colour at once; check
 *             against the photosensitivity gate once rendered. Not built,
 *             rendered or measured.
 * @assert sing[0].escapes == 0 && sing[0].period == 3
 */
inline Pattern square_period_three_window_intermittency() {
  Pattern p;
  p.name = "square_period_three_window_intermittency";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.2f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.45454545f, 0.0f};  // 1/2.2
  p.zn.zn[3] = {-0.79767167f, 0.0f}; // airplane c / 2.2

  p.color.hue_rotation = 0.33333333f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.96f;
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
  p.seed.hue = 0.33f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("window_walk",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0015f,
                            .amplitude = 0.013636364f,
                            .offset = -0.80454545f,
                            .phase = 0.08408376f}, // starts on the airplane
           control::ModTarget::Zn3Re)
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
