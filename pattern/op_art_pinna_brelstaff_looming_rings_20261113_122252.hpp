#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Op-Art: Pinna-Brelstaff Illusion (Counter-Rotating Rings under Looming).
 *
 * Scientific foundation:
 * - B. Pinna & G. Brelstaff (2000): two concentric rings of small tilted
 *   elements appear to rotate in opposite directions when the observer moves
 *   toward or away from the page. Radial expansion stimulates local motion
 *   detectors through small oriented apertures; each detector only reports
 *   the motion component perpendicular to its element's edges, so opposite
 *   tilts turn pure looming into opposite apparent rotations.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - zn0 animation (scale orbit, the largest authored amplitude here): real
 *   looming. The physical trigger of the illusion is performed by the
 *   pre-affine zoom, so the rings swim without the viewer moving.
 * - Inner = SymmetryBreak (z + 0.1 where arg z > 0, else z): a piecewise
 *   shear. The upper half-plane is displaced sideways each pass and the
 *   lower half is not, so the two halves accumulate opposite relative tilt:
 *   two families of oppositely slanted micro-elements.
 * - Near-unit affine gain (1.03, small counter-rotations): generations nest
 *   as slowly shrinking, slightly turned frames: the rings.
 * - Color = AtmosphericDrift: luma-preserving hue ramp, keeping the
 *   luminance-defined element edges crisp for the motion system.
 */
inline Pattern op_art_pinna_brelstaff_looming_rings() {
  constexpr float A0 = 0.10471976f;  // 6 deg
  constexpr float A2 = -0.03490659f; // -2 deg
  constexpr float PRE = 1.03f;
  constexpr float POST = 1.00f;
  const float zn2_im = POST * std::sin(A2);

  Pattern p;
  p.name = "op_art_pinna_brelstaff_looming_rings";
  p.inner = InnerFunction::SymmetryBreak;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {PRE * std::cos(A0), PRE * std::sin(A0)};
  p.zn.zn[1] = {0.0f, -0.01f};
  p.zn.zn[2] = {POST * std::cos(A2), zn2_im};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true; // looming
  p.zn.zn0_anim_speed = 0.16f;
  p.zn.zn0_anim_amp = 0.05f;

  p.color.hue_rotation = 0.008f;
  p.color.sat_scale = 1.06f;
  p.color.fade = 0.988f;
  p.color.color_mode = as_int(ColorMode::AtmosphericDrift);
  p.color.spiral_angle_coeff = 0.5f;
  p.color.spiral_radius_coeff = 0.24f;
  p.color.chroma_breath_amp = 0.16f;
  p.color.chroma_breath_freq = 0.20f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.014f;
  p.color.hue_velocity_breath = 0.008f;
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.78f;
  p.seed.hue_speed = 0.08f;
  p.seed.alpha = 0.90f;
  p.seed.pulse_freq = 0.08f;
  p.seed.pulse_amp = 0.12f;
  p.speed = 0.8f;

  p.motion
      .add("micro_element_tilt",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.07f,
                            .amplitude = 0.05f,
                            .offset = zn2_im},
           control::ModTarget::Zn2Im)
      .add("looming_hue_gradient",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.05f,
                            .amplitude = 0.15f,
                            .offset = 0.5f},
           control::ModTarget::SpiralAngleCoeff)
      .add("ring_chroma",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.09f,
                            .amplitude = 0.06f,
                            .offset = 0.16f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
