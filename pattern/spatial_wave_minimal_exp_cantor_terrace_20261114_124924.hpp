#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimal, Slot M2, Hole H1): Exponential Cantor Terrace.
 *
 * Mathematical & dynamical structure:
 * - Devaney exponential family beta * exp(w) + gamma near channel bifurcation (|beta| > 1/e)
 *   on a tilted, off-center affine window w = zn0 * z + zn1 with Clamp + Simplex border.
 * - Tilted orientation (~18.5 deg) and positive real window offset create diagonal Cantor-bouquet
 *   escape terraces on the right channel alongside a contracting lobe on Re(w) < 0.
 * - Value-linear SpatialWave (mode 4) + O9 bounded -cos hue clock (hv_base = 0).
 *
 * Atlas v4 metrics (fidelity: Class P exact dynamical family, N = 512):
 * - Regime: ZoomClamp (O = 1.0249, W = 2.5092, tau1 = 1.5176, g2/g0 = 1.3521, exp_m = 0.8145).
 * - Law L4r predicted closed-form slope alpha_L4r = 3.2023 vs RTX 3090 measured V-slope = 3.1693
 *   (luma_std = 0.1211, mean_luma = 0.4689, 0% clipped).
 * - Novelty: fills Hole H1 (Exp x Clamp: 0/270 prior presets); |corr| vs devaney_sine_cantor_bouquet
 *   = 0.1003 (< 0.75); max staged pairwise |corr| = 0.4513.
 */
inline Pattern spatial_wave_minimal_exp_cantor_terrace() {
  constexpr float ZN0_RE = 2.15f;
  constexpr float ZN0_IM = 0.72f;
  constexpr float ZN1_RE = 0.85f;
  constexpr float ZN1_IM = 0.18f;
  constexpr float ZN2_RE = 0.22f;
  constexpr float ZN2_IM = -0.06f;

  Pattern p;
  p.name = "spatial_wave_minimal_exp_cantor_terrace";
  p.inner = InnerFunction::Exp;
  p.reduction = ReductionType::Clamp;

  p.zn.zn[0] = {ZN0_RE, ZN0_IM};
  p.zn.zn[1] = {ZN1_RE, ZN1_IM};
  p.zn.zn[2] = {ZN2_RE, ZN2_IM};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.03f;
  p.color.sat_scale = 1.02f;
  p.color.fade = 0.930f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = -0.38f;
  p.color.spiral_radius_coeff = 0.45f;
  p.color.chroma_breath_amp = 0.15f;
  p.color.chroma_breath_freq = 0.089f;
  p.color.chroma_breath_phase = 0.12f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.064f;
  p.color.hue_velocity_drift = 0.012f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.045f;
  p.seed.hue = 0.52f;
  p.seed.hue_speed = 0.038f;
  p.seed.alpha = 0.78f;
  p.seed.pulse_freq = 0.089f;
  p.seed.pulse_amp = 0.0f;

  p.speed = 0.80f;

  p.motion
      .add("exp_bifurcation_lambda",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.055f,
                            .amplitude = 0.035f,
                            .offset = ZN2_RE},
           control::ModTarget::Zn2Re)
      .add("terrace_hue_phase",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.037f,
                            .amplitude = 0.030f,
                            .offset = 0.03f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
