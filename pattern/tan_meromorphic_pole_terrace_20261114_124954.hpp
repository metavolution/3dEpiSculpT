#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Collection Preset (Slot C03, Hole H1): Tan Meromorphic Pole Terrace.
 *
 * Mathematical & dynamical structure:
 * - Meromorphic tan(z) pole pre-image portals under Clamp over a Cross seed.
 * - Off-axis rotation zn0 = {1.173, 0.241} and shift zn1 = {0.113, 0.1915} keep cos(w)
 *   strictly nonzero on the 512^2 lattice (zero Inf/NaN in FP32 & FP16) while framing
 *   dense pre-pole terraces.
 * - Value-linear PhantomFractal (mode 16 with luma_shape_depth == 0.0f) + O9 bounded hue clock.
 *
 * Atlas v4 metrics (fidelity: Class P meromorphic pole-terrace pullback, N = 512):
 * - Regime: Contracting (O = -6.8430, W = 1.2064, tau1 = 7.4221, g2/g0 = 1.0019, exp_m = 0.2285).
 * - RTX 3090 measured V-slope = 3.8921 (luma_std = 0.0693, mean_luma = 0.0609, hf_fraction = 0.0018,
 *   0% clipped).
 * - Novelty: fills Hole H1 (Tan x Clamp: 0/270 prior presets); max staged pairwise |corr| = 0.2753.
 */
inline Pattern tan_meromorphic_pole_terrace() {
  constexpr float ZN1_IM = 0.1915f;
  constexpr float ZN2_RE = 0.78f;

  Pattern p;
  p.name = "tan_meromorphic_pole_terrace";
  p.inner = InnerFunction::Tan;
  p.reduction = ReductionType::Clamp;

  p.zn.zn[0] = {1.173f, 0.241f};
  p.zn.zn[1] = {0.113f, ZN1_IM};
  p.zn.zn[2] = {ZN2_RE, -0.12f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.02f;
  p.color.sat_scale = 1.04f;
  p.color.fade = 0.90f;
  p.color.color_mode = as_int(ColorMode::PhantomFractal);
  p.color.spiral_angle_coeff = 0.50f;
  p.color.spiral_radius_coeff = 0.45f;
  p.color.chroma_breath_amp = 0.22f;
  p.color.chroma_breath_freq = 0.079f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.062f;
  p.color.hue_velocity_drift = 0.015f;
  p.color.luma_shape_depth = 0.0f; // required for PhantomFractal value-linearity
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.04f;
  p.seed.hue = 0.78f;
  p.seed.hue_speed = 0.032f;
  p.seed.alpha = 0.42f;
  p.seed.pulse_freq = 0.079f;
  p.seed.pulse_amp = 0.0f;

  p.speed = 0.80f;

  p.motion
      .add("pole_portal_shift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.047f,
                            .amplitude = 0.06f,
                            .offset = ZN1_IM},
           control::ModTarget::Zn1Im)
      .add("meromorphic_gain",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.036f,
                            .amplitude = 0.08f,
                            .offset = ZN2_RE},
           control::ModTarget::Zn2Re);

  return p;
}

} // namespace epi::presets
