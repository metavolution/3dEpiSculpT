#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Six symmetric forks conserve the sum of daughter radii cubed. A short translating
 * afterimage separates the thick parent conduits from their terminal fan.
 *
 * Fidelity: I, up to 6 local steps. Symmetric branching conduits with cubic radius conservation.
 * Cell: Identity / Clamp / MurrayTree. Seed width controls root vessel radius.
 * Nearest: seashell_meinhardt_accretion_lip (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; static source cached between parameter edits.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern morpho_murray_terminal_canopy() {
  Pattern p;
  p.name = "morpho_murray_terminal_canopy";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.994f, 0.0f};
  p.zn.zn[3] = {0.003f, 0.011f};

  p.seed.type = SeedType::MurrayTree;
  p.seed.border_width = 0.61f;
  p.seed.hue = 0.16f;
  p.seed.alpha = 0.57f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.93f;
  p.color.sat_scale = 1.0f;
  p.color.hue_rotation = 0.014f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.hue_velocity_breath = 0.001f;
  p.color.chroma_breath_freq = 0.07f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_amp = 0.025f;
  p.zn.zn0_anim_speed = 0.056f;
  return p;
}

} // namespace epi::presets
