#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: Seven Ring Sources on a Heptagon.
 *
 * Mathematics and physics:
 * - z -> e^(i theta) z + c (1 - e^(i theta)) is a rotation by theta about the
 *   point c. With theta = 2 pi / 7 it has order 7. The seed is a travelling
 *   ring wave centred at 0; generation n is that wave centred at the n-th
 *   rotated image of 0, emitted n steps earlier. The steady image is
 *   therefore seven ring sources on a regular heptagon of radius |c| around
 *   c, each one step later in phase than its neighbour: a circular phased
 *   array.
 * - The blend adds weighted, rectified waves, so their fundamentals
 *   superpose linearly: the pattern near the centre has the seven-fold
 *   quasi-periodic order of seven plane waves, and further out the rings
 *   cross in moire rosettes. The phase step per source makes the whole
 *   figure turn.
 * - c enters zn3 linearly, so two lanes with one sinusoid in fixed ratio
 *   breathe the array radius exactly while the angle stays locked.
 *
 * Pipeline: Identity, zn0 = e^(2 pi i / 7) exactly; Clamp; WaveRing seed.
 * c = 0.38 +- 0.13 on the real axis.
 *
 * Mechanism assertion: neutral orbit fraction ~ 1; azimuthal harmonic 7
 * about the point c dominant.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_heptagonal_ring_array_moire() {
  Pattern p;
  p.name = "spatial_wave_heptagonal_ring_array_moire";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.62349f, 0.78183f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.14307f, -0.2971f};

  p.color.hue_rotation = 0.05f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.93f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 1.4f;
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
  p.seed.hue = 0.6f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.84f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("array_radius_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.015f,
                            .amplitude = 0.05f,
                            .offset = 0.14307f},
           control::ModTarget::Zn3Re)
      .add("array_radius_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.015f,
                            .amplitude = 0.1038f,
                            .offset = -0.2971f,
                            .phase = 0.5f},
           control::ModTarget::Zn3Im)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.021f,
                            .amplitude = 0.02f,
                            .offset = 0.05f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
