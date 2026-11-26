#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Advanced 3.5D): Angular Spectroscopy of a Chladni Plate; the
 * Hue Step Reads Out Selection Rules.
 *
 * Mathematics:
 * - T(z) = e^(i pi/4) z: an eighth turn about the plate's centre, Clamp.
 *   Exact: the turn keeps r, the Chladni seed is zero for r >= 1, and
 *   clamped points stay at r >= 1.
 * - Expand the plate in angular harmonics, P = sum_k P_k(r) e^(i k phi). Its
 *   modulus is invariant under the quarter turn (x, y) -> (-y, x), so only
 *   k in 4Z occur. Opponent colour with step h gives
 *     C = sum_k P_k(r) e^(i k phi) / (1 - rho e^(2 pi i (h + k/8))):
 *   the loop is a spectrometer in angle, resonant at h = -k/8 mod 1.
 * - With k in 4Z the only lines are h = 0 (k = 0 mod 8, the eightfold part
 *   of the plate) and h = 1/2 (k = 4 mod 8, the part that changes sign under
 *   the eighth turn). Every other h is forbidden by symmetry: at h = 1/4
 *   the gain is |1 - i rho|^-1 = 0.71 against 1/(1 - rho) = 67 on a line.
 *   Exactly as selection rules forbid transitions in a symmetric molecule.
 * - The lane sweeps h from 1/2 to 0 and back. A Sine lane lingers at its
 *   extremes, so the picture rests on the two allowed lines and passes
 *   quickly through the dark forbidden band between them. The light
 *   channel, unrotated, is always the eightfold average.
 *
 * Pipeline: Identity, zn0 = e^(i pi/4); Clamp; Chladni at alpha 0.12;
 * fade 0.985; Opponent with h on the sweep lane (0.002 Hz, rest at 1/2).
 *
 * Fidelity:   identity for every lane value
 * Cell:       Identity / Clamp / ChladniCymatics / Opponent
 * Nearest:    identity_minimal_octagonal_chladni_opponent_moire (the same
 *             map, held on the h = 1/2 line); this row sweeps the whole
 *             spectrum. identity_lock_in_amplifier_tuned_by_the_hue_step is
 *             its counterpart in time.
 * Height:     age = passes; generation j is the plate turned by j/8.
 * Hue clock:  bounded; step swept over [0, 1/2].
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. As h moves, the colour
 *             of generation j turns j times as fast as h; at fade 0.985 the
 *             weight past j = 150 is under 0.1. The seed blend (1 - alpha I)
 *             makes the filter slightly nonlinear. Not built, rendered or
 *             measured.
 * @assert p_N >= 0.75
 */
inline Pattern identity_angular_spectroscopy_selection_rules_of_a_plate() {
  Pattern p;
  p.name = "identity_angular_spectroscopy_selection_rules_of_a_plate";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.70710678f, 0.70710678f}; // e^(i pi/4)
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.5f; // the k = 4 mod 8 line
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
  p.seed.hue = 0.12f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.12f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("spectral_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.002f,
                            .amplitude = 0.25f,
                            .offset = 0.25f,
                            .phase = 0.25f}, // starts on h = 1/2
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
