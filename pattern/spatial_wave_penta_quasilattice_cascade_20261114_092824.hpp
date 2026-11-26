#pragma once

#include "base.hpp"

namespace epi::presets {

// [3C] Order-5 rotation beta=e^{2pi i/5} (|beta|=1, Lambda=0: zero minification => zero aliasing) symmetrizes the 4-octave 1/f fBm lattice seed into a 5-fold quasilattice; q^4>=0.9.
/**
 * 4x3 Orthogonal Spatial Wave Matrix -- cell 3C.
 *
 * Row 3 (Crystallographic, 5-fold) x Column C (Harmonic Cascade).
 * - T(z) = omega z, omega = e^{2 pi i / 5} (Identity, zn2 = omega): beta^5 = 1
 *   so the feedback superposes exactly five rotated copies of the seed.
 * - OrganicNoise with w = 1 is a 4-octave value-noise fBm on the integer
 *   lattice (amplitude 1/2 per octave => 1/f amplitude spectrum, top octave
 *   32 cycles/unit = 64 cycles/frame << 256 Nyquist at 512^2).
 * - |beta| = 1 means Lambda = 0: the map never minifies, so no copy is ever
 *   pushed above Nyquist (L4 aliasing term identically zero).
 * - Per-orientation weights decay by q = fade*(1 - a_bar) per step; with
 *   fade = 0.99 and alpha = 0.06 (a_bar ~ 0.015), q^4 ~ 0.904 >= 0.9 keeps the
 *   five orientations within 10% => near-exact C5 rosette quasilattice.
 * - QuasiPeriodicWave (mode 14) with spiral_a = 5/sqrt2 makes its
 *   sqrt2*spiral_a*angle wave exactly 5-fold: polychromatic pentagonal hue.
 *
 * Temporal ladder: f_8 = 0.06 * 2^(8/12) = 0.09524 Hz (12-TET; no two
 * cells share a frequency). Seed hue = 8/12 on the matrix hue wheel.
 */
inline Pattern spatial_wave_penta_quasilattice_cascade() {
  Pattern p;
  p.name = "spatial_wave_penta_quasilattice_cascade";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.309017f, 0.9510565f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.99f;
  p.color.color_mode = as_int(ColorMode::QuasiPeriodicWave);
  p.color.spiral_angle_coeff = 3.5355339f;
  p.color.spiral_radius_coeff = 0.5f;
  p.color.chroma_breath_amp = 0.25f;
  p.color.chroma_breath_freq = 0.0952441f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f; // O9 bounded hue clock
  p.color.hue_velocity_breath = 0.149609f;
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;

  p.seed.type = SeedType::OrganicNoise;
  p.seed.border_width = 1.0f;
  p.seed.hue = 0.6666667f;
  p.seed.hue_speed = 0.008f;
  p.seed.alpha = 0.06f;
  p.seed.pulse_freq = 0.0952441f;
  p.seed.pulse_amp = 0.0f; // envelope == 1: normalized, non-strobing injection

  p.speed = 0.75f;

  return p;
}

} // namespace epi::presets
