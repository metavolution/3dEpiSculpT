#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Breathing cosmos — exp(z) + c as a Julia-set-analog feedback loop.
 *
 * The exponential map z → exp(z) + c has fundamentally different dynamics
 * from the polynomial Mandelbrot (z → z² + c). Its critical point is at
 * z = -∞, which means the Julia set structure is governed by the orbit of
 * the *asymptotic value* c, not a finite critical point. The Julia set of
 * exp(z) + c consists of uncountably many curves ("hairs") that extend to
 * infinity — producing organic, tendril-like fractal geometry.
 *
 * The post-translate zn3 = (0, π/2) = (0, 1.5708) places c at the point
 * where exp(c) = iπ/2 · e^0 — on the boundary between bounded and
 * escaping orbits for the exponential family. This is the exponential
 * analog of the Mandelbrot cardioid cusp (c = 0.25 for z²+c).
 *
 * The pre-affine uses near-unity scale so the exponential dominates.
 * The result is slowly breathing, organic tendril structures that expand
 * and contract over time.
 */
inline Pattern breathing_cosmos() {
  constexpr float HALF_PI = 1.5707963268f;

  Pattern p;
  p.name = "breathing_cosmos";
  p.inner = InnerFunction::Exp;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {0.98f, 0.0f};     // near-identity pre-scale
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};      // unity post-scale
  p.zn.zn[3] = {0.0f, HALF_PI};   // c = iπ/2 — exponential critical boundary
  p.color.hue_rotation = 0.007f;
  p.color.sat_scale = 1.005f;
  p.color.fade = 0.991f;           // slow decay for long-lived tendrils
  // Mode 4: very slow spatial wave for cosmic drift
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff  = 0.18f;
  p.color.spiral_radius_coeff = 0.11f;
  p.color.chroma_breath_amp   = 0.06f;
  p.color.chroma_breath_freq  = 0.17f;
  p.color.chroma_breath_phase = 0.045f;
  p.color.hue_velocity_base   = 0.007f;
  p.color.hue_velocity_breath = 0.004f;
  p.color.hue_velocity_drift  = 0.0015f;
  p.color.luma_shape_depth    = 0.20f;
  p.color.luma_shape_center   = 1.4f;
  p.seed.border_width = 0.08f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.8f;
  p.speed = 0.35f;

  // ── Motion layer: HSLS drift-rate modulation + slow hue drift ──────────
  //
  // dual_axis_drift: modulates phi_2 and psi_2 (HSLS azimuth/elevation
  // drift RATES, not axis positions) with quadrature sines (π/2 apart).
  // The GPU kernel computes phi = phi_1 + phi_2 × time, so varying phi_2
  // changes how fast the HSLS colour axis rotates — slower at the sine
  // troughs, faster at the peaks.  The quadrature offset prevents both axes
  // from speeding/slowing in lockstep, producing aperiodic colour variation.
  //
  // hue_drift lane: oscillates hue_rot ±0.004 around 0.007 over 28 s.
  // The cosmos "breathes" spectrally — imperceptible frame-to-frame but
  // visible across minute-scale time horizons, fitting the cosmic-timescale
  // aesthetic.
  p.motion = control::PatternMotion::dual_axis_drift(
      /*phi2_centre=*/0.05f, /*psi2_centre=*/0.10f,
      /*amp=*/0.035f, /*period_s=*/36.0f);
  p.motion.add("hue_drift",
      control::ModLane{.wave         = control::Waveform::Sine,
                       .frequency_hz = 1.0f / 28.0f,
                       .amplitude    = 0.004f,
                       .offset       = 0.007f},  // resting = preset hue_rotation
      control::ModTarget::HueRot);
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.176f;
  p.zn.zn0_anim_amp = 0.035f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.148f;
  p.zn.zn1_anim_amp = 0.050f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.119f;
  p.seed.pulse_amp = 0.17f;


  return p;
}

} // namespace epi::presets
