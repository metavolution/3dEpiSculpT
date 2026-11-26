#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Exponential rosette vortex preset.
 *
 * Scientific inspiration:
 * - A rosette is a symmetric radial pattern with n-fold rotational symmetry,
 *   found in Gothic rose windows, Islamic geometric art, and the orbital
 *   mechanics of precessing ellipses.
 * - Exp inner (e^z) is the universal covering map of the punctured plane C*:
 *   it wraps the infinite horizontal strip onto concentric annuli. Under
 *   hexagonal rotation (pi/3 = 60 deg), six copies of this covering map
 *   interleave, creating a 6-petaled rosette where each petal is a section
 *   of the universal cover made visible through the feedback iteration.
 * - The moderate pre-contraction (0.78) keeps exp's argument small enough
 *   that the Taylor expansion e^z ≈ 1 + z + z^2/2 contributes visible
 *   quadratic curvature — the petals have SHAPE, not just radial structure.
 *   This distinguishes the rosette from the pure radial tunnel of
 *   logarithmic_spiral_abyss.
 * - Post-rotation at sqrt(3) radians — irrational and the characteristic
 *   length scale of hexagonal geometry (the height of an equilateral
 *   triangle with unit side) — ensures the rosette petals never exactly
 *   overlap, creating dense, fractal-like vortex structure within each petal.
 * - Alive color (mode 3) with strong angular coupling maps the 6-fold
 *   rotational symmetry directly onto hue, creating distinct color sectors
 *   like a stained-glass rose window spinning slowly inward.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern exponential_rosette_vortex() {
  // pi/3 (60 deg): hexagonal, 6-fold rosette symmetry
  constexpr float C3  = 0.5000000000f;  // cos(pi/3) = 1/2
  constexpr float S3  = 0.8660254038f;  // sin(pi/3) = sqrt(3)/2
  // sqrt(3) rad: hexagonal characteristic length, irrational
  constexpr float CS3 = -0.1605629080f; // cos(sqrt(3))
  constexpr float SS3 = 0.9870283545f;  // sin(sqrt(3))

  constexpr float PRE  = 1.2122f;  // moderate contraction: visible quadratic curvature
  constexpr float POST = 0.9227f;  // contraction preserves petal geometry

  Pattern p;
  p.name = "exponential_rosette_vortex";
  p.inner = InnerFunction::Exp;
  p.reduction = ReductionType::Torus;    // smooth wrapping for vortex flow

  p.zn.zn[0] = {C3 * PRE, S3 * PRE};
  p.zn.zn[1] = {0.0378f, -0.0321f};       // tiny offset: asymmetric petal detail
  p.zn.zn[2] = {CS3 * POST, SS3 * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.15f;
  p.color.fade = 0.986f;                // high fade: deep vortex trails
  p.color.color_mode = as_int(ColorMode::Alive);               // Alive: angular coupling → color sectors
  p.color.spiral_angle_coeff = 0.96f;   // near-maximum angle → 6 color sectors
  p.color.spiral_radius_coeff = 0.52f;  // strong radial gradient → depth rings
  p.color.chroma_breath_amp = 0.16f;    // breathing envelope
  p.color.chroma_breath_freq = 0.13f;   // slow breath → majestic pulsation
  p.color.chroma_breath_phase = 0.41f;  // spatial phase offset
  p.color.hue_velocity_base = 0.015f;
  p.color.hue_velocity_breath = 0.019f; // moderate breath dominance
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.24f;     // luminance rings enhance depth
  p.color.luma_shape_center = 1.9f;

  p.seed.border_width = 0.089f;         // narrow: rosette dominates
  p.seed.hue_speed = 0.125f;
  p.seed.alpha = 0.92f;
  p.speed = 0.65f;                      // moderate: let the rosette breathe
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.101f;
  p.zn.zn0_anim_amp = 0.048f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.151f;
  p.zn.zn1_anim_amp = 0.042f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.084f;
  p.seed.pulse_amp = 0.19f;

  return p;
}

} // namespace epi::presets
