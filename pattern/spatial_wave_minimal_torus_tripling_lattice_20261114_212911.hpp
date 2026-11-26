#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimal): Torus Tripling, z -> 3 z on the mirror fold.
 *
 * Mathematics:
 * - The Torus reduction is a triangle wave of period 4 with mirror lines at the
 *   odd integers. An affine map z -> n z + c passes through that fold without
 *   a crease exactly when n is an integer and c = 1 - n (mod 2) in each
 *   component: odd n with even shift, even n with odd shift. n = 3, c = 0 is
 *   the smallest odd case. In each coordinate the folded map is the degree-3
 *   Chebyshev map seen through x = sin(pi u / 2); it is uniformly expanding by
 *   3 and mixing.
 * - The steady image is sum_n g^n (seed o F^n). With uniform stretch ln 3 the
 *   ring-averaged spectrum slope has nothing to fit:
 *       alpha = 2 + 2 |ln g| / ln 3,   g = fade * (1 - mean seed alpha).
 *   The Fade lane therefore sweeps the slope directly.
 *
 * Pipeline: Identity, zn0 = 3 exactly and never modulated; Torus; Cross seed.
 * Generation n draws the axis lines at every triadic position k / 3^n, mirrored:
 * a ruled lattice at all scales, one hue step per generation. The sampler's
 * N/(N-1) dilation leaves seams of relative size 1/N per generation.
 *
 * Mechanism assertion: expanding orbit fraction ~ 1; log-stretch spread ~ 0.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_minimal_torus_tripling_lattice() {
  Pattern p;
  p.name = "spatial_wave_minimal_torus_tripling_lattice";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {3.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.11f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.66f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 1.0f;
  p.color.chroma_breath_amp = 0.06f;
  p.color.chroma_breath_freq = 3.0f;
  p.color.chroma_breath_phase = 0.04f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.56f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.9f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("cascade_slope_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.017f,
                            .amplitude = 0.08f,
                            .offset = 0.66f},
           control::ModTarget::Fade)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.023f,
                            .amplitude = 0.035f,
                            .offset = 0.11f},
           control::ModTarget::HueRot)
      .add("wave_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.031f,
                            .amplitude = 0.03f,
                            .offset = 0.06f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
