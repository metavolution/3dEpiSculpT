#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Hyperbolic mirror kaleidoscope preset.
 *
 * Scientific inspiration:
 * - A classical kaleidoscope works by reflecting light between two or three
 *   flat mirrors. The hyperbolic tangent tanh(z) = (e^z - e^{-z})/(e^z + e^{-z})
 *   creates NONLINEAR mirrors: it maps the entire complex plane into the
 *   strip {|Re| < 1}, with saturation walls at Re(z) = +/-1.
 * - The key to infinite density: the pre-affine EXPANDS (|zn[0]| = 1.02),
 *   pushing structure outward, but tanh immediately compresses it back to
 *   [-1,1]. This expand→saturate→wrap cycle folds structure upon itself
 *   with each iteration, creating exponentially increasing fine detail —
 *   like two parallel mirrors generating infinite reflected copies.
 * - Pre-rotation at 2*pi/5 (72 deg) creates pentagonal kaleidoscopic
 *   symmetry — the non-crystallographic 5-fold order that, like
 *   quasicrystals, never forms a periodic lattice but instead generates
 *   aperiodic tilings of extraordinary visual richness.
 * - Post-rotation at the golden angle (2*pi/phi^2 = 2*pi*(2-phi)) ensures
 *   the pentagonal reflections never align coherently, maximizing the
 *   density of the folded pattern — the same angle that sunflower seeds
 *   use to achieve optimal packing.
 * - Grid reduction sharpens the tanh saturation walls into visible "mirror
 *   edges," completing the kaleidoscopic illusion.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern hyperbolic_mirror_kaleidoscope() {
  // 2*pi/5 (72 deg): pentagonal, non-crystallographic
  constexpr float C5  = 0.3090169944f;  // cos(2*pi/5)
  constexpr float S5  = 0.9510565163f;  // sin(2*pi/5)
  // Golden angle: 2*pi*(2-phi) ≈ 2.39996 rad
  constexpr float CGA = -0.7373688781f; // cos(golden angle)
  constexpr float SGA = 0.6754902943f;  // sin(golden angle)

  constexpr float PRE  = 1.5476f;  // EXPANDING: tanh compresses it back → density
  constexpr float POST = 1.3102f;  // contraction on output for stability

  Pattern p;
  p.name = "hyperbolic_mirror_kaleidoscope";
  p.inner = InnerFunction::Tanh;
  p.reduction = ReductionType::Grid;     // sharp mirror edges

  p.zn.zn[0] = {C5 * PRE, S5 * PRE};
  p.zn.zn[1] = {0.1907f, -0.1962f};       // symmetry-breaking perturbation
  p.zn.zn[2] = {CGA * POST, SGA * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.019f;        // fast hue cycling → kaleidoscope color
  p.color.sat_scale = 1.17f;            // vivid, saturated
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::Alive);               // Alive: organic phase dynamics
  p.color.spiral_angle_coeff = 0.95f;   // maximum angle coupling → pinwheel
  p.color.spiral_radius_coeff = 0.48f;  // strong radial rings
  p.color.chroma_breath_amp = 0.20f;    // large breathing → pulsing kaleidoscope
  p.color.chroma_breath_freq = 0.18f;   // moderate oscillation rate
  p.color.chroma_breath_phase = 0.62f;  // large spatial phase offset
  p.color.hue_velocity_base = 0.020f;   // fast base velocity
  p.color.hue_velocity_breath = 0.026f; // dominant breath → pulsing color bands
  p.color.hue_velocity_drift = 0.005f;
  p.color.luma_shape_depth = 0.30f;     // strong luminance contrast
  p.color.luma_shape_center = 2.0f;

  p.seed.border_width = 0.090f;
  p.seed.hue_speed = 0.11f;             // fast seed cycling → rainbow border
  p.seed.alpha = 0.92f;
  p.speed = 0.76f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.097f;
  p.zn.zn0_anim_amp = 0.038f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.154f;
  p.zn.zn1_anim_amp = 0.041f;
  p.zn.zn1_anim_k = 2.0f;
  p.seed.pulse_freq = 0.111f;
  p.seed.pulse_amp = 0.16f;

  return p;
}

} // namespace epi::presets
