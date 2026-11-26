#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimal): Weierstrass Sum of Chladni Figures, z -> 2 z on Grid.
 *
 * Mathematics:
 * - The Weierstrass function sum_n a^n cos(b^n x) is continuous and, for
 *   a b >= 1, nowhere differentiable. The feedback loop builds the same kind of
 *   sum by itself: with z -> 2 z the steady image is sum_n g^n S(2^n z), a
 *   lacunarity-2 octave stack of the seed S with amplitude ratio g. For a
 *   purely additive stack the surface would have Hurst exponent
 *   H = -log2 g and box dimension 3 - H for 1/2 < g < 1; the convex blend used
 *   here follows that up to the seed-alpha weighting.
 * - On Grid any Gaussian-integer multiplier descends to the torus with any
 *   shift, so the map is seamless and the translation can be animated freely.
 *
 * Pipeline: Identity, zn0 = 2 exactly; Grid; ChladniCymatics seed (its mode
 * numbers drift by themselves, so every octave carries the same slowly
 * changing nodal figure). Zn3 traces a slow Lissajous glide, which is exact on
 * Grid. The Fade lane sweeps the roughness.
 *
 * Mechanism assertion: expanding orbit fraction ~ 1; log-stretch spread ~ 0.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_minimal_weierstrass_chladni_octaves() {
  Pattern p;
  p.name = "spatial_wave_minimal_weierstrass_chladni_octaves";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {2.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.1f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.78f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 1.0f;
  p.color.chroma_breath_amp = 0.06f;
  p.color.chroma_breath_freq = 2.0f;
  p.color.chroma_breath_phase = 0.04f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.5f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.8f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("roughness_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.015f,
                            .amplitude = 0.06f,
                            .offset = 0.78f},
           control::ModTarget::Fade)
      .add("lattice_glide_x",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.009f,
                            .amplitude = 0.5f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("lattice_glide_y",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0056f,
                            .amplitude = 0.5f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Im)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.021f,
                            .amplitude = 0.03f,
                            .offset = 0.1f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
