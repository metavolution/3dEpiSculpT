#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Aharonov-Bohm Magnetic Flux Tube & Berry Wavefront Dislocation.
 *
 * Scientific foundation:
 * - Y. Aharonov & D. Bohm (1959, Phys. Rev.) and M.V. Berry et al. (1980,
 *   Eur. J. Phys. "Wavefront dislocations in the Aharonov-Bohm effect"):
 *   matter waves diffracting around a shielded solenoid carrying magnetic flux
 *   Phi = alpha * (h/e) acquire a geometric gauge phase exp(i * alpha * theta),
 *   splitting interference fringes at the origin into a topological fork
 *   dislocation!
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = F4 (0.8 * (cos(z) + exp(i*z)), id 24): coherent superposition of a
 *   symmetric standing wave cos(z) and a chiral directional gauge wave exp(i*z).
 * - Color = LumaLockedHueDrift (mode 10) + PolarAngle spatial mode: locks exact
 *   wavefront luminance while winding hue topologically by polar angle theta
 *   around the flux singularity at zn1 = (0.11, -0.07).
 */
inline Pattern aharonov_bohm_flux_dislocation() {
  Pattern p;
  p.name = "aharonov_bohm_flux_dislocation";
  p.inner = InnerFunction::F4;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.42f, 0.35f};
  p.zn.zn[1] = {0.11f, -0.07f}; // shielded solenoid flux core offset
  p.zn.zn[2] = {1.18f, -0.26f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.14f;
  p.zn.zn0_anim_amp = 0.044f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.17f;
  p.zn.zn1_anim_amp = 0.052f;
  p.zn.zn1_anim_k = 3.0f;

  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 0.92f;
  p.color.fade = 0.985f;
  p.color.color_mode = as_int(ColorMode::LumaLockedHueDrift);
  p.color.spatial_mode = ColorSpatialMode::PolarSpiral;
  p.color.spiral_angle_coeff = 1.0f; // Burgers charge n = 1 winding
  p.color.spiral_radius_coeff = 0.48f;
  p.color.chroma_breath_amp = 0.16f;
  p.color.chroma_breath_freq = 0.14f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.020f;
  p.color.hue_velocity_breath = 0.012f;
  p.color.hue_velocity_drift = 0.005f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.06f;
  p.seed.hue = 0.48f;
  p.seed.hue_speed = 0.11f;
  p.seed.alpha = 0.94f;
  p.seed.pulse_freq = 0.09f;
  p.seed.pulse_amp = 0.18f;

  p.speed = 0.88f;

  p.motion
      .add("solenoid_flux_quantum",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.074f,
                            .amplitude = 0.38f,
                            .offset = 1.0f},
           control::ModTarget::SpiralAngleCoeff)
      .add("berry_dislocation_shear",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.048f,
                            .amplitude = 0.085f,
                            .offset = -0.26f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Im)
      .add("gauge_radial_phase",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.063f,
                            .amplitude = 0.18f,
                            .offset = 0.48f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("aharonov_hue_velocity",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.089f,
                            .amplitude = 0.010f,
                            .offset = 0.020f},
           control::ModTarget::HueVelocityBase);

  return p;
}

} // namespace epi::presets
