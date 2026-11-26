#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Lichtenberg dielectric discharge preset.
 *
 * Scientific foundation:
 * - Lichtenberg figures are the fractal branching patterns created by
 *   electrical discharge through a dielectric medium.  The electric
 *   potential V in the region away from conductors satisfies Laplace's
 *   equation nabla^2 V = 0.  Solutions to Laplace's equation in 2D are
 *   exactly the real and imaginary parts of holomorphic (analytic)
 *   functions of a complex variable.
 *
 * - tan(z) IS a holomorphic function, so Re(tan(z)) and Im(tan(z))
 *   both satisfy Laplace's equation.  The POLES of tan at z = pi/2 + k*pi
 *   represent electrode positions (points of infinite potential).  The
 *   striated patterns between poles are equipotential lines and field
 *   lines of the electrostatic solution.
 *
 * - Under iterated feedback, the pole singularities create HIERARCHICAL
 *   branching: each iteration maps the striated structure back through
 *   the pole lattice, creating finer and finer sub-branches.  This is
 *   the same mechanism as real dielectric breakdown: the field
 *   concentrates at branch tips (near poles), triggering further
 *   discharge channels.  Fractal dimension of real Lichtenberg figures
 *   is approximately 1.7.
 *
 * - Pre-rotation at 2*pi/7 (heptagonal) creates 7-fold branching angles.
 *   7 is prime, so the branches never close into regular polygons --
 *   they form the chaotic, tree-like structure of real lightning.
 *
 * - Post-rotation at sqrt(3) radians (irrational, pi-incommensurate)
 *   ensures successive branch generations never align.  The combination
 *   of rational pre-rotation and irrational post-rotation creates the
 *   characteristic mix of ordered backbone and chaotic sub-branches
 *   seen in real discharge patterns.
 *
 * - Grid reduction creates periodic electrode array geometry (capacitor
 *   plate boundaries), and the tile edges become the primary discharge
 *   initiation sites.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern lichtenberg_discharge() {
  // 2*pi/7 (51.4 deg): heptagonal, prime-order branching
  constexpr float C7  = 0.6234898019f;   // cos(2*pi/7)
  constexpr float S7  = 0.7818314825f;   // sin(2*pi/7)
  // sqrt(3) radians (99.2 deg): irrational, pi-incommensurate
  constexpr float CR3 = -0.1606113978f;  // cos(sqrt(3))
  constexpr float SR3 = 0.9870222622f;   // sin(sqrt(3))

  constexpr float PRE  = 1.1641f;  // moderate: accesses pole structure
  constexpr float POST = 1.1295f;  // strong contraction to tame poles

  Pattern p;
  p.name = "lichtenberg_discharge";
  p.inner = InnerFunction::Tan;          // Laplace equation solution
  p.reduction = ReductionType::Grid;     // electrode array boundaries

  p.zn.zn[0] = {C7 * PRE, S7 * PRE};
  p.zn.zn[1] = {0.0518f, -0.0367f};       // stochastic asymmetry (real discharge is random)
  p.zn.zn[2] = {CR3 * POST, SR3 * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.018f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.986f;                // moderate: discharge events are transient
  p.color.color_mode = as_int(ColorMode::Alive);               // Alive: reveals fractal branch depth
  p.color.spiral_angle_coeff = 1.20f;   // strong angle->hue: branching direction visible
  p.color.spiral_radius_coeff = 0.55f;  // concentric discharge rings
  p.color.chroma_breath_amp = 0.19f;    // discharge flickering
  p.color.chroma_breath_freq = 0.14f;
  p.color.chroma_breath_phase = 0.35f;
  p.color.hue_velocity_base = 0.016f;
  p.color.hue_velocity_breath = 0.013f;
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.22f;     // luminance bands = discharge intensity
  p.color.luma_shape_center = 2.0f;

  p.seed.border_width = 0.079f;         // narrow: discharge pattern dominates
  p.seed.hue_speed = 0.14f;             // fast cycling: electrical energy
  p.seed.alpha = 0.92f;
  p.speed = 0.82f;                      // brisk: electrical dynamics
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.181f;
  p.zn.zn0_anim_amp = 0.047f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.130f;
  p.zn.zn1_anim_amp = 0.051f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.082f;
  p.seed.pulse_amp = 0.16f;

  return p;
}

} // namespace epi::presets
