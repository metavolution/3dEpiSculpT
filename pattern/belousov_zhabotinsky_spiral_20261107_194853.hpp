#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Belousov-Zhabotinsky spiral wave preset.
 *
 * Scientific inspiration:
 * - The Belousov-Zhabotinsky (BZ) reaction is a canonical chemical oscillator
 *   that spontaneously forms target patterns and rotating spiral waves in a
 *   thin layer of reagent. The reaction cycles between oxidized (blue) and
 *   reduced (red) states via autocatalytic feedback — the chemical analog
 *   of excitable media like cardiac tissue.
 * - Cosh inner: cosh(z) = (e^z + e^{-z})/2 is a symmetric expansion that
 *   creates dome-shaped amplitude profiles. Unlike tanh (which saturates),
 *   cosh grows symmetrically in both directions — modeling the activator
 *   species concentration that builds up symmetrically before triggering.
 *   Cosh also has no zeros on the real line (minimum at cosh(0) = 1),
 *   which prevents the dead zones that sin/cos create.
 * - Pre-rotation at 2*pi*(e-2) uses the fractional part of Euler's number
 *   for an irrational angle that produces dense, never-repeating spiral arms.
 * - Post-rotation at sqrt(2) radians is incommensurate with pi, so the
 *   pre and post rotations never synchronize — the hallmark of the BZ
 *   reaction's quasiperiodic oscillation between multiple chemical states.
 * - Grid reduction creates sharp wavefronts at tile boundaries, emulating
 *   the abrupt phase transitions in the BZ reaction's excitation front.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern belousov_zhabotinsky_spiral() {
  // 2*pi*(e-2): Euler-derived irrational angle
  constexpr float CE = -0.1979574842f;  // cos(2*pi*(e-2))
  constexpr float SE = -0.9801425128f;  // sin(2*pi*(e-2))
  // sqrt(2) radians: pi-incommensurate
  constexpr float CR = 0.1556174932f;   // cos(sqrt(2))
  constexpr float SR = 0.9877659460f;   // sin(sqrt(2))

  constexpr float PRE  = 0.88f;  // moderate contraction (cosh grows)
  constexpr float POST = 0.84f;

  Pattern p;
  p.name = "belousov_zhabotinsky_spiral";
  p.inner = InnerFunction::Cosh;
  p.reduction = ReductionType::Grid;

  p.zn.zn[0] = {CE * PRE, SE * PRE};
  p.zn.zn[1] = {0.015f, -0.012f};       // small perturbation breaks radial symmetry
  p.zn.zn[2] = {CR * POST, SR * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.016f;
  p.color.sat_scale = 1.16f;            // vivid — BZ reagent is famously colorful
  p.color.fade = 0.977f;
  p.color.color_mode = as_int(ColorMode::Alive);               // Alive: oscillatory triple-velocity
  p.color.spiral_angle_coeff = 0.82f;   // strong angular coupling (spiral arms)
  p.color.spiral_radius_coeff = 0.38f;  // radial rings (target patterns)
  p.color.chroma_breath_amp = 0.22f;    // large amplitude → oxidation/reduction pulse
  p.color.chroma_breath_freq = 0.16f;   // BZ oscillation period
  p.color.chroma_breath_phase = 0.58f;  // phase offset from spatial origin
  p.color.hue_velocity_base = 0.017f;   // base hue rotation
  p.color.hue_velocity_breath = 0.024f; // dominant breath → pulsing oxidation wave
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.27f;     // luminance depth for wavefront contrast
  p.color.luma_shape_center = 1.8f;

  p.seed.border_width = 0.025f;         // moderate seed → sustains oscillation
  p.seed.hue_speed = 0.092f;            // fast seed cycling (reagent injection)
  p.seed.alpha = 0.91f;
  p.speed = 0.88f;                      // brisk — chemical oscillations are dynamic
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.114f;
  p.zn.zn0_anim_amp = 0.037f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.155f;
  p.zn.zn1_anim_amp = 0.050f;
  p.zn.zn1_anim_k = 5.0f;
  p.seed.pulse_freq = 0.096f;
  p.seed.pulse_amp = 0.20f;

  return p;
}

} // namespace epi::presets
