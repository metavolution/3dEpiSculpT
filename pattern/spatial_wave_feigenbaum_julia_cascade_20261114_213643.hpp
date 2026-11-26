#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: Julia Sets along the Period-Doubling Cascade.
 *
 * Mathematics:
 * - w -> w^2 + c for real c from -0.75 down to -1.40. At c = -3/4 the
 *   attracting fixed point doubles (the basilica); at -5/4 the two-cycle
 *   doubles; the doublings accumulate at the Feigenbaum point
 *   c = -1.401155 at the ratio 4.669. All Julia sets on the way are connected
 *   and symmetric in both axes, and each doubling pinches every bulb of the
 *   previous set in two.
 *
 * Pipeline: Square, zn0 = 1.85, zn2 = 1/1.85, zn3 = c / 1.85, so w = 1.85 z
 * and the whole Julia set (|w| < 1.8) is in the window; Clamp; Simplex border
 * seed. Escaping orbits end on the border and are coloured by generation,
 * which draws the set from outside as escape terraces. A Triangle lane takes
 * c through the cascade and back, stopping short of the Feigenbaum point.
 *
 * Mechanism assertion: escaped class outside the set, bounded class inside;
 * mirror symmetry in both axes at every c.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_feigenbaum_julia_cascade() {
  Pattern p;
  p.name = "spatial_wave_feigenbaum_julia_cascade";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.85f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.5405f, 0.0f};
  p.zn.zn[3] = {-0.581f, 0.0f};

  p.color.hue_rotation = 0.07f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.94f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.5f;
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
  p.seed.hue = 0.88f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("c_through_the_cascade",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.008f,
                            .amplitude = 0.176f,
                            .offset = -0.581f},
           control::ModTarget::Zn3Re)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.015f,
                            .amplitude = 0.025f,
                            .offset = 0.07f},
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
