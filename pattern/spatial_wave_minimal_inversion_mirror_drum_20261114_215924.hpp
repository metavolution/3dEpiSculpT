#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimal): a Drum and its Inversion, z -> c / z.
 *
 * Mathematics:
 * - z -> c / z is an involution for every complex c: applying it twice gives
 *   z back. It exchanges the inside and the outside of the circle
 *   |z| = sqrt|c| and fixes the two points +-sqrt(c). As a map of the Riemann
 *   sphere it is a half turn about the axis through those points.
 * - The steady image is the seed plus its image under the involution. With
 *   c = 0.36 the circle of radius 0.6 is the mirror: the annulus
 *   0.36 < |z| < 0.6 and the annulus 0.6 < |z| < 1 are exchanged, each showing
 *   the other's part of the Chladni figure turned inside out. Circles and
 *   lines go to circles and lines and all angles are kept, so the nodal net
 *   stays a net. Inside |z| < 0.36 the image leaves the window and is clamped.
 * - Because every c gives an involution, c can be animated freely: its
 *   modulus moves the mirror circle and its argument turns the inverted copy.
 *
 * Pipeline: Invert, zn0 = 1, zn2 = c; Clamp; ChladniCymatics seed.
 *
 * Mechanism assertion: period 2 for every orbit that stays in the window;
 * log-stretch sums to 0 over one period.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_minimal_inversion_mirror_drum() {
  Pattern p;
  p.name = "spatial_wave_minimal_inversion_mirror_drum";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.36f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.5f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.97f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.9f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.45f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.75f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("mirror_radius",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.011f,
                            .amplitude = 0.06f,
                            .offset = 0.36f},
           control::ModTarget::Zn2Re)
      .add("mirror_turn",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.007f,
                            .amplitude = 0.05f,
                            .offset = 0.0f},
           control::ModTarget::Zn2Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.025f,
                            .amplitude = 0.03f,
                            .offset = 0.1f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
