#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Op-Art: Fraser Spiral ("False Spiral") Twisted-Cord Illusion.
 *
 * Scientific foundation:
 * - J. Fraser (1908): a set of perfectly CONCENTRIC circles drawn as twisted
 *   two-tone cords on an oblique background is seen, irresistibly, as a
 *   spiral. Orientation-selective V1 cells signal the local tilt of each cord
 *   segment; the visual system integrates those tilts along the contour and
 *   infers a curve that never closes. Tracing a ring with a finger breaks it.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Seed = WaveRing: concentric rings, the true (closed) geometry.
 * - Inner = Tanh with a small 9 deg / -4 deg rotation pair: each feedback
 *   generation re-draws the rings slightly rotated and compressed, laying an
 *   oblique texture across them: the tilt cue.
 * - Color = IsoluminantVibrate: the kernel flips hue by a fixed gap wherever
 *   sin(pitch*log|T| + bands*x + t) changes sign, while holding luminance.
 *   This cuts every ring into alternating two-tone segments (the twisted
 *   cord) whose boundaries are oblique to the rings.
 * - Motion: cord twist angle (Zn2Im), two-tone hue gap, ring pitch and
 *   segment density sweep through weak and strong regimes of the illusion.
 */
inline Pattern op_art_fraser_spiral_twisted_cord() {
  constexpr float A0 = 0.15707963f;  // 9 deg
  constexpr float A2 = -0.06981317f; // -4 deg
  constexpr float PRE = 1.22f;
  constexpr float POST = 1.24f;
  const float zn2_im = POST * std::sin(A2);

  Pattern p;
  p.name = "op_art_fraser_spiral_twisted_cord";
  p.inner = InnerFunction::Tanh;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {PRE * std::cos(A0), PRE * std::sin(A0)};
  p.zn.zn[1] = {0.012f, -0.008f};
  p.zn.zn[2] = {POST * std::cos(A2), zn2_im};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.10f;
  p.zn.zn0_anim_amp = 0.03f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.13f;
  p.zn.zn1_anim_amp = 0.03f;
  p.zn.zn1_anim_k = 4.0f;

  p.color.hue_rotation = 0.016f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.984f;
  p.color.color_mode = as_int(ColorMode::IsoluminantVibrate);
  p.color.spiral_angle_coeff = 0.5f;   // two-tone hue gap (complementary)
  p.color.spiral_radius_coeff = 2.0f;  // cord pitch in log-radius
  p.color.chroma_breath_amp = 0.20f;
  p.color.chroma_breath_freq = 3.0f;   // segments across the field
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.030f;
  p.color.hue_velocity_breath = 0.018f;
  p.color.hue_velocity_drift = 0.007f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.07f;
  p.seed.hue = 0.05f;
  p.seed.hue_speed = 0.09f;
  p.seed.alpha = 0.93f;
  p.seed.pulse_freq = 0.15f;
  p.seed.pulse_amp = 0.20f;
  p.speed = 0.9f;

  p.motion
      .add("cord_twist_angle",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.06f,
                            .amplitude = 0.07f,
                            .offset = zn2_im},
           control::ModTarget::Zn2Im)
      .add("two_tone_hue_gap",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.08f,
                            .amplitude = 0.12f,
                            .offset = 0.5f},
           control::ModTarget::SpiralAngleCoeff)
      .add("cord_ring_pitch",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.07f,
                            .amplitude = 0.40f,
                            .offset = 2.0f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("segment_density",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.05f,
                            .amplitude = 0.60f,
                            .offset = 3.0f},
           control::ModTarget::ChromaBreathFreq);

  return p;
}

} // namespace epi::presets
