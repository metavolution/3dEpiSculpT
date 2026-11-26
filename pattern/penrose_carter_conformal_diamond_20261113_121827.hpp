#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Penrose-Carter Conformal Diagram: Compactified Causal Diamonds.
 *
 * Scientific foundation:
 * - R. Penrose (1963) and B. Carter (1966): rewrite spacetime in null
 *   coordinates u = t - x, v = t + x and squash each with a bounded monotone
 *   function (U = atan u or tanh u). Infinity becomes a finite boundary
 *   (scri+, scri-, i0), light rays stay at 45 deg, and the whole causal
 *   structure fits into a diamond. Black-hole and cosmological diagrams tile
 *   such diamonds along their null edges.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Pre-affine rotated by exactly +45 deg: screen (x, t) -> null coordinates.
 * - Inner = Tanh: the compactifying function. It has no critical points
 *   (sech^2 never vanishes) and saturates to +/-1 = conformal infinity.
 * - Post-affine rotated by -45 deg: back to an upright diamond.
 * - Reduction = PareidoliaCathedral: torus fold plus a reflection across the
 *   anti-diagonal x + y = 0, i.e. a reflection in a NULL line. Each fold
 *   glues a diamond to its neighbour along a light-like edge, as in the
 *   maximally extended diagrams.
 * - Color = DepthEcho: hue follows the .w history channel = number of passes
 *   since a pixel was last seeded, a discrete conformal time.
 */
inline Pattern penrose_carter_conformal_diamond() {
  constexpr float NULL_ANGLE = 0.78539816f; // pi/4: light cone
  constexpr float PRE = 1.50f;
  constexpr float POST = 1.32f;
  const float zn2_im = POST * std::sin(-NULL_ANGLE);

  Pattern p;
  p.name = "penrose_carter_conformal_diamond";
  p.inner = InnerFunction::Tanh;
  p.reduction = ReductionType::PareidoliaCathedral;
  p.zn.zn[0] = {PRE * std::cos(NULL_ANGLE), PRE * std::sin(NULL_ANGLE)};
  p.zn.zn[1] = {0.20f, -0.17f};
  p.zn.zn[2] = {POST * std::cos(-NULL_ANGLE), zn2_im};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.10f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.19f;
  p.zn.zn1_anim_amp = 0.04f;
  p.zn.zn1_anim_k = 4.0f;

  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.987f;
  p.color.color_mode = as_int(ColorMode::DepthEcho);
  p.color.spiral_angle_coeff = 2.0f;    // hue turns per unit of history depth
  p.color.spiral_radius_coeff = 0.88f;
  p.color.chroma_breath_amp = 0.25f;    // saturation gain with depth
  p.color.chroma_breath_freq = 0.15f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.022f;
  p.color.hue_velocity_breath = 0.014f;
  p.color.hue_velocity_drift = 0.006f;  // spatial/depth hue mix (mostly depth)
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.08f;
  p.seed.hue = 0.62f;
  p.seed.hue_speed = 0.09f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.09f;
  p.seed.pulse_amp = 0.17f;
  p.speed = 0.8f;

  p.motion
      .add("null_cone_tilt",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.064f,
                            .amplitude = 0.085f,
                            .offset = zn2_im},
           control::ModTarget::Zn2Im)
      .add("conformal_factor_rings",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.049f,
                            .amplitude = 0.24f,
                            .offset = 0.88f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("horizon_chroma",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.083f,
                            .amplitude = 0.08f,
                            .offset = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("scri_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.039f,
                            .amplitude = 0.008f,
                            .offset = 0.014f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
