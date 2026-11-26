#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Weierstrass fractal foam preset.
 *
 * Scientific inspiration:
 * - The Weierstrass function W(x) = sum a^n cos(b^n pi x) is continuous
 *   everywhere but differentiable nowhere: a canonical fractal curve.
 * - Here the cos inner function supplies the trigonometric backbone, while
 *   pre/post affine rotations at 2*pi/7 (prime-order symmetry, no lattice
 *   alignment) and 2*pi/e (irrational, guaranteeing dense orbit on the
 *   circle) create a quasiperiodic feedback whose iterates never exactly
 *   repeat, emulating the nowhere-smooth character of the Weierstrass sum.
 * - Grid reduction's sharp tile boundaries introduce the discontinuous
 *   derivative artifacts that visually reference fractal roughness.
 * - Spatial wave frequencies at phi and phi^2 create self-similar harmonic
 *   cascading across scales (phi^2 = phi + 1, the golden recursion).
 *
 * Pipeline: transform -> reduce -> sample -> color(mode4) -> seed -> blend
 */
inline Pattern weierstrass_fractal_foam() {
  // 2*pi/7: prime-order rotation, no crystallographic lattice
  constexpr float C7 = 0.6234898019f;   // cos(2*pi/7)
  constexpr float S7 = 0.7818314825f;   // sin(2*pi/7)
  // 2*pi/e: irrational rotation, dense orbit on SO(2)
  constexpr float CE = -0.9117339148f;  // cos(2*pi/e)
  constexpr float SE = -0.4107812905f;  // sin(2*pi/e)

  constexpr float PHI    = 1.6180339887f; // golden ratio
  constexpr float PHI_SQ = 2.6180339887f; // phi^2 = phi + 1
  constexpr float INV_PHI = 0.6180339887f; // 1/phi

  constexpr float PRE  = 0.935f; // slight contraction (stable feedback)
  constexpr float POST = 0.878f; // stronger post-contraction (damping)

  Pattern p;
  p.name = "weierstrass_fractal_foam";
  p.inner = InnerFunction::Cos;
  p.reduction = ReductionType::Grid;

  p.zn.zn[0] = {C7 * PRE, S7 * PRE};
  p.zn.zn[1] = {0.014f, -0.009f};      // slight asymmetric perturbation
  p.zn.zn[2] = {CE * POST, SE * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.008f;
  p.color.sat_scale = 1.14f;
  p.color.fade = 0.978f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = INV_PHI;     // hue x-slope at 1/phi
  p.color.spiral_radius_coeff = PHI;        // wave1 spatial freq = phi
  p.color.chroma_breath_amp = 0.17f;        // wave1 amplitude
  p.color.chroma_breath_freq = PHI_SQ;      // wave2 spatial freq = phi^2
  p.color.chroma_breath_phase = 0.08f;      // wave2 amplitude
  p.color.hue_velocity_base = 0.014f;       // wave1 temporal speed
  p.color.hue_velocity_breath = 0.014f * INV_PHI; // wave2 speed scaled by 1/phi
  p.color.hue_velocity_drift = 0.003f;      // base drift
  p.color.luma_shape_depth = 0.28f;         // saturation breathing amplitude
  p.color.luma_shape_center = PHI;          // saturation breathing freq = phi

  p.seed.border_width = 0.013f;
  p.seed.hue_speed = 0.058f;
  p.seed.alpha = 0.89f;
  p.speed = 0.74f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.120f;
  p.zn.zn0_anim_amp = 0.043f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.225f;
  p.zn.zn1_anim_amp = 0.046f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.135f;
  p.seed.pulse_amp = 0.21f;

  return p;
}

} // namespace epi::presets
