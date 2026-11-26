#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Translate and invert to the modular fundamental domain. Each reduction deepens the hue
 * toward the accumulating cusps on the real axis.
 *
 * Fidelity: I, up to 8 local steps. Upper half-plane reduction by integer shifts and inversion.
 * Cell: Identity / Torus / ModularChamber. Seed width controls height of the upper half-plane window.
 * Nearest: hyperbolic_mirror_kaleidoscope (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern topo_modular_cusp_script() {
  Pattern p;
  p.name = "topo_modular_cusp_script";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.seed.type = SeedType::ModularChamber;
  p.seed.border_width = 0.54f;
  p.seed.hue = 0.53f;
  p.seed.alpha = 0.66f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.91f;
  p.color.sat_scale = 1.0f;
  p.color.hue_rotation = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.hue_velocity_breath = 0.001f;
  p.color.chroma_breath_freq = 0.07f;
  return p;
}

} // namespace epi::presets
