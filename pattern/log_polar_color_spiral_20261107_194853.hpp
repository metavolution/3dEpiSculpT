#pragma once

#include "../../pattern.hpp"

namespace epi::presets {

inline Pattern log_polar_color_spiral() {
  Pattern p;
  p.name = "log_polar_color_spiral";
  p.inner = InnerFunction::P1; // polynomial warp
  p.reduction = ReductionType::Torus; // seamless folding

  // Geometry: supercritical log-polar spiral coupling
  p.zn.zn[0] = Complexf{1.14f, 0.18f};
  p.zn.zn[1] = Complexf{-0.016f, 0.012f};
  p.zn.zn[2] = Complexf{0.98f, -0.09f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.19f;
  p.zn.zn0_anim_amp = 0.07f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.13f;
  p.zn.zn1_anim_amp = 0.018f;
  p.zn.zn1_anim_k = 5.0f;

  // Enable LogPolarColorSpiral color mode (11)
  p.color.color_mode = presets::as_int(presets::ColorMode::LogPolarColorSpiral);
  p.color.hue_rotation = 0.008f; // h0
  p.color.sat_scale = 1.05f;     // C0
  
  // Log-polar hue spiral coefficients
  p.color.spiral_angle_coeff = 2.0f;  // aθ
  p.color.spiral_radius_coeff = 1.35f; // ar
  
  // Chroma breathing envelope
  p.color.chroma_breath_amp = 0.26f;   // b
  p.color.chroma_breath_freq = 0.09f;  // ω
  p.color.chroma_breath_phase = 1.0f;  // ϕ
  p.color.hue_velocity_base = 0.016f;
  
  // Lightness shaping matched to len = 0.01 * |t_z|
  p.color.luma_shape_center = 0.012f;  // r0
  p.color.luma_shape_depth = 0.06f;    // d

  // Persistence for spiral arm layering
  p.color.fade = 0.986f;

  p.seed.border_width = 0.075f;
  p.seed.hue_speed = 0.08f;
  p.seed.alpha = 0.94f;
  p.speed = 0.85f;

  return p;
}

} // namespace epi::presets
