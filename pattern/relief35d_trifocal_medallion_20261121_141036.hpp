#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Each terrace has constant total distance to three foci. One moving focus shifts the
 * balance between round outer rings and triangular inner terraces.
 *
 * Fidelity: A, analytic field. Level sets of the sum of distances to three foci.
 * Cell: Identity / Torus / TrifocalGauge. Seed width controls height of the third focus.
 * Nearest: spatial_wave_minimal_confocal_saddle (curatorial comparison).
 * Height: Relief observer samples retained age and luminance.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern relief35d_trifocal_medallion() {
  Pattern p;
  p.name = "relief35d_trifocal_medallion";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.seed.type = SeedType::TrifocalGauge;
  p.seed.border_width = 0.24f;
  p.seed.hue = 0.81f;
  p.seed.alpha = 0.64f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.89f;
  p.color.sat_scale = 1.0f;
  p.color.hue_rotation = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.hue_velocity_breath = 0.001f;
  p.color.chroma_breath_freq = 0.07f;
  p.relief_35d = true;
  p.relief_depth_range = 0.85f;
  return p;
}

} // namespace epi::presets
