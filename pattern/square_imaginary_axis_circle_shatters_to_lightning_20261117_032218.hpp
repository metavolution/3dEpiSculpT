#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Square (Advanced 3.5D): up the Imaginary Axis; a Circle Shatters into
 * Dust and Reassembles as Lightning.
 *
 * Mathematics:
 * - T(z) = 2 z^2 + c/2 (w -> w^2 + c, w = 2z), c = i s, s on a Sine lane over
 *   [0, 1] (0.0015 Hz), starting at s = 0.
 * - s = 0: J is the unit circle (w -> w^2). For s < 0.6356 c is in the main
 *   cardioid: an attracting fixed point, J a quasicircle that wrinkles
 *   more and more as the multiplier |1 - sqrt(1 - 4c)| grows to 1.
 * - At s = 0.6356 the axis leaves the cardioid through the boundary point
 *   of rotation number 0.3096 (between the 1/3 and 1/4 bulbs, which it
 *   misses); the critical orbit escapes for every sampled s in
 *   [0.64, 0.95] (in 59, 26, 21, 16, 38, 21, 20, 11, 10 passes): the
 *   Julia set is Cantor dust.
 * - s = 1: c = i, the Misiurewicz point: 0 -> i -> -1 + i -> -i -> -1 + i,
 *   the dendrite (square_minimal_misiurewicz_lightning_dendrite). The
 *   Sine lane lingers at both ends: a long circle, a long lightning, and
 *   a quick passage through dust between them.
 * - Exactness: |c| <= 1, so K lies in |z| <= 0.809, inside the Simplex band;
 *   border points escape and stay clamped.
 *
 * Pipeline: Square, zn0 = 2, zn1 = 0, zn2 = 1/2, zn3 = i s/2 on the lane;
 * Clamp; Simplex border; fade 0.94; OKLab step 1/9.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Square / Clamp / Simplex / OKLabColorRotation
 * Nearest:    square_mandelbrot_tip_segment_shatters_to_horseshoe (the real
 *             axis's end); this is the imaginary axis's.
 * Height:     age = passes before exit.
 * Hue clock:  bounded; step 1/9 +- 0.005.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; crossing point,
 *             rotation number and escape counts evaluated arithmetically
 *             (s sampled at 0.05 steps; tiny decorations between samples not
 *             excluded). Not built, rendered or measured.
 * @assert sing[0].escapes == 0 && sing[0].period == 1
 */
inline Pattern square_imaginary_axis_circle_shatters_to_lightning() {
  Pattern p;
  p.name = "square_imaginary_axis_circle_shatters_to_lightning";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.5f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f}; // c = 0: the circle

  p.color.hue_rotation = 0.11111111f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.94f;
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
      .add("axis_climb",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0015f,
                            .amplitude = 0.25f,
                            .offset = 0.25f,
                            .phase = 0.75f}, // starts at s = 0
           control::ModTarget::Zn3Im)
      .add("terrace_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
                            .amplitude = 0.005f,
                            .offset = 0.11111111f},
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
