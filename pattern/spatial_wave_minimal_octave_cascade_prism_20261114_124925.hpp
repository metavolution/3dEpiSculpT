#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimal, Slot M3 / Roster E2): Octave Cascade Prism.
 *
 * Mathematical & dynamical structure:
 * - ExpandingTiling Sin + Torus + WaveRing cascade with calibrated retention fade = 0.892
 *   and quadrature PatternMotion sweep on (Zn2Re, Zn2Im) plus Fade so octave scale, twist,
 *   and Law L4r cascade slope breathe smoothly at bounded exposure.
 * - Value-linear SpatialWave (mode 4) + O9 bounded -cos hue clock (hv_base = 0).
 *
 * Atlas v4 metrics (fidelity: Class P exact L4r cascade design, N = 512):
 * - Regime: ExpandingTiling (O = 5.0966, W = 1.3134, tau1 = 5.7584, g2/g0 = 1.0111, exp_m = 0.3655).
 * - Law L4r predicted pressure slope alpha_L4r = 2.5302 (inside [2.4, 3.2]) vs RTX 3090 measured
 *   V-slope = 2.6070 (luma_std = 0.0541, mean_luma = 0.0553, hf_fraction = 0.0503, 0% clipped).
 * - Novelty: dedicated animated L4r prism filling E2; max staged pairwise |corr| = 0.0967.
 */
inline Pattern spatial_wave_minimal_octave_cascade_prism() {
  constexpr float ZN2_RE = 1.36f;
  constexpr float ZN2_IM = 0.32f;
  constexpr float FADE_BASE = 0.892f;

  Pattern p;
  p.name = "spatial_wave_minimal_octave_cascade_prism";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.22f, 0.27f};
  p.zn.zn[1] = {0.20f, -0.15f};
  p.zn.zn[2] = {ZN2_RE, ZN2_IM};
  p.zn.zn[3] = {0.05f, 0.08f};

  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 1.06f;
  p.color.fade = FADE_BASE;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.55f;
  p.color.spiral_radius_coeff = 0.72f;
  p.color.chroma_breath_amp = 0.20f;
  p.color.chroma_breath_freq = 0.097f;
  p.color.chroma_breath_phase = 0.14f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.075f;
  p.color.hue_velocity_drift = 0.015f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.32f;
  p.seed.hue_speed = 0.030f;
  p.seed.alpha = 0.84f;
  p.seed.pulse_freq = 0.097f;
  p.seed.pulse_amp = 0.0f;

  p.speed = 0.85f;

  p.motion
      .add("octave_scale_sweep_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.062f,
                            .amplitude = 0.12f,
                            .offset = ZN2_RE,
                            .phase = 0.0f},
           control::ModTarget::Zn2Re)
      .add("octave_twist_sweep_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.062f,
                            .amplitude = 0.12f,
                            .offset = ZN2_IM,
                            .phase = 0.25f},
           control::ModTarget::Zn2Im)
      .add("cascade_fade_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.041f,
                            .amplitude = 0.03f,
                            .offset = FADE_BASE},
           control::ModTarget::Fade);

  return p;
}

} // namespace epi::presets
