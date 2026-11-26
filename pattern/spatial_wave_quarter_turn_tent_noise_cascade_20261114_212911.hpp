#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: Quarter-Turn Tent Cascade, z -> 2i z + (1 + i).
 *
 * Mathematics:
 * - The crease-free rule on the Torus mirror fold also admits purely imaginary
 *   integers: 2i is even, so it takes the odd shift 1 + i. The map is the
 *   two-dimensional tent map composed with a quarter turn,
 *   (x, y) -> (1 - 2|y|, 1 - 2|x|): each generation halves the scale and swaps
 *   the axes. Uniform stretch ln 2, mixing, no seams.
 * - Fed with a noise seed, the loop is an exact octave synthesiser: the steady
 *   image is sum_n g^n noise(F^n z) with spectrum slope
 *   alpha = 2 + 2 |ln g| / ln 2, set by retention alone. The Fade lane moves
 *   it between a shallow, grainy cascade and a steep, cloudy one while the map
 *   stays fixed.
 *
 * Pipeline: Identity, zn0 = (0, 2), zn3 = (1, 1), exact and never modulated;
 * Torus; OrganicNoise seed with low alpha so that many generations stay
 * visible; SpatialWave relief with an angular term so the axis swap shows as
 * alternating hue sectors.
 *
 * Mechanism assertion: expanding orbit fraction ~ 1; log-stretch spread ~ 0.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_quarter_turn_tent_noise_cascade() {
  Pattern p;
  p.name = "spatial_wave_quarter_turn_tent_noise_cascade";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {0.0f, 2.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {1.0f, 1.0f};

  p.color.hue_rotation = 0.07f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.86f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.25f;
  p.color.spiral_radius_coeff = 1.0f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 2.0f;
  p.color.chroma_breath_phase = 0.04f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.06f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::OrganicNoise;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.72f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 0.35f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("cascade_slope_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.014f,
                            .amplitude = 0.05f,
                            .offset = 0.86f},
           control::ModTarget::Fade)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.018f,
                            .amplitude = 0.03f,
                            .offset = 0.07f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.026f,
                            .amplitude = 0.04f,
                            .offset = 0.1f},
           control::ModTarget::ChromaBreathAmp)
      .add("saturation_breath",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.011f,
                            .amplitude = 0.08f,
                            .offset = 1.0f},
           control::ModTarget::SatScale);

  return p;
}

} // namespace epi::presets
