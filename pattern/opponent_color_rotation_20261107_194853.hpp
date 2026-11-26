/**
 * ============================================================================
 * EPI2 - PRESET: OPPONENT COLOR ROTATION
 * ============================================================================
 *
 * Perceptual opponent-channel chromatic rotation (YCoCg / Hurvich–Jameson
 * red–green vs. blue–yellow opponent axes) over a counter-sheared trigonometric
 * torus recurrence. Replaces the non-existent external `res/seed_noise.png`
 * placeholder with a self-contained procedural OrganicNoise boundary and
 * multi-parameter opponent-axis + post-affine shear animation.
 */

#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern make_opponent_color_rotation() {
  Pattern p;
  p.name = "opponent_color_rotation";

  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Torus;

  // Expansive pre-affine spiral + animated pre-translate & post-shear
  p.zn.zn[0] = {1.16f, 0.24f};
  p.zn.zn[1] = {0.06f, -0.04f};
  p.zn.zn[2] = {0.96f, -0.14f};
  p.zn.zn[3] = {0.02f, 0.03f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.078f;
  p.zn.zn0_anim_amp = 0.065f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.061f;
  p.zn.zn1_anim_amp = 0.095f;
  p.zn.zn1_anim_k = 3.0f;

  // Color Mode 6: Opponent (YCoCg perceptual opponent-axis rotation)
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.hue_rotation = 0.032f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.988f;
  p.color.spiral_angle_coeff = 1.65f;
  p.color.spiral_radius_coeff = 0.85f;
  p.color.chroma_breath_amp = 0.26f;
  p.color.chroma_breath_freq = 0.16f;
  p.color.hue_velocity_base = 0.045f;
  p.color.hue_velocity_breath = 0.022f;
  p.color.hue_velocity_drift = 0.010f;
  p.color.luma_shape_depth = 0.16f;
  p.color.luma_shape_center = -4.20f;

  p.seed.type = SeedType::OrganicNoise;
  p.seed.border_width = 0.085f;
  p.seed.hue = 0.14f;
  p.seed.hue_speed = 0.22f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.21f;
  p.seed.pulse_amp = 0.24f;

  p.speed = 1.05f;

  p.motion
      .add("opponent_pitch",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.11f,
                            .amplitude = 0.42f,
                            .offset = 1.65f},
           control::ModTarget::SpiralAngleCoeff)
      .add("post_shear_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.073f,
                            .amplitude = 0.085f,
                            .offset = -0.14f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Im)
      .add("chroma_pulse",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.089f,
                            .amplitude = 0.09f,
                            .offset = 0.26f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
