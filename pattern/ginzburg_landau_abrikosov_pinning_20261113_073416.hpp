#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Ginzburg-Landau Superconducting Abrikosov Fluxoid Pinning Lattice.
 *
 * Scientific foundation:
 * - V.L. Ginzburg & L.D. Landau (1950) and A.A. Abrikosov (1957, Nobel 2003):
 *   between lower and upper critical fields H_c1 < H < H_c2, a Type-II
 *   superconductor admits quantized magnetic vortices (Phi_0 = h / 2e) whose
 *   order parameter |psi(z)| vanishes at C_6 triangular lattice nodes while
 *   supercurrents circulate around cosh/parabolic free-energy wells.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = F8 (0.5 * (cosh(z) + z^2), id 28): combines harmonic confinement z^2
 *   with hyperbolic Josephson barrier cosh(z)!
 * - Pre-affine rotated by 60 deg (pi/3) and post-affine by 120 deg (2*pi/3) to
 *   enforce triangular Abrikosov coordination.
 * - Color = PaletteMorph (mode 7): warm-to-cryogenic cosine phase sweep.
 */
inline Pattern ginzburg_landau_abrikosov_pinning() {
  constexpr float C60  =  0.5000000000f;
  constexpr float S60  =  0.8660254038f;
  constexpr float C120 = -0.5000000000f;
  constexpr float S120 =  0.8660254038f;

  Pattern p;
  p.name = "ginzburg_landau_abrikosov_pinning";
  p.inner = InnerFunction::F8;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.38f * C60, 1.38f * S60};
  p.zn.zn[1] = {0.03f, -0.02f};
  p.zn.zn[2] = {1.12f * C120, 1.12f * S120};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.13f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.16f;
  p.zn.zn1_anim_amp = 0.048f;
  p.zn.zn1_anim_k = 6.0f; // hexagonal flux-creep oscillation

  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.88f;
  p.color.fade = 0.990f;
  p.color.color_mode = as_int(ColorMode::PaletteMorph);
  p.color.spiral_angle_coeff = 3.0f;
  p.color.spiral_radius_coeff = 0.55f;
  p.color.chroma_breath_amp = 0.22f;
  p.color.chroma_breath_freq = 0.14f;
  p.color.chroma_breath_phase = 6.0f;
  p.color.hue_velocity_base = 0.08f;
  p.color.hue_velocity_breath = 0.21f;
  p.color.hue_velocity_drift = 0.42f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.092f;
  p.seed.hue = 0.58f;
  p.seed.hue_speed = 0.09f;
  p.seed.alpha = 0.94f;
  p.seed.pulse_freq = 0.08f;
  p.seed.pulse_amp = 0.16f;

  p.speed = 0.86f;

  p.motion
      .add("applied_field_flux_density",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.071f,
                            .amplitude = 0.65f,
                            .offset = 3.0f},
           control::ModTarget::SpiralAngleCoeff)
      .add("london_penetration_depth",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.044f,
                            .amplitude = 0.20f,
                            .offset = 0.55f,
                            .phase = 0.25f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("lorentz_depinning_creep",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.059f,
                            .amplitude = 0.045f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Im)
      .add("cryo_palette_breath",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.088f,
                            .amplitude = 0.07f,
                            .offset = 0.22f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
