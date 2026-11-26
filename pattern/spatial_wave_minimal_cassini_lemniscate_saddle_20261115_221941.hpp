#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimal 3.5D): Cassini Ovals and the Lemniscate Saddle.
 *
 * Mathematics:
 * - The seed is a ring wave in |w|. Pulled back once through
 *   T(z) = z^2 - c it is a ring wave in |z^2 - c| = |z - f| |z + f| with
 *   f = sqrt(c): the level curves are Cassini ovals with foci +-f. For a
 *   level below c they are two ovals, one round each focus; at the level c
 *   they are the lemniscate of Bernoulli, with its node at z = 0; above c
 *   they are one curve.
 * - z = 0 is the critical point of T, a saddle of |T|. A ring that travels
 *   outward in |w| therefore leaves each focus as a separate loop and the
 *   two loops join at the origin when the ring passes |w| = c. This is the
 *   one change of topology a Morse function with a single saddle allows,
 *   and it repeats once per wave period.
 * - Generation n is a ring wave in |T^n(z)|, a polynomial lemniscate with
 *   2^n foci, the n-th pre-images of 0, and saddles at the pre-images of
 *   the critical point. The foci gather on the Julia set.
 * - For c = 1/4 the attracting fixed point is (1 - sqrt 2) / 2 = -0.2071
 *   with multiplier 1 - sqrt 2 = -0.4142, so deep generations settle to one
 *   phase and the first three carry the picture.
 *
 * Pipeline: Square, zn3 = -c; Clamp; WaveRing. The lanes move c over
 * 0.16 .. 0.34 (foci at +-0.40 .. +-0.58) with a small imaginary part that
 * turns the focal axis. Every such c lies in the main cardioid. The corners
 * of the frame lie outside the filled Julia set and are pinned by Clamp.
 *
 * Fidelity:   identity
 * Cell:       Square / Clamp / WaveRing / SpatialWave
 * Nearest:    spatial_wave_minimal_two_source_hyperbolae has two sources
 *             from an isometry and no saddle; here the two foci come from
 *             the branching of z^2 and the subject is the merging at the
 *             critical point.
 * Height:     age = generation n = log2 of the number of foci.
 * Hue clock:  bounded (hue_velocity_base = 0).
 * Fastest lane: 0.027 Hz
 * Status:     designed from source on 2026-11-15; not built, rendered or
 *             measured.
 * @assert sing[0].period == 1 && sing[0].rho_abs within 1 % of 0.4142 &&
 *         p_C >= 0.8
 */
inline Pattern spatial_wave_minimal_cassini_lemniscate_saddle() {
  Pattern p;
  p.name = "spatial_wave_minimal_cassini_lemniscate_saddle";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {-0.25f, 0.0f};

  p.color.hue_rotation = 0.07f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 1.2f;
  p.color.chroma_breath_amp = 0.08f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.06f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.58f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.5f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("focal_distance_c",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.012f,
                            .amplitude = 0.09f,
                            .offset = -0.25f},
           control::ModTarget::Zn3Re)
      .add("focal_axis_turn",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.008f,
                            .amplitude = 0.07f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.027f,
                            .amplitude = 0.03f,
                            .offset = 0.08f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
