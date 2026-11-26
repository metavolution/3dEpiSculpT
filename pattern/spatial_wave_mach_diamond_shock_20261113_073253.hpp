#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Advanced 1): Supersonic Prandtl-Meyer Mach Diamond Shock Train.
 *
 * Scientific foundation:
 * - When a supersonic compressible jet exits an over- or under-expanded nozzle,
 *   oblique shock waves and Prandtl-Meyer expansion fans reflect repeatedly off
 *   the free jet boundary, forming periodic standing Mach diamonds (shock cells)
 *   spaced by L ~= D * sqrt(M^2 - 1).
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = TP4 (0.8 * (cosh(z) + exp(z)) = 0.4 * (3*e^z + e^{-z})): inherently
 *   directional downstream amplification (3*e^z) paired with upstream acoustic
 *   reflection (e^{-z}) and transverse sinusoidal shock crossings!
 * - Reduction = PareidoliaCathedral: diagonal x + y = 0 reflection fold creates
 *   crisp 45-degree Mach-cone diamond facets.
 * - Color = SpatialWave + PolarAngle spatial mode: golden-to-silver ratio
 *   travelling pressure waves (1.618 : 2.414) sweep across the shock lattice.
 */
inline Pattern spatial_wave_mach_diamond_shock() {
  Pattern p;
  p.name = "spatial_wave_mach_diamond_shock";
  p.inner = InnerFunction::TP4;
  p.reduction = ReductionType::PareidoliaCathedral;

  p.zn.zn[0] = {1.08f, 0.24f};
  p.zn.zn[1] = {0.06f, -0.04f};
  p.zn.zn[2] = {0.82f, -0.18f};
  p.zn.zn[3] = {-0.12f, 0.08f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.19f;
  p.zn.zn0_anim_amp = 0.055f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.23f;
  p.zn.zn1_anim_amp = 0.065f;
  p.zn.zn1_anim_k = 4.0f;

  p.color.hue_rotation = 0.016f;
  p.color.sat_scale = 1.042f;
  p.color.fade = 0.976f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spatial_mode = ColorSpatialMode::PolarSpiral;
  p.color.spiral_angle_coeff = 0.88f;
  p.color.spiral_radius_coeff = 1.618f;
  p.color.chroma_breath_amp = 0.19f;
  p.color.chroma_breath_freq = 2.414f;
  p.color.chroma_breath_phase = 0.15f;
  p.color.hue_velocity_base = 0.028f;
  p.color.hue_velocity_breath = -0.021f;
  p.color.hue_velocity_drift = 0.008f;
  p.color.luma_shape_depth = 0.15f;
  p.color.luma_shape_center = 3.6f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.055f;
  p.seed.hue = 0.58f;
  p.seed.hue_speed = 0.16f;
  p.seed.alpha = 0.94f;
  p.seed.pulse_freq = 0.14f;
  p.seed.pulse_amp = 0.22f;

  p.speed = 1.05f;

  p.motion
      .add("nozzle_pressure_ratio",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.088f,
                            .amplitude = 0.065f,
                            .offset = 0.82f},
           control::ModTarget::Zn2Re)
      .add("mach_cone_angle",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.114f,
                            .amplitude = 0.35f,
                            .offset = 1.618f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("shock_cell_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.063f,
                            .amplitude = 0.055f,
                            .offset = -0.12f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Re)
      .add("plume_sat_pulse",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.079f,
                            .amplitude = 0.05f,
                            .offset = 0.15f},
           control::ModTarget::LumaShapeDepth);

  return p;
}

} // namespace epi::presets
