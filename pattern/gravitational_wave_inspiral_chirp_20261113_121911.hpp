#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Gravitational-Wave Inspiral Chirp and Ringdown.
 *
 * Scientific foundation:
 * - A compact binary loses orbital energy to quadrupole radiation (Peters
 *   1964). To leading order the wave frequency sweeps upward as
 *   f(t) ~ (t_c - t)^(-3/8), governed by the chirp mass
 *   M_c = (m1*m2)^(3/5) / (m1+m2)^(1/5). After merger the remnant rings down
 *   in damped quasi-normal modes. First observed as GW150914 (LIGO, 2015).
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = F10 (0.9*(z*tanh(z) + cos(exp(z)))): cos(exp(z)) is an
 *   exponentially chirped oscillation: equal steps in Re z multiply the local
 *   frequency by a constant factor, the runaway sweep of the late inspiral
 *   (an exponential stand-in for the -3/8 power law). z*tanh(z) is the
 *   smooth, bounded-slope orbital envelope.
 * - Seed = WaveRing: damped concentric rings (sin(20r - 5t) * exp(-2r)) are a
 *   literal ringdown.
 * - zn1 rose animation with k = 2: quadrupole (l = 2) symmetry of the source.
 * - Color = QuasiPeriodicWave with log-radius pitch 0.375 = 3/8.
 * - Motion: a SAWTOOTH on the ring pitch rises steadily and snaps back:
 *   chirp, merger, next event.
 */
inline Pattern gravitational_wave_inspiral_chirp() {
  constexpr float A0 = 0.43633231f;  // 25 deg
  constexpr float A2 = -0.34906585f; // -20 deg
  constexpr float PRE = 1.32f;
  constexpr float POST = 1.20f;
  const float zn2_im = POST * std::sin(A2);

  Pattern p;
  p.name = "gravitational_wave_inspiral_chirp";
  p.inner = InnerFunction::F10;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {PRE * std::cos(A0), PRE * std::sin(A0)};
  p.zn.zn[1] = {0.04f, -0.025f};
  p.zn.zn[2] = {POST * std::cos(A2), zn2_im};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.12f;
  p.zn.zn0_anim_amp = 0.04f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.16f;
  p.zn.zn1_anim_amp = 0.045f;
  p.zn.zn1_anim_k = 2.0f; // quadrupole

  p.color.hue_rotation = 0.006f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::QuasiPeriodicWave);
  p.color.spiral_angle_coeff = 0.5f;
  p.color.spiral_radius_coeff = 0.375f; // 3/8
  p.color.chroma_breath_amp = 0.14f;
  p.color.chroma_breath_freq = 0.10f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.013f;
  p.color.hue_velocity_breath = 0.008f;
  p.color.hue_velocity_drift = 0.005f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.07f;
  p.seed.hue = 0.66f;
  p.seed.hue_speed = 0.10f;
  p.seed.alpha = 0.93f;
  p.seed.pulse_freq = 0.10f;
  p.seed.pulse_amp = 0.20f;
  p.speed = 0.85f;

  p.motion
      .add("chirp_mass_sweep",
           control::ModLane{.wave = control::Waveform::Sawtooth,
                            .frequency_hz = 0.04f,
                            .amplitude = 0.12f,
                            .offset = 0.375f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("orbital_decay",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.055f,
                            .amplitude = 0.07f,
                            .offset = zn2_im},
           control::ModTarget::Zn2Im)
      .add("ringdown_chroma",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.09f,
                            .amplitude = 0.05f,
                            .offset = 0.14f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
