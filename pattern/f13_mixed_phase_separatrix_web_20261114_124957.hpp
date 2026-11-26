#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Collection Preset (Slot C05, Hole H4+H2): F13 Mixed-Phase Separatrix Web.
 *
 * Mathematical & dynamical structure:
 * - Singleton inner F13: 0.8 * (cos(z) - 1) * (sin(z) + 1) couples a super-attracting critical
 *   basin near z = 0 (calm dark islands) with rapid hyperbolic growth away from the real axis
 *   (separatrix filaments clamping onto the fBm OrganicNoise border).
 * - Value-linear AtmosphericDrift (mode 13) + O9 bounded -cos hue clock (hv_base = 0).
 *
 * Atlas v4 metrics (fidelity: Class P singleton inner F13 + Hole H2, N = 512):
 * - Regime: ZoomClamp (O = 10.8320, W = 4.6977, tau1 = 6.3964, g2/g0 = 1.7015, exp_m = 2.3727).
 * - Law L4r predicted slope alpha_L4r = 14.0000 (upper solver bracket) vs RTX 3090 measured
 *   V-slope = 3.1778 (luma_std = 0.0895, mean_luma = 0.1992, hf_fraction = 0.0018, 0% clipped).
 * - Novelty: fills Holes H4 + H2 (only 1 prior F13 preset with AmmannBeenker; 0 OrganicNoise x
 *   Clamp in 270); max staged pairwise |corr| = 0.4268.
 */
inline Pattern f13_mixed_phase_separatrix_web() {
  constexpr float ZN0_RE = 1.58f;

  Pattern p;
  p.name = "f13_mixed_phase_separatrix_web";
  p.inner = InnerFunction::F13;
  p.reduction = ReductionType::Clamp;

  p.zn.zn[0] = {ZN0_RE, 0.26f};
  p.zn.zn[1] = {0.14f, -0.09f};
  p.zn.zn[2] = {0.72f, 0.15f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.01f;
  p.color.sat_scale = 1.05f;
  p.color.fade = 0.925f;
  p.color.color_mode = as_int(ColorMode::AtmosphericDrift);
  p.color.spiral_angle_coeff = 0.45f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.18f;
  p.color.chroma_breath_freq = 0.073f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.066f;
  p.color.hue_velocity_drift = 0.018f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::OrganicNoise;
  p.seed.border_width = 0.10f;
  p.seed.hue = 0.48f;
  p.seed.hue_speed = 0.035f;
  p.seed.alpha = 0.78f;
  p.seed.pulse_freq = 0.073f;
  p.seed.pulse_amp = 0.0f;

  p.speed = 0.80f;

  p.motion
      .add("separatrix_scale_breath",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.041f,
                            .amplitude = 0.07f,
                            .offset = ZN0_RE},
           control::ModTarget::Zn0Re)
      .add("drift_hue_precession",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.027f,
                            .amplitude = 0.025f,
                            .offset = 0.01f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
