#pragma once

#include "base.hpp"

namespace epi::presets {

// Suite1/II mu=1/7: eps=pi/2 is the exact injectivity limit of sin on the frame -- the first fold caustic touches x=+-1 and sin's x/y anisotropy breaks D4 -> D2.
/**
 * Phase Transition Suite -- Suite 1 (Symmetry Breaking), Stage II.
 *
 * Order parameter mu = mu_c = (pi/2 - pi/4)/(2pi - pi/4) = 1/7 exactly.
 * - eps = pi/2: Re(eps z) spans exactly [-pi/2, pi/2], where cos(eps z) = 0
 *   first occurs on the frame edge: onset of wavefront folding.
 * - The cubic term -(eps^2/6) z^3 is only C2-equivariant, and
 *   sin(x+iy) = sin x cosh y + i cos x sinh y is periodic in x but
 *   hyperbolic in y: the eigenmode's D4 symmetry breaks to D2 here.
 * - Collinear gain T'(0) = eps*A = 1 + 6.125 mu (1-mu) = 1.75 > 1: the
 *   origin turns repelling at the same parameter (nonlinear phase
 *   modulation of the copies).
 *
 * Interpolation contract: identical inner/reduction/seed/colour mode/flags,
 * no motion lanes, identical time-phase floats, constant seam integers;
 * every other field is affine in the suite order parameter mu, so the
 * three stages are collinear (see tools/gen_phase_transition_suites.py).
 */
inline Pattern phase_suite_symmetry_ii_fold_onset() {
  Pattern p;
  p.name = "phase_suite_symmetry_ii_fold_onset";
  p.inner = InnerFunction::Sin;            // suite-constant
  p.reduction = ReductionType::Torus;      // suite-constant
  
  p.zn.zn[0] = {1.5707963f, 0.0f}; // eps(mu) = pi/4 + mu * 7pi/4
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.1140846f, 0.0f}; // A(mu) = 4/pi + mu * (1/(2pi) - 4/pi)
  p.zn.zn[3] = {0.0f, 0.0f};
  
  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.8017454f;
  p.color.color_mode = as_int(ColorMode::LumaLockedHueDrift); // suite-constant
  p.color.spiral_angle_coeff = 0.0f;  // seam integer (charge 0), suite-constant
  p.color.spiral_radius_coeff = 0.2060993f;
  p.color.chroma_breath_amp = 0.0771429f;
  p.color.chroma_breath_freq = 0.07f;  // time-phase, suite-constant
  p.color.chroma_breath_phase = 0.0f; // seam integer 2*pi*0, suite-constant
  p.color.hue_velocity_base = 0.0f;   // O9 bounded clock, suite-constant
  p.color.hue_velocity_breath = 0.1099557f; // +-0.25 turn, suite-constant
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;
  
  p.seed.type = SeedType::ChladniCymatics; // (n, m) = (3, 5) plate eigenmode, suite-constant
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.58f;
  p.seed.hue_speed = 0.01f;           // time-phase, suite-constant
  p.seed.alpha = 0.6739337f; // peak-normalized: a = P(1-g)/(1-Pg), P = 0.92
  p.seed.pulse_freq = 0.07f;
  p.seed.pulse_amp = 0.0f;
  
  p.speed = 0.6f;                     // suite-constant

  return p;
}

} // namespace epi::presets
