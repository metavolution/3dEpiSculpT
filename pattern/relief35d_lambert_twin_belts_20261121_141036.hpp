#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Two equal-area latitude bands slide around a tilted sphere. Age rises between the bands
 * and preserves the outline of the disk as it turns.
 *
 * Fidelity: A, analytic field. Equal-area projection of two latitude ribbons.
 * Cell: Identity / Torus / LambertBelt. Seed width controls absolute latitude of both belts.
 * Nearest: spatial_wave_riemann_sphere_golden_rotation (curatorial comparison).
 * Height: Relief observer samples retained age and luminance.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern relief35d_lambert_twin_belts() {
  Pattern p;
  p.name = "relief35d_lambert_twin_belts";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.seed.type = SeedType::LambertBelt;
  p.seed.border_width = 0.88f;
  p.seed.hue = 0.49f;
  p.seed.alpha = 0.76f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.86f;
  p.color.sat_scale = 1.0f;
  p.color.hue_rotation = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.hue_velocity_breath = 0.001f;
  p.color.chroma_breath_freq = 0.07f;
  p.relief_35d = true;
  p.relief_depth_range = 1.2f;
  return p;
}

} // namespace epi::presets
