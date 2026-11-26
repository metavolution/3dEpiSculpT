#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimal): Three-Gap Theorem on a Kronecker Torus Translation.
 *
 * Scientific foundation:
 * - Steinhaus' three-gap theorem (proved by Sos, Swierczkowski, Suranyi,
 *   1957-58): the points {n alpha mod 1}, n = 1..N, cut the circle into gaps
 *   of at most three distinct lengths, the largest being the sum of the other
 *   two. For the golden rotation the gaps follow the Fibonacci word.
 * - Kronecker-Weyl: an irrational translation of the torus equidistributes.
 *   Two independent rotations give the square Fibonacci quasi-lattice
 *   (Lifshitz 2002), a genuine quasicrystal model with 4-fold symmetry.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Identity with zn0 = 1: T(z) = z + v, a pure translation. There is
 *   no stretching at all; the entire picture comes from the Reduce stage.
 * - Reduce = Grid, period 2: the translation becomes a rotation of the torus.
 *   v = (2/phi^2, 2(sqrt(2) - 1)): golden step in x, silver step in y.
 * - Seed = thin Simplex border = the two wrap lines of the torus. A pixel is
 *   lit at generation n when z + n v lands on a wrap line, so the visible
 *   lines sit at x = -n * alpha and y = -n * beta (mod 2): the Kronecker
 *   sequence itself. Line brightness is fade^n and hue advances once per n, so
 *   the reading order of the sequence is visible.
 * - Fade = 0.94 keeps about 35 terms per axis, enough to see three (and only
 *   three) gap widths without the lines merging.
 * - Motion: Zn3Re drifts alpha across the convergents 3/8 and 5/13 of the
 *   golden ratio. At a rational step the lines collapse onto q positions, a
 *   resonance that opens and closes as alpha moves on.
 */
inline Pattern spatial_wave_minimal_three_gap_kronecker() {
  constexpr float GOLDEN_STEP = 0.76393202f; // 2 / phi^2
  constexpr float SILVER_STEP = 0.82842712f; // 2 * (sqrt(2) - 1)

  Pattern p;
  p.name = "spatial_wave_minimal_three_gap_kronecker";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {GOLDEN_STEP, SILVER_STEP};

  p.color.hue_rotation = 0.021f; // hue encodes the index n of each line
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.94f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 1.0f;
  p.color.chroma_breath_amp = 0.04f;
  p.color.chroma_breath_freq = 3.0f;
  p.color.chroma_breath_phase = 0.025f;
  p.color.hue_velocity_base = 0.006f;
  p.color.hue_velocity_breath = 0.004f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.014f;
  p.seed.hue = 0.12f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.7f;

  p.motion
      .add("rotation_number_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.013f,
                            .amplitude = 0.02f,
                            .offset = GOLDEN_STEP},
           control::ModTarget::Zn3Re)
      .add("index_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.017f,
                            .amplitude = 0.009f,
                            .offset = 0.021f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
