#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Overlapping twist disks fold a striped tracer before a hyperbolic-sine pullback records it.
 * Light resets along filaments, leaving older ridges between them.
 *
 * Fidelity: I, up to 6 local steps. Passive dye pulled through two overlapping compact disk twists.
 * Cell: Sinh / Torus / LinkedTwists. Seed width controls angular displacement inside each twist disk.
 * Nearest: dyn_linked_twist_dye_braid (curatorial comparison).
 * Height: Relief observer samples retained age and luminance.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern relief35d_linked_twist_fossils() {
  Pattern p;
  p.name = "relief35d_linked_twist_fossils";
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {0.48f, 0.64f};
  p.zn.zn[1] = {0.035f, -0.076f};
  p.zn.zn[2] = {-0.62f, 0.92f};
  p.zn.zn[3] = {0.06f, 0.09f};

  p.seed.type = SeedType::LinkedTwists;
  p.seed.border_width = 0.70f;
  p.seed.hue = 0.76f;
  p.seed.alpha = 0.90f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.75f;
  p.color.sat_scale = 1.0f;
  p.color.hue_rotation = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.hue_velocity_breath = 0.001f;
  p.color.chroma_breath_freq = 0.07f;
  p.relief_35d = true;
  p.relief_depth_range = 2.4f;
  return p;
}

} // namespace epi::presets
