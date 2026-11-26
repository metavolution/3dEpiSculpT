#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Growth shells are levels of an octagonal support gauge. A slight rotation per step
 * preserves ghosts of the disappearing diagonal facets.
 *
 * Fidelity: A, analytic field. Intersection of two competing families of crystal support planes.
 * Cell: Identity / Torus / WulffFacets. Seed width controls diagonal facet support.
 * Nearest: morpho_voronoi_crystal_grains (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern morpho_wulff_facet_competition() {
  Pattern p;
  p.name = "morpho_wulff_facet_competition";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.997f, 0.03f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.seed.type = SeedType::WulffFacets;
  p.seed.border_width = 0.72f;
  p.seed.hue = 0.53f;
  p.seed.alpha = 0.62f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.92f;
  p.color.sat_scale = 1.0f;
  p.color.hue_rotation = 0.014f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.hue_velocity_breath = 0.001f;
  p.color.chroma_breath_freq = 0.07f;
  return p;
}

} // namespace epi::presets
