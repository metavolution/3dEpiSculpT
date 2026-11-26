#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * A direct spherical source and its opposite-sign image cancel at the mirror. Color tracks
 * the path difference as the source rises and falls.
 *
 * Fidelity: A, analytic field. Opposite-phase direct and reflected spherical waves.
 * Cell: Identity / Torus / LloydImage. Seed width controls source height above the mirror.
 * Nearest: spatial_wave_minimal_two_source_hyperbolae (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern spectral_lloyd_dark_mirror() {
  Pattern p;
  p.name = "spectral_lloyd_dark_mirror";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.seed.type = SeedType::LloydImage;
  p.seed.border_width = 0.58f;
  p.seed.hue = 0.45f;
  p.seed.alpha = 0.77f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.88f;
  p.color.sat_scale = 1.0f;
  p.color.hue_rotation = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.hue_velocity_breath = 0.001f;
  p.color.chroma_breath_freq = 0.07f;
  return p;
}

} // namespace epi::presets
