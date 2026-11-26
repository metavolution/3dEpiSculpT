#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Saltatory Conduction: Action Potentials Jumping Between Nodes of Ranvier.
 *
 * Scientific foundation:
 * - Huxley & Staempfli (1949): in a myelinated axon the membrane is insulated
 *   except at periodic ~1 um gaps, the nodes of Ranvier, where sodium channels
 *   cluster. Under the myelin the signal spreads passively following the
 *   cable equation, lambda^2 * V'' = V, whose solutions are sinh and cosh of
 *   x/lambda; each node regenerates the full spike. The impulse "jumps" node
 *   to node at up to ~100 m/s.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Sinh: the passive cable solution along an internode.
 * - Reduction = Grid: a hard periodic wrap. Each tile is one internode; the
 *   discontinuous tile edges are the nodes where the signal is rebuilt.
 * - Seed = Cross: energy injected only on the two thin axis lines of every
 *   tile, exp(-20*|x|), a spatially sharp, node-like source. sinh(0) = 0
 *   places the axon centre exactly on it.
 * - Seed pulse at 0.30 Hz with strong amplitude: a spike train, with a
 *   refractory modulation lane on the pulse amplitude.
 * - Color = ChromaBreathing: hue is a static function of angle and log|T|
 *   (membrane potential map); only saturation breathes: depolarization.
 */
inline Pattern saltatory_ranvier_node_conduction() {
  constexpr float A0 = 0.17453293f;  // 10 deg
  constexpr float A2 = -0.27925268f; // -16 deg
  constexpr float PRE = 1.30f;
  constexpr float POST = 1.04f;
  const float zn2_re = POST * std::cos(A2);

  Pattern p;
  p.name = "saltatory_ranvier_node_conduction";
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {PRE * std::cos(A0), PRE * std::sin(A0)};
  p.zn.zn[1] = {0.05f, -0.03f};
  p.zn.zn[2] = {zn2_re, POST * std::sin(A2)};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.13f;
  p.zn.zn0_anim_amp = 0.04f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.17f;
  p.zn.zn1_anim_amp = 0.04f;
  p.zn.zn1_anim_k = 2.0f;

  p.color.hue_rotation = 0.012f;
  p.color.sat_scale = 1.03f;
  p.color.fade = 0.985f;
  p.color.color_mode = as_int(ColorMode::ChromaBreathing);
  p.color.spiral_angle_coeff = 2.0f;
  p.color.spiral_radius_coeff = 0.60f;
  p.color.chroma_breath_amp = 0.24f;
  p.color.chroma_breath_freq = 0.25f;
  p.color.chroma_breath_phase = 6.2831853f;
  p.color.hue_velocity_base = 0.02f;
  p.color.hue_velocity_breath = 0.01f;
  p.color.hue_velocity_drift = 0.002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.07f;
  p.seed.hue = 0.60f;
  p.seed.hue_speed = 0.10f;
  p.seed.alpha = 0.90f;
  p.seed.pulse_freq = 0.30f;
  p.seed.pulse_amp = 0.30f;
  p.speed = 0.85f;

  p.motion
      .add("membrane_potential_rings",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.07f,
                            .amplitude = 0.20f,
                            .offset = 0.60f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("internode_length",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.05f,
                            .amplitude = 0.05f,
                            .offset = zn2_re},
           control::ModTarget::Zn2Re)
      .add("sodium_channel_chroma",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.09f,
                            .amplitude = 0.08f,
                            .offset = 0.24f},
           control::ModTarget::ChromaBreathAmp)
      .add("refractory_spike_gain",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.06f,
                            .amplitude = 0.10f,
                            .offset = 0.30f},
           control::ModTarget::SeedPulseAmp);

  return p;
}

} // namespace epi::presets
