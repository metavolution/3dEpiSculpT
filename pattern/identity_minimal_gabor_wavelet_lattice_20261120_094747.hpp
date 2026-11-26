#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Showcase of the GaborLattice seed (seed id 6), rendered nearly bare.
 *
 * - T(z) = A sin(eps z) with eps = pi/4, A = 4/pi: T'(0) = 1 and the cubic warp
 *   stays under 5% inside |z| < 1, so the 4x4 lattice of Gabor atoms (3 carrier
 *   cycles per cell under a Gaussian envelope, sigma ~ 1/6 cell) keeps its
 *   square symmetry. sin is injective on |Re w| < pi/2: no fold.
 * - The carriers rotate together, theta = 0.3 t, so the lattice turns through
 *   every orientation with period pi/0.3 (the carrier is symmetric under +pi);
 *   feedback (fade e^(-1/4), tau = 4) leaves short rotational trails.
 * - alpha = 0.7178 pins the steady peak at P = 0.92: a = P(1-g)/(1-Pg), g = fade.
 */
inline Pattern identity_minimal_gabor_wavelet_lattice() {
  Pattern p;
  p.name = "identity_minimal_gabor_wavelet_lattice";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {0.7853982f, 0.0f}; // eps = pi/4
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.2732395f, 0.0f}; // A = 4/pi
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.7788008f;
  p.color.color_mode = as_int(ColorMode::LumaLockedHueDrift);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.06f;
  p.color.chroma_breath_freq = 0.07f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.1099557f; // +-0.25 turn, bounded
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;

  p.seed.type = SeedType::GaborLattice;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.08f;                // amber atoms; carrier sign shifts hue by +-0.1
  p.seed.hue_speed = 0.01f;
  p.seed.alpha = 0.7178163f;         // peak-normalized, P = 0.92
  p.seed.pulse_freq = 0.07f;
  p.seed.pulse_amp = 0.0f;

  p.speed = 0.6f;

  return p;
}

} // namespace epi::presets
