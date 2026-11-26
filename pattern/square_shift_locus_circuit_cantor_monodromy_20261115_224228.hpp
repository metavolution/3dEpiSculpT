#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Advanced 3.5D: A Circuit Round the Mandelbrot Set, and What It Does to
 * the Dust.
 *
 * Mathematics:
 * - For c outside the Mandelbrot set the critical point of z^2 + c escapes
 *   and the Julia set is a Cantor set. The level curve of escape time that
 *   passes through the critical point is a figure of eight; each of its
 *   two lobes holds a smaller figure of eight through a pre-image of the
 *   critical point, and so on. The region between one level and the next
 *   is a pair of pants at the first stage, two pairs at the second, 2^n at
 *   the n-th. The dust is the set of ends of this binary tree.
 * - The outside of the Mandelbrot set is not simply connected, and a
 *   closed circuit of c round the set does not return the dust to itself
 *   pointwise. The two branches of the inverse, +-sqrt(z - c), change
 *   places: the circuit exchanges the two lobes of every figure of eight
 *   at every level. A second circuit restores them. This is the monodromy
 *   of the shift locus, here the one non-trivial automorphism of the
 *   one-sided shift on two symbols (Blanchard, Devaney and Keen 1991).
 * - The first figure of eight has its lobes along +-sqrt(-c), so it turns
 *   at half the speed of c: one circuit, half a turn.
 *
 * Pipeline: Square with zn2 = 2.2 and zn3 = c / 2.2, which is z^2 + c in
 * the frame coordinate z / 2.2. Two Sine lanes in quadrature carry c round
 * the circle c = -0.75 + 1.5 e^{it} once in 250 s. On that circle the
 * critical point escapes past 1000 within 5 to 7 steps at every one of 720
 * sample points, so the circuit stays outside the set, and the Julia set
 * stays inside |z| <= 2.08, within the frame. Clamp with the Simplex
 * border is the bailout, so age is escape time and the bands are the
 * pants. The tour passes the cusp side (c = 0.75), the top of the set, the
 * tip of the antenna (c = -2.25) and the underside.
 *
 * Fidelity:   identity
 * Cell:       Square / Clamp / Simplex / DepthEcho
 * Nearest:    spatial_wave_cardioid_multiplier_tour travels along the
 *             cardioid, where the Julia set is connected and there is an
 *             attracting point; this circuit is wholly outside, where
 *             nothing attracts and the loop has monodromy.
 * Height:     age = escape time = depth in the binary tree.
 * Hue clock:  bounded (hue_velocity_base = 0).
 * Fastest lane: 0.019 Hz
 * Status:     designed from source on 2026-11-15; the circuit was checked
 *             against the critical orbit in a separate arithmetic check;
 *             not built, rendered or measured.
 * @assert sing[0].escapes == 1 && p_X >= 0.99
 */
inline Pattern square_shift_locus_circuit_cantor_monodromy() {
  Pattern p;
  p.name = "square_shift_locus_circuit_cantor_monodromy";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {2.2f, 0.0f};
  p.zn.zn[3] = {0.34090909f, 0.0f}; // c / 2.2 at c = 0.75

  p.color.hue_rotation = 0.045f;
  p.color.sat_scale = 0.92f;
  p.color.fade = 0.93f;
  p.color.color_mode = as_int(ColorMode::DepthEcho);
  p.color.spiral_angle_coeff = 0.12f; // hue per unit of age
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.15f; // saturation gained with age
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.03f;
  p.color.hue_velocity_drift = 0.0f; // 0 = hue from age only
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.03f; // lit beyond |z| = 2.13; the dust ends at 2.08
  p.seed.hue = 0.75f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("circuit_re", // Re c / 2.2 = (-0.75 + 1.5 cos t) / 2.2
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 0.68181818f,
                            .offset = -0.34090909f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Re)
      .add("circuit_im", // Im c / 2.2 = 1.5 sin t / 2.2
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 0.68181818f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.019f,
                            .amplitude = 0.06f,
                            .offset = 0.15f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
