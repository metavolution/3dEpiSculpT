#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern oklch_opponent_drift() {
  Pattern p;
  p.name = "oklch_opponent_drift";
  
  // A swirling organic pattern where OKLCH really shines
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Torus;

  // Gently twisting and expanding parameters
  p.zn.zn[0] = {1.3486f, 0.2380f};
  p.zn.zn[1] = {0.0547f, -0.0327f};
  
  p.zn.zn[2] = {1.2043f, -0.1417f};
  p.zn.zn[3] = {0.0f, 0.0f};

  // Color Mode 17: OKLab/OKLCH Color Rotation
  p.color.color_mode = presets::as_int(presets::ColorMode::OKLabColorRotation);
  p.color.sat_scale = 1.15f;
  p.color.fade = 0.985f;
  p.color.hue_rotation = 0.03f;
  
  // Moderate breathing and spiraling to showcase the smooth color transitions
  p.color.spiral_angle_coeff = 0.6f;
  p.color.spiral_radius_coeff = 0.3f;
  p.color.chroma_breath_amp = 0.25f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 1.5f;

  // Soft seed to keep the focus on feedback and color
  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.083f;
  p.seed.hue_speed = 0.101f;

  p.motion.add("oklch_color_drift",
      control::ModLane{.wave         = control::Waveform::Sine,
                       .frequency_hz = 0.05f,
                       .amplitude    = 0.018f,
                       .offset       = 0.028f},
      control::ModTarget::HueRot);
  p.seed.alpha = 0.92f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.124f;
  p.zn.zn0_anim_amp = 0.046f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.136f;
  p.zn.zn1_anim_amp = 0.054f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.088f;
  p.seed.pulse_amp = 0.19f;


  return p;
}

} // namespace epi::presets
