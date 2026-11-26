#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimal): the Anharmonic Map z -> 1 / (1 - z).
 *
 * Mathematics:
 * - z -> 1 / (1 - z) sends 0 -> 1 -> infinity -> 0. It is a Mobius map of
 *   order 3, and with z -> 1 - z it generates the six-element anharmonic
 *   group, the symmetries of the cross-ratio. Its fixed points are the two
 *   primitive sixth roots of unity, e^(+-i pi / 3); it is a third of a turn
 *   of the Riemann sphere about the axis through them.
 * - Being of order 3 and neutral, it makes the steady image the seed
 *   averaged over three copies: S(z), S(1 / (1 - z)) and S((z - 1) / z).
 *   Straight lines become circles through 1 and through 0.
 * - The four numbers of the map are -1, 1, 1, 0. The points 1 and 0 are sent
 *   to infinity after one and two steps; near them the image leaves the
 *   window and is clamped, which marks the two poles.
 * - A small lane on zn1 detunes the order-3 lock: the three copies drift
 *   apart into a slow rotation and close up again.
 *
 * Pipeline: Invert, zn0 = -1, zn1 = 1, zn2 = 1, zn3 = 0; Clamp; Cross seed.
 *
 * Mechanism assertion: neutral orbit fraction dominant; period 3 for orbits
 * that stay in the window.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_minimal_anharmonic_triality() {
  Pattern p;
  p.name = "spatial_wave_minimal_anharmonic_triality";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {-1.0f, 0.0f};
  p.zn.zn[1] = {1.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.3333f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.975f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.8f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.56f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.85f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("triality_detune",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.012f,
                            .amplitude = 0.01f,
                            .offset = 0.0f},
           control::ModTarget::Zn1Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.026f,
                            .amplitude = 0.04f,
                            .offset = 0.1f},
           control::ModTarget::ChromaBreathAmp)
      .add("saturation_breath",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.015f,
                            .amplitude = 0.07f,
                            .offset = 1.0f},
           control::ModTarget::SatScale);

  return p;
}

} // namespace epi::presets
