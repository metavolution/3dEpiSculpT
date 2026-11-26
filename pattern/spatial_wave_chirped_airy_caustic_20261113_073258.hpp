#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Advanced 4): Exponentially Chirped Airy-Pearcey Rainbow Caustic.
 *
 * Scientific foundation:
 * - Near an optical fold or cusp catastrophe (G.B. Airy 1838 rainbow integral;
 *   T. Pearcey 1946 diffraction cusp), coalescing stationary-phase rays generate
 *   asymmetric supernumerary interference fringes whose spatial frequency chirps
 *   nonlinearly away from the bright caustic ridge.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = F10 (0.9 * (z * tanh(z) + cos(exp(z)))): combines bounded envelope
 *   growth z*tanh(z) with exponentially chirped phase oscillation cos(e^z),
 *   producing razor-sharp accelerating fringe cascades across the complex plane!
 * - Color = SpatialWave + LinearX: dispersive supernumerary rainbow bands sweep
 *   across the chirped caustic manifolds.
 */
inline Pattern spatial_wave_chirped_airy_caustic() {
  Pattern p;
  p.name = "spatial_wave_chirped_airy_caustic";
  p.inner = InnerFunction::F10;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.22f, 0.31f};
  p.zn.zn[1] = {0.05f, -0.03f};
  p.zn.zn[2] = {1.14f, -0.22f};
  p.zn.zn[3] = {-0.04f, 0.06f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.15f;
  p.zn.zn0_anim_amp = 0.046f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.18f;
  p.zn.zn1_anim_amp = 0.054f;
  p.zn.zn1_anim_k = 3.0f; // fold-cusp 3-ray coalescence symmetry

  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 1.044f;
  p.color.fade = 0.979f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spatial_mode = ColorSpatialMode::HorizontalWave;
  p.color.spiral_angle_coeff = 0.92f;
  p.color.spiral_radius_coeff = 1.32472f; // plastic constant rho (x^3 = x + 1)
  p.color.chroma_breath_amp = 0.20f;
  p.color.chroma_breath_freq = 1.75488f;  // rho^2
  p.color.chroma_breath_phase = 0.16f;
  p.color.hue_velocity_base = 0.026f;
  p.color.hue_velocity_breath = -0.018f;
  p.color.hue_velocity_drift = 0.008f;
  p.color.luma_shape_depth = 0.15f;
  p.color.luma_shape_center = 3.8f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.055f;
  p.seed.hue = 0.02f;
  p.seed.hue_speed = 0.15f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.10f;
  p.seed.pulse_amp = 0.20f;

  p.speed = 0.98f;

  p.motion
      .add("supernumerary_chirp",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.089f,
                            .amplitude = 0.34f,
                            .offset = 1.32472f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("pearcey_cusp_twist",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.067f,
                            .amplitude = 0.09f,
                            .offset = -0.22f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Im)
      .add("rainbow_angular_dispersion",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.054f,
                            .amplitude = 0.24f,
                            .offset = 0.92f},
           control::ModTarget::SpiralAngleCoeff)
      .add("caustic_ridge_pulse",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.104f,
                            .amplitude = 0.06f,
                            .offset = 0.20f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
