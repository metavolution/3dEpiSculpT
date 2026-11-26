#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Recursive Memory: Wagon-Wheel Effect, Temporal Aliasing in the Loop.
 *
 * Scientific foundation:
 * - Sampling theorem: a rotation sampled at a fixed frame rate is known only
 *   modulo the symmetry of the wheel. A wheel with N spokes turning
 *   2 pi / N + eps per frame is indistinguishable from one turning eps per
 *   frame, and as eps passes through zero the apparent rotation stops and
 *   reverses although the true speed barely changes. This is the wagon-wheel
 *   effect of film and of stroboscopic tachometry.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Identity with |zn0| = 1: one pass is a rigid rotation by
 *   alpha = 2 pi / 7 + eps, about 3090 degrees per second at 60 frames per
 *   second. The feedback buffer is the persistence of vision: each frame
 *   keeps the previous one, turned once more and dimmed by Fade.
 * - Seed = Cross: four arms. Seven turned copies give 28 spokes.
 * - Color = Classic with hue step 1/7: after seven passes a spoke is back
 *   where it started with its original hue, so on resonance the copies stack
 *   exactly and the wheel is a still, seven-coloured star.
 * - Off resonance each stack is displaced by 7 eps per revolution and
 *   leaves a fading trail on one side. The trail shows the apparent motion,
 *   which is slow and can run either way, not the true one.
 * - Motion: eps = 0.02 sin(2 pi * 0.02 t) radians per frame. Twice per
 *   cycle it passes through zero: the trails shorten, vanish and reappear on
 *   the other side. (First-order lanes; |zn0| stays within 0.02 percent of
 *   1, so the wheel neither grows nor shrinks visibly.)
 * - The corners of the frame lie outside the wheel and are folded by Torus.
 */
inline Pattern wagon_wheel_stroboscopic_alias() {
  constexpr float COS_A = 0.62348980f; // cos(2 pi / 7)
  constexpr float SIN_A = 0.78183148f; // sin(2 pi / 7)
  constexpr float DETUNE = 0.02f;      // radians per frame, peak
  constexpr float DETUNE_HZ = 0.02f;

  Pattern p;
  p.name = "wagon_wheel_stroboscopic_alias";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {COS_A, SIN_A};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.14285714f; // 1/7
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.985f;
  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f; // bounded hue clock: step stays near 1/7
  p.color.hue_velocity_breath = 0.0006f;
  p.color.hue_velocity_drift = 0.0002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.08f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.90f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 1.0f;

  p.motion
      .add("strobe_detune_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = DETUNE_HZ,
                            .amplitude = -SIN_A * DETUNE,
                            .offset = COS_A},
           control::ModTarget::Zn0Re)
      .add("strobe_detune_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = DETUNE_HZ,
                            .amplitude = COS_A * DETUNE,
                            .offset = SIN_A},
           control::ModTarget::Zn0Im)
      .add("spoke_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.009f,
                            .amplitude = 0.2f,
                            .offset = 0.0f},
           control::ModTarget::SeedHue);

  return p;
}

} // namespace epi::presets
