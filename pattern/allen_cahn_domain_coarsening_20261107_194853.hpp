#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Allen-Cahn-inspired domain coarsening preset.
 *
 * Scientific inspiration:
 * - phase-separation and grain-boundary dynamics from Allen-Cahn style
 *   reaction terms (u - u^3) used in materials science and biology.
 *
 * Mapping to this engine's canonical OpenCL flow:
 * - wrapped affine + cubic inner approximates bistable domain competition,
 * - torus reduction preserves continuity while allowing long-run coarsening,
 * - recursive sample/blend evolves moving interfaces and annihilating defects,
 * - alive color mode tracks interface activity rather than static texture.
 *
 * Pipeline is unchanged:
 * transform -> reduce -> sample -> color -> seed -> blend.
 */
inline Pattern allen_cahn_domain_coarsening() {
  Pattern p;
  p.name = "allen_cahn_domain_coarsening";
  p.inner = InnerFunction::Cube;
  p.reduction = ReductionType::Torus;

  // Mild anisotropy to promote curved domain walls and defect motion.
  p.zn.zn[0] = {1.3066f, 0.3183f};
  p.zn.zn[1] = {0.2087f, -0.2059f};
  p.zn.zn[2] = {1.2540f, -0.2461f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.007f;
  p.color.sat_scale = 1.18f;
  p.color.fade = 0.978f;
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 0.85f;
  p.color.spiral_radius_coeff = 0.44f;
  p.color.chroma_breath_amp = 0.18f;
  p.color.chroma_breath_freq = 0.14f;
  p.color.chroma_breath_phase = 0.21f;
  p.color.hue_velocity_base = 0.018f;
  p.color.hue_velocity_breath = 0.017f;
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.08f;
  p.color.luma_shape_center = -4.25f;

  p.seed.border_width = 0.091f;
  p.seed.hue_speed = 0.112f;
  p.seed.alpha = 0.92f;
  p.speed = 0.57f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.136f;
  p.zn.zn0_anim_amp = 0.048f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.166f;
  p.zn.zn1_anim_amp = 0.046f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.136f;
  p.seed.pulse_amp = 0.18f;

  return p;
}

} // namespace epi::presets
