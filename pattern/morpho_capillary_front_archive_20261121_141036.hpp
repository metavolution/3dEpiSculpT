#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Six interface modes amplify or decay at their own linear rates. Stacked marker layers
 * record how surface tension selects a length scale.
 *
 * Fidelity: A, analytic field. Finite linear interface spectrum with capillary stabilization.
 * Cell: Identity / Torus / CapillaryFront. Seed width controls linear destabilizing growth rate.
 * Nearest: swift_hohenberg_labyrinth (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern morpho_capillary_front_archive() {
  Pattern p;
  p.name = "morpho_capillary_front_archive";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.seed.type = SeedType::CapillaryFront;
  p.seed.border_width = 0.71f;
  p.seed.hue = 0.43f;
  p.seed.alpha = 0.78f;
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
  return p;
}

} // namespace epi::presets
