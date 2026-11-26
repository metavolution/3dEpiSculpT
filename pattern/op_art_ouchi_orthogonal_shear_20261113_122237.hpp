#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Op-Art: Ouchi-Spillmann Illusion (Orthogonal Textures in Relative Motion).
 *
 * Scientific foundation:
 * - H. Ouchi (1977), analysed by Spillmann and by Fermueller, Pless &
 *   Aloimonos (2000): a disc of horizontally elongated checks on a ground of
 *   vertically elongated checks appears to float and slide. Elongated
 *   elements give a biased motion estimate (the aperture problem favours
 *   motion across the long edges); under the tiny random image motion of
 *   fixational eye movements the two orthogonal textures get different
 *   velocity estimates and segregate into independently moving surfaces.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Sin: Re = sin(x)*cosh(y), Im = cos(x)*sinh(y): anisotropic, so it
 *   turns the seed cross into elongated cells.
 * - Pre-affine rotated by ~90 deg: every feedback generation is drawn at
 *   right angles to the previous one. Successive generations are
 *   orthogonally elongated textures, the two Ouchi surfaces, nested.
 * - Reduction = Grid + Seed = Cross: a hard checker lattice of thin lines.
 * - Motion: two NoiseHash lanes jitter Zn3 (smoothed value noise at 0.8 Hz):
 *   an explicit model of fixational drift / microsaccades, the driver of the
 *   illusion, so the surfaces slide even on a fixated display.
 * - Color = LumaLockedHueDrift: hue is set by geometry with luminance held,
 *   so the texture contrast (the motion signal) is purely achromatic.
 */
inline Pattern op_art_ouchi_orthogonal_shear() {
  constexpr float ORTHO = 1.62079633f; // pi/2 + 0.05
  constexpr float A2 = -0.12f;
  constexpr float PRE = 1.22f;
  constexpr float POST = 1.12f;
  const float zn2_re = POST * std::cos(A2);

  Pattern p;
  p.name = "op_art_ouchi_orthogonal_shear";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {PRE * std::cos(ORTHO), PRE * std::sin(ORTHO)};
  p.zn.zn[1] = {0.04f, -0.03f};
  p.zn.zn[2] = {zn2_re, POST * std::sin(A2)};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.09f;
  p.zn.zn0_anim_amp = 0.03f;

  p.color.hue_rotation = 0.012f;
  p.color.sat_scale = 0.94f;  // mode 10 uses sat_scale as the saturation itself
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::LumaLockedHueDrift);
  p.color.spiral_angle_coeff = 2.0f;
  p.color.spiral_radius_coeff = 0.50f;
  p.color.chroma_breath_amp = 0.15f;
  p.color.chroma_breath_freq = 0.10f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.020f;
  p.color.hue_velocity_breath = 0.012f;
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.07f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.10f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.10f;
  p.seed.pulse_amp = 0.15f;
  p.speed = 0.85f;

  p.motion
      .add("check_aspect_ratio",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.05f,
                            .amplitude = 0.06f,
                            .offset = zn2_re},
           control::ModTarget::Zn2Re)
      .add("figure_ground_rings",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.07f,
                            .amplitude = 0.20f,
                            .offset = 0.50f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("fixational_drift_x",
           control::ModLane{.wave = control::Waveform::NoiseHash,
                            .frequency_hz = 0.8f,
                            .amplitude = 0.02f,
                            .offset = 0.0f,
                            .seed = 7u},
           control::ModTarget::Zn3Re)
      .add("fixational_drift_y",
           control::ModLane{.wave = control::Waveform::NoiseHash,
                            .frequency_hz = 0.8f,
                            .amplitude = 0.02f,
                            .offset = 0.0f,
                            .seed = 19u},
           control::ModTarget::Zn3Im);

  return p;
}

} // namespace epi::presets
