#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * FitzHugh-Nagumo-inspired excitable tissue spiral-wave preset.
 *
 * Scientific inspiration:
 * - Spiral defects in excitable media (cardiac tissue and chemical reactors)
 *   where local nonlinear response + transport memory creates rotating
 *   wavefronts and phase singularities.
 *
 * Mapping to the canonical engine flow:
 * - `tanh` inner provides saturating excitability-like nonlinearity,
 * - torus reduction enforces periodic domain recurrence (classic PDE boundary),
 * - feedback sampling carries the transport/memory term across frames,
 * - alive color mode emphasizes phase rotation and wavefront breathing.
 *
 * Pipeline remains unchanged:
 * transform -> reduce -> sample -> color -> seed -> blend.
 */
inline Pattern fitzhugh_nagumo_spiral_tissue() {
  Pattern p;
  p.name = "fitzhugh_nagumo_spiral_tissue";
  p.inner = InnerFunction::Tanh;
  p.reduction = ReductionType::Torus;

  // Supercritical rotational coupling driving spiral wavefronts across torus rim.
  p.zn.zn[0] = {1.38f, 0.42f};
  p.zn.zn[1] = {0.014f, -0.019f};
  p.zn.zn[2] = {1.06f, -0.33f};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.078f;
  p.zn.zn0_anim_amp = 0.044f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.054f;
  p.zn.zn1_anim_amp = 0.032f;
  p.zn.zn1_anim_k = 3.0f;

  p.color.hue_rotation = 0.0075f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.988f;
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 0.92f;
  p.color.spiral_radius_coeff = 0.36f;
  p.color.chroma_breath_amp = 0.14f;
  p.color.chroma_breath_freq = 0.12f;
  p.color.chroma_breath_phase = 0.52f;
  p.color.hue_velocity_base = 0.012f;
  p.color.hue_velocity_breath = 0.017f;
  p.color.hue_velocity_drift = 0.0024f;
  p.color.luma_shape_depth = 0.21f;
  p.color.luma_shape_center = 2.2f;

  p.seed.border_width = 0.080f;
  p.seed.hue_speed = 0.041f;
  p.seed.alpha = 0.92f;
  p.speed = 0.66f;
  return p;
}

} // namespace epi::presets
