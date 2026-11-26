#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Cephalopod Passing-Cloud Chromatophore & Reflectin Iridophore Camouflage.
 *
 * Scientific foundation:
 * - Messenger (2001, Biol. Rev.) & Reiter et al. (2018, Nature 562): cuttlefish
 *   (Sepia officinalis) generate rapid "passing cloud" skin waves via central
 *   pattern generators driving radial muscles around pigment-filled
 *   chromatophore saccules, overlaid on acetylcholine-tunable Bragg stacks of
 *   reflectin protein platelets (iridophores).
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = TP1 (cos(z) - tanh(z)): asymmetric neuromuscular contraction wave
 *   pairing smooth saccule dilation (tanh) with periodic band spacing (cos).
 * - Color = Opponent (mode 6, YCoCg perceptual opponent space): models the
 *   physiological opposition between warm ommochrome chromatophores and cool
 *   structural cyan/green iridophores.
 */
inline Pattern cephalopod_chromatophore_iridophore() {
  Pattern p;
  p.name = "cephalopod_chromatophore_iridophore";
  p.inner = InnerFunction::TP1;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.24f, 0.21f};
  p.zn.zn[1] = {0.052f, -0.034f};
  p.zn.zn[2] = {1.12f, -0.14f};
  p.zn.zn[3] = {0.015f, 0.025f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.115f;
  p.zn.zn0_anim_amp = 0.050f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.145f;
  p.zn.zn1_anim_amp = 0.068f;
  p.zn.zn1_anim_k = 3.0f;

  p.color.hue_rotation = 0.022f;
  p.color.sat_scale = 1.11f;
  p.color.fade = 0.985f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 1.95f;
  p.color.spiral_radius_coeff = 1.18f;
  p.color.chroma_breath_amp = 0.28f;
  p.color.chroma_breath_freq = 0.24f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.032f;
  p.color.hue_velocity_breath = 0.020f;
  p.color.hue_velocity_drift = 0.007f;
  p.color.luma_shape_depth = 0.21f;
  p.color.luma_shape_center = -4.18f;

  p.seed.type = SeedType::OrganicNoise;
  p.seed.border_width = 0.075f;
  p.seed.hue = 0.12f;
  p.seed.hue_speed = 0.18f;
  p.seed.alpha = 0.93f;
  p.seed.pulse_freq = 0.19f;
  p.seed.pulse_amp = 0.23f;

  p.speed = 0.96f;

  p.motion
      .add("passing_cloud_phase",
           control::ModLane{.wave = control::Waveform::Sawtooth,
                            .frequency_hz = 0.14f,
                            .amplitude = 3.14159f,
                            .offset = 0.0f},
           control::ModTarget::ChromaBreathPhase)
      .add("saccule_expansion",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.095f,
                            .amplitude = 0.48f,
                            .offset = 1.95f},
           control::ModTarget::SpiralAngleCoeff)
      .add("reflectin_shear",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.068f,
                            .amplitude = 0.085f,
                            .offset = -0.14f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Im)
      .add("mantle_luma_wave",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.081f,
                            .amplitude = 0.07f,
                            .offset = 0.21f},
           control::ModTarget::LumaShapeDepth);

  return p;
}

} // namespace epi::presets
