#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Apollonian gasket bloom preset.
 *
 * Scientific inspiration:
 * - The Apollonian gasket is a fractal packing of mutually tangent circles:
 *   start with three mutually tangent circles, then recursively fill every
 *   curvilinear triangle with the unique circle tangent to all three sides.
 *   The curvatures satisfy Descartes' Circle Theorem:
 *     (k1 + k2 + k3 + k4)^2 = 2(k1^2 + k2^2 + k3^2 + k4^2)
 * - Exp inner: e^z is the universal covering map of the punctured plane.
 *   Its radial growth creates dramatic "bloom" patterns — concentric rings
 *   that expand outward like circles being added to the packing. Combined
 *   with rotational feedback, the rings twist into logarithmic spirals that
 *   reference the conformal structure of circle-packing limits.
 * - Pre-rotation at pi/6 (30 deg) reflects hexagonal close-packing — the
 *   densest planar circle packing, and the local geometry around any circle
 *   in an Apollonian gasket that touches 6 neighbors.
 * - Post-rotation at 2*pi/sqrt(7): the curvature sequence in integer
 *   Apollonian packings grows quadratically, and sqrt(7) appears in the
 *   discriminant of the Descartes quadratic form for certain seed triples.
 * - Torus reduction wraps the exponential growth back into bounded domain,
 *   creating the recursive self-similar character of the gasket.
 * - Spatial wave (mode 4) with frequencies at 1/sqrt(3) and sqrt(3) encode
 *   the hexagonal dual of circle packing (triangular interstice geometry).
 *
 * Pipeline: transform -> reduce -> sample -> color(mode4) -> seed -> blend
 */
inline Pattern apollonian_gasket_bloom() {
  // pi/6 (30 deg): hexagonal close-packing angle
  constexpr float C6  = 0.8660254038f;  // cos(pi/6) = sqrt(3)/2
  constexpr float S6  = 0.5000000000f;  // sin(pi/6) = 1/2
  // 2*pi/sqrt(7): Descartes discriminant angle
  constexpr float CA  = -0.7193398003f; // cos(2*pi/sqrt(7))
  constexpr float SA  = 0.6946583705f;  // sin(2*pi/sqrt(7))

  constexpr float INV_SQRT3 = 0.5773502692f; // 1/sqrt(3)
  constexpr float SQRT3     = 1.7320508076f;

  constexpr float PRE  = 1.34f;   // drives exp across torus folds for circle packing blooms
  constexpr float POST = 1.22f;

  Pattern p;
  p.name = "apollonian_gasket_bloom";
  p.inner = InnerFunction::Exp;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {C6 * PRE, S6 * PRE};
  p.zn.zn[1] = {0.24f, -0.15f};          // spiral asymmetry into positive Re(z) bloom
  p.zn.zn[2] = {CA * POST, SA * POST};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.080f;
  p.zn.zn0_anim_amp = 0.036f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.057f;
  p.zn.zn1_anim_amp = 0.022f;
  p.zn.zn1_anim_k = 6.0f;

  p.color.hue_rotation = 0.011f;
  p.color.sat_scale = 1.18f;            // high saturation → jewel-like circle colors
  p.color.fade = 0.987f;                // sustained decay → full-canvas circle blooms
  p.color.color_mode = as_int(ColorMode::SpatialWave);               // Spatial wave: structured interference
  p.color.spiral_angle_coeff = 0.55f;   // hue x-slope
  p.color.spiral_radius_coeff = INV_SQRT3; // wave1 spatial freq: hexagonal dual
  p.color.chroma_breath_amp = 0.19f;    // wave1 amplitude
  p.color.chroma_breath_freq = SQRT3;   // wave2 spatial freq: triangular interstice
  p.color.chroma_breath_phase = 0.13f;  // wave2 amplitude
  p.color.hue_velocity_base = 0.016f;   // wave1 temporal speed
  p.color.hue_velocity_breath = 0.011f; // wave2 temporal speed
  p.color.hue_velocity_drift = 0.004f;  // base drift
  p.color.luma_shape_depth = 0.33f;     // strong sat breathing → pulsing rings
  p.color.luma_shape_center = 3.0f;     // high breathing freq → rapid shimmer

  p.seed.border_width = 0.080f;         // visible rim so exp blooms fill the domain
  p.seed.hue_speed = 0.113f;
  p.seed.alpha = 0.92f;
  p.speed = 0.78f;
  return p;
}

} // namespace epi::presets
