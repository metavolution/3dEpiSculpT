#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Minimal 3.5D: A Quarter Turn with a Fault, on the Torus. A Finite Quilt.
 *
 * Mathematics:
 * - SymmetryBreak slides the upper half of its input 0.1 along the dividing
 *   line. With zn0 = 0.2 and zn2 = 5i,
 *       T(z) = i (z + d H(z)) + zn3,   d = 0.5,
 *   where H is 1 on the band 0 < Im z < 1 and 0 below it. Grid wraps the
 *   result, and both the slide and the quarter turn are bijections of the
 *   torus, so T is an invertible piecewise isometry of the whole frame.
 * - Four steps turn through a full circle, and 1 + i + i^2 + i^3 = 0, so
 *   zn3 cancels: T^4 is a translation by a sum of terms +-d and +-i d, a
 *   vector of the lattice (1/2) Z[i]. On the torus of side 2 that lattice
 *   has 16 points. Hence every point of the frame is periodic, with period
 *   at most 64, for every value of zn3.
 * - The frame is therefore cut into finitely many rectangles, each carried
 *   rigidly round a cycle and returned exactly. For the values tried the
 *   periods are 4, 12, 16, 20 and 28. Nothing is expanded, nothing is
 *   lost, and there is no fractal remainder: the 90 degree case is
 *   crystallographic, where the 72 degree case is not.
 * - zn2 = (0, 5) is exact in binary floating point, so the turn is exact.
 *
 * Pipeline: SymmetryBreak / Grid / WaveRing. On a cell of period q the
 * picture is q ring sources, repeated exactly. The hue step is 1/4 turn
 * and every period is a multiple of 4, so each source keeps one of four
 * hues. Two lanes in quadrature carry zn3 round a circle of radius 0.2;
 * the quilt is re-cut continuously and stays exact throughout.
 *
 * Fidelity:   identity
 * Cell:       SymmetryBreak / Grid / WaveRing / Opponent
 * Nearest:    spatial_wave_goetz_pentagon_piecewise_rotation_mosaic is the
 *             same slip with a 72 degree turn on the plane: self-similar,
 *             periods unbounded, exact only where orbits stay in frame.
 *             Here the turn is 90 degrees on the torus: finite, and exact
 *             at every pixel.
 * Height:     age, which on a cell of period q repeats with period q.
 * Hue clock:  bounded (hue_velocity_base = 0).
 * Fastest lane: 0.023 Hz
 * Status:     designed from source on 2026-11-15; the map was iterated in
 *             a separate arithmetic check (all points periodic, periods as
 *             above); not built, rendered or measured.
 * @assert p_N >= 0.99 && rot.q == 4
 */
inline Pattern symmetry_break_minimal_quarter_turn_torus_quilt() {
  Pattern p;
  p.name = "symmetry_break_minimal_quarter_turn_torus_quilt";
  p.inner = InnerFunction::SymmetryBreak;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {0.2f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.0f, 5.0f}; // i / 0.2
  p.zn.zn[3] = {0.2f, 0.0f};

  p.color.hue_rotation = 0.25f; // a quarter turn of hue per pass
  p.color.sat_scale = 0.98f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.04f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.004f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.52f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 0.5f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("cut_offset_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 0.2f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Re)
      .add("cut_offset_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 0.2f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.023f,
                            .amplitude = 0.02f,
                            .offset = 0.04f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
