#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Bridget Riley "Blaze" Eccentric Serrated Vortex & Microsaccadic Shimmer.
 *
 * Perceptual neuroscience & op-art foundation:
 * - Bridget Riley (Blaze 1, 1964) & Zanker / Walker (2004, Vision Research):
 *   interlocking eccentric serrated wedges with abrupt radial phase reversals
 *   convert involuntary ocular microsaccades (1-3 Hz) into vivid counter-
 *   rotating streaming torque along zigzag locus boundaries.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Glitch (z + 0.1*sin(15*Re(z))) + Grid wrap: injects high-frequency
 *   saw-serrated phase slips into the pre-rotated radial field!
 * - Color = IsoluminantVibrate (mode 8): near-equiluminant complementary bands
 *   that silence magnocellular edge locking while maximizing parvocellular
 *   chromatic border flicker.
 */
inline Pattern op_art_riley_blaze_vortex() {
  Pattern p;
  p.name = "op_art_riley_blaze_vortex";
  p.inner = InnerFunction::Glitch;
  p.reduction = ReductionType::Grid;

  p.zn.zn[0] = {1.14f, 0.34f};
  p.zn.zn[1] = {0.065f, -0.042f};
  p.zn.zn[2] = {1.05f, -0.22f};
  p.zn.zn[3] = {-0.030f, 0.024f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.115f;
  p.zn.zn0_anim_amp = 0.048f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.148f;
  p.zn.zn1_anim_amp = 0.072f;
  p.zn.zn1_anim_k = 4.0f;

  p.color.hue_rotation = 0.018f;
  p.color.sat_scale = 1.15f;
  p.color.fade = 0.982f;
  p.color.color_mode = as_int(ColorMode::IsoluminantVibrate);
  p.color.spiral_angle_coeff = 4.0f;
  p.color.spiral_radius_coeff = 2.35f;
  p.color.chroma_breath_amp = 0.28f;
  p.color.chroma_breath_freq = 0.42f;
  p.color.chroma_breath_phase = 0.15f;
  p.color.hue_velocity_base = 0.035f;
  p.color.hue_velocity_breath = 0.020f;
  p.color.hue_velocity_drift = 0.008f;
  p.color.luma_shape_depth = 0.18f;
  p.color.luma_shape_center = -4.15f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.072f;
  p.seed.hue = 0.04f;
  p.seed.hue_speed = 0.16f;
  p.seed.alpha = 0.93f;
  p.seed.pulse_freq = 0.18f;
  p.seed.pulse_amp = 0.22f;

  p.speed = 0.98f;

  p.motion
      .add("blaze_wedge_pitch",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.094f,
                            .amplitude = 0.75f,
                            .offset = 4.0f},
           control::ModTarget::SpiralAngleCoeff)
      .add("eccentric_counter_torque",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.068f,
                            .amplitude = 0.11f,
                            .offset = -0.22f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Im)
      .add("serration_radial_density",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.081f,
                            .amplitude = 0.52f,
                            .offset = 2.35f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("microsaccade_flicker_rate",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.055f,
                            .amplitude = 0.14f,
                            .offset = 0.42f},
           control::ModTarget::ChromaBreathFreq);

  return p;
}

} // namespace epi::presets
