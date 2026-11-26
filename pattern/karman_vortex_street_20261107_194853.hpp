#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * von Karman vortex street inspired preset.
 *
 * Physical inspiration:
 * - alternating vortex shedding in a fluid wake behind an obstacle.
 *
 * Mapping to this engine's canonical flow:
 * - wrapped complex transform introduces advective shear + rotational offset,
 * - grid reduction enforces repeated wake-cell structure,
 * - feedback memory carries alternating shed trails frame-to-frame,
 * - alive color mode visualizes phase/velocity drift of the wake.
 *
 * This is fully compatible with the existing OpenCL pipeline:
 * transform -> reduce -> sample -> color -> seed -> blend.
 */
inline Pattern karman_vortex_street() {
  Pattern p;
  p.name = "karman_vortex_street";
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Grid;

  // Bluff-body wake advection + Strouhal alternating shedding trajectory.
  p.zn.zn[0] = {1.295f, 0.280f};
  p.zn.zn[1] = {0.075f, -0.032f};
  p.zn.zn[2] = {0.965f, -0.350f};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.19f;
  p.zn.zn0_anim_amp = 0.048f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.36f;
  p.zn.zn1_anim_amp = 0.068f;
  p.zn.zn1_anim_k = 2.0f;

  p.color.hue_rotation = 0.011f;
  p.color.sat_scale = 1.09f;
  p.color.fade = 0.987f;
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 0.58f;
  p.color.spiral_radius_coeff = 0.34f;
  p.color.chroma_breath_amp = 0.11f;
  p.color.chroma_breath_freq = 0.15f;
  p.color.chroma_breath_phase = 0.52f;
  p.color.hue_velocity_base = 0.017f;
  p.color.hue_velocity_breath = 0.022f;
  p.color.hue_velocity_drift = 0.0035f;
  p.color.luma_shape_depth = 0.045f;
  p.color.luma_shape_center = -4.32f;

  p.seed.border_width = 0.095f;
  p.seed.hue_speed = 0.068f;
  p.seed.alpha = 0.92f;
  p.speed = 0.78f;
  return p;
}

} // namespace epi::presets
