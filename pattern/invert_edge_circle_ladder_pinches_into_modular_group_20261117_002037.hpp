#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Advanced 3.5D): a Ladder of Hyperbolic Quadrilaterals that
 * Pinches into the Modular Triangle.
 *
 * Mathematics:
 * - T(z) = k / (z - 1) + 1, then the Torus fold: inversion in the circle of
 *   radius r = sqrt k about the edge midpoint 1, reflection in the real
 *   axis, box mirrors. The circle meets y = +-1 at the angle arccos(1/r)
 *   and x = 1 and the real axis at right angles.
 * - For r < 2 the circle misses x = -1 and the box minus the disc is one
 *   quadrilateral with angles pi/2, pi/2 at -1 +- i and arccos(1/r) at the
 *   two crossings. At k = sec^2(pi/n) those angles are pi/n and the
 *   reflection group of the quadrilateral is discrete:
 *       n = inf 1.0000   12 1.0718   8 1.1716   6 1.3333
 *       n = 5   1.5279    4 2.0000   3 4.0000
 * - At k = 4 (n = 3) the circle also touches x = -1, at -1. The
 *   quadrilateral pinches there into two (2, 3, infinity) triangles: the
 *   modular group. A new cusp is born and the fundamental domain falls
 *   into two pieces: a change of topology at the top of the ladder.
 * - For every k <= 4, a point z of the quadrilateral has |z - 1| > r, its
 *   image 1 + k conj(z - 1) / |z - 1|^2 has real part in (1 - r, 1] and
 *   imaginary part below k / |z - 1|^2 < 1 in size, so it stays in the box;
 *   the fold does nothing and the next pass returns z. Period 2 holds at
 *   every rung and between them.
 *
 * Pipeline: Invert, Torus, zn2 = k on a Sine lane from 1 to 4 at 0.003 Hz.
 * A Sine lingers at its ends: at k = 1 the circle touches y = +-1 at the
 * corners 1 +- i (two cusps, n = infinity); at k = 4 it touches x = -1 (the
 * modular pinch). In between it climbs through n = 12, 8, 6, 5 and 4.
 * Simplex border: the lit band runs along the quadrilateral's straight
 * sides, so every tile shows a lit outline. OKLab hue step 1/10 by depth.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Invert / Torus / Simplex / OKLabColorRotation
 * Nearest:    invert_corner_tiling_hyperbolic_resonance_ladder climbs
 *             (2, n, n) triangles with the circle on a corner; here the
 *             circle sits on an edge and the ladder ends in a pinch.
 * Height:     age = passes since reaching the quadrilateral.
 * Hue clock:  bounded; step 1/10.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17 (angles, rungs, the
 *             pinch and the period-2 property are proved above). Between
 *             rungs the group need not be discrete, and that every point
 *             reaches the quadrilateral is not proved. Not built, rendered
 *             or measured.
 * @assert p_X == 0 && p_C <= 0.05
 */
inline Pattern invert_edge_circle_ladder_pinches_into_modular_group() {
  constexpr float K_MID = 2.5f;
  constexpr float K_SWING = 1.5f; // k from 1 (n = inf) to 4 (n = 3)
  Pattern p;
  p.name = "invert_edge_circle_ladder_pinches_into_modular_group";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {-1.0f, 0.0f};
  p.zn.zn[2] = {K_MID, 0.0f};
  p.zn.zn[3] = {1.0f, 0.0f};

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
  p.seed.hue = 0.6f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("quadrilateral_ladder", // k = sec^2(pi/n), pinch at k = 4
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
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
