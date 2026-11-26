#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Collection Preset (Slot C06, Hole H4+H3): TP0 Octagonal Quasicrystal Gear.
 *
 * Mathematical & dynamical structure:
 * - Singleton inner TP0: 1.2 * (1 - (z^2 + tanh(z))/2) bounded safely inside the nearest tanh
 *   pole (|w| < pi/2) and folded by 8-fold AmmannBeenker over animated ChladniCymatics contours.
 * - Value-linear ChromaBreathing (mode 9) + O9 bounded hue clock + animated ChromaBreathAmp
 *   diaphragm lane.
 *
 * Atlas v4 metrics (fidelity: Class P singleton inner TP0 + Hole H3, N = 512):
 * - Regime: ExpandingTiling (O = 1.3952, W = 0.7028, tau1 = 10.2504, g2/g0 = 1.0041, exp_m = 0.2608).
 * - Law L4r predicted pressure slope alpha_L4r = 3.4526 vs RTX 3090 measured V-slope = 2.7311
 *   (luma_std = 0.0647, mean_luma = 0.0562, hf_fraction = 0.0089, 0% clipped).
 * - Novelty: fills Holes H4 + H3 (only 1 prior TP0 preset with Torus; first TP0 x AmmannBeenker
 *   + ChladniCymatics); max staged pairwise |corr| = 0.4513.
 */
inline Pattern tp0_octagonal_quasicrystal_gear() {
  constexpr float ZN2_IM = -0.18f;
  constexpr float BREATH_AMP = 0.32f;

  Pattern p;
  p.name = "tp0_octagonal_quasicrystal_gear";
  p.inner = InnerFunction::TP0;
  p.reduction = ReductionType::AmmannBeenker;

  p.zn.zn[0] = {0.96f, 0.22f};
  p.zn.zn[1] = {0.07f, 0.11f};
  p.zn.zn[2] = {0.84f, ZN2_IM};
  p.zn.zn[3] = {-0.12f, 0.05f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.02f;
  p.color.fade = 0.930f;
  p.color.color_mode = as_int(ColorMode::ChromaBreathing);
  p.color.spiral_angle_coeff = 1.00f;
  p.color.spiral_radius_coeff = 0.40f;
  p.color.chroma_breath_amp = BREATH_AMP;
  p.color.chroma_breath_freq = 0.095f;
  p.color.chroma_breath_phase = 0.25f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.050f;
  p.color.hue_velocity_drift = 0.010f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.88f;
  p.seed.hue_speed = 0.028f;
  p.seed.alpha = 0.52f;
  p.seed.pulse_freq = 0.095f;
  p.seed.pulse_amp = 0.0f;

  p.speed = 0.85f;

  p.motion
      .add("gear_post_twist",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.058f,
                            .amplitude = 0.08f,
                            .offset = ZN2_IM},
           control::ModTarget::Zn2Im)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.095f,
                            .amplitude = 0.15f,
                            .offset = BREATH_AMP},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
