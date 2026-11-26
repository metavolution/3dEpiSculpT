#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern living_palette_agent() {
  Pattern p;
  p.name = "living_palette_agent";
  
  // A swirling organic pattern where the stateless OKLab agent shines
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Torus;

  // Gently twisting and expanding parameters
  p.zn.zn[0] = {1.3466f, 0.2376f};
  p.zn.zn[1] = {0.0502f, -0.0310f};
  
  p.zn.zn[2] = {1.1576f, -0.1362f};
  p.zn.zn[3] = {0.0f, 0.0f};

  // Color Mode 21: Living Palette Agent
  p.color.color_mode = as_int(ColorMode::LivingPaletteAgent);
  p.color.sat_scale = 1.15f;
  p.color.fade = 0.985f;
  p.color.hue_rotation = 0.02f;
  
  // Quasi-irrational drift tuning parameters
  p.color.spiral_angle_coeff = 0.4f;
  p.color.spiral_radius_coeff = 0.2f;
  p.color.chroma_breath_amp = 0.35f;
  p.color.chroma_breath_freq = 0.06f;
  p.color.chroma_breath_phase = 1.5f;

  // Luma shaping driven by depth and time (centered at log_r ~ -4.25)
  p.color.luma_shape_depth = 0.07f;
  p.color.luma_shape_center = -4.25f;
  p.color.fade = 0.976f;
  p.color.sat_scale = 1.24f;
  p.color.hue_velocity_base = 0.18f; // time scalar for the Luma breathing wave in this mode

  // Organic Seed to ground the "Alive Artist" feeling
  p.seed.type = SeedType::OrganicNoise;
  p.seed.border_width = 0.081f;
  p.seed.hue_speed = 0.119f;

  p.motion.add("living_color_drift",
      control::ModLane{.wave         = control::Waveform::Sine,
                       .frequency_hz = 0.02f,
                       .amplitude    = 0.018f,
                       .offset       = 0.028f},
      control::ModTarget::HueRot);
  p.seed.alpha = 0.92f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.096f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.210f;
  p.zn.zn1_anim_amp = 0.047f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.129f;
  p.seed.pulse_amp = 0.19f;


  return p;
}

} // namespace epi::presets
