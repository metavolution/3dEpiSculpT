#pragma once

#include "base.hpp"

namespace epi::presets {

// [2A] Gaussian-integer similarity beta=1+i (|beta|=sqrt2, 45 deg/step) about vortex z*=0.5i: seed preimages G^-k(0) form a log-spiral bead chain (pitch atan(2ln2/pi)); tau=6.
/**
 * 4x3 Orthogonal Spatial Wave Matrix -- cell 2A.
 *
 * Row 2 (Chiral / Log-Spiral Vortex) x Column A (Sparse Solitary Pulses).
 * - T(z) = (1+i) z + 1/2 (Identity, zn2 = 1+i, zn3 = 0.5): fixed point
 *   z* = 0.5/(1-(1+i)) = 0.5i is a spiral source with multiplier 1+i.
 * - Bright pixels satisfy G^k(z) ~ 0, i.e. z ~ G^-k(0): beads shrinking by
 *   1/sqrt2 and rotating -45 deg per generation along the log spiral
 *   ln r = (ln sqrt2 / (pi/4)) theta around the phase-vortex singularity.
 * - Everything with |G(z)| beyond the frame clamps onto the e^{-2}-dim
 *   seed boundary (I^2 <= 0.018): calm black background.
 * - fade = e^{-1/6} (tau = 6 steps) keeps ~4 bead generations above the
 *   dark floor (scale sqrt2^-4 = 1/4: the chain winds 180 deg into z*).
 * - OKLab rotation (mode 17, zero floor lift) accumulates spiral_a = 1/tau
 *   hue turns per revolution per step => steady charge-1 hue vortex.
 *
 * Temporal ladder: f_3 = 0.06 * 2^(3/12) = 0.07135 Hz (12-TET; no two
 * cells share a frequency). Seed hue = 3/12 on the matrix hue wheel.
 */
inline Pattern spatial_wave_chiral_vortex_solitary_beads() {
  Pattern p;
  p.name = "spatial_wave_chiral_vortex_solitary_beads";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;

  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 1.0f};
  p.zn.zn[3] = {0.5f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.8464817f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.1666667f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 0.0713524f;
  p.color.chroma_breath_phase = 6.2831853f;
  p.color.hue_velocity_base = 0.0f; // O9 bounded hue clock
  p.color.hue_velocity_breath = 0.0017933f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.25f;
  p.seed.hue_speed = 0.025f;
  p.seed.alpha = 0.98f;
  p.seed.pulse_freq = 0.0713524f;
  p.seed.pulse_amp = 0.0f; // envelope == 1: normalized, non-strobing injection

  p.speed = 1.0f;

  return p;
}

} // namespace epi::presets
