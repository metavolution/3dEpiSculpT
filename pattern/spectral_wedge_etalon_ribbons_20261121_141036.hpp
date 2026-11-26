#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * An oblique similarity offsets successive equal-thickness fringes. Each resonance has an
 * Airy peak with a long asymmetric-looking wake supplied by memory.
 *
 * Fidelity: A, analytic field. Airy transmission through a curved optical wedge.
 * Cell: Identity / Grid / WedgeEtalon. Seed width controls finesse coefficient and transverse wedge curvature.
 * Nearest: spatial_wave_photonic_bandgap_smith_chart (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern spectral_wedge_etalon_ribbons() {
  Pattern p;
  p.name = "spectral_wedge_etalon_ribbons";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.993f, 0.023f};
  p.zn.zn[3] = {0.013f, -0.004f};

  p.seed.type = SeedType::WedgeEtalon;
  p.seed.border_width = 0.67f;
  p.seed.hue = 0.01f;
  p.seed.alpha = 0.46f;
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
  return p;
}

} // namespace epi::presets
