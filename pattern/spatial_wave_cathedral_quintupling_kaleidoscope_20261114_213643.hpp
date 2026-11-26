#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: Exact Kaleidoscope Cascade, z -> 5 z on the Cathedral fold.
 *
 * Mathematics:
 * - PareidoliaCathedral is the Torus mirror fold followed by a true mirror in
 *   the anti-diagonal: together a reflection group whose chamber is a
 *   triangle. z -> n z with n an odd integer passes through the Torus fold
 *   without a crease, and a real scaling commutes with every linear mirror,
 *   so 5z descends to the whole group. The feedback image is therefore
 *   mirror-symmetric by construction and has no seams, whatever the seed.
 * - Uniform stretch ln 5: alpha = 2 + 2 |ln g| / ln 5. Five-fold refinement
 *   per generation is fast, so retention is set low and each generation is
 *   25 times more numerous and correspondingly fainter.
 *
 * Pipeline: Identity, zn0 = 5 exactly and never modulated; PareidoliaCathedral;
 * OrganicNoise seed. A seed with no symmetry of its own makes the point: all
 * the order on screen comes from the group, at every scale at once.
 *
 * Mechanism assertion: expanding orbit fraction ~ 1; log-stretch spread ~ 0;
 * image invariant under the anti-diagonal mirror.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_cathedral_quintupling_kaleidoscope() {
  Pattern p;
  p.name = "spatial_wave_cathedral_quintupling_kaleidoscope";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::PareidoliaCathedral;
  p.zn.zn[0] = {5.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.13f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.6f;
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

  p.seed.type = SeedType::OrganicNoise;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.78f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 0.35f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("cascade_slope_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.016f,
                            .amplitude = 0.07f,
                            .offset = 0.6f},
           control::ModTarget::Fade)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.022f,
                            .amplitude = 0.04f,
                            .offset = 0.13f},
           control::ModTarget::HueRot)
      .add("saturation_breath",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.012f,
                            .amplitude = 0.08f,
                            .offset = 1.0f},
           control::ModTarget::SatScale);

  return p;
}

} // namespace epi::presets
