#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Advanced 3.5D: A Conformal Carpet. Every Tile Holds the Whole Picture.
 *
 * Mathematics:
 * - T(z) = k z^2 + c, wrapped back into the frame by Grid. The wrap lines
 *   pull back to two orthogonal families of hyperbolas,
 *       k (x^2 - y^2) + c = odd,     2 k x y = odd,
 *   which cut the frame into curved quadrilaterals, about 8 k^2 / 3 of
 *   them (24 at k = 3), small at the rim and large toward the centre.
 * - T is holomorphic, and it maps each quadrilateral one to one onto the
 *   whole frame. The picture F satisfies F = seed + colour(F o T), so each
 *   quadrilateral holds a conformal copy of the entire picture, each of
 *   whose quadrilaterals holds another. The self-similarity is exact and
 *   it is not a similarity: the copies are bent.
 * - At the centre nothing wraps. With c = 0 the disc |z| < 1 / k is the
 *   basin of the superattracting point 0 and its edge is the Julia set of
 *   k z^2, a circle. Inside, generation n is a ring wave in
 *   k^(2^n - 1) |z|^(2^n): zone plates of rising order. This is the eye.
 * - Every tile contains a copy of the eye, and so does every tile of every
 *   tile. What lies between the eyes is a carpet: the set of points whose
 *   orbits are never captured. At k = 3, c = 0 the eyes of the first
 *   twelve levels cover 57 % of the frame, rising by a few per cent with
 *   each level.
 * - T(-z) = T(z), and for real k and c also T(conj z) = conj T(z). With
 *   the radial seed these give F(-1, y) = F(1, y) and F(x, -1) = F(x, 1):
 *   the picture is continuous across the wrap, so tile edges show as
 *   mirror seams and not as jumps. For that reason both lanes are real.
 *
 * Pipeline: Square / Grid / WaveRing. One lane takes k over 2.5 .. 3.5, so
 * tiles are born at the rim and travel inward along the hyperbolas. The
 * other takes c over -0.18 .. 0.06; then the eye is the filled Julia set
 * of w^2 + k c with k c between -0.63 and 0.21, inside the main cardioid,
 * and every eye in the carpet changes shape together.
 *
 * Fidelity:   identity
 * Cell:       Square / Grid / WaveRing / OKLab
 * Nearest:    hurwitz_complex_continued_fraction has the same principle
 *             with 1 / z, which has no attracting basin and so no holes;
 *             square_fresnel_edge_diffraction is Square on Grid below unit
 *             gain, read as an optical analogy.
 * Height:     age = the level of the tile a point lies in.
 * Hue clock:  bounded (hue_velocity_base = 0).
 * Fastest lane: 0.022 Hz
 * Status:     designed from source on 2026-11-15; the covered fraction is
 *             from a separate arithmetic check; not built, rendered or
 *             measured.
 * @assert winding == 2 && sing[0].period == 1 && p_E >= 0.85
 */
inline Pattern square_hyperbolic_lattice_carpet_of_eyes() {
  Pattern p;
  p.name = "square_hyperbolic_lattice_carpet_of_eyes";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {3.0f, 0.0f};
  p.zn.zn[3] = {-0.06f, 0.0f};

  p.color.hue_rotation = 0.09f;
  p.color.sat_scale = 0.97f;
  p.color.fade = 0.9f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.04f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.47f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.6f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("lattice_gain_k",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.005f,
                            .amplitude = 0.5f,
                            .offset = 3.0f},
           control::ModTarget::Zn2Re)
      .add("eye_shape_c",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.008f,
                            .amplitude = 0.12f,
                            .offset = -0.06f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Re)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.022f,
                            .amplitude = 0.03f,
                            .offset = 0.05f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
