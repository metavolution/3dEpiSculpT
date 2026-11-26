#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Minimal 3.5D): Mirror Arcs of the (2, 6, 6) Triangle Group.
 *
 * Mathematics:
 * - T(z) = k / (z - c) + c with c = 1 + i, then the Torus fold: inversion
 *   in the circle of radius sqrt k about the corner c, reflection in the
 *   mirror y = 1, and the box mirrors. Every pass is an element of the
 *   reflection group of the box and the circle.
 * - With sqrt k = r = 2 sec(pi/6) = 2.3094 (k = 16/3) the circle crosses
 *   the far mirrors x = -1 and y = -1 at exactly pi/6, and the near ones at
 *   right angles. Inside the box and outside the circle is one triangle at
 *   the corner -1 - i with angles pi/2, pi/6, pi/6: a hyperbolic (2, 6, 6)
 *   triangle, and a discrete group (Poincare). Six copies meet at each
 *   crossing point.
 *
 * Pipeline: Invert with zn0 = 1, zn1 = -c, zn2 = 16/3, zn3 = c; Torus. The
 * Cross seed draws the two axes in cyan and red (Cross gives the axes hues
 * half a turn apart), and the group bends them into arcs that meet six at
 * a time and shrink towards the upper right, where the limit set lies.
 * OKLab hue step 1/8 by depth. The image is stationary after a few dozen
 * passes; only the hue drifts.
 *
 * Fidelity:   identity
 * Cell:       Invert / Torus / Cross / OKLabColorRotation
 * Nearest:    invert_minimal_corner_horocycle_fan is the same corner circle
 *             at r = 2, tangent (n = infinity) instead of crossing at pi/6;
 *             invert_corner_tiling_hyperbolic_resonance_ladder sweeps r.
 * Height:     age = passes since landing.
 * Hue clock:  bounded; step 1/8 +- 0.01.
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-17. On a 300 x 300 grid in
 *             double precision 100 % of points were on a 2-cycle within 400
 *             passes (median landing 1, 90 % within 6); 64-pass exponent
 *             median 0.034, none below -0.01. A numpy model of the pass
 *             was drawn to check composition. Not built, rendered by the
 *             engine or measured.
 * @assert p_X == 0 && p_C <= 0.05
 */
inline Pattern invert_minimal_hexagonal_corner_mirror_arcs() {
  Pattern p;
  p.name = "invert_minimal_hexagonal_corner_mirror_arcs";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {-1.0f, -1.0f};
  p.zn.zn[2] = {5.3333333f, 0.0f}; // (2 sec(pi/6))^2: far mirrors crossed at pi/6
  p.zn.zn[3] = {1.0f, 1.0f};

  p.color.hue_rotation = 0.125f;
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
      .add("arc_hue_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.006f,
                            .amplitude = 0.01f,
                            .offset = 0.125f},
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
