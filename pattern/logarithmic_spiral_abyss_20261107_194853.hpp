#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Logarithmic spiral abyss preset.
 *
 * Scientific inspiration:
 * - The logarithmic (equiangular) spiral r = a*e^{b*theta} is the ONLY
 *   curve that is self-similar: it looks identical at every scale. This
 *   property makes it appear everywhere in nature — nautilus shells,
 *   hurricane arms, galaxy spiral arms, and phyllotactic growth patterns.
 * - The exponential map e^z sends vertical lines to concentric circles and
 *   horizontal lines to radial rays. Under iterative feedback with rotation,
 *   this naturally produces logarithmic spiral structure: each iteration
 *   generates a new ring that is a scaled, rotated copy of the previous —
 *   i.e., the iteration IS the logarithmic spiral equation.
 * - Pre-rotation at pi/4 (45 deg) creates 8-fold symmetry: after 8
 *   iterations the pattern completes one revolution, generating an
 *   octagonal spiral tunnel where each "floor" is rotated 45 deg from
 *   the last — creating the infinite-depth illusion.
 * - Post-rotation at pi/e (irrational, pi-incommensurate) ensures that the
 *   depth layers never exactly align, preventing periodicity and creating
 *   true fractal depth — each layer is a unique, slightly shifted copy.
 * - Very high fade (0.989) preserves many rings simultaneously, producing
 *   the visual sensation of looking down an infinite spiral staircase.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode4) -> seed -> blend
 */
inline Pattern logarithmic_spiral_abyss() {
  // pi/4 (45 deg): octagonal symmetry, 8-fold tunnel
  constexpr float C4  = 0.7071067812f;  // cos(pi/4) = sqrt(2)/2
  constexpr float S4  = 0.7071067812f;  // sin(pi/4) = sqrt(2)/2
  // pi/e: transcendental, pi-incommensurate depth layering
  constexpr float CPE = 0.4032593026f;  // cos(pi/e)
  constexpr float SPE = 0.9150951568f;  // sin(pi/e)

  constexpr float PRE  = 1.12f;  // drives exp across torus folds
  constexpr float POST = 0.97f;  // gentle contraction: many rings visible

  Pattern p;
  p.name = "logarithmic_spiral_abyss";
  p.inner = InnerFunction::Exp;
  p.reduction = ReductionType::Torus;    // smooth wrapping for seamless depth

  p.zn.zn[0] = {C4 * PRE, S4 * PRE};
  p.zn.zn[1] = {0.0f, 0.006f};          // tiny imaginary offset breaks radial symmetry
  p.zn.zn[2] = {CPE * POST, SPE * POST};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.064f;
  p.zn.zn0_anim_amp = 0.038f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.046f;
  p.zn.zn1_anim_amp = 0.020f;
  p.zn.zn1_anim_k = 8.0f;

  p.color.hue_rotation = 0.010f;
  p.color.sat_scale = 1.13f;
  p.color.fade = 0.989f;                // very high: many rings persist → deep tunnel
  p.color.color_mode = as_int(ColorMode::SpatialWave);               // spatial wave: ring structure
  p.color.spiral_angle_coeff = 0.42f;   // moderate hue x-slope
  p.color.spiral_radius_coeff = 0.25f;  // wave1 spatial freq: ring spacing
  p.color.chroma_breath_amp = 0.14f;    // wave1 amplitude
  p.color.chroma_breath_freq = 0.38f;   // wave2 spatial freq: finer rings
  p.color.chroma_breath_phase = 0.07f;  // wave2 amplitude (subtle)
  p.color.hue_velocity_base = 0.008f;   // slow temporal drift
  p.color.hue_velocity_breath = 0.005f; // slow breath → contemplative
  p.color.hue_velocity_drift = 0.002f;  // glacial base drift
  p.color.luma_shape_depth = 0.26f;     // depth-enhancing luminance rings
  p.color.luma_shape_center = 1.5f;     // low freq → wide luminance bands

  p.seed.border_width = 0.075f;         // visible rim for spiral tunnel feeding
  p.seed.hue_speed = 0.035f;
  p.seed.alpha = 0.92f;
  p.speed = 0.48f;                      // slow: hypnotic infinite descent
  return p;
}

} // namespace epi::presets
