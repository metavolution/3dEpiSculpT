#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Catalan cascade striation preset.
 *
 * Scientific inspiration:
 * - Catalan numbers C_n count the triangulations of a convex (n+2)-gon,
 *   full binary trees with n+1 leaves, and Dyck paths of length 2n.
 *   Their generating function satisfies C(x) = (1 - sqrt(1-4x))/(2x),
 *   a square-root singularity producing the asymptotic C_n ~ 4^n/(n^{3/2} sqrt(pi)).
 * - Tan inner function introduces pole singularities at pi/2 + k*pi, creating
 *   natural branching striations that evoke binary tree subdivision. As the
 *   feedback iterates, the stripe pattern undergoes hierarchical refinement
 *   reminiscent of recursive Catalan decomposition.
 * - Pre-rotation at 2*pi/5 (pentagon) connects to the fact that C_3 = 5
 *   triangulations of the pentagon -- the first non-trivial Catalan case.
 * - Post-rotation at pi*(sqrt(5)-1) (golden angle in radians, mod 2*pi)
 *   ensures the striations never align, densely packing angular space.
 * - Grid reduction amplifies the striation discontinuities at tile boundaries,
 *   creating a cascade of nested stripe domains.
 * - Spatial wave (mode 4) with wave frequencies at 1/5 and 2/5 (Catalan
 *   pentagon fractions) produces pentagrid-like color interference.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode4) -> seed -> blend
 */
inline Pattern catalan_cascade_striation() {
  // 2*pi/5 (72 deg): pentagonal symmetry, C_3 = 5
  constexpr float C5 = 0.3090169944f;   // cos(2*pi/5)
  constexpr float S5 = 0.9510565163f;   // sin(2*pi/5)
  // pi*(sqrt(5)-1) ≈ 3.8832... rad → mod 2*pi ≈ 0.60000... rad
  // cos/sin of pi*(sqrt(5)-1)
  constexpr float CP = -0.7705132428f;  // cos(pi*(sqrt(5)-1))
  constexpr float SP = -0.6374239898f;  // sin(pi*(sqrt(5)-1))

  constexpr float PRE  = 1.68f;  // sweeps across tan poles (±pi/2) to drive Catalan striations
  constexpr float POST = 1.18f;  // carries branch domains across grid boundaries and seed rim

  Pattern p;
  p.name = "catalan_cascade_striation";
  p.inner = InnerFunction::Tan;
  p.reduction = ReductionType::Grid;

  p.zn.zn[0] = {C5 * PRE, S5 * PRE};
  p.zn.zn[1] = {0.019f, 0.007f};        // asymmetric seed-breaking offset
  p.zn.zn[2] = {CP * POST, SP * POST};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.072f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.051f;
  p.zn.zn1_anim_amp = 0.025f;
  p.zn.zn1_anim_k = 5.0f;

  p.color.hue_rotation = 0.018f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.988f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.52f;    // hue x-slope
  p.color.spiral_radius_coeff = 0.20f;   // wave1 spatial freq = 1/5
  p.color.chroma_breath_amp = 0.15f;     // wave1 amplitude
  p.color.chroma_breath_freq = 0.40f;    // wave2 spatial freq = 2/5
  p.color.chroma_breath_phase = 0.11f;   // wave2 amplitude
  p.color.hue_velocity_base = 0.019f;    // wave1 temporal speed
  p.color.hue_velocity_breath = 0.012f;  // wave2 temporal speed
  p.color.hue_velocity_drift = 0.005f;   // base drift
  p.color.luma_shape_depth = 0.30f;      // sat breathing amplitude
  p.color.luma_shape_center = 2.618f;    // phi^2 (Catalan/Fibonacci link)

  p.seed.border_width = 0.095f;
  p.seed.hue_speed = 0.082f;
  p.seed.alpha = 0.94f;
  p.speed = 0.85f;
  return p;
}

} // namespace epi::presets
