#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimal): the Wallpaper Group p4 from One Multiplication.
 *
 * Mathematics:
 * - On Grid every Gaussian-integer multiplier descends to the torus with any
 *   shift. The unit i is the smallest interesting case: z -> i z + c is a
 *   quarter turn about the point c / (1 - i), of order 4, and together with
 *   the lattice translations it generates the wallpaper group p4.
 * - The map is an isometry, so nothing is stretched. The steady image is the
 *   seed averaged over the four turns, with weights 1, g, g^2, g^3 repeated:
 *   as g approaches 1 it becomes exactly p4-symmetric, whatever the seed.
 *   Retention is high and seed alpha low for that reason.
 * - The hue step per generation is a quarter of the hue circle, so the four
 *   orientations carry four hues and the colouring has period 4 as well: a
 *   colour symmetry of the same group.
 * - Any shift is exact on Grid, so the four-fold centres can glide freely.
 *
 * Pipeline: Identity, zn0 = i exactly; Grid; OrganicNoise seed, which has no
 * symmetry of its own. Zn3 traces a slow Lissajous figure.
 *
 * Mechanism assertion: neutral orbit fraction 1, log-stretch exactly 0;
 * image invariant under a quarter turn about c / (1 - i) up to the weights.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_minimal_p4_wallpaper_symmetriser() {
  Pattern p;
  p.name = "spatial_wave_minimal_p4_wallpaper_symmetriser";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {0.0f, 1.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.25f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.985f;
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

  p.seed.type = SeedType::OrganicNoise;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.1f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 0.12f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("centre_glide_x",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.007f,
                            .amplitude = 0.6f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("centre_glide_y",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0045f,
                            .amplitude = 0.6f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Im)
      .add("saturation_breath",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.013f,
                            .amplitude = 0.08f,
                            .offset = 1.0f},
           control::ModTarget::SatScale);

  return p;
}

} // namespace epi::presets
