#pragma once

#include "base.hpp"

namespace epi::presets {

// Suite1/I mu=0: T=(4/pi)sin(pi z/4) ~ z (gain 1, cubic warp <=5% inside |z|<1) shows the bare (3,5) Chladni plate eigenmode with its D4 symmetry intact.
/**
 * Phase Transition Suite -- Suite 1 (Symmetry Breaking), Stage I.
 *
 * Order parameter mu = 0 (pure linear regime).
 * - T(z) = A sin(eps z) with eps = pi/4, A = 4/pi: T'(0) = 1 and
 *   T(z) = z - (pi^2/96) z^3 + ..., so the ChladniCymatics seed
 *   |cos(3 pi x)cos(5 pi y) - cos(5 pi x)cos(3 pi y)| is rendered as the
 *   minimal standing eigenmode of the square plate (nodal lines lit).
 * - sin is injective on |Re w| < pi/2 and here |Re(eps z)| <= pi/4: no fold.
 * - fade = e^(-1/4) (tau = 4), alpha = 0.7178 pins the steady nodal peak at P = 0.92.
 *
 * Interpolation contract: identical inner/reduction/seed/colour mode/flags,
 * no motion lanes, identical time-phase floats, constant seam integers;
 * every other field is affine in the suite order parameter mu, so the
 * three stages are collinear (see tools/gen_phase_transition_suites.py).
 */
inline Pattern phase_suite_symmetry_i_standing_eigenmode() {
  Pattern p;
  p.name = "phase_suite_symmetry_i_standing_eigenmode";
  p.inner = InnerFunction::Sin;            // suite-constant
  p.reduction = ReductionType::Torus;      // suite-constant
  
  p.zn.zn[0] = {0.7853982f, 0.0f}; // eps(mu) = pi/4 + mu * 7pi/4
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.2732395f, 0.0f}; // A(mu) = 4/pi + mu * (1/(2pi) - 4/pi)
  p.zn.zn[3] = {0.0f, 0.0f};
  
  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.7788008f;
  p.color.color_mode = as_int(ColorMode::LumaLockedHueDrift); // suite-constant
  p.color.spiral_angle_coeff = 0.0f;  // seam integer (charge 0), suite-constant
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.06f;
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
  p.seed.alpha = 0.7178163f; // peak-normalized: a = P(1-g)/(1-Pg), P = 0.92
  p.seed.pulse_freq = 0.07f;
  p.seed.pulse_amp = 0.0f;
  
  p.speed = 0.6f;                     // suite-constant

  return p;
}

} // namespace epi::presets
