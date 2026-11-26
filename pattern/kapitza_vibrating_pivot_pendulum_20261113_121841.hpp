#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Kapitza Pendulum: Dynamic Stabilization of the Inverted State.
 *
 * Scientific foundation:
 * - P. L. Kapitza (1951): a rigid pendulum whose pivot vibrates vertically at
 *   high frequency feels the effective potential
 *   V_eff = -m*g*l*cos(theta) + (m*a^2*w^2/4)*sin^2(theta).
 *   For a^2*w^2 > 2*g*l the upside-down position theta = pi becomes a stable
 *   minimum. The same averaging explains the Paul trap and ponderomotive
 *   forces in plasmas.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Sin: the pendulum torque sin(theta).
 * - Post-affine rotated by almost pi: the feedback loop works around the
 *   INVERTED configuration, the state Kapitza's drive stabilizes.
 * - Motion lane "pivot_vibration" drives Zn2Re (the loop gain = effective
 *   gravity) at 0.9 Hz, an order of magnitude faster than every other lane:
 *   the fast drive on top of slow dynamics that the averaging method needs.
 * - Color = LogPolarColorSpiral: hue is SET (not accumulated) from screen
 *   angle and log|T|, so colours are the level sets of the map: phase-portrait
 *   rings and separatrix sectors. A small positive luma term (luma_d against a
 *   far-negative centre) gives a constant glow floor ~ luma_d/(1-fade), so
 *   contracting pockets never sink to black.
 */
inline Pattern kapitza_vibrating_pivot_pendulum() {
  constexpr float INVERTED = 3.00159265f; // pi - 0.14: just off the upright
  constexpr float POST = 1.22f;
  const float zn2_re = POST * std::cos(INVERTED);

  Pattern p;
  p.name = "kapitza_vibrating_pivot_pendulum";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.42f, 0.10f};
  p.zn.zn[1] = {0.02f, -0.012f};
  p.zn.zn[2] = {zn2_re, POST * std::sin(INVERTED)};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.13f;
  p.zn.zn1_anim_amp = 0.04f;
  p.zn.zn1_anim_k = 2.0f;

  p.color.hue_rotation = 0.012f;
  p.color.sat_scale = 1.10f;
  p.color.fade = 0.985f;
  p.color.color_mode = as_int(ColorMode::LogPolarColorSpiral);
  p.color.spiral_angle_coeff = 2.0f;
  p.color.spiral_radius_coeff = 1.2f;
  p.color.chroma_breath_amp = 0.22f;
  p.color.chroma_breath_freq = 0.17f;
  p.color.chroma_breath_phase = 6.2831853f;
  p.color.hue_velocity_base = 0.020f;
  p.color.hue_velocity_breath = 0.012f;
  p.color.hue_velocity_drift = 0.005f;
  p.color.luma_shape_depth = 0.008f;   // glow floor ~ 0.008/0.015 ~ 0.5
  p.color.luma_shape_center = -4.15f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.08f;
  p.seed.hue = 0.10f;
  p.seed.hue_speed = 0.08f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.11f;
  p.seed.pulse_amp = 0.18f;
  p.speed = 0.8f;

  p.motion
      .add("pivot_vibration",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.9f,
                            .amplitude = 0.05f,
                            .offset = zn2_re},
           control::ModTarget::Zn2Re)
      .add("effective_potential_rings",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.047f,
                            .amplitude = 0.35f,
                            .offset = 1.2f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("ponderomotive_chroma",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.095f,
                            .amplitude = 0.07f,
                            .offset = 0.22f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
