#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimal): Two Ring Sources from a Half Turn.
 *
 * Mathematics and physics:
 * - z -> -z + c is a half turn about c / 2, a map of order 2. The seed is a
 *   travelling ring wave centred at 0, so the steady image holds two ring
 *   systems, one centred at 0 and one at c, the second one step later in
 *   phase, with weights 1, g, g^2, ...
 * - The blend adds weighted, rectified waves; their fundamentals superpose
 *   linearly. Two circular waves of equal wavelength cross along the family
 *   of hyperbolae with foci at the two centres, the pattern of Young's two
 *   sources. Moving c changes the number of fringes between the foci.
 * - The map is an isometry of order 2, so the picture is a steady state at
 *   every moment: nothing alternates from field to field.
 *
 * Pipeline: Identity, zn0 = -1 exactly, zn3 = c; Clamp; WaveRing seed. The
 * lanes move c over 0.25 .. 0.75 on the real axis with a small vertical sway.
 *
 * Mechanism assertion: neutral orbit fraction 1, log-stretch exactly 0;
 * image symmetric under the half turn about c / 2 up to the weights.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_minimal_two_source_hyperbolae() {
  Pattern p;
  p.name = "spatial_wave_minimal_two_source_hyperbolae";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {-1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.5f, 0.0f};

  p.color.hue_rotation = 0.5f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.97f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 1.2f;
  p.color.chroma_breath_amp = 0.09f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.55f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.5f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("source_separation",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.013f,
                            .amplitude = 0.25f,
                            .offset = 0.5f},
           control::ModTarget::Zn3Re)
      .add("source_sway",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.008f,
                            .amplitude = 0.15f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.027f,
                            .amplitude = 0.03f,
                            .offset = 0.09f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
