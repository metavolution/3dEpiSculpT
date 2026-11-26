#pragma once

#include "base.hpp"
#include <cmath>

namespace epi::presets {

/**
 * Strange attractor — sinh feedback with incommensurate affine rotations.
 *
 * MATHEMATICAL INSIGHT: The composition of two rotations at angles that
 * are irrational multiples of each other (here 2π/7 ≈ 51.4° and 2π/11
 * ≈ 32.7°) produces quasi-periodic orbits that NEVER exactly repeat.
 * This is the Jacobi rotation principle from dynamical systems theory.
 *
 * When a hyperbolic function (sinh) is sandwiched between these two
 * irrational rotations, the resulting map has no stable periodic points.
 * Instead, orbits densely fill fractal-like regions — the hallmark of
 * a strange attractor. The visual result is folded, sheet-like structures
 * that resemble Lorenz or Rössler attractor cross-sections.
 *
 * The scaling factors (0.42 pre, 0.95 post) ensure sinh's exponential
 * growth is tamed: pre-affine contracts to keep |z| in sinh's
 * quasi-linear regime, post-affine preserves the folded geometry.
 *
 * Grid reduction creates the characteristic "folded" look — structure
 * reflects at domain boundaries like a strange attractor bouncing off
 * the walls of its embedding space.
 *
 * Alive color mode 3 (non-uniform hue velocity) is used instead of
 * mode 4, because the strange attractor geometry IS the visual story —
 * the color should follow the dynamics, not impose its own spatial field.
 */
inline Pattern strange_attractor() {
  // 2π/7 ≈ 0.8976 rad → heptagonal rotation
  // constexpr float A7  = 2.0f * 3.14159265f / 7.0f;
  constexpr float C7  = 0.6234898019f; // cos(2π/7)
  constexpr float S7  = 0.7818314825f; // sin(2π/7)
  // 2π/11 ≈ 0.5712 rad → hendecagonal rotation
  constexpr float C11 = 0.8412535328f; // cos(2π/11)
  constexpr float S11 = 0.5406408170f; // sin(2π/11)
  // Scale factors coupled so sinh(x+iy) reaches the Grid turning boundary along
  // both real (sinh) and imaginary (sin) axes while folding multi-sheet ribbons.
  constexpr float PRE  = 1.32f;
  constexpr float POST = 1.02f;

  Pattern p;
  p.name = "strange_attractor";
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Grid;
  // Pre-affine: 2π/7 rotation scaled to 1.32
  p.zn.zn[0] = {C7 * PRE, S7 * PRE};
  p.zn.zn[1] = {0.015f, -0.011f};
  // Post-affine: 2π/11 rotation scaled to 1.02
  p.zn.zn[2] = {C11 * POST, S11 * POST};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.16f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.11f;
  p.zn.zn1_anim_amp = 0.016f;
  p.zn.zn1_anim_k = 7.0f;
  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.988f;
  // Mode 3: alive (non-uniform hue velocity follows the dynamics)
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff  = 0.8f;
  p.color.spiral_radius_coeff = 0.4f;
  p.color.chroma_breath_amp   = 0.14f;
  p.color.chroma_breath_freq  = 0.10f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base   = 0.018f;
  p.color.hue_velocity_breath = 0.008f;
  p.color.hue_velocity_drift  = 0.003f;
  p.color.luma_shape_depth    = 0.0f;
  p.color.luma_shape_center   = 0.0f;
  p.seed.border_width = 0.110f;
  p.seed.hue_speed = 0.10f;
  p.seed.alpha = 0.95f;
  p.speed = 0.9f;
  return p;
}

} // namespace epi::presets
