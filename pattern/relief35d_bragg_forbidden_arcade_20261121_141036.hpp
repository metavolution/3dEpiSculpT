#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * A cosine pullback repeatedly images missing diffraction orders. Surviving narrow peaks
 * reset age inside vaulted chambers of retained light.
 *
 * Fidelity: A, analytic field. Extinctions of a centered two-site diffraction array.
 * Cell: Cos / Torus / BraggBasis. Seed width controls relative displacement of the two-site basis.
 * Nearest: spectral_centered_bragg_extinctions (curatorial comparison).
 * Height: Relief observer samples retained age and luminance.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; static source cached between parameter edits.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern relief35d_bragg_forbidden_arcade() {
  Pattern p;
  p.name = "relief35d_bragg_forbidden_arcade";
  p.inner = InnerFunction::Cos;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.20f, 0.72f};
  p.zn.zn[1] = {-0.15f, 0.23f};
  p.zn.zn[2] = {-0.80f, 0.45f};
  p.zn.zn[3] = {0.12f, -0.08f};

  p.seed.type = SeedType::BraggBasis;
  p.seed.border_width = 0.5f;
  p.seed.hue = 0.32f;
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
  p.relief_depth_range = 2.6f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_amp = 0.085f;
  p.zn.zn0_anim_speed = 0.085f;
  return p;
}

} // namespace epi::presets
