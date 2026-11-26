#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Ten steps of the unimodular transfer trace recursion resolve spectral approximants across
 * energy and contrast. A slow horizontal scan carries band edges into memory.
 *
 * Fidelity: I, up to 10 local steps. Half-trace recursion of a Fibonacci transfer chain.
 * Cell: Identity / Clamp / FibonacciTrace. Seed width controls upper contrast of the energy/contrast chart.
 * Nearest: quasicrystal_plastic_pisot_envelope (curatorial comparison).
 * Height: Native color view. The fourth channel tracks recursive age.
 * Hue clock: Bounded oscillatory feedback clock. Source hue encodes the local observable.
 * Fastest lane: Per-field feedback; static source cached between parameter edits.
 * Status: Authored atlas preset.
 * @assert bounded_hue_clock
 */
inline Pattern dyn_fibonacci_transfer_spectrum() {
  Pattern p;
  p.name = "dyn_fibonacci_transfer_spectrum";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.008f, 0.0f};

  p.seed.type = SeedType::FibonacciTrace;
  p.seed.border_width = 0.76f;
  p.seed.hue = 0.63f;
  p.seed.alpha = 0.82f;
  p.seed.hue_speed = 0.0f;
  p.seed.pulse_amp = 0.0f;

  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.fade = 0.86f;
  p.color.sat_scale = 1.0f;
  p.color.hue_rotation = 0.014f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.hue_velocity_breath = 0.001f;
  p.color.chroma_breath_freq = 0.07f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_amp = 0.035f;
  p.zn.zn0_anim_speed = 0.065f;
  return p;
}

} // namespace epi::presets
