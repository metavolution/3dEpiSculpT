#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Minimal 3.5D): Two Ring Sources, Stationary Hyperbolas and
 * Outflowing Ellipses.
 *
 * Mathematics:
 * - T(z) = -z + 2c on the Grid torus: a half turn about c, exact because
 *   -1 is a Gaussian unit; T^2 = id. Generation 1 is the ring wave centred
 *   at 2c, generation 0 the one at 0: two coherent sources, distance
 *   d = 2|c| = 0.9 apart.
 * - Light (unrotated sum) adds the two ring families. Where crests meet,
 *   r1 - r2 or r1 + r2 is a multiple of the crest spacing 2 pi/20: the moire
 *   draws the confocal hyperbolas r1 - r2 = const (stationary, since both
 *   rings expand at the same speed: the interference fringes of two
 *   coherent sources, about 2d x 20/2 pi = 5.7 of them between the
 *   sources) and the confocal ellipses r1 + r2 = const, which flow outward.
 *   Foci at the two sources.
 * - Opponent with step 1/2 on the 2-cycle: chroma is the difference of the
 *   two families, so the colour says which source is nearer in phase;
 *   it changes sign across the perpendicular bisector.
 * - Two lanes in quadrature move 2c round a circle of radius 0.9: the
 *   second source orbits the first and the hyperbolas turn with it.
 *
 * Pipeline: Identity, zn0 = -1, zn2 = 1, zn3 = 2c on the lanes (one orbit in
 * 400 s); Grid; WaveRing at alpha 0.3; fade 0.9; Opponent step 1/2.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Identity / Grid / WaveRing / Opponent
 * Nearest:    invert_modular_wave_meets_its_mirror_image (a source and its
 *             image in an inversion); here two point sources in the plane.
 * Height:     age = passes; the two generations are the two sources.
 * Hue clock:  bounded; step 1/2.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. WaveRing intensities
 *             are half-wave rectified, so the fringes are a moire of crests,
 *             not amplitude interference. The minimum-image wrap puts faint
 *             seams (ring intensity < e^-2) at distance 1 from the moving
 *             source. Not built, rendered or measured.
 * @assert p_N >= 0.95
 */
inline Pattern identity_minimal_two_source_moire_hyperbolas() {
  constexpr float SEP = 0.9f; // |2c|
  constexpr float ORBIT_HZ = 0.0025f;
  Pattern p;
  p.name = "identity_minimal_two_source_moire_hyperbolas";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {-1.0f, 0.0f}; // half turn
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {SEP, 0.0f};   // 2c: the second source

  p.color.hue_rotation = 0.5f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.9f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.03f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.1f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.3f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("second_source_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = SEP,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Re)
      .add("second_source_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = SEP,
                            .offset = 0.0f,
                            .phase = 0.0f},
           control::ModTarget::Zn3Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.015f,
                            .offset = 0.03f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
