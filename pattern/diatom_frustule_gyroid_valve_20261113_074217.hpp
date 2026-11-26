#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Coscinodiscus Diatom Biosilica Frustule Photonic Sieve Valve.
 *
 * Scientific foundation:
 * - Sumper (2002, Science 295) & Noyes et al. (2008): centric diatom frustules
 *   self-assemble hierarchical 2D photonic-crystal silica valves (foramen ->
 *   cribellum -> cribrilarium) via phase separation of polyamine droplets,
 *   functioning simultaneously as mechanical armor and wavelength-selective
 *   diffraction concentrators for chloroplasts.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = F13 (0.7 * sinh(z) * cos(z)): orthogonal hyperbolic-trigonometric
 *   product lattice generating nested multi-scale pore chambers.
 * - Reduction = AmmannBeenker (8-fold octagonal quasicrystal reflection):
 *   reproduces quasiperiodic centric-valve pore packing!
 * - Color = SpectralPrism (mode 20): thin-film photonic Bragg dispersion.
 */
inline Pattern diatom_frustule_gyroid_valve() {
  Pattern p;
  p.name = "diatom_frustule_gyroid_valve";
  p.inner = InnerFunction::F13;
  p.reduction = ReductionType::AmmannBeenker;

  p.zn.zn[0] = {1.52f, 0.32f};
  p.zn.zn[1] = {0.032f, -0.024f};
  p.zn.zn[2] = {1.32f, -0.16f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.092f;
  p.zn.zn0_anim_amp = 0.044f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.118f;
  p.zn.zn1_anim_amp = 0.048f;
  p.zn.zn1_anim_k = 8.0f;

  p.color.hue_rotation = 0.016f;
  p.color.sat_scale = 1.82f;
  p.color.fade = 0.989f;
  p.color.color_mode = as_int(ColorMode::SpectralPrism);
  p.color.spiral_angle_coeff = 2.6f;
  p.color.spiral_radius_coeff = 1.38f;
  p.color.chroma_breath_amp = 0.26f;
  p.color.chroma_breath_freq = 0.18f;
  p.color.chroma_breath_phase = 0.10f;
  p.color.hue_velocity_base = 0.00f;   // R cosine phase
  p.color.hue_velocity_breath = 0.33f; // G cosine phase (120 deg)
  p.color.hue_velocity_drift = 0.67f;  // B cosine phase (240 deg)
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.088f;
  p.seed.hue = 0.42f;
  p.seed.hue_speed = 0.12f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.10f;
  p.seed.pulse_amp = 0.18f;

  p.speed = 0.86f;

  p.motion
      .add("bragg_diffraction_order",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.079f,
                            .amplitude = 0.65f,
                            .offset = 2.6f},
           control::ModTarget::SpiralAngleCoeff)
      .add("cribellum_aperture",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.049f,
                            .amplitude = 0.068f,
                            .offset = 1.32f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Re)
      .add("iridescent_dispersion",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.096f,
                            .amplitude = 0.36f,
                            .offset = 1.38f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chloroplast_sat_breath",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.064f,
                            .amplitude = 0.10f,
                            .offset = 1.82f},
           control::ModTarget::SatScale);

  return p;
}

} // namespace epi::presets
