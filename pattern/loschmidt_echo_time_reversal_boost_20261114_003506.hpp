#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Recursive Memory: Loschmidt Echo of a Hyperbolic Boost.
 *
 * Scientific foundation:
 * - Loschmidt's objection (1876): the microscopic laws are reversible, so
 *   reversing every velocity should make a system retrace its history. The
 *   Loschmidt echo measures how well it actually does once the reversed
 *   evolution is imperfect; its decay is a standard measure of
 *   irreversibility (Peres 1984; Jalabert and Pastawski 2001).
 * - Einstein's velocity addition w = (u + v)/(1 + u v) is the Mobius boost
 *   B(z) = (z + beta)/(1 + beta z) of the Poincare disk, a hyperbolic
 *   translation with fixed points -1 and +1. Its inverse is the boost by
 *   -beta. Rapidities add: n boosts give tanh(n artanh beta).
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Invert with zn0 = beta, zn1 = 1, zn2 = -(1 - beta^2)/beta,
 *   zn3 = 1/beta gives T = B exactly. Flipping the sign of zn0, zn2 and zn3
 *   gives B^-1 exactly.
 * - Motion is the experiment: three Square lanes, locked in frequency and
 *   phase, flip those three signs every 2.5 s. For 150 frames the loop
 *   composes B, and the image is squeezed toward one boundary point while
 *   fresh seed streams in behind it; for the next 150 it composes B^-1 and
 *   the image is carried back.
 * - The map is a bijection, so in exact arithmetic the reversal would be
 *   perfect. What is lost is real: detail squeezed below one pixel near the
 *   boundary is averaged by the bilinear sampler and cannot be restored, the
 *   fade removes a fixed fraction per frame, and the seed overwrites the
 *   band. The returning image is the echo; its blur is the coarse-graining
 *   that makes a reversible map irreversible in practice.
 * - beta = 0.02 keeps one pass to about 25 pixels at the centre.
 *   Cancellation in 50 - 49.98/(1 + 0.02 z) costs about 4e-6 in float32,
 *   1/200 of a pixel at 2560.
 * - Color = Classic with a small hue step per pass, so rapidity is read as
 *   hue.
 */
inline Pattern loschmidt_echo_time_reversal_boost() {
  constexpr float BETA = 0.02f;
  constexpr float POLE_GAIN = 49.98f; // (1 - beta^2) / beta
  constexpr float SHIFT = 50.0f;      // 1 / beta
  constexpr float REVERSAL_HZ = 0.2f; // 2.5 s forward, 2.5 s back

  Pattern p;
  p.name = "loschmidt_echo_time_reversal_boost";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {BETA, 0.0f};
  p.zn.zn[1] = {1.0f, 0.0f};
  p.zn.zn[2] = {-POLE_GAIN, 0.0f};
  p.zn.zn[3] = {SHIFT, 0.0f};

  p.color.hue_rotation = 0.006f;
  p.color.sat_scale = 1.04f;
  p.color.fade = 0.992f;
  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f; // bounded hue clock
  p.color.hue_velocity_breath = 0.0006f;
  p.color.hue_velocity_drift = 0.0002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.08f;
  p.seed.hue = 0.62f;
  p.seed.hue_speed = 0.09f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.07f;
  p.seed.pulse_amp = 0.10f;
  p.speed = 1.0f;

  // Square is +1 on the first half cycle. A half-cycle phase gives -1 first.
  p.motion
      .add("velocity_reversal_beta",
           control::ModLane{.wave = control::Waveform::Square,
                            .frequency_hz = REVERSAL_HZ,
                            .amplitude = BETA,
                            .offset = 0.0f},
           control::ModTarget::Zn0Re)
      .add("velocity_reversal_pole",
           control::ModLane{.wave = control::Waveform::Square,
                            .frequency_hz = REVERSAL_HZ,
                            .amplitude = POLE_GAIN,
                            .offset = 0.0f,
                            .phase = 0.5f},
           control::ModTarget::Zn2Re)
      .add("velocity_reversal_shift",
           control::ModLane{.wave = control::Waveform::Square,
                            .frequency_hz = REVERSAL_HZ,
                            .amplitude = SHIFT,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("rapidity_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.031f,
                            .amplitude = 0.003f,
                            .offset = 0.006f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
