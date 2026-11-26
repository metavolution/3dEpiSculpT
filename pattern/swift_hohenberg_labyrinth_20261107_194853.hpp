#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Swift-Hohenberg-inspired labyrinth preset.
 *
 * Scientific inspiration:
 * - finite-wavelength pattern-forming instability (stripe/labyrinth regimes)
 *   that appears in convection and reaction-diffusion-style media.
 *
 * Mapping to this engine's canonical OpenCL flow:
 * - wrapped affine + `cos` inner emphasize banded standing-wave structure,
 * - grid reduction enforces repeated wavelength cells (labyrinth tiling),
 * - feedback recursion supplies nonlinearity and mode competition over time,
 * - spatial-wave color mapping highlights stripe phase drift and domain walls.
 *
 * Pipeline is unchanged:
 * transform -> reduce -> sample -> color -> seed -> blend.
 */
inline Pattern swift_hohenberg_labyrinth() {
  Pattern p;
  p.name = "swift_hohenberg_labyrinth";
  p.inner = InnerFunction::Cos;
  p.reduction = ReductionType::Grid;

  // Stripe-forming anisotropy with mild shear and rotational coupling.
  p.zn.zn[0] = {1.2728f, 0.4757f};
  p.zn.zn[1] = {0.0436f, -0.0251f};
  p.zn.zn[2] = {1.1601f, -0.3531f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.006f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.984f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.84f;
  p.color.spiral_radius_coeff = 0.47f;
  p.color.chroma_breath_amp = 0.10f;
  p.color.chroma_breath_freq = 0.73f;
  p.color.chroma_breath_phase = 0.13f;
  p.color.hue_velocity_base = 0.010f;
  p.color.hue_velocity_breath = 0.008f;
  p.color.hue_velocity_drift = 0.002f;
  p.color.luma_shape_depth = 0.28f;
  p.color.luma_shape_center = 2.9f;

  p.seed.border_width = 0.079f;
  p.seed.hue_speed = 0.106f;
  p.seed.alpha = 0.92f;
  p.speed = 0.58f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.096f;
  p.zn.zn0_anim_amp = 0.037f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.176f;
  p.zn.zn1_anim_amp = 0.038f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.093f;
  p.seed.pulse_amp = 0.19f;

  return p;
}

} // namespace epi::presets
