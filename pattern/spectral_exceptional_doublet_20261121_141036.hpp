#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Read the real splitting and the linewidth splitting as crossed contours. Encircling either
 * exceptional point winds the discriminant phase once.
 *
 * Fidelity: A, analytic field. Complex splitting of a balanced gain-loss mode pair.
 * Cell: Identity / Torus / ExceptionalDoublet. Seed width controls coupling between the modes.
 * Nearest: spectral_polarization_lemon_star_field (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern spectral_exceptional_doublet() {
  Pattern p;
  p.name = "spectral_exceptional_doublet";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.seed.type = SeedType::ExceptionalDoublet;
  p.seed.border_width = 0.4f;
  p.seed.hue = 0.67f;
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
