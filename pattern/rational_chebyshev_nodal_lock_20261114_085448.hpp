#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

// Exact 3:2 x 2:1 (|beta|=3) Chebyshev-Torus commuting endomorphism lock with 4 : 1/ln(3) harmonic triad resonance.
/**
 * Rational Resonance Lock 1: Ternary Chebyshev-Torus Nodal Resonator.
 *
 * Analytical derivation:
 * - Transform: Inner = Identity, zn0 = 3/2, zn2 = 2 => exact integer multiplier
 *   beta = zn0 * zn2 = 3.0 + 0i (zn1 = zn3 = 0).
 * - Under Torus reduction cos((pi/2) * torus_reduce(3*x)) = T_3(cos((pi/2)*x)),
 *   forming an exact commuting Chebyshev endomorphism of [-1, 1]^2:
 *     * Boundary lock: 3*(+-1) = +-3 -> torus_reduce(+-3) = -+1.
 *     * Zero-contour preimages of Cross seed (z_x=0 or z_y=0): 3*x in {0, +-2}
 *       => x in {0, +-2/3} at step 1, refining to exact ternary nodes
 *       2*m / 3^k without irrational phase drift or moire beating.
 * - Nyquist & gain budget (Laws L3, L4): Lambda = ln(3) = 1.0986123; dyadic
 *   fade = 7/8 = 0.875 gives forgetting time tau = 7.49 steps and spectral
 *   slope alpha = 2 + 2*ln(8/7)/ln(3) = 2.2431 (< 2% Nyquist energy at 2560^2).
 * - Color = SpectralPrism (mode 20): sat_scale = 8/7 = 1.14285714 satisfies
 *   sat_scale * fade == 1.0 exactly (unit peak amplitude without clipping),
 *   spiral_angle_coeff = 4.0 (D4 azimuthal lock), spiral_radius_coeff = 1/ln(3)
 *   (1 exact color cycle per 3x generation), 120-deg RGB triad (0, 1/3, 2/3).
 */
inline Pattern rational_chebyshev_nodal_lock() {
  const float inv_ln3 = 1.0f / std::log(3.0f); // 0.91023923f: 1 cycle per 3x scale

  Pattern p;
  p.name = "rational_chebyshev_nodal_lock";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.5f, 0.0f}; // exact 3:2 pre-scale
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {2.0f, 0.0f}; // exact 2:1 post-scale => beta = 3
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 8.0f / 7.0f; // exact reciprocal of fade = 7/8
  p.color.fade = 7.0f / 8.0f;      // 0.875 exact dyadic decay
  p.color.color_mode = as_int(ColorMode::SpectralPrism);
  p.color.spiral_angle_coeff = 4.0f;     // exact D4 4-fold azimuthal harmonic
  p.color.spiral_radius_coeff = inv_ln3; // exact 1:1 log_3 sheet harmonic
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.125f;   // 1/8 Hz exact dyadic rate
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;            // R triad phase: 0/3
  p.color.hue_velocity_breath = 1.0f / 3.0f;   // G triad phase: 1/3
  p.color.hue_velocity_drift = 2.0f / 3.0f;    // B triad phase: 2/3
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.0f;  // bounded harmonic clock via motion lane
  p.seed.alpha = 0.9375f;   // 15/16 exact dyadic injection
  p.seed.pulse_freq = 0.125f;
  p.seed.pulse_amp = 0.0f;  // hold unit envelope env(t) == 1.0

  p.speed = 1.0f;

  p.motion
      .add("harmonic_triad_phase",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0625f, // 1/16 Hz exact
                            .amplitude = 0.125f,
                            .offset = 0.0f},
           control::ModTarget::HueRot)
      .add("octave_sheet_lock",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.03125f, // 1/32 Hz (2:1 harmonic lock)
                            .amplitude = 0.25f * inv_ln3,
                            .offset = inv_ln3},
           control::ModTarget::SpiralRadiusCoeff);

  return p;
}

} // namespace epi::presets
