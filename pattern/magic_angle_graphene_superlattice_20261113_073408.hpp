#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Bistritzer-MacDonald Twisted Bilayer Graphene Magic-Angle Superlattice.
 *
 * Scientific foundation:
 * - Bistritzer & MacDonald (2011, PNAS) and Cao et al. (2018, Nature): rotating
 *   two graphene Dirac lattices by a small twist angle (~1.1-2.0 deg) creates a
 *   macroscopic moire superlattice of alternating AA-stacked flat-band islands
 *   and AB/BA Bernal saddle domains where Fermi velocity vanishes.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Moire (0.5 * (sin(z) + sin(z * e^{i * 2 deg})), id 37): directly
 *   evaluates two wave lattices twisted by 2 degrees!
 * - Pre-affine rotated by pi/3 (60 deg honeycomb K-valley) and post-affine by
 *   pi/6 (30 deg armchair/zigzag duality), supercritical scale for full-field
 *   moire supercell tiling.
 * - Color = MoireInterference (mode 12): clamped multiplicative beat envelope.
 */
inline Pattern magic_angle_graphene_superlattice() {
  constexpr float C60 = 0.5000000000f;
  constexpr float S60 = 0.8660254038f;
  constexpr float C30 = 0.8660254038f;
  constexpr float S30 = 0.5000000000f;

  Pattern p;
  p.name = "magic_angle_graphene_superlattice";
  p.inner = InnerFunction::Moire;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.54f * C60, 1.54f * S60};
  p.zn.zn[1] = {0.025f, -0.018f};
  p.zn.zn[2] = {1.32f * C30, -1.32f * S30};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.12f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.15f;
  p.zn.zn1_anim_amp = 0.048f;
  p.zn.zn1_anim_k = 6.0f; // C_6 honeycomb AA/AB breathing

  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.18f;
  p.color.fade = 0.989f;
  p.color.color_mode = as_int(ColorMode::MoireInterference);
  p.color.spiral_angle_coeff = 3.0f;
  p.color.spiral_radius_coeff = 1.45f;
  p.color.chroma_breath_amp = 0.28f;
  p.color.chroma_breath_freq = 0.16f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.022f;
  p.color.hue_velocity_breath = 0.014f;
  p.color.hue_velocity_drift = 0.005f;
  p.color.luma_shape_depth = 0.24f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.088f;
  p.seed.hue = 0.56f;
  p.seed.hue_speed = 0.10f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.08f;
  p.seed.pulse_amp = 0.16f;

  p.speed = 0.88f;

  p.motion
      .add("twist_angle_microstrain",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.076f,
                            .amplitude = 0.65f,
                            .offset = 3.0f},
           control::ModTarget::SpiralAngleCoeff)
      .add("flat_band_dispersion",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.047f,
                            .amplitude = 0.36f,
                            .offset = 1.45f,
                            .phase = 0.25f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("interlayer_heterostrain",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.062f,
                            .amplitude = 0.042f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("aa_island_luminance",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.091f,
                            .amplitude = 0.07f,
                            .offset = 0.24f},
           control::ModTarget::LumaShapeDepth);

  return p;
}

} // namespace epi::presets
