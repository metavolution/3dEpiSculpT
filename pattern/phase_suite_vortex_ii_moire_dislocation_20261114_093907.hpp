#pragma once

#include "base.hpp"

namespace epi::presets {

// Suite2/II mu=0.5869: copies 0 and 1 separate by exactly lambda/2=pi/20 around the fixed core z*=i/2 -- first interference null opens, nucleating a localized +-1 dislocation pair in a detuned moire envelope.
/**
 * Phase Transition Suite -- Suite 2 (Vortex & Defect Nucleation), Stage II.
 *
 * Order parameter mu = mu_c = 0.586864, solving |gamma|/|beta| = lambda/2.
 * - T(z) = beta z + gamma with beta = 1 + mu(e^{i pi/6} - 1), gamma = mu gamma3:
 *   the fixed point z* = gamma/(1-beta) = i/2 is the SAME for all mu > 0, so
 *   the defect core never drifts along the interpolation path.
 * - |beta| = 0.9670, arg beta = 17.665 deg: successive ring
 *   families are detuned in scale and rotation -> moire beat envelope.
 * - Two-source interference has no complete nodal line for separation
 *   D < lambda/2; at D = lambda/2 the first null appears, and with unequal
 *   copy amplitudes (fade) it closes into a localized dislocation pair.
 *
 * Interpolation contract: identical inner/reduction/seed/colour mode/flags,
 * no motion lanes, identical time-phase floats, constant seam integers;
 * every other field is affine in the suite order parameter mu, so the
 * three stages are collinear (see tools/gen_phase_transition_suites.py).
 */
inline Pattern phase_suite_vortex_ii_moire_dislocation() {
  Pattern p;
  p.name = "phase_suite_vortex_ii_moire_dislocation";
  p.inner = InnerFunction::Identity;       // suite-constant
  p.reduction = ReductionType::Torus;      // suite-constant
  
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.9213751f, 0.293432f}; // beta(mu) = 1 + mu (e^(i pi/6) - 1)
  p.zn.zn[3] = {0.146716f, 0.0393124f}; // gamma(mu) = mu (1 - e^(i pi/6)) i/2
  
  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.9200444f; // tau = 12 steps = one beta3 orbit, suite-constant
  p.color.color_mode = as_int(ColorMode::MoireInterference); // suite-constant
  p.color.spiral_angle_coeff = 1.0f;  // seam integer: charge-1 moire grating, suite-constant
  p.color.spiral_radius_coeff = 6.0f; // log-radial grating count, suite-constant
  p.color.chroma_breath_amp = 0.1760592f; // moire hue depth, affine in mu
  p.color.chroma_breath_freq = 0.09f;  // time-phase, suite-constant
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;   // O9 bounded clock, suite-constant
  p.color.hue_velocity_breath = 0.1413717f; // +-0.25 turn, suite-constant
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.2347456f; // moire saturation depth, affine in mu
  p.color.luma_shape_center = 0.5f;
  
  p.seed.type = SeedType::WaveRing;   // sin(20 r - 5 t) e^{-2r}, suite-constant
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.12f;
  p.seed.hue_speed = 0.015f;          // time-phase, suite-constant
  p.seed.alpha = 0.85f;
  p.seed.pulse_freq = 0.09f;
  p.seed.pulse_amp = 0.0f;
  
  p.speed = 0.8f;                     // suite-constant

  return p;
}

} // namespace epi::presets
