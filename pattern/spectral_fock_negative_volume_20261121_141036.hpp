#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Three radial sign changes distinguish negative and positive Wigner lobes. A displaced,
 * squeezed oscillator ellipse moves across its previous positions.
 *
 * Fidelity: A, analytic field. Signed third oscillator-state Wigner density.
 * Cell: Identity / Torus / FockWigner. Seed width controls position/momentum display scales.
 * Nearest: spatial_wave_laguerre_gauss_vortex (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern spectral_fock_negative_volume() {
  Pattern p;
  p.name = "spectral_fock_negative_volume";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.seed.type = SeedType::FockWigner;
  p.seed.border_width = 0.62f;
  p.seed.hue = 0.07f;
  p.seed.alpha = 0.72f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.88f;
  p.color.sat_scale = 1.0f;
  p.color.hue_rotation = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.hue_velocity_breath = 0.001f;
  p.color.chroma_breath_freq = 0.07f;
  return p;
}

} // namespace epi::presets
