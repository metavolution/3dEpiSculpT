#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Minimal 3.5D): the Modular Triangle in a Mirror Box.
 *
 * Mathematics:
 * - T(z) = 4 / (z - 1) + 1, then the Torus fold. 4 / (z - 1) + 1 is
 *   inversion in the circle |z - 1| = 2 followed by reflection in the real
 *   axis, the horizontal line through the circle's centre; the box is
 *   symmetric under that reflection. The fold is reflection in x = +-1,
 *   y = +-1. Every pass is a product of reflections in circles and lines.
 * - The circle has its centre on the edge midpoint 1. It crosses y = 1 and
 *   y = -1 at 1 - sqrt 3 +- i at the angle arccos(1/2) = pi/3, touches
 *   x = -1 at -1, and meets x = 1 and the real axis at right angles.
 * - Inside the box and outside the circle lie two pieces, at the corners
 *   -1 +- i. Each is a triangle with angles pi/2 (the corner), pi/3 (the
 *   crossing) and 0 (the touching point -1): the (2, 3, infinity) triangle,
 *   whose reflection group is the extended modular group PGL(2, Z). The
 *   picture is drawn on the tiling of the modular group.
 * - On a piece, z -> 4 / (z - 1) + 1 lands inside the box (|Re| and |Im|
 *   both stay below 1 because |z - 1| > 2), the fold does nothing, and the
 *   second pass returns z exactly: every point of the pieces has period 2.
 *
 * Pipeline: Invert with zn0 = 1, zn1 = -1, zn2 = 4, zn3 = 1; Torus; Cross
 * seed; OKLab hue step 1/6, because six triangles meet at every order-3
 * vertex. The lit real axis is a symmetry line of the whole configuration,
 * so its images are edges of the tiling; the lit imaginary axis is not, and
 * its images cross the tiles. Both shrink into the cusp at -1 and its
 * images.
 *
 * Fidelity:   identity
 * Cell:       Invert / Torus / Cross / OKLabColorRotation
 * Nearest:    invert_minimal_ford_mirror_box_circle_web (circle at the
 *             centre, triangle (2, inf, inf)); invert_minimal_hexagonal_
 *             corner_mirror_arcs (circle at a corner, (2, 6, 6)). Here the
 *             circle sits on an edge and the triangle is the modular one.
 * Height:     age = passes since reaching a piece.
 * Hue clock:  bounded; step 1/6 +- 0.01.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17 (angles, triangle type
 *             and the period-2 property are proved above). That every
 *             other point reaches a piece is expected from the group but
 *             not proved. Not built, rendered or measured.
 * @assert p_X == 0 && p_C <= 0.05
 */
inline Pattern invert_minimal_modular_triangle_mirror_box() {
  Pattern p;
  p.name = "invert_minimal_modular_triangle_mirror_box";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {-1.0f, 0.0f}; // centre the circle on the edge midpoint 1
  p.zn.zn[2] = {4.0f, 0.0f};  // radius 2: pi/3 with y = +-1, tangent to x = -1
  p.zn.zn[3] = {1.0f, 0.0f};

  p.color.hue_rotation = 0.16666667f;
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

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.52f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("sixfold_hue_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.006f,
                            .amplitude = 0.01f,
                            .offset = 0.16666667f},
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
