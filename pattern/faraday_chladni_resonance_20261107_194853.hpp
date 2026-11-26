#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Faraday/Chladni resonance preset.
 *
 * Scientific inspiration:
 * - Standing-wave nodal structures from parametrically driven fluid surfaces
 *   (Faraday waves) and plate eigenmodes (Chladni figures).
 * - Visualized here as quasiperiodic interference under iterative feedback.
 *
 * Canonical pipeline remains unchanged:
 * transform -> reduce -> sample -> color(mode4 spatial wave) -> seed -> blend
 */
inline Pattern faraday_chladni_resonance() {
  constexpr float SQRT2 = 1.4142135624f;
  constexpr float SQRT3 = 1.7320508076f;
  constexpr float PRE = 0.918f;
  constexpr float POST = 0.892f;

  // 2*pi/12 (30 deg)
  constexpr float C12 = 0.8660254038f;
  constexpr float S12 = 0.5000000000f;
  // 2*pi*(sqrt(3)-1)
  constexpr float CQ = -0.1378756076f;
  constexpr float SQ = -0.9904492020f;

  Pattern p;
  p.name = "faraday_chladni_resonance";
  p.inner = InnerFunction::Cos;
  p.reduction = ReductionType::Grid;

  p.zn.zn[0] = {C12 * PRE, S12 * PRE};
  p.zn.zn[1] = {0.008f, -0.005f};
  p.zn.zn[2] = {CQ * POST, SQ * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 1.18f;
  p.color.fade = 0.976f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.79f; // hue x-slope
  p.color.spiral_radius_coeff = SQRT2; // wave1 spatial frequency
  p.color.chroma_breath_amp = 0.21f; // wave1 amplitude
  p.color.chroma_breath_freq = SQRT3; // wave2 spatial frequency
  p.color.chroma_breath_phase = 0.10f; // wave2 amplitude
  p.color.hue_velocity_base = 0.023f; // wave1 temporal speed
  p.color.hue_velocity_breath = 0.016f; // wave2 temporal speed
  p.color.hue_velocity_drift = 0.009f; // base drift
  p.color.luma_shape_depth = 0.32f; // saturation breathing amplitude
  p.color.luma_shape_center = 4.4f; // saturation breathing frequency

  p.seed.border_width = 0.010f;
  p.seed.hue_speed = 0.070f;
  p.seed.alpha = 0.91f;
  p.speed = 0.98f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.158f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.206f;
  p.zn.zn1_anim_amp = 0.055f;
  p.zn.zn1_anim_k = 5.0f;
  p.seed.pulse_freq = 0.096f;
  p.seed.pulse_amp = 0.19f;

  return p;
}

} // namespace epi::presets
