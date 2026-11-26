#pragma once

#include "base.hpp"

namespace epi::presets {

// [3A] D6 lattice: w=sqrt3*z^3+1 (fold-crest centring) => S(R(w)) even in w-1 but not 90-deg symmetric: exact z->e^{i pi/3}z invariance; 1.6% odd-level band (w=0.008) lights, tau=1.5.
/**
 * 4x3 Orthogonal Spatial Wave Matrix -- cell 3A.
 *
 * Row 3 (Crystallographic, 6-fold) x Column A (Sparse Solitary Pulses).
 * - T(z) = sqrt3 z^3 + 1 (Cube, zn2 = sqrt3 = |1 + e^{i pi/3}|, the hexagonal
 *   lattice ratio; zn3 = 1 = Torus fold crest). Every seed is D4-symmetric,
 *   so without the shift S(R(i w)) = S(R(w)) would make the pattern 12-fold.
 *   The triangle fold is EVEN about the crest Re w = 1 (tri(1+u) = 1-|u|)
 *   and odd in Im w, so S(R(1 + w)) = S(R(1 - w)) but not S(R(1 + i w)):
 *   z -> e^{i pi/3} z (w -> -w) is an exact symmetry, z -> e^{i pi/6} z is
 *   not, plus conjugation => exactly the D6 point group.
 * - Simplex border w = 0.008 is nonzero only within +-w of odd Torus level
 *   sets (1 - 0.992^2 = 1.6% of the folded measure, keeping the cubic
 *   inflection disk |sqrt3 z^3| < w tight at r < 0.167): razor-thin hexagonal
 *   node curves on >=88% dark background. fade = e^{-1/1.5} (tau = 1.5 steps).
 * - Zn2Re lane breathes the lattice constant sqrt3*(1 +- 0.05) at f_6 (a
 *   real scale keeps the D6 invariance exact): node curves travel radially
 *   as solitary pulses along the rosette arms.
 * - PhantomFractal (mode 16, luma_d = 0) quantizes hue into 12 sectors (2x6)
 *   with spiral_a = 6 hue turns per revolution: polychromatic D6.
 *
 * Temporal ladder: f_6 = 0.06 * 2^(6/12) = 0.08485 Hz (12-TET; no two
 * cells share a frequency). Seed hue = 6/12 on the matrix hue wheel.
 */
inline Pattern spatial_wave_hex_lattice_solitary_nodes() {
  Pattern p;
  p.name = "spatial_wave_hex_lattice_solitary_nodes";
  p.inner = InnerFunction::Cube;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.7320508f, 0.0f};
  p.zn.zn[3] = {1.0f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.5134171f;
  p.color.color_mode = as_int(ColorMode::PhantomFractal);
  p.color.spiral_angle_coeff = 6.0f;
  p.color.spiral_radius_coeff = 0.9102392f;
  p.color.chroma_breath_amp = 0.2f;
  p.color.chroma_breath_freq = 0.0848528f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f; // O9 bounded hue clock
  p.color.hue_velocity_breath = 0.1332865f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.008f;
  p.seed.hue = 0.5f;
  p.seed.hue_speed = 0.022f;
  p.seed.alpha = 0.98f;
  p.seed.pulse_freq = 0.0848528f;
  p.seed.pulse_amp = 0.0f; // envelope == 1: normalized, non-strobing injection

  p.speed = 1.0f;

  p.motion
      .add("hex_lattice_breathing",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0848528f,
                            .amplitude = 0.0866025f,
                            .offset = 1.7320508f},
           control::ModTarget::Zn2Re);

  return p;
}

} // namespace epi::presets
