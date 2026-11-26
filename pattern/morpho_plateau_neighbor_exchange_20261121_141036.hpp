#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Two trivalent vertices collapse and reopen across the other axis. Retained light records
 * the orientation that the current four-cell junction has lost.
 *
 * Fidelity: A, analytic field. Four-cell T1 edge exchange with Plateau junction angles.
 * Cell: Identity / Torus / FoamExchange. Seed width controls maximum length of the exchanging edge.
 * Nearest: field_voronoi_boundary_migration_chronograph (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern morpho_plateau_neighbor_exchange() {
  Pattern p;
  p.name = "morpho_plateau_neighbor_exchange";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.987f, -0.015f};
  p.zn.zn[3] = {0.003f, 0.0f};

  p.seed.type = SeedType::FoamExchange;
  p.seed.border_width = 0.74f;
  p.seed.hue = 0.92f;
  p.seed.alpha = 0.38f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.965f;
  p.color.sat_scale = 1.0f;
  p.color.hue_rotation = 0.014f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.hue_velocity_breath = 0.001f;
  p.color.chroma_breath_freq = 0.07f;
  return p;
}

} // namespace epi::presets
