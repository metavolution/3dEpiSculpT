#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Hermann-Lingelbach Scintillating Quasilattice Lateral-Inhibition Grid.
 *
 * Retinal neurophysiology & op-art foundation:
 * - Ludimar Hermann (1870) & Schrauf, Lingelbach & Wist (1997, Vision Research
 *   37, "The Scintillating Grid Illusion"): when bright bars intersect at
 *   multi-valent nodes, concentric ON-center / OFF-surround retinal ganglion
 *   receptive fields (Difference-of-Gaussians Mexican-hat filter) receive four-
 *   or eight-way surround suppression vs. two-way along bar segments, causing
 *   transient dark/luminous sparks to scintillate across peripheral vertices.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = TP3 (sin(z) * tan(z)) + AmmannBeenker 8-fold octagonal reflection:
 *   sharp meromorphic pole lines cross at 45/90-degree octagonal star vertices!
 * - Color = QuasiPeriodicWave (mode 14): golden-ratio phi incommensurate wave
 *   superposition making peripheral vertex sparks dance non-periodically.
 */
inline Pattern op_art_scintillating_hermann_quasilattice() {
  Pattern p;
  p.name = "op_art_scintillating_hermann_quasilattice";
  p.inner = InnerFunction::TP3;
  p.reduction = ReductionType::AmmannBeenker;

  p.zn.zn[0] = {1.12f, 0.18f};
  p.zn.zn[1] = {0.026f, -0.019f};
  p.zn.zn[2] = {0.94f, -0.11f};
  p.zn.zn[3] = {0.015f, -0.012f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.094f;
  p.zn.zn0_anim_amp = 0.038f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.124f;
  p.zn.zn1_anim_amp = 0.046f;
  p.zn.zn1_anim_k = 8.0f;

  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 1.11f;
  p.color.fade = 0.984f;
  p.color.color_mode = as_int(ColorMode::QuasiPeriodicWave);
  p.color.spiral_angle_coeff = 3.236f; // 2 * phi
  p.color.spiral_radius_coeff = 1.618f; // phi
  p.color.chroma_breath_amp = 0.27f;
  p.color.chroma_breath_freq = 0.23f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.027f;
  p.color.hue_velocity_breath = 0.017f;
  p.color.hue_velocity_drift = 0.006f;
  p.color.luma_shape_depth = 0.24f;
  p.color.luma_shape_center = -4.16f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.070f;
  p.seed.hue = 0.58f;
  p.seed.hue_speed = 0.14f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.15f;
  p.seed.pulse_amp = 0.20f;

  p.speed = 0.90f;

  p.motion
      .add("ganglion_surround_pitch",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.086f,
                            .amplitude = 0.68f,
                            .offset = 3.236f},
           control::ModTarget::SpiralAngleCoeff)
      .add("quasilattice_phason_shift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.053f,
                            .amplitude = 0.042f,
                            .offset = 0.015f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Re)
      .add("scintillation_radial_wave",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.074f,
                            .amplitude = 0.38f,
                            .offset = 1.618f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("vertex_spark_contrast",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.112f,
                            .amplitude = 0.075f,
                            .offset = 0.24f},
           control::ModTarget::LumaShapeDepth);

  return p;
}

} // namespace epi::presets
