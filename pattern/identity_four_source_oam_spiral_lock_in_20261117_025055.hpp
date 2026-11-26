#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Advanced 3.5D): a Four-Element Phased Array on a Torus; the Hue
 * Step Dials the Orbital Angular Momentum of a Turning Spiral Wave.
 *
 * Mathematics:
 * - T(z) = i z + c on the Grid torus, c = 0.6: a quarter turn about
 *   z* = 0.3 + 0.3i, exact (Gaussian unit). The ring source at 0 has the
 *   orbit 0, 0.6, 0.6 + 0.6i, 0.6i: generation n is the ring wave of n
 *   frames ago (the delay line) from source j = n mod 4.
 * - Opponent, linear: C = sum_n rho^n e^(2 pi i n h) I_(n mod 4)(z, t - n
 *   Delta). For the temporal harmonic k of the ring wave, n = j + 4m gives
 *     sum_j e^(i j (2 pi h + 5 k Delta)) A_j sum_m (rho^4 e^(4 i (2 pi h +
 *     5 k Delta)))^m,
 *   resonant when 4 (2 pi h + 5 k Delta) = 0 mod 2 pi. For the fundamental
 *   the lines are h_m = 5 Delta / 2 pi + m/4: 0.0106, 0.2606, 0.5106,
 *   0.7606 at 60 fps, speed 0.8. On line m the four sources radiate with
 *   relative phases j m / 4: the array of identity_hexagonal_phased_array_
 *   colour_vortex, now locked to the travelling wave in time.
 * - The field then satisfies C(T z) = e^(-2 pi i m/4) C(z): a phase
 *   singularity at z* of charge -m mod 4, and since the temporal phase
 *   advances with t, the constant-colour curves are spiral arms that turn:
 *   a beam with orbital angular momentum. m = 0: plain rings (in-phase
 *   array, no vortex); m = 1 and 3: one-armed spirals of opposite sense;
 *   m = 2: two arms.
 * - The lane sweeps h over [0, 0.8] (Sine, 0.0005 Hz); it crosses the four
 *   lines in turn. Lines of the second harmonic (h = 0.0212 + m/4) give
 *   fainter copies.
 *
 * Pipeline: Identity, zn0 = i, zn2 = 1, zn3 = 0.6; Grid; WaveRing at alpha
 * 0.08; fade 0.985; Opponent with the sweep lane, rest on the m = 1 line.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Identity / Grid / WaveRing / Opponent
 * Nearest:    identity_lock_in_amplifier_tuned_by_the_hue_step (one source,
 *             no array); identity_p4_orbifold_vortex_antivortex_crystal (the
 *             same group, static plates, no lock-in).
 * Height:     age = passes = delay; n mod 4 = source.
 * Hue clock:  bounded; step swept over [0, 0.8].
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; line positions evaluated
 *             arithmetically. Frame-rate dependent (Delta = speed / fps);
 *             the minimum-image wrap leaves faint seams (ring intensity
 *             < e^-2); the charge sign follows the Opponent convention of
 *             E7h. Generation n's colour turns n times as fast as h (0.13
 *             turn/s at n = 100 at the sweep's fastest). Not built, rendered
 *             or measured.
 * @assert p_N >= 0.95
 */
inline Pattern identity_four_source_oam_spiral_lock_in() {
  Pattern p;
  p.name = "identity_four_source_oam_spiral_lock_in";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {0.0f, 1.0f}; // i
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.6f, 0.0f}; // c; centre z* = 0.3 + 0.3i

  p.color.hue_rotation = 0.26061033f; // the m = 1 line at 60 fps
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
      .add("oam_dial",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0005f,
                            .amplitude = 0.4f,
                            .offset = 0.4f,
                            .phase = 0.94334995f}, // starts on m = 1
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
