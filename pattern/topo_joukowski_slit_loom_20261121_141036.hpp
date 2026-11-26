#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Two foci pin the inverse Joukowski coordinate net. Across the connecting slit the limiting
 * angular coordinates lie on opposite banks.
 *
 * Fidelity: A, analytic field. Elliptic coordinates on the inverse Joukowski cover.
 * Cell: Identity / Torus / JoukowskiSheets. Seed width controls focal separation.
 * Nearest: spatial_wave_minimal_confocal_saddle (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern topo_joukowski_slit_loom() {
  Pattern p;
  p.name = "topo_joukowski_slit_loom";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.seed.type = SeedType::JoukowskiSheets;
  p.seed.border_width = 0.54f;
  p.seed.hue = 0.71f;
  p.seed.alpha = 0.67f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.87f;
  p.color.sat_scale = 1.0f;
  p.color.hue_rotation = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.hue_velocity_breath = 0.001f;
  p.color.chroma_breath_freq = 0.07f;
  return p;
}

} // namespace epi::presets
