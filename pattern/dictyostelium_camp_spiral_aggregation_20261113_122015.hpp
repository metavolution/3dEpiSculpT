#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Dictyostelium discoideum: cAMP Relay Waves and Aggregation Territories.
 *
 * Scientific foundation:
 * - Starving social amoebae signal with cyclic AMP. Each cell that detects a
 *   pulse relays it and then turns refractory (Martiel & Goldbeter 1987), so
 *   the lawn is an excitable medium: autonomous pacemaker cells emit target
 *   waves every ~6 min, broken wavefronts curl into spirals, and cells stream
 *   up the waves into aggregation territories (Tomchik & Devreotes 1981).
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Exp: |T| = |zn2|*exp(Re w), arg T = Im w. Level sets of |T| are
 *   straight fronts and exp winds them: a log-spiral wave geometry. Exp has
 *   no critical points, so no cell-free super-attracting holes.
 * - Seed = WaveRing, sampled at the WARPED coordinate R(T(z)): wherever T(z)
 *   passes a torus lattice point (0, +/-2, +/-2i, ...) a full set of
 *   travelling rings appears. With |T| up to ~3.9 the frame holds several
 *   such centres strung along the exponential spiral: multiple pacemakers,
 *   each emitting its own target pattern, competing for territory.
 * - Seed pulse 0.10 Hz: the periodic cAMP relay.
 * - Color = Alive (relay, phosphodiesterase decay, refractory drift).
 */
inline Pattern dictyostelium_camp_spiral_aggregation() {
  constexpr float A0 = 0.95531662f;  // atan(sqrt(2)): magic angle
  constexpr float A2 = -0.73682300f;
  constexpr float PRE = 1.00f;
  constexpr float POST = 0.95f;
  const float zn2_im = POST * std::sin(A2);

  Pattern p;
  p.name = "dictyostelium_camp_spiral_aggregation";
  p.inner = InnerFunction::Exp;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {PRE * std::cos(A0), PRE * std::sin(A0)};
  p.zn.zn[1] = {0.006f, -0.004f};
  p.zn.zn[2] = {POST * std::cos(A2), zn2_im};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.12f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.20f;
  p.zn.zn1_anim_amp = 0.05f;
  p.zn.zn1_anim_k = 3.0f;

  p.color.hue_rotation = 0.010f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.987f;
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 1.0f;
  p.color.spiral_radius_coeff = 0.48f;
  p.color.chroma_breath_amp = 0.22f;
  p.color.chroma_breath_freq = 0.12f;
  p.color.chroma_breath_phase = 0.30f;
  p.color.hue_velocity_base = 0.022f;   // relay
  p.color.hue_velocity_breath = 0.014f; // phosphodiesterase decay
  p.color.hue_velocity_drift = 0.005f;  // refractory drift
  p.color.luma_shape_depth = 0.22f;
  p.color.luma_shape_center = 1.8f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.06f;
  p.seed.hue = 0.28f;
  p.seed.hue_speed = 0.07f;
  p.seed.alpha = 0.90f;
  p.seed.pulse_freq = 0.10f;
  p.seed.pulse_amp = 0.22f;
  p.speed = 0.6f;

  p.motion
      .add("camp_relay_pitch",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.06f,
                            .amplitude = 0.12f,
                            .offset = 0.48f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("aggregation_stream_twist",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.05f,
                            .amplitude = 0.06f,
                            .offset = zn2_im},
           control::ModTarget::Zn2Im)
      .add("phosphodiesterase_breath",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.085f,
                            .amplitude = 0.07f,
                            .offset = 0.22f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
