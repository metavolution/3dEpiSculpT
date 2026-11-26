#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Callen-Welton Fluctuation-Dissipation: the z*coth(z) Law of Quantum Noise.
 *
 * Scientific foundation:
 * - H. Callen & T. Welton (1951): the noise spectrum of any linear dissipative
 *   system is S(w) = hbar*w*coth(hbar*w / 2kT) * Re Z(w). With
 *   x = hbar*w/2kT the universal factor is x*coth(x): it tends to 1 for
 *   x -> 0 (classical Johnson-Nyquist noise, 4kTR) and to |x| for large x
 *   (zero-point fluctuations). The same function is the mean energy of a
 *   thermal quantum oscillator, E = (hbar*w/2)*coth(hbar*w/2kT).
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = ZTI (2z/tan(iz) = -2i * z*coth(z)): the Callen-Welton factor
 *   continued to complex frequency. It is even, finite at 0 (value -2i) and
 *   has poles at z = i*pi*k, the Matsubara frequencies; the pre-scale 0.92
 *   keeps the frame strictly inside the first Matsubara pole.
 * - |T| >= 2*|zn2| = 1.72 > 1 everywhere, so every pixel folds across the
 *   torus border at least once: a thin seed (0.012) suffices.
 * - Color = OKLab rotation: perceptually uniform hue rotation ~ temperature.
 * - Seed hue starts thermal red; motion sweeps temperature (ring pitch) and
 *   zero-point chroma.
 */
inline Pattern callen_welton_thermal_coth_noise() {
  constexpr float A0 = 0.52359878f;  // 30 deg
  constexpr float A2 = -1.04719755f; // -60 deg
  constexpr float PRE = 0.92f;  // inside the first Matsubara pole |Im w| = pi
  constexpr float POST = 0.86f;

  Pattern p;
  p.name = "callen_welton_thermal_coth_noise";
  p.inner = InnerFunction::ZTI;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {PRE * std::cos(A0), PRE * std::sin(A0)};
  p.zn.zn[1] = {0.0f, -0.004f};
  p.zn.zn[2] = {POST * std::cos(A2), POST * std::sin(A2)};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.11f;
  p.zn.zn0_anim_amp = 0.05f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.20f;
  p.zn.zn1_anim_amp = 0.04f;
  p.zn.zn1_anim_k = 4.0f;

  p.color.hue_rotation = 0.030f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.991f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 1.0f;
  p.color.spiral_radius_coeff = 0.30f;
  p.color.chroma_breath_amp = 0.25f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 6.2831853f;
  p.color.hue_velocity_base = 0.020f;
  p.color.hue_velocity_breath = 0.012f;
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.012f;
  p.seed.hue = 0.02f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.84f;
  p.seed.pulse_freq = 0.09f;
  p.seed.pulse_amp = 0.18f;
  p.speed = 0.5f;

  p.motion
      .add("temperature_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.06f,
                            .amplitude = 0.12f,
                            .offset = 0.30f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("zero_point_chroma",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.08f,
                            .amplitude = 0.10f,
                            .offset = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("nyquist_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.05f,
                            .amplitude = 0.012f,
                            .offset = 0.030f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
