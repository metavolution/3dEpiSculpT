#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimal): Pursuit Whirl, the Four-Bug Problem as an Exit-Time
 * Map.
 *
 * Scientific foundation:
 * - Four bugs at the corners of a square, each chasing its neighbour at equal
 *   speed, stay on the corners of a shrinking, turning square and trace four
 *   logarithmic spirals that meet at the centre (Lucas 1877, Brocard). Drawing
 *   the square at equal time steps gives the "whirl" of nested squares, each
 *   inscribed in the last with its corners on the previous sides.
 * - A square turned by theta and inscribed in the unit square has scale
 *   1 / (cos theta + sin theta). So the similarity z -> a z with
 *   |a| = cos theta + sin theta and arg a = theta maps each square of the
 *   whirl onto the next larger one.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Identity, zn0 = (cos t + sin t) * e^{i t} with t = 10 degrees.
 * - Reduce = Clamp. No preset in the catalogue used Clamp before. A point
 *   pushed outside the square is pinned to the border, where the Simplex seed
 *   has full alpha, so the pixel takes the seed colour at once. The picture is
 *   therefore the exact first-exit time of z -> a z: generation n is the ring
 *   between the squares a^-n S and a^-(n+1) S, flat-coloured and hard-edged.
 *   With Torus the escaped points would fold back and blur this into a spiral
 *   haze (compare spatial_wave_minimal_golden_whirl).
 * - Color = SpatialWave: one hue step per generation plus a shallow horizontal
 *   wave that is summed along each spiral orbit.
 * - Motion: Zn0Im swings the turn angle between about 7 and 13 degrees, so the
 *   whirl tightens and relaxes around the exactly inscribed case.
 */
inline Pattern spatial_wave_minimal_pursuit_whirl() {
  // (cos 10 + sin 10) * (cos 10, sin 10): the inscribed-square similarity.
  constexpr float WHIRL_RE = 1.14085620f;
  constexpr float WHIRL_IM = 0.20116265f;

  Pattern p;
  p.name = "spatial_wave_minimal_pursuit_whirl";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {WHIRL_RE, WHIRL_IM};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.045f;
  p.color.sat_scale = 1.02f;
  p.color.fade = 0.975f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 1.0f;
  p.color.chroma_breath_amp = 0.03f;
  p.color.chroma_breath_freq = 2.0f;
  p.color.chroma_breath_phase = 0.02f;
  p.color.hue_velocity_base = 0.008f;
  p.color.hue_velocity_breath = -0.005f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.02f;
  p.seed.hue_speed = 0.035f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f; // exit colour is exact, not a pulse average
  p.speed = 0.7f;

  p.motion
      .add("pursuit_turn_angle",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.03f,
                            .amplitude = 0.06f,
                            .offset = WHIRL_IM},
           control::ModTarget::Zn0Im)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.021f,
                            .amplitude = 0.02f,
                            .offset = 0.045f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
