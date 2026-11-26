#pragma once

#include "../../pattern.hpp"

namespace epi::presets {

/**
 * WaveRing Resonance preset.
 * Showcases the newly added WaveRing SeedType.
 * It uses a concentric ring expanding seed and couples it
 * with a slow Tanh tunnel compression to create a deeply
 * resonant pulsating corridor.
 */
inline Pattern wave_ring_resonance() {
  Pattern p;
  p.name = "wave_ring_resonance";

  p.inner = InnerFunction::Tanh; // Tunnel compression
  p.reduction = ReductionType::Grid;
  p.speed = 0.85f;

  p.zn.zn[0] = {1.5262f, 0.0969f};   // supercritical pre-scale crosses tanh saturation knee
  p.zn.zn[1] = {0.2739f, -0.1703f};
  p.zn.zn[2] = {1.3251f, 0.0811f};   // resonant inward spiral
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.18f;
  p.zn.zn0_anim_amp = 0.07f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.12f;
  p.zn.zn1_anim_amp = 0.018f;
  p.zn.zn1_anim_k = 4.0f;

  // Setup the newly added seed type
  p.seed.type = SeedType::WaveRing;
  p.seed.hue = 0.6f;             // starting at deep blue
  p.seed.hue_speed = 0.119f;      // slowly drifting hue
  p.seed.alpha = 0.92f;
  p.seed.saturation = 0.92f;

  // Use an Alive Color system to respond to the rings
  p.color.color_mode = presets::as_int(presets::ColorMode::Alive); // Alive color mode
  p.color.hue_rotation = 0.006f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.982f;

  p.color.spiral_angle_coeff = 0.35f;
  p.color.spiral_radius_coeff = 1.6f; // hue tied strongly to radius
  p.color.chroma_breath_amp = 0.24f;
  p.color.chroma_breath_freq = 0.10f;
  p.color.hue_velocity_base = 0.018f;
  p.color.luma_shape_center = -4.2f;
  p.color.luma_shape_depth = 0.05f;
  p.seed.border_width = 0.080f;

  return p;
}

} // namespace epi::presets
