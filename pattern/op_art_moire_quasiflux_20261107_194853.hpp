#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Op-art moire interference preset (physics-grounded quasiperiodic wavefield).
 *
 * Scientific basis:
 * - Moire patterns arise from superposition of near-periodic lattices with
 *   slight angular/frequency mismatch, producing large-scale beat structures.
 * - Here, quasiperiodic drift is built from two incommensurate spatial-wave
 *   frequencies (phi^-1 and sqrt(2)) over a wrapped feedback domain.
 *
 * Mapping to canonical pipeline:
 * transform -> reduce -> sample -> color(mode4 spatial wave) -> seed -> blend
 */
inline Pattern op_art_moire_quasiflux() {
  constexpr float PHI = 1.6180339887f;
  constexpr float INV_PHI = 0.6180339887f;
  constexpr float SQRT2 = 1.4142135624f;

  // 2*pi/9 (40 deg)
  constexpr float C9 = 0.7660444431f;
  constexpr float S9 = 0.6427876097f;
  // 2*pi*(sqrt(2)-1)
  constexpr float CQ = -0.8582161857f;
  constexpr float SQ = 0.5132883972f;

  constexpr float PRE = 1.30f;
  constexpr float POST = 0.97f;

  Pattern p;
  p.name = "op_art_moire_quasiflux";
  p.inner = InnerFunction::Cos;
  p.reduction = ReductionType::Grid;

  // Two non-commensurate wrapped transforms for beat-scale moire structure.
  p.zn.zn[0] = {C9 * PRE, S9 * PRE};
  p.zn.zn[1] = {0.010f, -0.007f};
  p.zn.zn[2] = {CQ * POST, -SQ * POST};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.17f;
  p.zn.zn0_anim_amp = 0.065f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.11f;
  p.zn.zn1_anim_amp = 0.016f;
  p.zn.zn1_anim_k = 9.0f;

  p.color.hue_rotation = 0.013f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.74f; // hue x-slope
  p.color.spiral_radius_coeff = INV_PHI; // wave1 spatial freq
  p.color.chroma_breath_amp = 0.19f; // wave1 amplitude
  p.color.chroma_breath_freq = SQRT2; // wave2 spatial freq
  p.color.chroma_breath_phase = 0.11f; // wave2 amplitude
  p.color.hue_velocity_base = 0.021f; // wave1 time speed
  p.color.hue_velocity_breath = 0.014f; // wave2 time speed
  p.color.hue_velocity_drift = 0.007f; // base drift
  p.color.luma_shape_depth = 0.24f; // saturation breathing amplitude
  p.color.luma_shape_center = 2.0f * PHI; // saturation breathing frequency

  p.seed.border_width = 0.070f;
  p.seed.hue_speed = 0.073f;
  p.seed.alpha = 0.92f;
  p.speed = 0.92f;
  return p;
}

} // namespace epi::presets
