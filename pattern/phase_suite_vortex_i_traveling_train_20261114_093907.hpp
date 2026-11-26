#pragma once

#include "base.hpp"

namespace epi::presets {

// Suite2/I mu=0: beta=1, gamma=0 (rigid frame, Torus inactive): one coherent WaveRing train sin(20r-5t)e^{-2r} travelling outward at 0.25 units/s, zero defects.
/**
 * Phase Transition Suite -- Suite 2 (Vortex & Defect Nucleation), Stage I.
 *
 * Order parameter mu = 0.
 * - T(z) = z: every feedback copy coincides with the fresh seed, so the
 *   image is a single phase-coherent outward travelling ring train
 *   (wavelength 2pi/20, phase velocity 5/20 = 0.25 units/s).
 * - Moire colour depth (chroma_breath_amp, luma_shape_depth) is 0: no beat.
 * - fade = e^(-1/12): the trail behind each crest is 12 steps long.
 *
 * Interpolation contract: identical inner/reduction/seed/colour mode/flags,
 * no motion lanes, identical time-phase floats, constant seam integers;
 * every other field is affine in the suite order parameter mu, so the
 * three stages are collinear (see tools/gen_phase_transition_suites.py).
 */
inline Pattern phase_suite_vortex_i_traveling_train() {
  Pattern p;
  p.name = "phase_suite_vortex_i_traveling_train";
  p.inner = InnerFunction::Identity;       // suite-constant
  p.reduction = ReductionType::Torus;      // suite-constant
  
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f}; // beta(mu) = 1 + mu (e^(i pi/6) - 1)
  p.zn.zn[3] = {0.0f, 0.0f}; // gamma(mu) = mu (1 - e^(i pi/6)) i/2
  
  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.9200444f; // tau = 12 steps = one beta3 orbit, suite-constant
  p.color.color_mode = as_int(ColorMode::MoireInterference); // suite-constant
  p.color.spiral_angle_coeff = 1.0f;  // seam integer: charge-1 moire grating, suite-constant
  p.color.spiral_radius_coeff = 6.0f; // log-radial grating count, suite-constant
  p.color.chroma_breath_amp = 0.0f; // moire hue depth, affine in mu
  p.color.chroma_breath_freq = 0.09f;  // time-phase, suite-constant
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;   // O9 bounded clock, suite-constant
  p.color.hue_velocity_breath = 0.1413717f; // +-0.25 turn, suite-constant
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f; // moire saturation depth, affine in mu
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
