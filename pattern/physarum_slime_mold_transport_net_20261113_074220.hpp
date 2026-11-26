#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Physarum polycephalum Peristaltic Shuttle-Streaming Transport Network.
 *
 * Scientific foundation:
 * - Nakagaki, Yamada & Toth (2000, Nature 407) & Tero et al. (2010, Science
 *   327): the acellular plasmodial slime mold Physarum polycephalum solves
 *   shortest-path and fault-tolerant Steiner-network problems via actomyosin
 *   peristaltic shuttle streaming, thickening high-flux protoplasmic veins
 *   (dD/dt = |Q|^mu - gamma*D) while pruning dead-end capillaries.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = F11 (sin(z^2)): hyperbolic-parabolic level curves sin(x^2-y^2 +
 *   2ixy) weave anastomosing protoplasmic tubes and bifurcating vascular hubs.
 * - Color = Alive (mode 3) calibrated at log_r = -4.25 so protoplasmic veins
 *   glow amber-gold/chartreuse against deep forest substrate.
 */
inline Pattern physarum_slime_mold_transport_net() {
  Pattern p;
  p.name = "physarum_slime_mold_transport_net";
  p.inner = InnerFunction::F11;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.32f, 0.27f};
  p.zn.zn[1] = {0.045f, -0.038f};
  p.zn.zn[2] = {1.15f, -0.16f};
  p.zn.zn[3] = {-0.022f, 0.018f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.098f;
  p.zn.zn0_anim_amp = 0.046f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.128f;
  p.zn.zn1_anim_amp = 0.058f;
  p.zn.zn1_anim_k = 3.0f;

  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.07f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 1.75f;
  p.color.spiral_radius_coeff = 1.12f;
  p.color.chroma_breath_amp = 0.26f;
  p.color.chroma_breath_freq = 0.16f;
  p.color.chroma_breath_phase = 0.22f;
  p.color.hue_velocity_base = 0.021f;
  p.color.hue_velocity_breath = 0.015f;
  p.color.hue_velocity_drift = 0.006f;
  p.color.luma_shape_depth = 0.26f;
  p.color.luma_shape_center = -4.25f;

  p.seed.type = SeedType::OrganicNoise;
  p.seed.border_width = 0.074f;
  p.seed.hue = 0.16f;
  p.seed.hue_speed = 0.10f;
  p.seed.alpha = 0.93f;
  p.seed.pulse_freq = 0.13f;
  p.seed.pulse_amp = 0.21f;

  p.speed = 0.90f;

  p.motion
      .add("shuttle_streaming_amp",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.11f,
                            .amplitude = 0.09f,
                            .offset = 0.26f},
           control::ModTarget::ChromaBreathAmp)
      .add("peristaltic_vein_pitch",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.068f,
                            .amplitude = 0.34f,
                            .offset = 1.12f,
                            .phase = 0.25f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chemotactic_forage_re",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.053f,
                            .amplitude = 0.048f,
                            .offset = -0.022f},
           control::ModTarget::Zn3Re)
      .add("protoplasmic_hue_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.044f,
                            .amplitude = 0.008f,
                            .offset = 0.014f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
