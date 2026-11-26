#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimal): the Two-Dimensional Tent Map, z -> 2 z + (1 + i).
 *
 * Mathematics:
 * - Even multiplier with odd shift is the other crease-free case on the Torus
 *   mirror fold. In each coordinate x -> fold(2x + 1) = 1 - 2|x|: the tent map,
 *   the textbook exactly mixing interval map (conjugate to the logistic map at
 *   r = 4 and to the Bernoulli shift). Here it acts on both axes at once.
 * - Uniform stretch ln 2, so alpha = 2 + 2 |ln g| / ln 2 with
 *   g = fade * (1 - mean seed alpha).
 *
 * Pipeline: Identity, zn0 = 2 and zn3 = (1, 1), both exact and never
 * modulated; Torus; WaveRing seed. Generation n shows 4^n mirrored copies of
 * the travelling ring system at scale 2^-n, so every scale pulses with the
 * same wave, each generation one step later in time and one step along in hue.
 *
 * Mechanism assertion: expanding orbit fraction ~ 1; log-stretch spread ~ 0.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_minimal_tent_map_ring_bloom() {
  Pattern p;
  p.name = "spatial_wave_minimal_tent_map_ring_bloom";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {2.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {1.0f, 1.0f};

  p.color.hue_rotation = 0.09f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.8f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 2.0f;
  p.color.chroma_breath_amp = 0.07f;
  p.color.chroma_breath_freq = 1.0f;
  p.color.chroma_breath_phase = 0.04f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.08f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.84f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("cascade_slope_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.019f,
                            .amplitude = 0.05f,
                            .offset = 0.8f},
           control::ModTarget::Fade)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.013f,
                            .amplitude = 0.03f,
                            .offset = 0.09f},
           control::ModTarget::HueRot)
      .add("wave_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.029f,
                            .amplitude = 0.03f,
                            .offset = 0.07f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
