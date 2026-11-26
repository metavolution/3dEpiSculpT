#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: Rigid Rotation of the Riemann Sphere, seen in the plane.
 *
 * Mathematics:
 * A Mobius map is written here as z -> zn2 / (zn0 z + zn1) + zn3 with the
 * Invert inner; any values of the four numbers give a Mobius map.
 * - An elliptic Mobius map with antipodal fixed points p and -1/conj(p) is
 *   the stereographic image of a rigid rotation of the sphere about the axis
 *   through them. Here p = 0.25, q = -4 and the angle is the golden angle
 *   (137.5 deg). The invariant curves are the circles of Apollonius of p and
 *   q: non-concentric circles around p, the image of the sphere's parallels.
 * - (z' - p)/(z' - q) = k (z - p)/(z - q), k = e^(i theta), gives
 *   zn0 = 1 - k, zn1 = k p - q, zn3 = (p - q k)/(1 - k), zn2 = (1 - k) - zn3 zn1.
 *   The pole lies on the line x = -1.875, outside the window.
 * - The map is neutral: nothing is stretched or forgotten except by fade, so
 *   the image is the seed averaged along the golden rotation, the most evenly
 *   spread rotation there is.
 *
 * Pipeline: Invert; Clamp; Cross seed centred at 0, off the axis, so each
 * generation draws the two lines as a pair of circles on the turning sphere.
 * Parallels that leave the window on the left are clamped there. A small lane
 * on zn1 tips the map slightly to either side of neutral.
 *
 * Mechanism assertion: neutral orbit fraction dominant; log-stretch averages
 * to 0 along every closed parallel.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_riemann_sphere_golden_rotation() {
  Pattern p;
  p.name = "spatial_wave_riemann_sphere_golden_rotation";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.7374f, -0.6755f};
  p.zn.zn[1] = {3.8157f, 0.1689f};
  p.zn.zn[2] = {9.0312f, -3.5113f};
  p.zn.zn[3] = {-1.875f, 0.8262f};

  p.color.hue_rotation = 0.055f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.965f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.9f;
  p.color.chroma_breath_amp = 0.12f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.12f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.85f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("neutrality_breath",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.011f,
                            .amplitude = 0.012f,
                            .offset = 0.1689f},
           control::ModTarget::Zn1Im)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.019f,
                            .amplitude = 0.02f,
                            .offset = 0.055f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.027f,
                            .amplitude = 0.04f,
                            .offset = 0.12f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
