#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Five conservative subdivisions assign a measurable local mass exponent to every tile.
 * Quarter-turn child charts break the product-measure stripes.
 *
 * Fidelity: I, up to 5 local steps. Conservative four-child multiplicative mass cascade.
 * Cell: Identity / Torus / CascadeMeasure. Seed width controls mass imbalance between daughters.
 * Nearest: riesz_product_lacunary_multifractal (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; static source cached between parameter edits.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern field_conservative_mass_tapestry() {
  Pattern p;
  p.name = "field_conservative_mass_tapestry";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.0f, 1.0f};
  p.zn.zn[3] = {0.0625f, 0.125f};

  p.seed.type = SeedType::CascadeMeasure;
  p.seed.border_width = 0.76f;
  p.seed.hue = 0.56f;
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
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_amp = 0.016f;
  p.zn.zn0_anim_speed = 0.07f;
  return p;
}

} // namespace epi::presets
