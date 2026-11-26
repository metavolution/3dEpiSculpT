#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Collection Preset (Slot C02, Hole H4): F7 Rotated Hyperbolic Shear Hall.
 *
 * Mathematical & dynamical structure:
 * - First preset in epiC exercising InnerFunction::F7 (sin(i*z) = i*sinh(z)), which turns
 *   real exponential stretch into imaginary oscillatory shear, folded by PareidoliaCathedral
 *   (Torus triangle fold + diagonal x + y < 0 reflection) over a Cross seed.
 * - Value-linear OKLabColorRotation (mode 17) + O9 bounded -cos hue clock (hv_base = 0).
 *
 * Atlas v4 metrics (fidelity: Class P untouched inner F7 + dihedral fold, N = 512):
 * - Regime: Neutral (O = -0.0905, W = 0.4532, tau1 = 4.7829, g2/g0 = 1.1827, exp_m = 1.0173).
 * - RTX 3090 measured V-slope = 3.6136 (luma_std = 0.0607, mean_luma = 0.0566, hf_fraction = 0.0038,
 *   0% clipped, finite in FP32 & FP16).
 * - Novelty: fills Hole H4 (0/270 prior F7 presets); max staged pairwise |corr| = 0.1402.
 */
inline Pattern f7_rotated_hyperbolic_shear_hall() {
  constexpr float ZN0_IM = 0.24f;

  Pattern p;
  p.name = "f7_rotated_hyperbolic_shear_hall";
  p.inner = InnerFunction::F7;
  p.reduction = ReductionType::PareidoliaCathedral;

  p.zn.zn[0] = {1.18f, ZN0_IM};
  p.zn.zn[1] = {0.06f, -0.04f};
  p.zn.zn[2] = {0.88f, 0.16f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 1.05f;
  p.color.fade = 0.900f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.50f;
  p.color.spiral_radius_coeff = 0.35f;
  p.color.chroma_breath_amp = 0.16f;
  p.color.chroma_breath_freq = 0.083f;
  p.color.chroma_breath_phase = 0.30f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.055f;
  p.color.hue_velocity_drift = 0.014f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.04f;
  p.seed.hue = 0.62f;
  p.seed.hue_speed = 0.030f;
  p.seed.alpha = 0.48f;
  p.seed.pulse_freq = 0.083f;
  p.seed.pulse_amp = 0.0f;

  p.speed = 0.85f;

  p.motion
      .add("f7_shear_twist",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.044f,
                            .amplitude = 0.06f,
                            .offset = ZN0_IM},
           control::ModTarget::Zn0Im)
      .add("oklab_hue_swing",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.029f,
                            .amplitude = 0.030f,
                            .offset = 0.015f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
