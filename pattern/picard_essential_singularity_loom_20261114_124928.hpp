#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Collection Preset (Slot C01, Hole H4): Picard Essential Singularity Loom.
 *
 * Mathematical & dynamical structure:
 * - InnerFunction::FractalStar (z * sin(1/z)) has an isolated essential singularity at z = 0
 *   with exponential sinh(1/y) growth along the imaginary axis.
 * - Calibrated off-origin affine window w = zn0 * z + zn1 (|w| bounded away from 0 across [-1, 1]^2)
 *   plus post-centering zn3 = {-0.92, 0.18} keeps FP32/FP16 finite while projecting dense nested
 *   Picard sheets onto the active WaveRing core under Clamp + QuasiPeriodicWave (mode 14).
 *
 * Atlas v4 metrics (fidelity: Class P essential-singularity window, N = 512):
 * - Regime: Contracting (O = -22.1008, W = 2.9618, tau1 = 15.4783, g2/g0 = 1.0014, exp_m = 0.1206).
 * - RTX 3090 measured V-slope = 3.4017 (luma_std = 0.0549, mean_luma = 0.0678, hf_fraction = 0.0049,
 *   0% clipped, finite in FP32 & FP16).
 * - Novelty: fills Hole H4 (only 1 prior FractalStar preset in 270, with Torus; first FractalStar x
 *   Clamp + WaveRing); max staged pairwise |corr| = 0.2089.
 */
inline Pattern picard_essential_singularity_loom() {
  constexpr float ZN1_RE = 0.42f;
  constexpr float ZN1_IM = 0.48f;

  Pattern p;
  p.name = "picard_essential_singularity_loom";
  p.inner = InnerFunction::FractalStar;
  p.reduction = ReductionType::Clamp;

  p.zn.zn[0] = {0.72f, 0.18f};
  p.zn.zn[1] = {ZN1_RE, ZN1_IM};
  p.zn.zn[2] = {1.35f, -0.25f};
  p.zn.zn[3] = {-0.92f, 0.18f};

  p.color.hue_rotation = 0.025f;
  p.color.sat_scale = 1.04f;
  p.color.fade = 0.945f;
  p.color.color_mode = as_int(ColorMode::QuasiPeriodicWave);
  p.color.spiral_angle_coeff = 1.50f;
  p.color.spiral_radius_coeff = 0.85f;
  p.color.chroma_breath_amp = 0.22f;
  p.color.chroma_breath_freq = 0.091f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.068f;
  p.color.hue_velocity_drift = 0.021f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.35f;
  p.seed.hue_speed = 0.028f;
  p.seed.alpha = 0.82f;
  p.seed.pulse_freq = 0.091f;
  p.seed.pulse_amp = 0.0f;

  p.speed = 0.85f;

  p.motion
      .add("picard_window_x",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.039f,
                            .amplitude = 0.05f,
                            .offset = ZN1_RE},
           control::ModTarget::Zn1Re)
      .add("picard_window_y",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.051f,
                            .amplitude = 0.05f,
                            .offset = ZN1_IM},
           control::ModTarget::Zn1Im);

  return p;
}

} // namespace epi::presets
