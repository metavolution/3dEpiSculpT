#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * The three saddles at energy one sixth open together. Contours inside the Hill region encode
 * allowed momentum magnitude at each position.
 *
 * Fidelity: A, analytic field. Zero-velocity contours of the Henon-Heiles Hamiltonian.
 * Cell: Identity / Torus / HillPassages. Seed width controls offset from the three-saddle energy.
 * Nearest: tanh_pendulum_resonance_abyss (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; source geometry.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern dyn_hill_three_pass_gate() {
  Pattern p;
  p.name = "dyn_hill_three_pass_gate";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.seed.type = SeedType::HillPassages;
  p.seed.border_width = 0.5f;
  p.seed.hue = 0.08f;
  p.seed.alpha = 0.74f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.85f;
  p.color.sat_scale = 1.0f;
  p.color.hue_rotation = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.hue_velocity_breath = 0.001f;
  p.color.chroma_breath_freq = 0.07f;
  return p;
}

} // namespace epi::presets
