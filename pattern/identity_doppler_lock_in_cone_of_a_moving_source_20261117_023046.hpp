#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Advanced 3.5D): a Doppler Lock-in; the Tuned Colour Picks Out a
 * Cone of Directions round a Moving Source.
 *
 * Mathematics:
 * - The delay line (identity_retarded_time_wake_of_a_ring_source) and the
 *   lock-in (identity_lock_in_amplifier_tuned_by_the_hue_step) combined:
 *   T(z) = z + tau on the Grid torus, generation n is the ring wave of n
 *   frames ago centred at -n tau, and Opponent turns it by 2 pi n h:
 *     C = sum_k a_k sum_n rho^n e^(i n (2 pi h + 5 k Delta))
 *                      e^(i k (20 |z + n tau| - 5 t)).
 * - Far from the track, |z + n tau| = |z| + n |tau| cos chi + ..., chi the
 *   angle between z and tau. The phase per pass becomes
 *   2 pi h + k (5 Delta + 20 |tau| cos chi): the frequency seen in
 *   direction chi is shifted by the source's motion, a Doppler shift. For
 *   the fundamental the resonance is
 *     cos chi = (2 pi h - 5 Delta) / (20 |tau|):
 *   for each h, one cone of directions about the track lights up.
 * - At 60 fps, speed 0.8, |tau| = 0.002: 5 Delta = 0.0667, 20 |tau| = 0.04,
 *   so h in [0.00424, 0.01698] covers every direction. The top of the sweep
 *   lights the direction of travel, the bottom the direction it came from,
 *   the middle (h = 0.01061) the broadside. The second harmonic adds a
 *   fainter cone at cos chi = (pi h - 5 Delta)/(20 |tau|) (-0.83 at the
 *   middle). The lane sweeps h once round in 500 s: the cone opens and
 *   closes like a searchlight that turns in angle, not in space.
 *
 * Pipeline: Identity, zn3 = tau = (0.002, 0); Grid; WaveRing at alpha 0.08;
 * fade 0.985; Opponent with the sweep lane on h.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Identity / Grid / WaveRing / Opponent
 * Nearest:    identity_lock_in_amplifier_tuned_by_the_hue_step (source at
 *             rest: the resonance is a ring, not a cone).
 * Height:     age = passes = delay.
 * Hue clock:  bounded; step swept over [0.00424, 0.01698].
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. The cone law is the far
 *             field (|z| much more than n |tau|, i.e. beyond about 0.15 at
 *             this fade); frame-rate dependent (Delta = speed / fps); the
 *             minimum-image wrap leaves seams where ring intensity < e^-2.
 *             Not built, rendered or measured.
 * @assert p_N >= 0.95
 */
inline Pattern identity_doppler_lock_in_cone_of_a_moving_source() {
  Pattern p;
  p.name = "identity_doppler_lock_in_cone_of_a_moving_source";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.002f, 0.0f}; // tau, per frame

  p.color.hue_rotation = 0.01061033f; // broadside resonance at 60 fps
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
      .add("doppler_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.002f,
                            .amplitude = 0.00636620f, // 20 |tau| / 2 pi
                            .offset = 0.01061033f},   // 5 Delta / 2 pi
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
