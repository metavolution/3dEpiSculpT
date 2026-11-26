#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Spatial Wave (Advanced 6): Laguerre-Gauss Optical Vortex (Orbital Angular
 * Momentum Beam).
 *
 * Scientific foundation:
 * - Allen, Beijersbergen, Spreeuw & Woerdman (1992): paraxial LG_p^l modes
 *   carry orbital angular momentum l*hbar per photon. Their phase is a helix
 *   exp(i*l*phi) around a dark core, with p concentric radial node rings, and
 *   they acquire the Gouy phase (2p + |l| + 1)*atan(z/z_R) through focus.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = F8 (0.5*(cosh(z) + z^2)): quadratic wavefront curvature (z^2, the
 *   paraxial lens phase) blended with a cosh amplitude envelope.
 * - Seed = WaveRing: travelling concentric rings are the radial node rings
 *   (p-index) of the mode; they are strongest on the optical axis, exactly
 *   where F8's flat critical region would otherwise stay dark.
 * - Color = SpatialWave + PolarSpiral spatial mode: every feedback pass adds
 *   the polar angle to the hue, i.e. one unit of topological charge per pass
 *   (a literal exp(i*l*phi) phase helix wound by the recurrence).
 * - Motion: Gouy phase slip on Zn2Im, charge pitch and radial-node breathing.
 */
inline Pattern spatial_wave_laguerre_gauss_vortex() {
  constexpr float A0 = 0.78539816f;  // pi/4
  constexpr float A2 = -1.74532925f; // -100 deg
  constexpr float PRE = 1.36f;
  constexpr float POST = 1.14f;
  const float zn2_im = POST * std::sin(A2);

  Pattern p;
  p.name = "spatial_wave_laguerre_gauss_vortex";
  p.inner = InnerFunction::F8;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {PRE * std::cos(A0), PRE * std::sin(A0)};
  p.zn.zn[1] = {0.03f, -0.02f};
  p.zn.zn[2] = {POST * std::cos(A2), zn2_im};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.13f;
  p.zn.zn0_anim_amp = 0.04f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.16f;
  p.zn.zn1_anim_amp = 0.05f;
  p.zn.zn1_anim_k = 2.0f;

  p.color.hue_rotation = 0.013f;
  p.color.sat_scale = 1.06f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spatial_mode = ColorSpatialMode::PolarSpiral;
  p.color.spiral_angle_coeff = 0.5f;
  p.color.spiral_radius_coeff = 0.75f;
  p.color.chroma_breath_amp = 0.17f;
  p.color.chroma_breath_freq = 1.5f;
  p.color.chroma_breath_phase = 0.11f;
  p.color.hue_velocity_base = 0.020f;
  p.color.hue_velocity_breath = -0.013f;
  p.color.hue_velocity_drift = 0.005f;
  p.color.luma_shape_depth = 0.15f;
  p.color.luma_shape_center = 2.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.07f;
  p.seed.hue = 0.70f;
  p.seed.hue_speed = 0.11f;
  p.seed.alpha = 0.94f;
  p.seed.pulse_freq = 0.09f;
  p.seed.pulse_amp = 0.17f;
  p.speed = 0.9f;

  p.motion
      .add("gouy_phase_slip",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.052f,
                            .amplitude = 0.07f,
                            .offset = zn2_im},
           control::ModTarget::Zn2Im)
      .add("topological_charge_pitch",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.067f,
                            .amplitude = 0.14f,
                            .offset = 0.5f},
           control::ModTarget::SpiralAngleCoeff)
      .add("radial_node_breath",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.081f,
                            .amplitude = 0.05f,
                            .offset = 0.17f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
