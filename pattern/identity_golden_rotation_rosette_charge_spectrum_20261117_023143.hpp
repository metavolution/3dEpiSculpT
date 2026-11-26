#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Advanced 3.5D): a Golden Rotation Resolves Every Harmonic of a
 * Plate; the Hue Sweep Climbs a Staircase of Vortex Charges.
 *
 * Mathematics:
 * - T(z) = e^(2 pi i g) z, g = 0.38196601 (the golden angle), Clamp, Chladni
 *   seed: exact (the turn keeps r; the seed is zero for r >= 1).
 * - With the plate's angular harmonics P = sum_k P_k(r) e^(i k phi), k in
 *   4Z (quarter-turn symmetry), Opponent with step h gives
 *     C = sum_k P_k(r) e^(i k phi) / (1 - rho e^(2 pi i (h + k g))).
 *   A finite rotation of order q puts all harmonics on q lines
 *   (identity_angular_spectroscopy_selection_rules_of_a_plate: two lines).
 *   An irrational rotation gives each harmonic its own line, at
 *   h = -k g mod 1, and the lines never coincide.
 * - On the line of harmonic k the chroma is that harmonic, ~ P_k(r)
 *   e^(i k phi): its colour winds k times round the centre, a vortex of
 *   charge k. In [0.3, 0.7] the lines are the harmonics k = 4 mod 8:
 *   k = +28, +20, +12, +4 at h = 0.30495, 0.36068, 0.41641, 0.47214 and
 *   -4, -12, -20, -28 at 0.52786, 0.58359, 0.63932, 0.69505.
 *   (k = 8j sit near the ends, 0.0557, 0.1115, ...). Sweeping h from the
 *   middle outward climbs a staircase of charges 4, 12, 20, 28, mirrored
 *   with opposite sign: colour rosettes of 4, 12, 20, 28 petals,
 *   chirality flipping at h = 1/2, with darkness between the lines (line
 *   width (1 - rho)/2 pi = 0.0024).
 *
 * Pipeline: Identity with zn0 = e^(2 pi i g); Clamp; Chladni at alpha 0.12;
 * fade 0.985; Opponent with h on a Sine lane over [0.3, 0.7] (0.0005 Hz,
 * 2000 s; it starts dark at h = 1/2, between the k = +-4 lines).
 *
 * Fidelity:   identity for every lane value
 * Cell:       Identity / Clamp / ChladniCymatics / Opponent
 * Nearest:    identity_minimal_golden_angle_chladni_three_gap_stack (the
 *             same map in OKLab: no resonance); selection_rules (finite
 *             turn: two lines).
 * Height:     age = passes; generation j is the plate turned by j g.
 * Hue clock:  bounded; step swept over [0.3, 0.7].
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; line positions evaluated
 *             arithmetically. Which harmonics are strong depends on the
 *             plate's mode drift and is not computed; a weak harmonic gives
 *             a dim line. The winding of the chroma is k only where that
 *             harmonic dominates. Generation j's colour turns j times as fast
 *             as h (0.06 turn/s at j = 100). Not built, rendered or measured.
 * @assert p_N >= 0.75
 */
inline Pattern identity_golden_rotation_rosette_charge_spectrum() {
  Pattern p;
  p.name = "identity_golden_rotation_rosette_charge_spectrum";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {-0.73736888f, 0.67549029f}; // e^(2 pi i g)
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.5f; // dark: between the k = +-4 lines
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.985f;
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

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.35f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.12f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("charge_staircase",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0005f,
                            .amplitude = 0.2f,
                            .offset = 0.5f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.015f,
                            .offset = 0.03f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
