#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Advanced 3.5D): the Feedback Loop as a Lock-in Amplifier, Tuned
 * by the Hue Step.
 *
 * Mathematics:
 * - With T = id the loop only remembers: generation n holds the seed of n
 *   frames ago, S(z, t - n Delta), Delta = speed / fps (see
 *   identity_retarded_time_wake_of_a_ring_source). Opponent colour turns
 *   the chroma of each pass by 2 pi h and is linear, so
 *     C(z, t) = sum_n rho^n e^(2 pi i n h) I(r, t - n Delta) (seed tint fixed).
 * - The WaveRing intensity I = max(0, sin(20 r - 5 t)) e^(-2r) is periodic
 *   in t; write it as sum_k a_k(r) e^(i k (20 r - 5 t)). Then
 *     C = sum_k a_k e^(i k (20 r - 5 t)) / (1 - rho e^(i (2 pi h + 5 k Delta))).
 *   This is a lock-in amplifier: harmonic k is picked out, with Lorentzian
 *   gain 1/(1 - rho) at h = |k| 5 Delta / 2 pi. On resonance the colour
 *   phase is the wave's phase: hue rings travel with the crests. Off
 *   resonance the gain falls to between 1/(1 + rho) and a few passes'
 *   worth. rho is the Q factor, h the tuning knob.
 * - A half-wave rectified sine has harmonics k = 0, 1 and the even k only
 *   (1/pi, 1/4, then 1/(pi (k^2 - 1))): the k = 3 line is missing. A
 *   selection rule from the waveform.
 * - At 60 fps and speed 0.8, h_k = k x 0.01061. The hue lane sweeps h over
 *   [0, 0.034]: DC at 0 (the time-averaged tint), the fundamental at
 *   0.0106 (amplitude 1/4), the second harmonic at 0.0212 (1/(3 pi)), and
 *   silence at 0.0318. The light channel is the unrotated average: static.
 *
 * Pipeline: Identity, all slots trivial; Clamp (never reached); WaveRing at
 * alpha 0.08 (near-linear blend); fade 0.985 (line width in h about
 * (1 - rho)/2 pi = 0.003); Opponent with the sweep lane at 0.002 Hz.
 *
 * Fidelity:   identity
 * Cell:       Identity / Clamp / WaveRing / Opponent
 * Nearest:    identity_retarded_time_wake_of_a_ring_source (the delay line
 *             with a moving source); here the source stands still and the
 *             delay line becomes a filter.
 * Height:     age = passes = delay.
 * Hue clock:  bounded; step swept over [0, 0.034].
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. Frame-rate dependent:
 *             h_k = 0.6366 k / fps (at 120 fps the sweep covers k = 1..6, and
 *             the missing line moves to 0.0159); frame-time jitter broadens
 *             the lines. The seed blend (1 - alpha I) makes the filter
 *             slightly nonlinear. Not built, rendered or measured.
 * @assert p_N >= 0.95
 */
inline Pattern identity_lock_in_amplifier_tuned_by_the_hue_step() {
  Pattern p;
  p.name = "identity_lock_in_amplifier_tuned_by_the_hue_step";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.01061033f; // the fundamental at 60 fps
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
      .add("tuning_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.002f,
                            .amplitude = 0.017f,
                            .offset = 0.017f,
                            .phase = 0.93867317f}, // starts on h_1
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
