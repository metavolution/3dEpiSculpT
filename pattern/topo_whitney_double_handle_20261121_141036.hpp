#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * A tilted section crosses both real rulings of a Whitney umbrella. The two ink colors
 * identify which sheet lies closer to the slice point.
 *
 * Fidelity: A, analytic field. Ruled Whitney umbrella in a tilted plane.
 * Cell: Identity / Torus / WhitneyFold. Seed width controls density of the ruled contours.
 * Nearest: topo_milnor_seifert_sinh_hall (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern topo_whitney_double_handle() {
  Pattern p;
  p.name = "topo_whitney_double_handle";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.seed.type = SeedType::WhitneyFold;
  p.seed.border_width = 0.46f;
  p.seed.hue = 0.14f;
  p.seed.alpha = 0.68f;
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
  return p;
}

} // namespace epi::presets
