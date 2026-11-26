#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Advanced 2): Berry-Klein Fractional Quantum Talbot Wave Carpet.
 *
 * Scientific foundation:
 * - H.F. Talbot (1836) and M.V. Berry & S. Klein (1996, "Integer, fractional and
 *   fractal Talbot effects"): a periodic grating illuminated by coherent waves
 *   reconstructs q shifted Gauss-sum copies of itself at rational propagation
 *   distances (p/q) * z_T, and a self-similar fractal wave carpet at irrational
 *   slices, driven by quadratic Fresnel phase exp(i k x^2 / 2z).
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = F3 (sin(z^2) * sin(i*z)): explicitly couples quadratic Fresnel phase
 *   oscillation sin(z^2) with hyperbolic grating modulation sin(i*z) = i*sinh(z).
 * - Reduction = Grid + Seed = ChladniCymatics: periodic diffraction grating
 *   fed by resonant nodal wave sandlines.
 * - Color = SpatialWave (mode 4): sqrt(3) and sqrt(5) irrational spatial wave
 *   frequencies weave multi-scale chromatic Gauss-sum interference tapestry.
 */
inline Pattern spatial_wave_berry_talbot_carpet() {
  Pattern p;
  p.name = "spatial_wave_berry_talbot_carpet";
  p.inner = InnerFunction::F3;
  p.reduction = ReductionType::Grid;

  p.zn.zn[0] = {1.36f, 0.24f};
  p.zn.zn[1] = {0.03f, -0.02f};
  p.zn.zn[2] = {1.16f, -0.19f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.16f;
  p.zn.zn0_anim_amp = 0.048f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.19f;
  p.zn.zn1_anim_amp = 0.055f;
  p.zn.zn1_anim_k = 6.0f;

  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 1.045f;
  p.color.fade = 0.980f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.84f;
  p.color.spiral_radius_coeff = 1.73205f; // sqrt(3)
  p.color.chroma_breath_amp = 0.18f;
  p.color.chroma_breath_freq = 2.23607f;  // sqrt(5)
  p.color.chroma_breath_phase = 0.14f;
  p.color.hue_velocity_base = 0.024f;
  p.color.hue_velocity_breath = 0.017f;
  p.color.hue_velocity_drift = 0.007f;
  p.color.luma_shape_depth = 0.14f;
  p.color.luma_shape_center = 4.2f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.055f;
  p.seed.hue = 0.68f;
  p.seed.hue_speed = 0.14f;
  p.seed.alpha = 0.93f;
  p.seed.pulse_freq = 0.11f;
  p.seed.pulse_amp = 0.18f;

  p.speed = 0.96f;

  p.motion
      .add("talbot_fractional_z",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.082f,
                            .amplitude = 0.38f,
                            .offset = 1.73205f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("gauss_sum_shear",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.051f,
                            .amplitude = 0.085f,
                            .offset = -0.19f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Im)
      .add("grating_harmonic_pitch",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.069f,
                            .amplitude = 0.22f,
                            .offset = 0.84f},
           control::ModTarget::SpiralAngleCoeff)
      .add("fresnel_chroma_breath",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.095f,
                            .amplitude = 0.06f,
                            .offset = 0.18f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
