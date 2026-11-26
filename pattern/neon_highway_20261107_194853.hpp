#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern neon_highway() {
  Pattern p;
  p.name = "neon_highway";
  p.inner = InnerFunction::Tanh;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {1.5290f, 0.1699f};
  p.zn.zn[2] = {1.3333f, 0.0702f};
  p.color.hue_rotation = 0.025f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.986f;

  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.50f;
  p.color.spiral_radius_coeff = 0.35f;
  p.color.chroma_breath_amp = 0.14f;
  p.color.chroma_breath_freq = 0.42f;
  p.color.chroma_breath_phase = 0.09f;
  p.color.hue_velocity_base = 0.028f;
  p.color.hue_velocity_breath = 0.016f;
  p.color.hue_velocity_drift = 0.009f;
  p.color.luma_shape_depth = 0.12f;
  p.color.luma_shape_center = 3.0f;

  p.seed.border_width = 0.083f;
  p.seed.hue_speed = 0.25f;
  p.seed.alpha = 0.92f;
  p.speed = 1.4f;

  // ── Motion layer: aperiodic translation wander + hue tempo ────────────
  //
  // Neon highway runs fast (speed=1.4) with Grid reduction — the tanh
  // inner creates compressed, tunnel-like corridors.  Static zn1 = {0,0}
  // fixes the vanishing point at the origin.  Shifting zn1 via two
  // independent NoiseHash lanes (different seeds) moves the vanishing
  // point aperiodically through the complex plane, making the tunnel
  // appear to curve and drift without ever establishing a repeating orbit.
  //
  // NoiseHash amplitude 0.013 keeps the drift subtle — at full zoom
  // (zn0 magnitude ≈ 0.906) a 0.013 translation shift corresponds to
  // roughly 1.4% of the domain width, imperceptible as a jump but
  // cumulatively significant over the ~18-second wander period.
  //
  // hue_tempo: the preset's fast chroma_breath_freq=0.42 already provides
  // high-frequency colour variation; the hue_tempo lane adds a slow
  // 18-second sweep ±0.009 around hue_rotation=0.025, shifting the
  // overall chromatic "temperature" of the highway between warm and cool.
  p.motion = control::PatternMotion::translation_wander(
      /*re_centre=*/0.0f, /*im_centre=*/0.0f,
      /*amp=*/0.013f, /*period_s=*/18.0f, /*seed=*/7u);
  p.motion.add("hue_tempo",
      control::ModLane{.wave         = control::Waveform::Sine,
                       .frequency_hz = 1.0f / 18.0f,
                       .amplitude    = 0.009f,
                       .offset       = 0.025f},  // resting = preset hue_rotation
      control::ModTarget::HueRot);
  p.zn.zn[1] = {0.2371f, -0.1849f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.139f;
  p.zn.zn0_anim_amp = 0.036f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.212f;
  p.zn.zn1_anim_amp = 0.049f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.138f;
  p.seed.pulse_amp = 0.21f;


  return p;
}

} // namespace epi::presets
