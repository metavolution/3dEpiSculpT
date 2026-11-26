#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Sweep the mass of a two-band lattice through a gap closing. Sign changes separate two hue
 * families. Logarithmic magnitude contours resolve the concentrated curvature at the center.
 *
 * Fidelity: A, analytic field. Two-band lattice Berry curvature near a gap closing.
 * Cell: Identity / Torus / BerryPlaquette. Seed width controls central lattice mass.
 * Nearest: bloch_oscillation_wannier_stark_ladder (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern field_berry_gap_inversion() {
  Pattern p;
  p.name = "field_berry_gap_inversion";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.008f, 0.004f};

  p.seed.type = SeedType::BerryPlaquette;
  p.seed.border_width = 0.64f;
  p.seed.hue = 0.47f;
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
  return p;
}

} // namespace epi::presets
