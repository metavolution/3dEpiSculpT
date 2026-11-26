#pragma once

#include "base.hpp"

namespace epi::presets {

// Suite2/III mu=1: beta=e^{i pi/6} (order 12, |beta|=1) rotates 12 ring sources around z*=i/2, Torus mirrors replicate them: >=3-wave interference -> dense phase-vortex / spiral lattice.
/**
 * Phase Transition Suite -- Suite 2 (Vortex & Defect Nucleation), Stage III.
 *
 * Order parameter mu = 1.
 * - beta^12 = 1: the 12 most recent seed copies sit on the circle of radius
 *   1/2 about z* = i/2 (a rotating emitter -> spiral wavefronts), and the
 *   Torus mirror fold replicates the emitter into a lattice.
 * - Interference of >= 3 coherent ring families generically produces
 *   isolated phase singularities: a dense topological-defect lattice.
 * - |beta| = 1 means zero minification (no aliasing); moire colour depth is
 *   at its maximum (0.30 hue, 0.40 saturation).
 *
 * Interpolation contract: identical inner/reduction/seed/colour mode/flags,
 * no motion lanes, identical time-phase floats, constant seam integers;
 * every other field is affine in the suite order parameter mu, so the
 * three stages are collinear (see tools/gen_phase_transition_suites.py).
 */
inline Pattern phase_suite_vortex_iii_defect_lattice() {
  Pattern p;
  p.name = "phase_suite_vortex_iii_defect_lattice";
  p.inner = InnerFunction::Identity;       // suite-constant
  p.reduction = ReductionType::Torus;      // suite-constant
  
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.8660254f, 0.5f}; // beta(mu) = 1 + mu (e^(i pi/6) - 1)
  p.zn.zn[3] = {0.25f, 0.0669873f}; // gamma(mu) = mu (1 - e^(i pi/6)) i/2
  
  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.9200444f; // tau = 12 steps = one beta3 orbit, suite-constant
  p.color.color_mode = as_int(ColorMode::MoireInterference); // suite-constant
  p.color.spiral_angle_coeff = 1.0f;  // seam integer: charge-1 moire grating, suite-constant
  p.color.spiral_radius_coeff = 6.0f; // log-radial grating count, suite-constant
  p.color.chroma_breath_amp = 0.3f; // moire hue depth, affine in mu
  p.color.chroma_breath_freq = 0.09f;  // time-phase, suite-constant
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;   // O9 bounded clock, suite-constant
  p.color.hue_velocity_breath = 0.1413717f; // +-0.25 turn, suite-constant
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.4f; // moire saturation depth, affine in mu
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
