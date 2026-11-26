#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * A circular mirror focuses parallel rays onto a two-cusp nephroid. The slowly rocking
 * caustic etches a luminous seam in older glass.
 *
 * Fidelity: A, analytic field. Reflection caustic of a circular mirror under parallel light.
 * Cell: Identity / Torus / NephroidGlass. Seed width controls radius scale of the mirror caustic.
 * Nearest: darkfield_pearcey_cusp_caustic (curatorial comparison).
 * Height: Relief observer samples retained age and luminance.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern relief35d_nephroid_glass() {
  Pattern p;
  p.name = "relief35d_nephroid_glass";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.seed.type = SeedType::NephroidGlass;
  p.seed.border_width = 0.58f;
  p.seed.hue = 0.1f;
  p.seed.alpha = 0.66f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.95f;
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
