#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: Hysteresis of a Mean-Field Magnet, m -> tanh(K m + h).
 *
 * Physics and mathematics:
 * - With an applied field h the mean-field equation is m = tanh(K m + h).
 *   For K > 1 and small |h| it has two stable solutions. At the coercive
 *   field one of them meets the unstable solution and both vanish (a
 *   saddle-node); the pair of such lines in the (K, h) plane is the cusp
 *   catastrophe. For K = 1.8 the condition K sech^2(u) = 1 gives
 *   tanh u = 0.667, m = 0.667, and h_c = +-0.395.
 * - Iterating the equation in the complex plane, the two states have two
 *   basins. Sweeping h past h_c removes one attractor: its basin is left
 *   with a bottleneck where the state used to be, and drains into the other.
 *   Sweeping back restores it at the opposite sign of h, which is the
 *   hysteresis loop.
 * - h is the pre-translation zn1, so one lane sweeps it exactly.
 *
 * Pipeline: Tanh, zn0 = K = 1.8, zn1 = h, zn2 = 1; Clamp; ChladniCymatics
 * seed. A Triangle lane takes h from -0.6 to +0.6 and back. The poles of
 * tanh at K y + Im h = +-pi/2 (y = +-0.87) sit near the top and bottom edges.
 *
 * Mechanism assertion: contracting class dominant; two attracting real
 * fixed points for |h| < 0.395, one beyond.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_ising_field_hysteresis_cusp() {
  Pattern p;
  p.name = "spatial_wave_ising_field_hysteresis_cusp";
  p.inner = InnerFunction::Tanh;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.8f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.05f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.8f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.64f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.75f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("applied_field_h",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.009f,
                            .amplitude = 0.6f,
                            .offset = 0.0f},
           control::ModTarget::Zn1Re)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.017f,
                            .amplitude = 0.02f,
                            .offset = 0.05f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.024f,
                            .amplitude = 0.04f,
                            .offset = 0.1f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
