#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Square (Advanced 3.5D): at the Tip of the Mandelbrot Set a Segment
 * Shatters into a Horseshoe.
 *
 * Mathematics:
 * - T(z) = 2.4 z^2 + c/2.4 is w -> w^2 + c in w = 2.4 z. A Sine lane moves
 *   c over [-2.2, -1.9], across the tip c = -2 of the Mandelbrot set.
 * - c = -2 (the rest state): the Chebyshev map. w = zeta + 1/zeta conjugates
 *   it to zeta -> zeta^2, the Julia set is the segment [-2, 2], and the
 *   levels |zeta| = R are the confocal ellipses with foci +-2 (here
 *   +-0.833 on screen). Exit-time terraces are preimages of the square
 *   border and tend to these ellipses with depth: a nest of confocal
 *   ellipses, the equipotentials of a charged rod.
 * - c < -2: the critical value escapes, and the Julia set is a Cantor set
 *   on the real axis, a Smale horseshoe (|f'| > 1 on it). The rod breaks
 *   into dust, and the inner terraces pinch through its gaps into beads.
 * - -2 < c <= -1.9: inside the set near its tip, crowded with Misiurewicz
 *   parameters and tiny windows; the Julia sets are dendrites with spines
 *   along the real axis.
 * - Exactness: for |c| <= 2.2, K lies in |w| <= 2.065, |z| <= 0.861, inside
 *   the Simplex band; the border has |w| >= 2.4, which escapes
 *   monotonically, so clamped points stay clamped.
 *
 * Pipeline: Square, zn0 = 2.4, zn1 = 0, zn2 = 1/2.4, zn3 = c/2.4 on the lane
 * (offset -2.05/2.4, amplitude 0.15/2.4, 0.0015 Hz, starting at c = -2);
 * Clamp; Simplex border; fade 0.94; OKLab step 1/10.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Square / Clamp / Simplex / OKLabColorRotation
 * Nearest:    square_period_three_window_intermittency and
 *             square_parabolic_implosion_cauliflower_gate (other
 *             real-axis crossings); this one crosses the end of the set.
 * Height:     age = passes before exit.
 * Hue clock:  bounded; step 1/10 +- 0.005.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; bound and lane phase
 *             evaluated arithmetically. The ellipse law holds for the deep
 *             terraces; the outer ones follow the square border. Not built,
 *             rendered or measured.
 * @assert sing[0].escapes == 0
 */
inline Pattern square_mandelbrot_tip_segment_shatters_to_horseshoe() {
  Pattern p;
  p.name = "square_mandelbrot_tip_segment_shatters_to_horseshoe";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.4f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.41666667f, 0.0f};  // 1/2.4
  p.zn.zn[3] = {-0.83333333f, 0.0f}; // c/2.4 at c = -2

  p.color.hue_rotation = 0.1f;
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
  p.seed.hue = 0.9f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("tip_crossing",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0015f,
                            .amplitude = 0.0625f,
                            .offset = -0.85416667f,
                            .phase = 0.05408672f}, // starts at c = -2
           control::ModTarget::Zn3Re)
      .add("terrace_tint",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
                            .amplitude = 0.005f,
                            .offset = 0.1f},
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
