#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Eighteen successive chords share one caustic circle. Hue blends the normals of nearby chords
 * as the incidence angle moves through near-closed polygons.
 *
 * Fidelity: I, up to 18 local steps. Confocal caustic of a circular billiard chord orbit.
 * Cell: Identity / Torus / CircleBilliard. Seed width controls incidence angle.
 * Nearest: bessel_whispering_gallery_caustic (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern dyn_billiard_caustic_strings() {
  Pattern p;
  p.name = "dyn_billiard_caustic_strings";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.007f, 0.004f};

  p.seed.type = SeedType::CircleBilliard;
  p.seed.border_width = 0.58f;
  p.seed.hue = 0.51f;
  p.seed.alpha = 0.46f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;
  p.seed.resolution_div = 2;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.954f;
  p.color.sat_scale = 1.0f;
  p.color.hue_rotation = 0.014f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.hue_velocity_breath = 0.001f;
  p.color.chroma_breath_freq = 0.07f;
  return p;
}

} // namespace epi::presets
