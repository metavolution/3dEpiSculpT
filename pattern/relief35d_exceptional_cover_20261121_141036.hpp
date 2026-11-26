#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * A translated quadratic pullback makes two preimages of each generic degeneracy. Equal-
 * splitting contours cut terraces through the double cover.
 *
 * Fidelity: A, analytic field. Complex splitting of a balanced gain-loss mode pair.
 * Cell: Square / Grid / ExceptionalDoublet. Seed width controls coupling between the modes.
 * Nearest: spectral_exceptional_doublet (curatorial comparison).
 * Height: Relief observer samples retained age and luminance.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern relief35d_exceptional_cover() {
  Pattern p;
  p.name = "relief35d_exceptional_cover";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {0.98f, 0.23f};
  p.zn.zn[1] = {-0.08f, 0.04f};
  p.zn.zn[2] = {0.89f, -0.26f};
  p.zn.zn[3] = {-0.21f, 0.18f};

  p.seed.type = SeedType::ExceptionalDoublet;
  p.seed.border_width = 0.7f;
  p.seed.hue = 0.55f;
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
  p.relief_depth_range = 1.8f;
  return p;
}

} // namespace epi::presets
