#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * The equilibrium order jumps across the curved first-order boundary and grows continuously
 * across the straight boundary. They meet at the tricritical point.
 *
 * Fidelity: A, analytic field. Global equilibrium of an even sixth-order Landau potential.
 * Cell: Identity / Torus / LandauCoexistence. Seed width controls vertical range of the quadratic coefficient.
 * Nearest: spatial_wave_ising_field_hysteresis_cusp (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern field_landau_coexistence_fan() {
  Pattern p;
  p.name = "field_landau_coexistence_fan";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.seed.type = SeedType::LandauCoexistence;
  p.seed.border_width = 0.43f;
  p.seed.hue = 0.76f;
  p.seed.alpha = 0.72f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.87f;
  p.color.sat_scale = 1.0f;
  p.color.hue_rotation = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.hue_velocity_breath = 0.001f;
  p.color.chroma_breath_freq = 0.07f;
  return p;
}

} // namespace epi::presets
