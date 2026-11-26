#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Kelvin-Stuart Cat's-Eye Shear Vortex (Liouville Euler Solution).
 *
 * Scientific foundation:
 * - Lord Kelvin (1880) and J.T. Stuart (1967, J. Fluid Mech.): exact steady
 *   vortex-street solutions of the 2D incompressible Euler vorticity equation
 *   nabla^2 psi = exp(-2*psi) form periodic chains of closed elliptical
 *   "cat's-eye" recirculation eddies bounded by hyperbolic saddle heteroclinic
 *   separatrices where passive tracers wind differentially.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = FluidVortex (z * exp(i * sin(z)), id 42): exact complex differential
 *   phase-shear twist modulated by sin(z) = sin(x)cosh(y) + i cos(x)sinh(y)!
 * - Reduction = Grid + Seed = OrganicNoise: periodic shear-layer channel fed by
 *   turbulent fBm vorticity filaments.
 * - Color = DepthEcho (mode 5): colors long-trapped eddy cores (high frame.w)
 *   distinctly from freshly entrained braid separatrices.
 */
inline Pattern kelvin_cats_eye_shear_vortex() {
  Pattern p;
  p.name = "kelvin_cats_eye_shear_vortex";
  p.inner = InnerFunction::FluidVortex;
  p.reduction = ReductionType::Grid;

  p.zn.zn[0] = {1.34f, 0.19f};
  p.zn.zn[1] = {0.04f, -0.025f};
  p.zn.zn[2] = {1.12f, -0.24f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.15f;
  p.zn.zn0_anim_amp = 0.046f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.19f;
  p.zn.zn1_anim_amp = 0.058f;
  p.zn.zn1_anim_k = 2.0f; // 2-eddy pairing mode

  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.14f;
  p.color.fade = 0.976f;
  p.color.color_mode = as_int(ColorMode::DepthEcho);
  p.color.spiral_angle_coeff = 0.48f; // depth_hue_scale
  p.color.spiral_radius_coeff = 0.42f;
  p.color.chroma_breath_amp = 0.16f;  // depth_sat_boost (depth clamped to [0,1])
  p.color.chroma_breath_freq = 0.12f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f; // subtle depth_luma_scale lift without washout
  p.color.hue_velocity_breath = 0.010f;
  p.color.hue_velocity_drift = 0.55f; // balanced spatial + depth stratification
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::OrganicNoise;
  p.seed.border_width = 0.105f;
  p.seed.hue = 0.54f;
  p.seed.hue_speed = 0.095f;
  p.seed.alpha = 0.93f;
  p.seed.pulse_freq = 0.08f;
  p.seed.pulse_amp = 0.18f;

  p.speed = 0.90f;

  p.motion
      .add("shear_layer_vorticity",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.084f,
                            .amplitude = 0.095f,
                            .offset = -0.24f},
           control::ModTarget::Zn2Im)
      .add("cats_eye_braid_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.052f,
                            .amplitude = 0.055f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Re)
      .add("eddy_echo_stratification",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.067f,
                            .amplitude = 0.12f,
                            .offset = 0.34f},
           control::ModTarget::SpiralAngleCoeff)
      .add("turbulent_boundary_width",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.046f,
                            .amplitude = 0.022f,
                            .offset = 0.105f},
           control::ModTarget::SeedWidth);

  return p;
}

} // namespace epi::presets
