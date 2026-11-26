#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Advanced 3.5D): Young's Two-Source Interferometer with the Hue
 * Step as Phase Shifter.
 *
 * Mathematics:
 * - T(z) = -z + 2c on the Grid torus (exact; T^2 = id), 2c = 0.9: ring
 *   sources at 0 and 0.9, generation n from source n mod 2, emitted n
 *   frames ago (the delay line).
 * - Opponent, linear. For the temporal harmonic k of the ring wave,
 *     C = (A_0 + e^(i phi) A_1) / (1 - rho^2 e^(2 i phi)),
 *     phi = 2 pi h + 5 k Delta,  A_j = a_k e^(i k 20 r_j):
 *   the coherent sum of two sources with relative phase phi. Unlike the
 *   moire of identity_minimal_two_source_moire_hyperbolas, this is true
 *   amplitude interference: bright where 20 (r_0 - r_1) + phi = 0 mod 2 pi,
 *   dark nulls half a fringe away, on confocal hyperbolas (about 5.7
 *   fringes between the sources).
 * - The lane sweeps h from h_1 = 5 Delta / 2 pi = 0.0106 (phi = 0: in phase)
 *   to h_1 + 1/2 (phi = pi: anti-phase) and back. The fringes slide
 *   continuously by half a fringe: bright and dark hyperbolas swap. The
 *   gain 1/|1 - rho^2 e^(2 i phi)| is 33.6 at both ends and 0.51 in the
 *   middle, so the pattern glows at the two resonances and dims between.
 *   On the perpendicular bisector (r_0 = r_1) it is bright in phase and
 *   null in anti-phase: the two-element array's broadside lobe swings
 *   into a null.
 *
 * Pipeline: Identity, zn0 = -1, zn2 = 1, zn3 = 0.9; Grid; WaveRing at alpha
 * 0.08; fade 0.985; Opponent with the sweep lane (offset 0.2606, amplitude
 * 0.25, 0.002 Hz, starting in phase).
 *
 * Fidelity:   identity for every lane value
 * Cell:       Identity / Grid / WaveRing / Opponent
 * Nearest:    identity_minimal_two_source_moire_hyperbolas (same sources,
 *             intensity moire); identity_lock_in_amplifier_tuned_by_the_hue_
 *             step (one source).
 * Height:     age = passes = delay; n mod 2 = source.
 * Hue clock:  bounded; step swept over [0.0106, 0.5106].
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; lines and gains
 *             evaluated arithmetically. Frame-rate dependent
 *             (Delta = speed / fps); the minimum-image wrap leaves faint seams
 *             (ring intensity < e^-2) at distance 1 from the second source.
 *             Not built, rendered or measured.
 * @assert p_N >= 0.95
 */
inline Pattern identity_young_interferometer_phase_shifter() {
  Pattern p;
  p.name = "identity_young_interferometer_phase_shifter";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {-1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.9f, 0.0f}; // second source at 2c = 0.9

  p.color.hue_rotation = 0.01061033f; // in phase, 60 fps
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

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.08f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("phase_shifter",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.002f,
                            .amplitude = 0.25f,
                            .offset = 0.26061033f,
                            .phase = 0.75f}, // starts in phase
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
