#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Jesus Rafael Soto "Vibrations & Penetrables" Vernier Moire Dematerialization.
 *
 * Kinetic & perceptual art foundation:
 * - Jesus Rafael Soto (Vibrations, 1958; Penetrables, 1967): suspending a
 *   foreground array of fine metal/nylon rods slightly in front of a striated
 *   ground plane generates a low-spatial-frequency vernier beat (k_beat =
 *   k_front - k_back) whose apparent velocity is magnified by k_front / k_beat,
 *   visually dissolving solid matter into pure kinetic energy.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = TP2 (cos(z) + sin(z) = sqrt(2)*sin(z + pi/4)): 45-degree phase-
 *   coupled dual grating beating against the orthogonal Grid wrap domain!
 * - Color = MoireInterference (mode 12) + Cross seed rods + quadrature vernier
 *   detuning on Zn2Re / Zn2Im.
 */
inline Pattern op_art_soto_vibrant_penetrable() {
  Pattern p;
  p.name = "op_art_soto_vibrant_penetrable";
  p.inner = InnerFunction::TP2;
  p.reduction = ReductionType::Grid;

  p.zn.zn[0] = {1.38f, 0.14f};
  p.zn.zn[1] = {0.032f, -0.024f};
  p.zn.zn[2] = {1.16f, -0.09f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.096f;
  p.zn.zn0_anim_amp = 0.040f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.122f;
  p.zn.zn1_anim_amp = 0.052f;
  p.zn.zn1_anim_k = 4.0f;

  p.color.hue_rotation = 0.013f;
  p.color.sat_scale = 1.14f;
  p.color.fade = 0.984f;
  p.color.color_mode = as_int(ColorMode::MoireInterference);
  p.color.spiral_angle_coeff = 3.2f;
  p.color.spiral_radius_coeff = 1.68f;
  p.color.chroma_breath_amp = 0.27f;
  p.color.chroma_breath_freq = 0.21f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.024f;
  p.color.hue_velocity_breath = 0.015f;
  p.color.hue_velocity_drift = 0.006f;
  p.color.luma_shape_depth = 0.23f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.068f;
  p.seed.hue = 0.60f;
  p.seed.hue_speed = 0.12f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.12f;
  p.seed.pulse_amp = 0.19f;

  p.speed = 0.92f;

  p.motion
      .add("vernier_scale_detune",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.084f,
                            .amplitude = 0.065f,
                            .offset = 1.16f},
           control::ModTarget::Zn2Re)
      .add("vernier_angle_detune",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.052f,
                            .amplitude = 0.065f,
                            .offset = -0.09f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Im)
      .add("rod_harmonic_beat",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.071f,
                            .amplitude = 0.70f,
                            .offset = 3.2f},
           control::ModTarget::SpiralAngleCoeff)
      .add("penetrable_depth_shimmer",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.097f,
                            .amplitude = 0.07f,
                            .offset = 0.23f},
           control::ModTarget::LumaShapeDepth);

  return p;
}

} // namespace epi::presets
