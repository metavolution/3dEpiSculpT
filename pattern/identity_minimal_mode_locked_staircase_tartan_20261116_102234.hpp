#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Minimal 3.5D): a Mode-Locked Tartan on the Devil's Staircase.
 *
 * Mathematics:
 * - T(z) = 0.8 z + b, wrapped by Grid. Each axis is the contracted
 *   rotation X -> a X + B (mod 1) with a = 0.8, where X = (x + 1) / 2 and
 *   B = (1 - a + b) / 2.
 * - A contracted rotation has a rotation number rho(B), and rho is a
 *   complete devil's staircase: continuous, non-decreasing, and constant
 *   on an interval of B for every rational p/q. On that interval there is
 *   one attracting cycle of period q and every orbit falls onto it. The
 *   step for p/q has width
 *       (1 - a)^2 a^(q-1) / (1 - a^q):
 *   0.200, 0.089, 0.052, 0.035, 0.024 for q = 1 ... 5 at a = 0.8, and the
 *   steps together fill the whole parameter interval. What is left over,
 *   the B with irrational rho, is a set of measure zero on which the
 *   attractor is a Cantor set (a Denjoy counterexample made by hand).
 * - The wrap line has only finitely many preimages, because T leaves a gap
 *   and the backward orbit ends when it falls in. In the p/q step there
 *   are exactly q of them, the wrap line included.
 *
 * Pipeline: Identity with zn0 = 0.8, Grid, and zn3 = b on two Triangle
 * lanes, one per axis, that sweep B from 0.1 to 0.9 at unequal rates. At
 * any moment the frame is cut into q_x by q_y rectangles, permuted by T
 * and each showing the next one magnified by 1.25, so each tile holds ring
 * waves of every age about its own centres. The tile counts step through
 * the Farey sequence as the lanes move: between 1/2 and 1/3 comes 2/5,
 * then 3/7 and 3/8. A Triangle lane spends time on each step in
 * proportion to its width, so low periods dominate and high ones flicker
 * past as fine weave.
 *
 * Fidelity:   identity (a product of two exact contracted rotations)
 * Cell:       Identity / Grid / WaveRing / SpatialWave
 * Nearest:    minimal_golden_beta_shift_fibonacci_grid expands (x -> beta
 *             x mod 1, chaotic, one tiling); kuramoto_synchronization_
 *             manifold locks phases through a smooth coupling. Here the
 *             map contracts, every state is periodic, and the subject is
 *             the staircase itself.
 * Height:     age = number of passes; magnification 1.25 per pass.
 * Hue clock:  bounded (hue_velocity_base = 0).
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-16. The width formula was
 *             checked against direct iteration for q <= 5 at a = 0.7 and
 *             0.8, the locked fraction for q <= 60 was 1.0000, and the
 *             number of cut lines equalled the period at nine values of
 *             B. Not built, rendered or measured.
 * @assert p_C >= 0.99 && lambda_med within 1 % of -0.2231
 */
inline Pattern identity_minimal_mode_locked_staircase_tartan() {
  Pattern p;
  p.name = "identity_minimal_mode_locked_staircase_tartan";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {0.8f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.8f, 0.8f}; // B = 0.5 on both axes: rho = 2/5, 25 tiles

  p.color.hue_rotation = 0.06f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.9f;
  p.color.chroma_breath_amp = 0.06f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.04f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.04f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.60f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.5f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("staircase_x", // B from 0.1 to 0.9
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.0040f,
                            .amplitude = 0.8f,
                            .offset = 0.8f},
           control::ModTarget::Zn3Re)
      .add("staircase_y",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.0031f,
                            .amplitude = 0.8f,
                            .offset = 0.8f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.03f,
                            .offset = 0.06f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
