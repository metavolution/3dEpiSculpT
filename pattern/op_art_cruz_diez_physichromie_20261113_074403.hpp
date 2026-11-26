#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Carlos Cruz-Diez "Physichromie" Lamellar Additive Chromatic Induction.
 *
 * Kinetic & perceptual art foundation:
 * - Carlos Cruz-Diez (Physichromie series, 1959-2019): arrays of vertical
 *   colored lamellae act as directional light traps and reflectors; as a viewer
 *   walks laterally past the work, optical mixture and Bezold assimilation
 *   project a hovering "chromatic atmosphere" detached from the physical plane.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Sin + Grid wrap + ColorSpatialMode::LinearX: builds crisp vertical
 *   lamellar interference louvers coupled with horizontal phase stratification.
 * - Color = AtmosphericDrift (mode 13): continuous multi-band spectral veil
 *   modulated by lateral viewer-parallax motion on Zn3Re and ChromaBreathPhase.
 */
inline Pattern op_art_cruz_diez_physichromie() {
  Pattern p;
  p.name = "op_art_cruz_diez_physichromie";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Grid;

  p.zn.zn[0] = {1.46f, 0.08f};
  p.zn.zn[1] = {0.085f, -0.015f};
  p.zn.zn[2] = {1.18f, -0.06f};
  p.zn.zn[3] = {0.04f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.082f;
  p.zn.zn0_anim_amp = 0.036f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.105f;
  p.zn.zn1_anim_amp = 0.058f;
  p.zn.zn1_anim_k = 2.0f;

  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 1.10f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::AtmosphericDrift);
  p.color.spatial_mode = ColorSpatialMode::HorizontalWave;
  p.color.spiral_angle_coeff = 2.8f;
  p.color.spiral_radius_coeff = 1.25f;
  p.color.chroma_breath_amp = 0.26f;
  p.color.chroma_breath_freq = 0.16f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.026f;
  p.color.hue_velocity_breath = 0.016f;
  p.color.hue_velocity_drift = 0.007f;
  p.color.luma_shape_depth = 0.20f;
  p.color.luma_shape_center = -4.20f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.070f;
  p.seed.hue = 0.08f;
  p.seed.hue_speed = 0.13f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.10f;
  p.seed.pulse_amp = 0.18f;

  p.speed = 0.88f;

  p.motion
      .add("viewer_lateral_parallax",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.075f,
                            .amplitude = 0.095f,
                            .offset = 0.04f},
           control::ModTarget::Zn3Re)
      .add("lamella_louver_pitch",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.056f,
                            .amplitude = 0.65f,
                            .offset = 2.8f},
           control::ModTarget::SpiralAngleCoeff)
      .add("chromatic_atmosphere_sweep",
           control::ModLane{.wave = control::Waveform::Sawtooth,
                            .frequency_hz = 0.048f,
                            .amplitude = 3.14159f,
                            .offset = 0.0f},
           control::ModTarget::ChromaBreathPhase)
      .add("additive_hue_induction",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.063f,
                            .amplitude = 0.009f,
                            .offset = 0.015f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
