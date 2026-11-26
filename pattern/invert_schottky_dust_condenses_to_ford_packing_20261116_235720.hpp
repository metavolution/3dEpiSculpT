#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Advanced 3.5D): Schottky Dust Condenses into a Packing of Tangent
 * Circles.
 *
 * Mathematics:
 * - T(z) = k / z, then the Torus fold: inversion in the circle of radius
 *   sqrt k, reflection in the real axis, and reflection in the box mirrors
 *   x = +-1, y = +-1. Each pass is an element of a reflection group.
 * - k < 1: the circle misses all four lines. Every pair of mirrors is then
 *   disjoint or at right angles, so by Poincare's theorem the reflection
 *   group of the box and the circle is discrete, with the box minus the
 *   disc, one connected ring, as fundamental domain (the real axis is a
 *   symmetry of the whole configuration). On the ring |k / z| < sqrt k < 1,
 *   the fold does nothing, and T(T(z)) = z: period 2.
 * - k = 1: the circle touches the four lines. The ring pinches at +-1 and
 *   +-i into four corner triangles (angles pi/2, 0, 0) and the touching
 *   points become parabolic cusps. A degeneration of the group: the
 *   separated images of the disc are pressed together until they touch.
 *
 * Pipeline: Invert, Torus, zn2 = k on a Sine lane from 0.82 to 1.0 at
 * 0.004 Hz; a Sine lingers at its ends, so the picture rests on the dust
 * and on the packing. Simplex border, OKLab hue step 1/10 by depth. In a
 * numpy model of the pass, at k = 0.85 the circle images form four
 * separate flowers inside the frame circle; at k = 1 they touch the frame
 * and each other and nest into the cusps in rainbow fans. In between the
 * tiles grow continuously; hue changes only where a pixel's landing depth
 * changes, and at this rate that is slow.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Invert / Torus / Simplex / OKLabColorRotation
 * Nearest:    invert_minimal_ford_mirror_box_circle_web holds k = 1 and
 *             draws the axes with Cross; schottky_kleinian_limit_necklace
 *             wanders through generic slots. Here the subject is the
 *             degeneration itself.
 * Height:     age = passes since landing; depth terraces of 1/10 turn.
 * Hue clock:  bounded; step 1/10.
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-16. On a 300 x 300 grid in
 *             double precision, 100 % of points were on a 2-cycle within
 *             400 passes at k = 0.82, 0.86, 0.95, 0.99 and 1.0, and no
 *             64-pass exponent was below -0.01 (median 0.0015 at k = 0.91).
 *             The dust and packing descriptions come from the numpy model.
 *             Not built, rendered by the engine or measured.
 * @assert p_X == 0 && p_C <= 0.05
 */
inline Pattern invert_schottky_dust_condenses_to_ford_packing() {
  constexpr float K_MID = 0.91f;
  constexpr float K_SWING = 0.09f; // k from 0.82 to 1.0
  Pattern p;
  p.name = "invert_schottky_dust_condenses_to_ford_packing";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {K_MID, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.1f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0006f;
  p.color.hue_velocity_drift = 0.0002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.08f;
  p.seed.hue = 0.08f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("inversion_radius_squared",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = K_SWING,
                            .offset = K_MID},
           control::ModTarget::Zn2Re)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.025f,
                            .offset = 0.05f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
