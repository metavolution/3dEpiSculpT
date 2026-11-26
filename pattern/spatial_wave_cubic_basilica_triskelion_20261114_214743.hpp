#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: the Cubic Basilica, w -> w^3 + i.
 *
 * Mathematics:
 * - For w^3 + c the critical point 0 has orbit 0 -> c -> c^3 + c. It returns
 *   to 0 after two steps when c^2 = -1. At c = i the map therefore has a
 *   superattracting cycle {0, i} of period 2: the cubic counterpart of the
 *   basilica (w^2 - 1).
 * - Since (omega w)^3 = w^3 for omega a cube root of unity, the Julia set is
 *   invariant under a third of a turn about 0. Every bulb of the basilica
 *   pattern appears three times round each pinch point.
 * - The lanes move c on a circle of radius 0.03 around i, inside the
 *   period-2 component: the cycle stays attracting and the bulbs breathe.
 *
 * Pipeline: Cube, zn0 = 1.45, zn2 = 1 / 1.45, zn3 = c / 1.45, so w = 1.45 z
 * and the Julia set (|w| < 1.33) fits the window; Clamp; Simplex border seed
 * for escape terraces.
 *
 * Mechanism assertion: escaped class outside, bounded class inside with an
 * attracting 2-cycle near {0, i / 1.45}; three-fold rotational symmetry.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_cubic_basilica_triskelion() {
  Pattern p;
  p.name = "spatial_wave_cubic_basilica_triskelion";
  p.inner = InnerFunction::Cube;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.45f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.6897f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.6897f};

  p.color.hue_rotation = 0.08f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.94f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.667f;
  p.color.spiral_radius_coeff = 0.9f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.06f;
  p.seed.hue = 0.42f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("c_circle_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.014f,
                            .amplitude = 0.0207f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Re)
      .add("c_circle_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.014f,
                            .amplitude = 0.0207f,
                            .offset = 0.6897f},
           control::ModTarget::Zn3Im)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.018f,
                            .amplitude = 0.025f,
                            .offset = 0.08f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.027f,
                            .amplitude = 0.04f,
                            .offset = 0.1f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
