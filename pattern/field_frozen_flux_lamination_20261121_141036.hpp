#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Pull a flux function through two area-preserving shears. Its contours foliate the
 * transported cells and its perpendicular gradient is divergence-free.
 *
 * Fidelity: A, analytic field. Flux contours transported by two exact incompressible shears.
 * Cell: Identity / Torus / FrozenFlux. Seed width controls shear displacement.
 * Nearest: field_multipole_conformal_net (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern field_frozen_flux_lamination() {
  Pattern p;
  p.name = "field_frozen_flux_lamination";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.997f, 0.03f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.seed.type = SeedType::FrozenFlux;
  p.seed.border_width = 0.8f;
  p.seed.hue = 0.54f;
  p.seed.alpha = 0.61f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.92f;
  p.color.sat_scale = 1.0f;
  p.color.hue_rotation = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.hue_velocity_breath = 0.001f;
  p.color.chroma_breath_freq = 0.07f;
  return p;
}

} // namespace epi::presets
