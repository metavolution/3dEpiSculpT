#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Winfree-Panfilov Cardiac Scroll-Wave Rotor & Phase-Singularity Breakup.
 *
 * Scientific foundation:
 * - Arthur T. Winfree (1987, When Time Breaks Down) & A. V. Panfilov: in
 *   excitable cardiac myocardium, re-entrant action-potential waves organize
 *   around topological phase singularities (scroll-wave filaments) where
 *   depolarization and refractory recovery meet. Restitution steepness > 1
 *   causes flower-like cycloidal core meander and daughter-rotor shedding.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = F1 (0.8*(sin(z) + z^2)): couples quadratic branch-point rotor
 *   nucleation (z^2) with periodic wave-train propagation (sin(z)).
 * - Color = NeuralSynapse (mode 19): sharp voltage-gated depolarization spikes
 *   against refractory diastolic troughs.
 * - Multi-parameter motion: cycloidal rotor-core meander on Zn3 + restitution
 *   oscillation on ChromaBreathFreq and SpiralRadiusCoeff.
 */
inline Pattern cardiac_scroll_wave_fibrillation() {
  Pattern p;
  p.name = "cardiac_scroll_wave_fibrillation";
  p.inner = InnerFunction::F1;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.22f, 0.36f};
  p.zn.zn[1] = {0.065f, -0.045f};
  p.zn.zn[2] = {1.10f, -0.19f};
  p.zn.zn[3] = {0.025f, -0.020f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.135f;
  p.zn.zn0_anim_amp = 0.052f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.165f;
  p.zn.zn1_anim_amp = 0.075f;
  p.zn.zn1_anim_k = 5.0f;

  p.color.hue_rotation = 0.020f;
  p.color.sat_scale = 1.14f;
  p.color.fade = 0.983f;
  p.color.color_mode = as_int(ColorMode::NeuralSynapse);
  p.color.spiral_angle_coeff = 2.0f;
  p.color.spiral_radius_coeff = 1.48f;
  p.color.chroma_breath_amp = 0.32f;
  p.color.chroma_breath_freq = 0.28f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.034f;
  p.color.hue_velocity_breath = 0.022f;
  p.color.hue_velocity_drift = 0.008f;
  p.color.luma_shape_depth = 0.24f;
  p.color.luma_shape_center = -4.10f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.076f;
  p.seed.hue = 0.94f;
  p.seed.hue_speed = 0.16f;
  p.seed.alpha = 0.93f;
  p.seed.pulse_freq = 0.24f;
  p.seed.pulse_amp = 0.26f;

  p.speed = 1.04f;

  p.motion
      .add("rotor_meander_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.13f,
                            .amplitude = 0.055f,
                            .offset = 0.025f},
           control::ModTarget::Zn3Re)
      .add("rotor_meander_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.21f,
                            .amplitude = 0.055f,
                            .offset = -0.020f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Im)
      .add("restitution_dispersion",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.088f,
                            .amplitude = 0.42f,
                            .offset = 1.48f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("pacemaker_refractory_freq",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.062f,
                            .amplitude = 0.09f,
                            .offset = 0.28f},
           control::ModTarget::ChromaBreathFreq);

  return p;
}

} // namespace epi::presets
