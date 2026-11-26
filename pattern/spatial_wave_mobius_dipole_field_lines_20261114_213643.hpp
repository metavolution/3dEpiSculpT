#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: Hyperbolic Mobius Map, Field Lines of a Dipole.
 *
 * Mathematics:
 * A Mobius map is written here as z -> zn2 / (zn0 z + zn1) + zn3 with the
 * Invert inner; any values of the four numbers give a Mobius map.
 * - (z' - a)/(z' + a) = k (z - a)/(z + a) with real 0 < k < 1 is the
 *   hyperbolic class: a source at -a, a sink at +a, and every orbit moving
 *   along the circle through its starting point and both fixed points. Those
 *   circles are the field lines of a two-dimensional dipole; the orthogonal
 *   circles of Apollonius are its equipotentials.
 * - a = 0.5, k = 0.9: M(z) = 9.5 - 9 / (0.1 z + 0.95). The pole is at -9.5.
 * - The feedback reads history at M(z), so picture content streams the other
 *   way, from the sink to the source, and piles up in ever finer layers at
 *   -a: equal steps of generation are equal steps of potential.
 * - A small imaginary part on zn1 makes k complex: the field lines wind into
 *   a double spiral and back.
 *
 * Pipeline: Invert; Clamp; OrganicNoise seed at low alpha, which the flow
 * combs into streaks along the field lines.
 *
 * Mechanism assertion: contracting class dominant; all orbits end at +a.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_mobius_dipole_field_lines() {
  Pattern p;
  p.name = "spatial_wave_mobius_dipole_field_lines";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.1f, 0.0f};
  p.zn.zn[1] = {0.95f, 0.0f};
  p.zn.zn[2] = {-9.0f, 0.0f};
  p.zn.zn[3] = {9.5f, 0.0f};

  p.color.hue_rotation = 0.04f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.96f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.2f;
  p.color.spiral_radius_coeff = 0.8f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::OrganicNoise;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.6f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 0.3f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("spiral_twist",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.013f,
                            .amplitude = 0.03f,
                            .offset = 0.0f},
           control::ModTarget::Zn1Im)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.018f,
                            .amplitude = 0.015f,
                            .offset = 0.04f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.025f,
                            .amplitude = 0.04f,
                            .offset = 0.1f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
