#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Rotate a stereographic sphere through its nine reflection planes. Coordinate mirrors and
 * diagonal mirrors remain distinguishable at their crossings.
 *
 * Fidelity: A, analytic field. Stereographic section of the B3 reflection arrangement.
 * Cell: Identity / Torus / OctahedralMirrors. Seed width controls initial tilt of the mirror planes.
 * Nearest: spatial_wave_riemann_sphere_golden_rotation (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern topo_octahedral_mirror_globe() {
  Pattern p;
  p.name = "topo_octahedral_mirror_globe";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.seed.type = SeedType::OctahedralMirrors;
  p.seed.border_width = 0.64f;
  p.seed.hue = 0.16f;
  p.seed.alpha = 0.62f;
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
