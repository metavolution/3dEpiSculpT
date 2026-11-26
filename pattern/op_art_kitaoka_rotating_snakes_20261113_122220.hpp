#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Op-Art: Kitaoka "Rotating Snakes" Peripheral Drift Illusion.
 *
 * Scientific foundation:
 * - A. Kitaoka & H. Ashida (2003): a static image rotates in peripheral
 *   vision when it repeats an ASYMMETRIC luminance staircase
 *   (black -> dark blue -> white -> yellow -> black). High-contrast edges are
 *   processed faster than low-contrast ones (Conway et al. 2005; Backus &
 *   Oruc 2005), so each fixation produces an ordered burst of motion-detector
 *   responses; refreshed by every saccade or blink, the discs appear to turn.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Seed = Simplex border with a WIDE ramp (0.16): intensity rises linearly
 *   and then cuts off, a sawtooth luminance profile.
 * - Reduction = Grid: the discontinuous modular wrap repeats that ramp with a
 *   hard reset, an asymmetric staircase, where a mirrored torus fold would
 *   make it symmetric and kill the illusion.
 * - Inner = Exp: straight repeated ramps become concentric rings and radial
 *   spokes (log-polar), the snake discs.
 * - Color = Opponent with hue_rotation = 0.5 and all hue velocities ZERO: the
 *   kernel rotates the (Co, Cg) opponent plane by half a turn per pass, so
 *   successive generations alternate blue <-> yellow, Kitaoka's palette. With
 *   velocities zero the kernel's fallback adds time*0.01, slowly detuning the
 *   exact opponent flip so the colour pairing drifts.
 */
inline Pattern op_art_kitaoka_rotating_snakes() {
  constexpr float A0 = -0.61086524f; // -35 deg
  constexpr float A2 = 0.17453293f;  // 10 deg
  constexpr float PRE = 1.70f;
  constexpr float POST = 1.09f;
  const float zn2_im = POST * std::sin(A2);

  Pattern p;
  p.name = "op_art_kitaoka_rotating_snakes";
  p.inner = InnerFunction::Exp;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {PRE * std::cos(A0), PRE * std::sin(A0)};
  p.zn.zn[1] = {0.05f, 0.035f};
  p.zn.zn[2] = {POST * std::cos(A2), zn2_im};
  p.zn.zn[3] = {-0.02f, 0.02f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.08f;
  p.zn.zn0_anim_amp = 0.03f;

  p.color.hue_rotation = 0.5f;          // opponent flip per generation
  p.color.sat_scale = 1.06f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 1.0f;
  p.color.spiral_radius_coeff = 0.30f;
  p.color.chroma_breath_amp = 0.20f;
  p.color.chroma_breath_freq = 0.10f;
  p.color.chroma_breath_phase = 6.2831853f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.16f;
  p.seed.hue = 0.62f;       // blue
  p.seed.hue_speed = 0.01f;
  p.seed.alpha = 0.90f;
  p.seed.pulse_freq = 0.07f;
  p.seed.pulse_amp = 0.12f;
  p.speed = 0.7f;

  p.motion
      .add("peripheral_drift_twist",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.05f,
                            .amplitude = 0.07f,
                            .offset = zn2_im},
           control::ModTarget::Zn2Im)
      .add("luminance_ramp_width",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.04f,
                            .amplitude = 0.05f,
                            .offset = 0.16f},
           control::ModTarget::SeedWidth)
      .add("snake_ring_pitch",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.06f,
                            .amplitude = 0.12f,
                            .offset = 0.30f},
           control::ModTarget::SpiralRadiusCoeff);

  return p;
}

} // namespace epi::presets
