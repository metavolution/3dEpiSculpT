#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spectral & Optical: Gauss-Sum Theta Resonator in Colour Space.
 *
 * Scientific foundation:
 * - Incomplete Gauss sums S_N = sum_{n<N} exp(2 pi i (n u + tau n^2 / 2)) are
 *   partial Jacobi theta functions. For rational tau = p/q they are periodic
 *   and resonate on q-fold families of u; for irrational tau they trace the
 *   self-similar "curlicues" renormalised by Berry and Goldberg (1988). The
 *   same sums give the fractional Talbot revivals of a grating.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Identity with |zn0| = 1: T is a rigid rotation by alpha. Nothing
 *   expands, so every circle is a closed ring resonator and the feedback
 *   recirculates on it indefinitely. This is the one regime where the loop
 *   behaves as a cavity rather than as an orbit-trap renderer.
 * - Color = Opponent with spiral_angle_coeff = 1. Each pass rotates the
 *   chroma vector by 2 pi (c + polar angle). After n round steps the pixel at
 *   angle u has accumulated the phase
 *       n (c + u) + (alpha / 2 pi) n (n - 1) / 2,
 *   quadratic in n. Opponent colour is linear in RGB, so successive passes
 *   add as complex amplitudes and the steady chroma is
 *       sum_n rho^n exp(2 pi i (n (c + u) + tau n^2 / 2)),   tau = alpha/2 pi,
 *   a damped Gauss sum, with rho = fade * (1 - seed alpha) the cavity finesse.
 * - Seed = WaveRing, rotation-symmetric, so it feeds every ring evenly and
 *   its low alpha at large radius raises the finesse outward.
 * - What to look for: near alpha = 72 degrees (tau = 1/5) the chroma locks
 *   onto a few sharp spokes; off resonance the spokes curl. Luminance is
 *   untouched by the rotation, so colour appears and cancels on a steady
 *   grey ring pattern: interference in colour, not in brightness.
 * - Motion: zn0 swings alpha through the 1/5 resonance (first-order lanes,
 *   |zn0| stays within 0.07 percent of 1); the hue clock turns the spokes.
 */
inline Pattern gauss_sum_theta_chroma_resonator() {
  constexpr float COS_A = 0.30901699f; // cos 72 deg
  constexpr float SIN_A = 0.95105652f; // sin 72 deg
  constexpr float SWING = 0.035f;      // radians either side of resonance
  constexpr float SWING_HZ = 0.011f;

  Pattern p;
  p.name = "gauss_sum_theta_chroma_resonator";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {COS_A, SIN_A};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.992f; // high-Q cavity
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 1.0f;   // phase winds once round the cavity
  p.color.spiral_radius_coeff = 0.15f; // log-spiral twist of the fringes
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.004f; // detuning ramp: turns the spokes
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.07f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 0.50f; // input coupler
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("round_trip_angle_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SWING_HZ,
                            .amplitude = -SIN_A * SWING,
                            .offset = COS_A},
           control::ModTarget::Zn0Re)
      .add("round_trip_angle_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SWING_HZ,
                            .amplitude = COS_A * SWING,
                            .offset = SIN_A},
           control::ModTarget::Zn0Im)
      .add("fringe_twist",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.017f,
                            .amplitude = 0.12f,
                            .offset = 0.15f},
           control::ModTarget::SpiralRadiusCoeff);

  return p;
}

} // namespace epi::presets
