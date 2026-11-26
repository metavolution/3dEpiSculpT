#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh octagonal crystal abyss preset.
 *
 * Mathematical foundation:
 * - tanh(z) saturates at +/-1 for large |Re(z)|, creating natural
 *   "crystal walls" in the complex plane.  When the pre-affine EXPANDS
 *   (|zn0| = 1.06 > 1.0), each iteration pushes structure outward past
 *   the saturation threshold, but tanh immediately compresses it back
 *   into [-1,1].  This expand-saturate-fold cycle is the engine of
 *   infinite depth: each generation of crystalline structure is denser
 *   than the last by the expansion ratio.
 *
 * - Pre-rotation at pi/4 (45 deg, octagonal, 8-fold) creates the
 *   crystal lattice symmetry.  Octagonal quasicrystals (Ammann-Beenker
 *   tilings) have 8-fold rotational symmetry that cannot tile the plane
 *   periodically -- the iterated feedback produces a quasicrystalline
 *   lattice that never exactly repeats.
 *
 * - Grid reduction makes the tanh saturation walls VISIBLE as sharp
 *   crystal facets.  The combination of octagonal rotation + grid
 *   boundaries + tanh saturation creates a three-way interference:
 *   the 8-fold rotation generates octagonal cells, the grid creates
 *   rectangular facets, and tanh compresses everything into a dense
 *   crystalline lattice that recedes into infinite depth.
 *
 * - Post-rotation at -sqrt(3) radians (irrational counter-rotation)
 *   ensures successive crystal layers never align.  sqrt(3) is
 *   algebraic but pi-incommensurate, so the depth stacking is
 *   quasiperiodic -- like the incommensurate stacking of mineral
 *   crystal layers in natural agate.
 *
 * - Stability: 1.06 * 0.81 = 0.859 < 1.0, well within safe range.
 *   tanh's boundedness provides an additional safety margin: even if
 *   the linear product exceeds 1.0, tanh clips the growth.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern tanh_crystal_abyss() {
  // pi/4 (45 deg): octagonal crystal lattice symmetry
  constexpr float C4  = 0.7071067812f;   // cos(pi/4) = sqrt(2)/2
  constexpr float S4  = 0.7071067812f;   // sin(pi/4) = sqrt(2)/2
  // -sqrt(3) radians: irrational counter-rotation (quasiperiodic depth)
  constexpr float CR3 = -0.1606113978f;  // cos(sqrt(3)) = cos(-sqrt(3))
  constexpr float SR3N = -0.9870222622f; // sin(-sqrt(3))

  constexpr float PRE  = 1.06f;  // EXPANDING: tanh bounds -> density increase
  constexpr float POST = 0.81f;  // contraction for crystal lattice stability

  Pattern p;
  p.name = "tanh_crystal_abyss";
  p.inner = InnerFunction::Tanh;         // saturating -> infinite crystal depth
  p.reduction = ReductionType::Grid;     // sharp crystal facets

  p.zn.zn[0] = {C4 * PRE, S4 * PRE};
  p.zn.zn[1] = {0.004f, -0.006f};       // tiny lattice imperfection
  p.zn.zn[2] = {CR3 * POST, SR3N * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.10f;
  p.color.fade = 0.988f;                // high: deep crystal persistence
  p.color.color_mode = as_int(ColorMode::Alive);               // Alive: crystal phase dynamics
  p.color.spiral_angle_coeff = 1.10f;   // strong: reveals 8-fold sectors
  p.color.spiral_radius_coeff = 0.48f;  // depth rings through crystal layers
  p.color.chroma_breath_amp = 0.15f;
  p.color.chroma_breath_freq = 0.10f;
  p.color.chroma_breath_phase = 0.38f;
  p.color.hue_velocity_base = 0.013f;
  p.color.hue_velocity_breath = 0.009f;
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.20f;     // luminance: crystal facet contrast
  p.color.luma_shape_center = 2.0f;

  p.seed.border_width = 0.012f;         // narrow: crystal depth dominates
  p.seed.hue_speed = 0.07f;
  p.seed.alpha = 0.90f;
  p.speed = 0.50f;                      // slow: contemplative crystal descent
  return p;
}

} // namespace epi::presets
