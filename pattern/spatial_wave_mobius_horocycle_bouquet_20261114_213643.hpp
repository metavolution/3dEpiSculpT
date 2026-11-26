#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: Parabolic Mobius Map, the Horocycle Bouquet.
 *
 * Mathematics:
 * A Mobius map is written here as z -> zn2 / (zn0 z + zn1) + zn3 with the
 * Invert inner; any values of the four numbers give a Mobius map.
 * - M(z) = z / (1 + e z) has a single fixed point, 0, with multiplier 1: the
 *   parabolic class. Every orbit converges to 0, but only like 1 / (e n), and
 *   it travels along a circle through 0 tangent to one fixed direction. Those
 *   circles are the horocycles of the fixed point; together they form a
 *   bouquet of mutually tangent circles.
 * - Written out: zn0 = e, zn1 = 1, zn2 = -1/e, zn3 = 1/e, e = 0.35 e^(0.6 i).
 *   The pole -1/e lies at distance 2.86, outside the window.
 * - Parabolic maps are the boundary between the elliptic and the loxodromic
 *   class. zn1 = 1 + d splits the fixed point into two, sqrt(d)/e apart. The
 *   lanes move d round a circle of radius 0.004, so the map passes through
 *   rotating and spiralling neighbours of the parabolic one.
 *
 * Pipeline: Invert; Clamp; ChladniCymatics seed filling the disc. Algebraic
 * convergence means long transits, so retention is high: nodal figures are
 * drawn out along the tangent circles.
 *
 * Mechanism assertion: no expanding class; orbit distance to 0 decays like
 * 1/n, not geometrically.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_mobius_horocycle_bouquet() {
  Pattern p;
  p.name = "spatial_wave_mobius_horocycle_bouquet";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.2889f, 0.1976f};
  p.zn.zn[1] = {1.0f, 0.0f};
  p.zn.zn[2] = {-2.3581f, 1.6132f};
  p.zn.zn[3] = {2.3581f, -1.6132f};

  p.color.hue_rotation = 0.03f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.975f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.35f;
  p.color.spiral_radius_coeff = 0.7f;
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
  p.seed.hue = 0.52f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.7f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("fixed_point_split_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.017f,
                            .amplitude = 0.004f,
                            .offset = 1.0f},
           control::ModTarget::Zn1Re)
      .add("fixed_point_split_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.017f,
                            .amplitude = 0.004f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn1Im)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.014f,
                            .amplitude = 0.012f,
                            .offset = 0.03f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
