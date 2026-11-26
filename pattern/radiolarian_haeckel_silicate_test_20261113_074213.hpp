#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Haeckel Radiolarian Opalescent Silicate Endoskeleton Test.
 *
 * Scientific foundation:
 * - Ernst Haeckel (Kunstformen der Natur, Plate 91: Spumellaria / Nassellaria)
 *   & D'Arcy Wentworth Thompson (On Growth and Form): pelagic radiolarians
 *   precipitate amorphous hydrated silica (SiO2*nH2O) along minimal-energy
 *   hexagonal/icosahedral froth interfaces, forming perforated geodesic lattice
 *   tests studded with radial axopodial spicules.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = P1 (0.5*(z^3 - 3z), Chebyshev T_3 cubic polynomial): exact 3-fold
 *   critical-point branching that doubles into 6-fold silicate pore rosettes
 *   under PareidoliaCathedral dihedral mirror folding!
 * - Color = LivingPaletteAgent (mode 21) calibrated at log_r = -4.30 for
 *   opalescent mineral-to-bioluminescent protoplasmic contrast.
 */
inline Pattern radiolarian_haeckel_silicate_test() {
  Pattern p;
  p.name = "radiolarian_haeckel_silicate_test";
  p.inner = InnerFunction::P1;
  p.reduction = ReductionType::PareidoliaCathedral;

  p.zn.zn[0] = {1.38f, 0.22f};
  p.zn.zn[1] = {0.042f, 0.018f};
  p.zn.zn[2] = {1.06f, -0.085f};
  p.zn.zn[3] = {0.0f, 0.015f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.088f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.112f;
  p.zn.zn1_anim_amp = 0.054f;
  p.zn.zn1_anim_k = 6.0f;

  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.07f;
  p.color.fade = 0.987f;
  p.color.color_mode = as_int(ColorMode::LivingPaletteAgent);
  p.color.spiral_angle_coeff = 2.4f;
  p.color.spiral_radius_coeff = 0.92f;
  p.color.chroma_breath_amp = 0.25f;
  p.color.chroma_breath_freq = 0.17f;
  p.color.chroma_breath_phase = 0.25f;
  p.color.hue_velocity_base = 0.022f;
  p.color.hue_velocity_breath = 0.014f;
  p.color.hue_velocity_drift = 0.005f;
  p.color.luma_shape_depth = 0.26f;
  p.color.luma_shape_center = -4.30f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.068f;
  p.seed.hue = 0.54f;
  p.seed.hue_speed = 0.095f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.09f;
  p.seed.pulse_amp = 0.17f;

  p.speed = 0.84f;

  p.motion
      .add("axopodial_twist",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.068f,
                            .amplitude = 0.075f,
                            .offset = -0.085f},
           control::ModTarget::Zn2Im)
      .add("pore_harmonic",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.109f,
                            .amplitude = 0.55f,
                            .offset = 2.4f,
                            .phase = 0.20f},
           control::ModTarget::SpiralAngleCoeff)
      .add("opal_shell_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.052f,
                            .amplitude = 0.08f,
                            .offset = 0.26f},
           control::ModTarget::LumaShapeDepth)
      .add("mineral_hue_precession",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.041f,
                            .amplitude = 0.008f,
                            .offset = 0.014f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
