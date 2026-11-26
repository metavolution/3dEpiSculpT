#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Square (Advanced 3.5D): Parabolic Implosion, the Cauliflower Opens a Gate
 * and Shatters.
 *
 * Mathematics:
 * - T(z) = 2 z^2 + c/2 is w -> w^2 + c in w = 2z. A Sine lane moves c along
 *   the real axis through the cusp of the main cardioid:
 *   c = 1/4 + eps, eps in [-0.025, +0.025].
 * - eps < 0: an attracting fixed point near w = 1/2 (multiplier
 *   1 - sqrt(-4 eps)); the filled Julia set is a connected "cauliflower".
 *   eps = 0: the two fixed points merge into the parabolic point 1/2.
 *   eps > 0: they split off the real axis, the critical orbit escapes, and
 *   the Julia set becomes a Cantor dust: the connected set implodes.
 * - The ghost of the merged fixed points stays as a gate: an orbit takes
 *   about pi / sqrt(eps) passes to slip through (20 at eps = 0.025, more as
 *   eps -> 0+). With exit-time colouring, the bands of the former basin
 *   are those transit counts; each time pi / sqrt(eps) crosses an integer
 *   one band is born or swallowed at the gate (the Lavaurs phase turns).
 * - Exactness: for |c| <= 0.275, K_c lies in |w| <= 1.225, |z| <= 0.613,
 *   inside the Simplex band; border points escape and stay clamped.
 *
 * Pipeline: Square with zn0 = 2, zn1 = 0, zn2 = 1/2; zn3 = c/2 on a Sine lane
 * (offset 1/8, amplitude 1/80, 0.002 Hz: 500 s per breath), rest state at
 * the cusp c = 1/4. Clamp; Simplex border; fade 0.96 (about 25 generations
 * deep, so the gate is visible on both sides); OKLab step 1/20, so one hue
 * turn spans a transit of 20 passes.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Square / Clamp / Simplex / OKLabColorRotation
 * Nearest:    square_minimal_feigenbaum_point_edge_of_chaos (the other end
 *             of the real slice of the Mandelbrot set);
 *             spatial_wave_cardioid_multiplier_tour (round the cardioid, not
 *             through its cusp).
 * Height:     age = passes before exit; near the gate, the transit count.
 * Hue clock:  bounded; step 1/20 +- 0.003.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. The asymptotic
 *             pi / sqrt(eps) is the leading term only; band births at the
 *             gate change the exit-time colour of a region at once, so check
 *             them against the photosensitivity gate once rendered. Not
 *             built, rendered or measured.
 * @assert sing[0].escapes == 0
 */
inline Pattern square_parabolic_implosion_cauliflower_gate() {
  Pattern p;
  p.name = "square_parabolic_implosion_cauliflower_gate";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.5f, 0.0f};
  p.zn.zn[3] = {0.125f, 0.0f}; // c/2 at the cusp c = 1/4

  p.color.hue_rotation = 0.05f;
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
  p.seed.hue = 0.58f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("cusp_crossing",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.002f,
                            .amplitude = 0.0125f,
                            .offset = 0.125f},
           control::ModTarget::Zn3Re)
      .add("transit_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
                            .amplitude = 0.003f,
                            .offset = 0.05f},
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
