#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Two oppositely twisting disks stretch a single dye label. Outside their intersection the
 * original stripe family provides a scale for the folding.
 *
 * Fidelity: I, up to 6 local steps. Passive dye pulled through two overlapping compact disk twists.
 * Cell: Identity / Torus / LinkedTwists. Seed width controls angular displacement inside each twist disk.
 * Nearest: dyn_double_gyre_ftle_lace (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern dyn_linked_twist_dye_braid() {
  Pattern p;
  p.name = "dyn_linked_twist_dye_braid";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.seed.type = SeedType::LinkedTwists;
  p.seed.border_width = 0.7f;
  p.seed.hue = 0.33f;
  p.seed.alpha = 0.72f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;
  p.seed.resolution_div = 2;

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
