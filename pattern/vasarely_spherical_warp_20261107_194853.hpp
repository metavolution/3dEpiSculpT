#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Vasarely spherical warp preset.
 *
 * Scientific & artistic foundation:
 * - Victor Vasarely (1906-1997) pioneered kinetic op-art: flat geometric
 *   patterns that create the illusion of 3-dimensional form through precise
 *   mathematical distortion. His "planetary folklore" series (Vega, Arcturus,
 *   Orion) feature regular grids that appear to bulge outward like spheres.
 *
 * - The mathematical basis is CONFORMAL LENS DISTORTION. Near z=0, the
 *   Taylor expansion of cos(z) gives:
 *       cos(z) ≈ 1 - z²/2
 *   The -z²/2 term is EXACTLY a parabolic lens equation: it maps a flat
 *   grid into one that appears to curve over a convex surface. Points near
 *   the center barely move (cos(0) = 1); points away from center are
 *   displaced quadratically — the signature of spherical aberration in
 *   optics and the mathematical core of Vasarely's distortion technique.
 *
 * - Grid reduction creates the base tile pattern (the Vasarely grid).
 *   The cos-lens distortion warps these tiles: small, compressed tiles
 *   near the focus (the "sphere's equator") and normal tiles in flat
 *   regions — producing the 3D sphere-from-2D-grid illusion.
 *
 * - Post-translate zn3 shifts the focus off-center, matching Vasarely's
 *   characteristic asymmetric compositions where the sphere center does
 *   not coincide with the geometric center.
 *
 * - Pre-rotation at pi/18 (10 deg) gives a slight grid skew — Vasarely
 *   often tilted his grids slightly for visual tension.
 *
 * - Post-rotation at arctan(1/2) ≈ 26.57 deg: the angle whose cosine
 *   and sine are 2/sqrt(5) and 1/sqrt(5) — ratios that appear in the
 *   golden rectangle's diagonal and connect to Vasarely's use of
 *   phi-proportioned compositions.
 *
 * - The feedback iteration makes this KINETIC op-art: the spherical warp
 *   evolves over time, creating the pulsing, breathing quality that
 *   Vasarely's physical installations achieved through lenticular panels
 *   and layered transparencies.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode4) -> seed -> blend
 */
inline Pattern vasarely_spherical_warp() {
  // pi/18 (10 deg): slight grid tilt for visual tension
  constexpr float C18 = 0.9848077530f;  // cos(pi/18)
  constexpr float S18 = 0.1736481777f;  // sin(pi/18)
  // arctan(1/2) ≈ 26.57 deg: golden-rectangle diagonal angle
  constexpr float CA  = 0.8944271910f;  // cos(arctan(1/2)) = 2/sqrt(5)
  constexpr float SA  = 0.4472135955f;  // sin(arctan(1/2)) = 1/sqrt(5)

  constexpr float PRE  = 1.3435f;  // moderate contraction: cos stays in lens regime
  constexpr float POST = 1.2728f;

  Pattern p;
  p.name = "vasarely_spherical_warp";
  p.inner = InnerFunction::Cos;          // cos(z) ≈ 1 - z²/2 = parabolic lens
  p.reduction = ReductionType::Grid;     // creates the Vasarely tile grid

  p.zn.zn[0] = {C18 * PRE, S18 * PRE};
  p.zn.zn[1] = {0.0487f, -0.0399f};
  p.zn.zn[2] = {CA * POST, SA * POST};
  // Off-center focal point: the "sphere" doesn't sit at image center.
  // This asymmetry is characteristic of Vasarely's compositions.
  p.zn.zn[3] = {0.14f, -0.09f};

  p.color.hue_rotation = 0.012f;
  p.color.sat_scale = 1.14f;
  p.color.fade = 0.985f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);               // Spatial wave: colored tile stripes
  // Low spatial wave frequencies → broad color regions (like Vasarely's
  // color-blocked compositions where each tile zone is a distinct shade).
  p.color.spiral_angle_coeff = 0.38f;   // gentle left→right hue gradient
  p.color.spiral_radius_coeff = 0.22f;  // wave1 spatial freq (broad)
  p.color.chroma_breath_amp = 0.10f;    // wave1 amplitude (subtle)
  p.color.chroma_breath_freq = 0.28f;   // wave2 spatial freq
  p.color.chroma_breath_phase = 0.06f;  // wave2 amplitude (subtle)
  p.color.hue_velocity_base = 0.012f;   // wave1 temporal speed
  p.color.hue_velocity_breath = 0.008f; // wave2 temporal speed
  p.color.hue_velocity_drift = 0.003f;  // base rotation
  // Saturation breathing creates the "sphere pulsing" kinetic effect —
  // the 3D illusion strengthens and weakens over time.
  p.color.luma_shape_depth = 0.18f;     // sat breathing amplitude
  p.color.luma_shape_center = 2.2f;     // sat breathing frequency

  p.seed.border_width = 0.085f;         // narrow: pure geometric structure
  p.seed.hue_speed = 0.135f;
  p.seed.alpha = 0.92f;
  p.speed = 0.60f;                      // unhurried kinetic motion
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.108f;
  p.zn.zn0_anim_amp = 0.052f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.130f;
  p.zn.zn1_anim_amp = 0.038f;
  p.zn.zn1_anim_k = 5.0f;
  p.seed.pulse_freq = 0.139f;
  p.seed.pulse_amp = 0.20f;

  return p;
}

} // namespace epi::presets
