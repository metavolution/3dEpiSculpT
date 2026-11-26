#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh honeycomb tunnel preset.
 *
 * Mathematical foundation:
 * - tanh(z) = (e^z - e^{-z})/(e^z + e^{-z}) saturates at ±1 for large
 *   |Re(z)|, creating natural "walls" in the complex plane. When the
 *   pre-affine EXPANDS (|zn0| > 1.0), each iteration pushes structure
 *   outward, but tanh immediately compresses it back into [-1,1].
 *   This expand→saturate→fold cycle generates exponentially increasing
 *   fine detail — the mechanism behind infinite-depth tunnel illusions.
 *
 * - Pre-rotation at pi/3 (60 deg, hexagonal) creates 6-fold symmetry.
 *   After 6 iterations the pattern completes one revolution, but each
 *   iteration also folds via tanh, so the accumulated structure forms
 *   a HONEYCOMB: hexagonal cells nested within hexagonal cells, each
 *   generation smaller by the contraction ratio |zn2|.
 *
 * - Grid reduction sharpens the tanh saturation walls into visible cell
 *   boundaries. The combination of hexagonal rotation + grid tiling
 *   creates the characteristic Benard/honeycomb cell walls, but here
 *   with fractal depth (each cell contains smaller cells → tunnel).
 *
 * - Post-rotation at 2*pi/phi (golden rotation) ensures successive
 *   depth layers never align, preventing visual periodicity. The golden
 *   angle is MOST irrational — it produces the densest possible orbit,
 *   maximizing visual richness per iteration.
 *
 * - |zn0| = 1.04 is mildly expanding; tanh bounds the growth at each
 *   step, but the expand→fold accumulation creates the infinite depth.
 *   |zn2| = 0.90 provides mild contraction after the inner function,
 *   giving |zn0|×|zn2| = 0.936 < 1.0 for overall stability.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern tanh_honeycomb_tunnel() {
  // pi/3 (60 deg): hexagonal, creates honeycomb cell structure
  constexpr float C3  = 0.5f;             // cos(pi/3)
  constexpr float S3  = 0.8660254038f;    // sin(pi/3)
  // 2*pi/phi (golden rotation): most irrational depth layering
  constexpr float CG  = -0.7373688781f;   // cos(2*pi/phi)
  constexpr float SG  = -0.6754902943f;   // sin(2*pi/phi)

  constexpr float PRE  = 1.4946f;  // EXPANDING: tanh bounds → density increase
  constexpr float POST = 1.3434f;  // mild contraction for stability

  Pattern p;
  p.name = "tanh_honeycomb_tunnel";
  p.inner = InnerFunction::Tanh;         // saturating → infinite folding
  p.reduction = ReductionType::Grid;     // sharp honeycomb cell walls

  p.zn.zn[0] = {C3 * PRE, S3 * PRE};
  p.zn.zn[1] = {0.2183f, -0.2259f};       // slight asymmetry breaks 6-fold
  p.zn.zn[2] = {CG * POST, SG * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.016f;
  p.color.sat_scale = 1.15f;
  p.color.fade = 0.987f;                // high: many depth layers persist
  p.color.color_mode = as_int(ColorMode::Alive);               // Alive: organic depth dynamics
  p.color.spiral_angle_coeff = 0.88f;   // strong angle → reveals 6 sectors
  p.color.spiral_radius_coeff = 0.46f;  // radial bands → depth rings
  p.color.chroma_breath_amp = 0.17f;    // breathing: tunnel pulsation
  p.color.chroma_breath_freq = 0.13f;
  p.color.chroma_breath_phase = 0.42f;
  p.color.hue_velocity_base = 0.017f;
  p.color.hue_velocity_breath = 0.022f; // breath-dominant → depth pulse
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.28f;     // strong luminance → depth contrast
  p.color.luma_shape_center = 2.3f;

  p.seed.border_width = 0.091f;         // moderate-narrow: feedback dominated
  p.seed.hue_speed = 0.070f;
  p.seed.alpha = 0.92f;
  p.speed = 0.55f;                      // slow: mesmerizing tunnel descent
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.125f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.221f;
  p.zn.zn1_anim_amp = 0.041f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.137f;
  p.seed.pulse_amp = 0.15f;

  return p;
}

} // namespace epi::presets
