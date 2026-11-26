#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimal, Slot M1, Hole H2): Chladni Similarity Droste.
 *
 * Mathematical & dynamical structure:
 * - Pure rotating similarity pullback F(z) = mu * e^{i*theta} * z (Identity + Clamp)
 *   with mu = 1.11 and pi-incommensurate twist theta = 0.17 rad (~9.74 deg) over the
 *   animated ChladniCymatics standing-wave drum (n(t), m(t)).
 * - Clamped outer points land on r >= 1 where the Chladni circular mask smoothstep(0.8, 1.0, r)
 *   vanishes, isolating an interior logarithmic Droste cascade of standing-wave nodal lines.
 * - Value-linear SpatialWave (mode 4) + O9 bounded -cos hue clock (hv_base = 0).
 *
 * Atlas v4 metrics (fidelity: Class P exact similarity construction, N = 512):
 * - Regime: ZoomClamp (O = 3.3099, W = 0.0000, tau1 = 21.5800, g2/g0 = 1.0050, exp_m = 0.1517).
 * - Law L4r predicted slope alpha_L4r = 6.3219 vs RTX 3090 measured V-slope = 1.8213 (seed
 *   Chladni ridge spectrum dominates at mid-f, luma_std = 0.1093, mean_luma = 0.0574, 0% clipped).
 * - Novelty: fills Hole H2 (ChladniCymatics x Clamp: 0/270 prior presets); max staged
 *   pairwise silhouette |corr| = 0.1676.
 */
inline Pattern spatial_wave_minimal_chladni_similarity_droste() {
  // mu = 1.11, theta = 0.17 rad (~9.74 deg, pi-incommensurate chiral Droste twist)
  constexpr float ZN0_RE = 1.093999f; // 1.11 * cos(0.17)
  constexpr float ZN0_IM = 0.187792f; // 1.11 * sin(0.17)

  Pattern p;
  p.name = "spatial_wave_minimal_chladni_similarity_droste";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;

  p.zn.zn[0] = {ZN0_RE, ZN0_IM};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.02f;
  p.color.sat_scale = 1.04f;
  p.color.fade = 0.960f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.42f;
  p.color.spiral_radius_coeff = 0.618034f;
  p.color.chroma_breath_amp = 0.16f;
  p.color.chroma_breath_freq = 0.071f;
  p.color.chroma_breath_phase = 0.11f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.052f;
  p.color.hue_velocity_drift = 0.008f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.06f;
  p.seed.hue = 0.30f;
  p.seed.hue_speed = 0.025f;
  p.seed.alpha = 0.76f;
  p.seed.pulse_freq = 0.071f;
  p.seed.pulse_amp = 0.0f;

  p.speed = 0.85f;

  p.motion
      .add("droste_chiral_twist",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.043f,
                            .amplitude = 0.045f,
                            .offset = ZN0_IM},
           control::ModTarget::Zn0Im)
      .add("droste_hue_precession",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.031f,
                            .amplitude = 0.025f,
                            .offset = 0.02f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
