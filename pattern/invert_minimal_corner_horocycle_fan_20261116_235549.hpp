#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Minimal 3.5D): Horocycle Fans from a Corner Mirror.
 *
 * Mathematics:
 * - T(z) = 4 / (z - c) + c with c = 1 + i, followed by the Torus fold. This
 *   is inversion in the circle of radius 2 about the corner c, then
 *   reflection in the horizontal line through c, which is the mirror
 *   y = 1. Each pass is again an element of a reflection group: the four
 *   box mirrors and one circle.
 * - The circle crosses x = 1 and y = 1 at right angles (they pass through
 *   its centre) and touches x = -1 at -1 + i and y = -1 at 1 - i. Those two
 *   frame corners are parabolic cusps. Inside the box and outside the circle
 *   is a single triangle at the corner -1 - i, angles pi/2, 0, 0.
 * - On that triangle the conjugation in 4 / (z - c) is undone by the fold
 *   across y = 1, so T is the pure inversion there and T(T(z)) = z: period
 *   2. Every other point reaches it in finitely many passes.
 *
 * Pipeline: Invert with zn0 = 1, zn1 = -c, zn2 = 4, zn3 = c, and Torus. The
 * Simplex border band lies along the triangle's two straight sides, so
 * every tile that is an image of the triangle carries a lit edge. The tiles
 * shrink into the two cusps as fans of tangent circles (horocycles),
 * terraced in hue by depth with step 1/10, and the big black triangle in
 * the lower left is the one tile that needs no pass at all.
 *
 * Fidelity:   identity
 * Cell:       Invert / Torus / Simplex / OKLabColorRotation
 * Nearest:    invert_minimal_ford_mirror_box_circle_web puts the circle in
 *             the centre with radius 1, which gives four such triangles and
 *             four cusps in a symmetric rosette. Here one large circle
 *             leaves one triangle and the picture is seen from two cusps.
 * Height:     age = passes since landing.
 * Hue clock:  bounded; step 1/10 +- 0.01.
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-16. On a 400 x 400 grid in
 *             double precision, 100 % of points were on a 2-cycle within
 *             400 passes (median landing 1, 90 % within 4); exponent median
 *             0.008, none below -0.01. A numpy model of the pass was drawn
 *             to check composition. Not built, rendered by the engine or
 *             measured.
 * @assert p_X == 0 && p_C <= 0.05
 */
inline Pattern invert_minimal_corner_horocycle_fan() {
  Pattern p;
  p.name = "invert_minimal_corner_horocycle_fan";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {-1.0f, -1.0f}; // centre the circle on the corner c = 1 + i
  p.zn.zn[2] = {4.0f, 0.0f};   // radius squared: tangent to x = -1 and y = -1
  p.zn.zn[3] = {1.0f, 1.0f};

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
      .add("fan_hue_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.006f,
                            .amplitude = 0.01f,
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
