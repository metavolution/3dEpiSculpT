#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Collection Preset (Slot C04, Hole H1+H2): Cosh Catenary Escape Caustic.
 *
 * Mathematical & dynamical structure:
 * - Bilateral exponential catenary cosh(z) translated across its critical point cosh(0) = 1
 *   via zn3 = {-0.28, -0.06} and Clamped onto radial WaveRing shells.
 * - Value-linear MoireInterference (mode 12) + O9 bounded -cos hue clock (hv_base = 0).
 *
 * Atlas v4 metrics (fidelity: Class P catenary fold caustic, N = 512):
 * - Regime: Contracting (O = -5.9162, W = 0.9249, tau1 = 10.8360, g2/g0 = 1.0027, exp_m = 0.1521).
 * - RTX 3090 measured V-slope = 4.5099 (luma_std = 0.0542, mean_luma = 0.0473, hf_fraction = 0.0000,
 *   0% clipped).
 * - Novelty: fills Hole H1 (Cosh x Clamp: 0/270 prior presets); max staged pairwise |corr| = 0.1210.
 */
inline Pattern cosh_catenary_escape_caustic() {
  constexpr float ZN3_RE = -0.28f;

  Pattern p;
  p.name = "cosh_catenary_escape_caustic";
  p.inner = InnerFunction::Cosh;
  p.reduction = ReductionType::Clamp;

  p.zn.zn[0] = {1.32f, -0.28f};
  p.zn.zn[1] = {0.05f, 0.04f};
  p.zn.zn[2] = {0.62f, 0.18f};
  p.zn.zn[3] = {ZN3_RE, -0.06f};

  p.color.hue_rotation = 0.01f;
  p.color.sat_scale = 1.04f;
  p.color.fade = 0.925f;
  p.color.color_mode = as_int(ColorMode::MoireInterference);
  p.color.spiral_angle_coeff = 1.00f;
  p.color.spiral_radius_coeff = 0.85f;
  p.color.chroma_breath_amp = 0.14f;
  p.color.chroma_breath_freq = 0.067f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.048f;
  p.color.hue_velocity_drift = 0.012f;
  p.color.luma_shape_depth = 0.18f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.34f;
  p.seed.hue_speed = 0.025f;
  p.seed.alpha = 0.74f;
  p.seed.pulse_freq = 0.067f;
  p.seed.pulse_amp = 0.0f;

  p.speed = 0.85f;

  p.motion
      .add("catenary_caustic_shift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.052f,
                            .amplitude = 0.08f,
                            .offset = ZN3_RE},
           control::ModTarget::Zn3Re)
      .add("moire_hue_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.038f,
                            .amplitude = 0.028f,
                            .offset = 0.01f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
