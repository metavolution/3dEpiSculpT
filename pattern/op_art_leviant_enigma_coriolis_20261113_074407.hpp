#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Isia Leviant "Enigma" V5/MT Cortical Rotary Illusion & Coriolis Rings.
 *
 * Perceptual neuroscience foundation:
 * - Isia Leviant (Enigma, 1981) & Zeki, Watson & Frackowiak (1993, Proc. R.
 *   Soc. Lond. B 252): high-frequency radial spokes Orthogonal to concentric
 *   saturated chromatic annuli selectively activate human extrastriate motion
 *   complex V5/MT without physical retinal motion, inducing rapid spontaneous
 *   bidirectional ring circulation.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = P3 (z^3 / (1 + |z|^2)): smooth bounded rational 3-fold/6-fold spoke
 *   cascade whose denominator prevents blowup while preserving crisp angular
 *   phase spokes across concentric WaveRing annuli!
 * - Color = ChromaBreathing (mode 9) calibrated at Jensen-neutral sat_scale =
 *   1.032 with multi-parameter annular/spoke pitch oscillation.
 */
inline Pattern op_art_leviant_enigma_coriolis() {
  Pattern p;
  p.name = "op_art_leviant_enigma_coriolis";
  p.inner = InnerFunction::P3;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.94f, 0.36f};
  p.zn.zn[1] = {0.38f, -0.26f};
  p.zn.zn[2] = {1.88f, -0.24f};
  p.zn.zn[3] = {0.52f, -0.38f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.108f;
  p.zn.zn0_anim_amp = 0.048f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.138f;
  p.zn.zn1_anim_amp = 0.056f;
  p.zn.zn1_anim_k = 6.0f;

  p.color.hue_rotation = 0.018f;
  p.color.sat_scale = 1.032f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::ChromaBreathing);
  p.color.spiral_angle_coeff = 3.0f;
  p.color.spiral_radius_coeff = 2.15f;
  p.color.chroma_breath_amp = 0.22f;
  p.color.chroma_breath_freq = 0.26f;
  p.color.chroma_breath_phase = 0.12f;
  p.color.hue_velocity_base = 0.030f;
  p.color.hue_velocity_breath = 0.018f;
  p.color.hue_velocity_drift = 0.007f;
  p.color.luma_shape_depth = 0.21f;
  p.color.luma_shape_center = -4.18f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.078f;
  p.seed.hue = 0.82f;
  p.seed.hue_speed = 0.15f;
  p.seed.alpha = 0.94f;
  p.seed.pulse_freq = 0.16f;
  p.seed.pulse_amp = 0.21f;

  p.speed = 0.94f;

  p.motion
      .add("coriolis_ring_spacing",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.089f,
                            .amplitude = 0.55f,
                            .offset = 2.15f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("v5_mt_counter_torque",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.055f,
                            .amplitude = 0.11f,
                            .offset = -0.19f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Im)
      .add("spoke_harmonic_wave",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.072f,
                            .amplitude = 0.65f,
                            .offset = 3.0f},
           control::ModTarget::SpiralAngleCoeff)
      .add("annular_hue_velocity",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.061f,
                            .amplitude = 0.014f,
                            .offset = 0.030f},
           control::ModTarget::HueVelocityBase);

  return p;
}

} // namespace epi::presets
