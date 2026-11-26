#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Translate the finite diffraction plate slowly through its own reciprocal lattice. Odd
 * centered orders cancel while the finite array leaves sidelobes.
 *
 * Fidelity: A, analytic field. Extinctions of a centered two-site diffraction array.
 * Cell: Identity / Torus / BraggBasis. Seed width controls relative displacement of the two-site basis.
 * Nearest: identity_hexagonal_phased_array_colour_vortex (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; static source cached between parameter edits.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern spectral_centered_bragg_extinctions() {
  Pattern p;
  p.name = "spectral_centered_bragg_extinctions";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.007f, 0.004f};

  p.seed.type = SeedType::BraggBasis;
  p.seed.border_width = 0.5f;
  p.seed.hue = 0.54f;
  p.seed.alpha = 0.82f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.84f;
  p.color.sat_scale = 1.0f;
  p.color.hue_rotation = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.hue_velocity_breath = 0.001f;
  p.color.chroma_breath_freq = 0.07f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_amp = 0.062f;
  p.zn.zn0_anim_speed = 0.075f;
  return p;
}

} // namespace epi::presets
