#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Square (Advanced 3.5D): Elephant Valley; the Beak of the Cardioid Crossed
 * Sideways.
 *
 * Mathematics:
 * - T(z) = 2 z^2 + c/2 (w -> w^2 + c, w = 2z) with c = 0.27 + i y and a Sine
 *   lane on y over [-0.008, 0.008].
 * - Near its cusp the main cardioid's boundary is c(theta) = 1/4 +
 *   theta^2/4 + i theta^3/4 + ..., so the outside of the Mandelbrot set is
 *   a thin beak along the real axis right of 1/4, |Im c| below about
 *   (Re c - 1/4)^(3/2): elephant valley. At Re c = 0.27 the line leaves
 *   the cardioid at |y| = 0.0060 (checked: the fixed-point multiplier
 *   |1 - sqrt(1 - 4c)| is 1.0061 at y = 0.005 and 0.9998 at y = 0.006).
 * - Inside (|y| > 0.006) there is an attracting fixed point with multiplier
 *   just inside the circle and a small rotation, so the Julia set is a
 *   quasicircle whose inlets spiral: the elephants' trunks. Outside
 *   (|y| < 0.006) the Julia set is Cantor dust and orbits squeeze through
 *   the gate left by the merged fixed points (about pi / sqrt(eps) passes
 *   at the real axis, Boll's law at the cusp; 20 passes at y = 0, 64 at
 *   y = 0.005). The sign of y sets the sense of the spirals.
 * - The lane crosses the boundary four times per cycle: connected trunks,
 *   dust, trunks wound the other way, dust.
 * - Exactness: |c| <= 0.271, K lies in |z| <= 0.61, inside the Simplex
 *   band; border points escape and stay clamped.
 *
 * Pipeline: Square, zn0 = 2, zn2 = 1/2, zn3 = c/2 with Im on the lane
 * (amplitude 0.004, 0.002 Hz, rest at y = 0); Clamp; Simplex border;
 * fade 0.97; OKLab step 1/24.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Square / Clamp / Simplex / OKLabColorRotation
 * Nearest:    square_parabolic_implosion_cauliflower_gate (the cusp crossed
 *             along the real axis); square_seahorse_valley_bolls_pi (the
 *             other valley).
 * Height:     age = passes before exit; inside, the dark basin.
 * Hue clock:  bounded; step 1/24 +- 0.002.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; crossing points and
 *             transit counts evaluated arithmetically. Inside, close to the
 *             boundary, convergence takes more passes than the fade keeps, so
 *             the basin's dark interior appears only gradually. Band births
 *             at the gate: check against the photosensitivity gate once
 *             rendered. At the rest state (y = 0, outside M) the critical
 *             orbit escapes and K is dust of zero area, so the escaped class
 *             dominates; slow transits near the gate may exceed a census
 *             horizon, so the bound is expected, not proved. Not built,
 *             rendered or measured.
 * @assert p_X >= 0.5
 */
inline Pattern square_elephant_valley_crossing() {
  Pattern p;
  p.name = "square_elephant_valley_crossing";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.5f, 0.0f};
  p.zn.zn[3] = {0.135f, 0.0f}; // c/2 at c = 0.27

  p.color.hue_rotation = 0.04166667f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.97f;
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
  p.seed.hue = 0.08f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("beak_crossing",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.002f,
                            .amplitude = 0.004f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Im)
      .add("trunk_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
                            .amplitude = 0.002f,
                            .offset = 0.04166667f},
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
