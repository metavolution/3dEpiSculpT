#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Kelvin-Helmholtz shear-layer roll-up inspired preset.
 *
 * Scientific motivation:
 * - Kelvin-Helmholtz instability appears when two fluid layers with different
 *   velocities slide past each other; a small perturbation grows into
 *   characteristic billow/roll-up vortices.
 *
 * Mapping to this engine's canonical OpenCL flow:
 * - pre/post affine + `sinh` inner produce shear and curl amplification,
 * - grid reduction creates repeated shear cells (periodic wake strips),
 * - feedback sampling carries temporal advection memory,
 * - alive color mode visualizes local phase and vorticity-like drift.
 *
 * Pipeline remains unchanged:
 * transform -> reduce -> sample -> color -> seed -> blend.
 */
inline Pattern kelvin_helmholtz_rollup() {
  Pattern p;
  p.name = "kelvin_helmholtz_rollup";
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Grid;

  // Counter-streaming shear + rotational billow curl that couples across Grid
  // shear interfaces while folding cat's-eye vortex cores into the interior.
  p.zn.zn[0] = {1.265f, 0.340f};
  p.zn.zn[1] = {-0.036f, 0.018f};
  p.zn.zn[2] = {0.935f, -0.365f};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.21f;
  p.zn.zn0_anim_amp = 0.052f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.29f;
  p.zn.zn1_anim_amp = 0.044f;
  p.zn.zn1_anim_k = 2.0f;

  p.color.hue_rotation = 0.010f;
  p.color.sat_scale = 1.09f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 0.68f;
  p.color.spiral_radius_coeff = 0.36f;
  p.color.chroma_breath_amp = 0.11f;
  p.color.chroma_breath_freq = 0.12f;
  p.color.chroma_breath_phase = 0.46f;
  p.color.hue_velocity_base = 0.016f;
  p.color.hue_velocity_breath = 0.021f;
  p.color.hue_velocity_drift = 0.0035f;
  // Calibrated to canonical log_r = log(0.01 * |t_z|) ~ -4.3 so billow bands
  // alternate bright/dark ridges without draining luminance.
  p.color.luma_shape_depth = 0.046f;
  p.color.luma_shape_center = -4.30f;

  p.seed.type = SeedType::OrganicNoise;
  p.seed.border_width = 0.105f;
  p.seed.hue_speed = 0.055f;
  p.seed.alpha = 0.92f;
  p.speed = 0.78f;
  return p;
}

} // namespace epi::presets
