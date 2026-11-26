#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimalist 3): Orthogonal Standing-Wave Nodal Crosshair.
 *
 * Mathematical foundation:
 * - Standing eigenmodes on a 2D square resonator cos(k_x x + i k_y y) = cos(z)
 *   map the origin z = 0 directly to z' = 1 (the Grid cell boundary), locking
 *   an interior orthogonal laser crosshair (SeedType::Cross) into a crisp
 *   Bauhaus / Josef Albers geometric harmonic lattice.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Cos + Grid reduction: parabolic lens cos(z) ~= 1 - z^2/2 replicates
 *   clean orthogonal nodal lines across modular tile boundaries.
 * - Seed = Cross: sharp exp(-20|x|) + exp(-20|y|) orthogonal energy filaments
 *   with 180-degree complementary hue split between horizontal and vertical bars.
 * - Color = SpatialWave (mode 4): 1 : sqrt(2) harmonic wave modulation yields
 *   minimalist, jewel-bright chromatic blocks.
 */
inline Pattern spatial_wave_minimal_standing_node() {
  Pattern p;
  p.name = "spatial_wave_minimal_standing_node";
  p.inner = InnerFunction::Cos;
  p.reduction = ReductionType::Grid;

  p.zn.zn[0] = {1.120f, 0.028f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.035f, -0.024f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.10f;
  p.zn.zn0_anim_amp = 0.022f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.13f;
  p.zn.zn1_anim_amp = 0.030f;
  p.zn.zn1_anim_k = 4.0f;

  p.color.hue_rotation = 0.010f;
  p.color.sat_scale = 1.038f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.50f;
  p.color.spiral_radius_coeff = 0.50f;
  p.color.chroma_breath_amp = 0.12f;
  p.color.chroma_breath_freq = 0.7071f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.015f;
  p.color.hue_velocity_breath = -0.010f;
  p.color.hue_velocity_drift = 0.0035f;
  p.color.luma_shape_depth = 0.12f;
  p.color.luma_shape_center = 2.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.045f;
  p.seed.hue = 0.08f;
  p.seed.hue_speed = 0.085f;
  p.seed.alpha = 0.90f;
  p.seed.pulse_freq = 0.07f;
  p.seed.pulse_amp = 0.14f;

  p.speed = 0.70f;

  p.motion
      .add("resonator_mode_x",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.064f,
                            .amplitude = 0.14f,
                            .offset = 0.50f},
           control::ModTarget::SpiralAngleCoeff)
      .add("resonator_mode_y",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.045f,
                            .amplitude = 0.14f,
                            .offset = 0.50f,
                            .phase = 0.25f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("nodal_cross_breath",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.058f,
                            .amplitude = 0.035f,
                            .offset = 1.035f},
           control::ModTarget::Zn2Re);

  return p;
}

} // namespace epi::presets
