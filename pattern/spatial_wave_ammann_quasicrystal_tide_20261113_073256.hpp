#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Advanced 3): Ammann-Beenker Octagonal Phason Wave Tide.
 *
 * Scientific foundation:
 * - The Ammann-Beenker tiling is an aperiodic 8-fold quasicrystal projected from
 *   the 4D hypercubic lattice Z^4 with silver-ratio (delta_S = 1 + sqrt(2))
 *   self-inflation. Long-wavelength phason strain waves flip local rhomb/square
 *   vertices coherently across D_8 symmetry sectors.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = F2 (sin(z) * cosh(z)) + Reduction = AmmannBeenker: projects oscillating
 *   hyperbolic wavepackets directly through the pi/8 octagonal sector reflection!
 * - Pre-affine rotated at pi/8 (22.5 deg), post-affine at 3*pi/8 (67.5 deg).
 * - Color = SpatialWave + PolarAngle: silver-ratio spatial wave pair (1.0 : 2.4142)
 *   creates shimmering 8-fold aperiodic color tides.
 */
inline Pattern spatial_wave_ammann_quasicrystal_tide() {
  constexpr float C8  = 0.9238795325f; // cos(pi/8)
  constexpr float S8  = 0.3826834324f; // sin(pi/8)
  constexpr float C38 = 0.3826834324f; // cos(3*pi/8)
  constexpr float S38 = 0.9238795325f; // sin(3*pi/8)

  Pattern p;
  p.name = "spatial_wave_ammann_quasicrystal_tide";
  p.inner = InnerFunction::F2;
  p.reduction = ReductionType::AmmannBeenker;

  p.zn.zn[0] = {1.32f * C8, 1.32f * S8};
  p.zn.zn[1] = {0.022f, -0.016f};
  p.zn.zn[2] = {1.08f * C38, -1.08f * S38};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.14f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.17f;
  p.zn.zn1_anim_amp = 0.052f;
  p.zn.zn1_anim_k = 8.0f; // 8-petal D_8 phason modulation

  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.042f;
  p.color.fade = 0.981f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spatial_mode = ColorSpatialMode::PolarSpiral;
  p.color.spiral_angle_coeff = 0.78f;
  p.color.spiral_radius_coeff = 1.0f;
  p.color.chroma_breath_amp = 0.17f;
  p.color.chroma_breath_freq = 2.41421f; // silver ratio 1 + sqrt(2)
  p.color.chroma_breath_phase = 0.13f;
  p.color.hue_velocity_base = 0.022f;
  p.color.hue_velocity_breath = -0.015f;
  p.color.hue_velocity_drift = 0.006f;
  p.color.luma_shape_depth = 0.14f;
  p.color.luma_shape_center = 3.414f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.088f;
  p.seed.hue = 0.14f;
  p.seed.hue_speed = 0.12f;
  p.seed.alpha = 0.94f;
  p.seed.pulse_freq = 0.09f;
  p.seed.pulse_amp = 0.18f;

  p.speed = 0.92f;

  p.motion
      .add("phason_strain_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.072f,
                            .amplitude = 0.045f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("phason_strain_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.072f * 1.41421f,
                            .amplitude = 0.045f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Im)
      .add("silver_wave_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.059f,
                            .amplitude = 0.26f,
                            .offset = 1.0f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("octagonal_hue_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.044f,
                            .amplitude = 0.008f,
                            .offset = 0.014f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
