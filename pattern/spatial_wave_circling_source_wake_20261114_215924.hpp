#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: Wake of a Moving Source on the Torus.
 *
 * Mathematics and physics:
 * - z -> z + v is a translation. On Grid every translation is exact, so there
 *   are no seams however v moves. Generation n is the ring-wave seed centred
 *   n v behind the present one and emitted n steps earlier: the steady image
 *   is a line of sources with a linear phase delay, which is how a moving
 *   emitter is built up (and how a phased line array steers a beam).
 * - When the source moves faster than its rings spread, the delayed rings
 *   reinforce along two lines trailing behind it: the wake of a supersonic
 *   body, with half-angle sin(phi) = (ring speed) / (source speed).
 * - Two lanes in quadrature turn v slowly, so the source runs across the
 *   torus on a gently curving course and the wake bends with it.
 *
 * Pipeline: Identity, zn0 = 1; Grid; WaveRing seed with low alpha, so that
 * about thirty generations stay visible and form the trailing array.
 * |v| = 0.012 per step; the direction completes a turn in 50 s.
 *
 * Mechanism assertion: neutral orbit fraction 1, log-stretch exactly 0; the
 * image at any moment is a translate-and-delay sum of the seed.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_circling_source_wake() {
  Pattern p;
  p.name = "spatial_wave_circling_source_wake";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.012f, 0.0f};

  p.color.hue_rotation = 0.01f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.995f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 1.0f;
  p.color.chroma_breath_amp = 0.07f;
  p.color.chroma_breath_freq = 2.0f;
  p.color.chroma_breath_phase = 0.04f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.57f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.06f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("velocity_cos",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.012f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Re)
      .add("velocity_sin",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.012f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Im)
      .add("saturation_breath",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.012f,
                            .amplitude = 0.07f,
                            .offset = 1.0f},
           control::ModTarget::SatScale);

  return p;
}

} // namespace epi::presets
