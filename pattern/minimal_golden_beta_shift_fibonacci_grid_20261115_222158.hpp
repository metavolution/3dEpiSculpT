#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Minimal 3.5D: The Golden Beta-Shift Draws a Fibonacci Grid.
 *
 * Mathematics:
 * - x -> beta x, wrapped back into [-1, 1), is a beta-transformation
 *   (Renyi 1957, Parry 1960). For most beta the orbit of the wrap point
 *   never closes. For beta = phi = 1.6180340 the wrap point, approached
 *   from either side, returns after three steps (1 and -1 are one point):
 *     1 -> -1/phi^2 -> -1/phi -> -1      (from the left)
 *    -1 ->  1/phi^2 ->  1/phi ->  1      (from the right)
 *   so the map is Markov with cut points +-1/phi^2, +-1/phi, +-1.
 * - On that partition, with A a cell of length 1/phi^2, B of length
 *   1/phi^3 and C the central cell, one step acts as
 *     A -> A B,   B -> A,   C -> B C B.
 *   The first two rules are the Fibonacci substitution. After n steps every
 *   cell away from the centre has one of two lengths in the ratio phi, in
 *   the order of a Fibonacci word.
 * - 0 is fixed and has no other pre-image, so the central cell is never
 *   cut: it shrinks by phi each step and the lines close in on the two
 *   axes at +-1/phi^n.
 * - phi is real, so the plane map z -> phi z is this map on each axis and
 *   the picture is the product: squares and golden rectangles, the square
 *   Fibonacci tiling.
 *
 * Pipeline: Identity with zn0 = phi, Grid, Simplex border. The border is
 * the wrap line, so generation n lights the n-step pre-images of the wrap
 * line, each generation thinner by phi. The hue step per pass is 1/phi^2
 * of a turn (the golden angle), which keeps consecutive generations far
 * apart in hue. One lane moves beta by +-0.005 about phi: at the centre of
 * the swing the grid locks, away from it deep lines split into doublets.
 *
 * Fidelity:   identity
 * Cell:       Identity / Grid / Simplex / Opponent
 * Nearest:    spatial_wave_minimal_three_gap_kronecker is a translation
 *             (zero entropy, three gap lengths); this is an expanding
 *             Markov map (entropy ln phi per axis, two cell lengths).
 * Height:     age = generation of a line = level of the Markov refinement.
 * Hue clock:  bounded (hue_velocity_base = 0).
 * Fastest lane: 0.027 Hz
 * Status:     designed from source on 2026-11-15; not built, rendered or
 *             measured.
 * @assert p_E >= 0.99 && lambda_med within 1 % of 0.4812
 */
inline Pattern minimal_golden_beta_shift_fibonacci_grid() {
  Pattern p;
  p.name = "minimal_golden_beta_shift_fibonacci_grid";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {1.6180340f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.38196601f; // 1 / phi^2 of a turn per pass
  p.color.sat_scale = 0.97f;
  p.color.fade = 0.86f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.004f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.11f;
  p.seed.hue_speed = 0.01f;
  p.seed.alpha = 0.95f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("markov_detune",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.006f,
                            .amplitude = 0.005f,
                            .offset = 1.6180340f},
           control::ModTarget::Zn0Re)
      .add("line_weight",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.021f,
                            .amplitude = 0.02f,
                            .offset = 0.05f},
           control::ModTarget::SeedWidth)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.027f,
                            .amplitude = 0.03f,
                            .offset = 0.05f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
