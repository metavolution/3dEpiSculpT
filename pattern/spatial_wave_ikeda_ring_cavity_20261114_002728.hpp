#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Advanced): Ikeda Ring Cavity, a Holomorphic Round-Trip Map.
 *
 * Scientific foundation:
 * - Ikeda (1979) modelled a ring cavity holding a nonlinear dielectric: each
 *   round trip the field obeys E' = A + B E exp(i phi(E)), an injected pump A,
 *   a mirror loss/detuning B and an intensity-dependent phase. The map shows
 *   optical bistability, period doubling and the well-known spiral attractor.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = FluidVortex, w exp(i sin w). With the affine slots this is
 *       T(z) = zn3 + zn2 * w * exp(i sin w),   w = zn0 z,
 *   the Ikeda form with A = zn3, B = zn0 * zn2 and phase phi = sin w.
 * - Honest difference: Ikeda's phase depends on |E|^2 and is not analytic.
 *   The pipeline only has holomorphic inners, so phi = sin(w) is the analytic
 *   stand-in. The round-trip structure (pump + rotated, phase-kicked field)
 *   is the same; the attractor is not.
 * - What is drawn: the feedback loop composes the round trip once per frame.
 *   A pixel takes the colour of the seed frame it reaches after n round
 *   trips, so the image is the lamination of initial fields by exit time,
 *   i.e. the stable foliation of the cavity map, not its attractor.
 * - |exp(i sin w)| = exp(-cos x sinh y): gain above the axis on one side of
 *   each period and loss on the other, which is what bends the laminae.
 * - Color = SpatialWave; the horizontal waves are summed along each orbit.
 * - Motion: pump (Zn3Re), cavity detuning (Zn2Im) and wave pitch.
 */
inline Pattern spatial_wave_ikeda_ring_cavity() {
  constexpr float DETUNE_IM = 0.3583f; // 0.92 * sin(0.4)
  constexpr float PUMP = 0.18f;

  Pattern p;
  p.name = "spatial_wave_ikeda_ring_cavity";
  p.inner = InnerFunction::FluidVortex;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.25f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.8474f, DETUNE_IM}; // 0.92 * e^{0.4 i}: loss and detuning
  p.zn.zn[3] = {PUMP, 0.0f};         // injected field

  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.10f;
  p.color.fade = 0.987f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.35f;
  p.color.spiral_radius_coeff = 1.5f;
  p.color.chroma_breath_amp = 0.15f;
  p.color.chroma_breath_freq = 2.5f;
  p.color.chroma_breath_phase = 0.09f;
  p.color.hue_velocity_base = 0.018f;
  p.color.hue_velocity_breath = -0.011f;
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.12f;
  p.color.luma_shape_center = 2.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.09f;
  p.seed.hue = 0.47f;
  p.seed.hue_speed = 0.08f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.07f;
  p.seed.pulse_amp = 0.12f;
  p.speed = 0.9f;

  p.motion
      .add("pump_field",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.041f,
                            .amplitude = 0.10f,
                            .offset = PUMP},
           control::ModTarget::Zn3Re)
      .add("cavity_detuning",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.057f,
                            .amplitude = 0.08f,
                            .offset = DETUNE_IM},
           control::ModTarget::Zn2Im)
      .add("wave_pitch",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.033f,
                            .amplitude = 0.4f,
                            .offset = 1.5f},
           control::ModTarget::SpiralRadiusCoeff);

  return p;
}

} // namespace epi::presets
