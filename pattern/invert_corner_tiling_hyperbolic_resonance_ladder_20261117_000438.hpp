#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Advanced 3.5D): a Resonance Ladder of Hyperbolic Triangle Groups.
 *
 * Mathematics:
 * - T(z) = k / (z - c) + c with c = 1 + i, then the Torus fold: inversion
 *   in the circle of radius r = sqrt k about the corner c, composed with
 *   box mirrors. The circle always crosses x = 1 and y = 1 at right angles
 *   and meets the far mirrors x = -1, y = -1 at the angle theta with
 *   cos theta = 2 / r.
 * - theta = pi / n exactly when k = 4 sec^2(pi/n). Then the corner triangle
 *   at -1 - i has angles pi/2, pi/n, pi/n and the group is a discrete
 *   (2, n, n) triangle group (hyperbolic for n >= 5):
 *       n = inf 4.0000   12 4.2872   10 4.4223   8 4.6863
 *       n = 7   4.9277    6 5.3333    5 6.1115
 *   These are resonances: between them the angle is not a submultiple of
 *   pi and the group is in general not discrete.
 * - Even between the rungs the feedback stays calm: at k = 4, 4.5, 5,
 *   16/3, 5.8 and 6.11 every point of a 300 x 300 grid landed on a 2-cycle
 *   within 400 passes (at least 99.9 %).
 *
 * Pipeline: Invert, Torus, zn2 = k on a Sine lane from 4 to 6.1115 at
 * 0.003 Hz (5.6 minutes per climb and descent). A Sine lingers at its ends,
 * so the picture rests at n = infinity (tangent horocycle fans) and at
 * n = 5 (pentagonal crossings), and climbs through n = 12, 10, 8, 7, 6 in
 * between. Simplex border, OKLab hue step 1/10 by depth. In the numpy model
 * the tiles are bounded by thick red and yellow arcs that meet n at a time
 * and shrink into a fine lattice in the upper right.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Invert / Torus / Simplex / OKLabColorRotation
 * Nearest:    invert_minimal_corner_horocycle_fan holds n = infinity;
 *             invert_minimal_hexagonal_corner_mirror_arcs holds n = 6.
 *             Here the subject is the ladder between them.
 * Height:     age = passes since landing; landing deepens with n
 *             (90 % within 3 passes at k = 4, within 10 at k = 6.11).
 * Hue clock:  bounded; step 1/10.
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-17. Landing as above;
 *             64-pass exponent median 0.034 at k = 16/3 and 0.061 at 6.11,
 *             none below -0.01. A numpy model of the pass was drawn at
 *             k = 4, 4.69, 16/3 and 6.11. Not built, rendered by the engine
 *             or measured.
 * @assert p_X == 0 && p_C <= 0.05
 */
inline Pattern invert_corner_tiling_hyperbolic_resonance_ladder() {
  constexpr float K_MID = 5.0557280f;   // (4 + 4 sec^2(pi/5)) / 2
  constexpr float K_SWING = 1.0557280f; // reaches 4 and 6.1114560
  Pattern p;
  p.name = "invert_corner_tiling_hyperbolic_resonance_ladder";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {-1.0f, -1.0f};
  p.zn.zn[2] = {K_MID, 0.0f};
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
      .add("triangle_group_ladder", // k = 4 sec^2(pi/n), n from inf down to 5
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
