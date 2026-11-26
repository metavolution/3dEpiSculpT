#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Ammann-Beenker quasicrystal-inspired preset.
 *
 * Scientific inspiration:
 * - 8-fold quasiperiodic order from incommensurate wave directions, related to
 *   diffraction patterns in quasicrystalline materials and op-art moire fields.
 *
 * Mapping to this engine's canonical OpenCL flow:
 * - pre/post affine use two non-commensurate rotations (pi/8 and 2*pi*(sqrt(2)-1)),
 * - `cos` inner acts as a standing-wave nonlinearity,
 * - torus reduction keeps smooth bounded recurrence for stable long-run feedback,
 * - spatial-wave color emphasizes slowly drifting quasiperiodic phase bands.
 *
 * Pipeline is unchanged:
 * transform -> reduce -> sample -> color -> seed -> blend.
 */
inline Pattern ammann_beenker_quasicrystal() {
  constexpr float SQRT2 = 1.4142135624f;
  constexpr float SILVER = 1.0f + SQRT2;       // 1 + sqrt(2)
  constexpr float INV_SILVER = 0.4142135624f;  // sqrt(2) - 1
  constexpr float PRE = 1.42f;
  constexpr float POST = 0.98f;
  // pi/8 (22.5 deg)
  constexpr float C8 = 0.9238795325f;
  constexpr float S8 = 0.3826834324f;
  // 2*pi*(sqrt(2)-1): irrationally related to pi/8 for quasiperiodic drift.
  constexpr float CQ = -0.8582161857f;
  constexpr float SQ = 0.5132883972f;

  Pattern p;
  p.name = "ammann_beenker_quasicrystal";
  p.inner = InnerFunction::Cos;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {C8 * PRE, S8 * PRE};
  p.zn.zn[1] = {0.011f, -0.017f};
  p.zn.zn[2] = {CQ * POST, SQ * POST};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.066f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.047f;
  p.zn.zn1_anim_amp = 0.025f;
  p.zn.zn1_anim_k = 8.0f;

  p.color.hue_rotation = 0.005f;
  p.color.sat_scale = 1.10f;
  p.color.fade = 0.988f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.7071f;
  p.color.spiral_radius_coeff = INV_SILVER;
  p.color.chroma_breath_amp = 0.13f;
  p.color.chroma_breath_freq = 0.6180f;
  p.color.chroma_breath_phase = 0.271f;
  p.color.hue_velocity_base = 0.009f;
  p.color.hue_velocity_breath = 0.013f;
  p.color.hue_velocity_drift = 0.0025f;
  p.color.luma_shape_depth = 0.24f;
  p.color.luma_shape_center = SILVER;

  p.seed.border_width = 0.080f;
  p.seed.hue_speed = 0.052f;
  p.seed.alpha = 0.92f;
  p.speed = 0.63f;
  return p;
}

} // namespace epi::presets
