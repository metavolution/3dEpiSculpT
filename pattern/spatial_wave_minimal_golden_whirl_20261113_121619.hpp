#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Spatial Wave (Minimalist 4): van Iterson Logarithmic Golden-Angle Lattice.
 *
 * Mathematical foundation:
 * - A pure complex multiplication z -> a*z with a = lambda * exp(i*psi) has
 *   logarithmic spirals as invariant curves. Choosing the divergence
 *   psi = 2*pi/phi^2 (the golden angle, 137.5078 deg) reproduces van Iterson's
 *   logarithmic phyllotaxis lattice: generation n sits at radius lambda^-n and
 *   angle n*psi, and the eye groups generations into Fibonacci parastichies.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Identity: the whole recurrence is the affine pullback
 *   I_{n+1}(z) = I_n(a*z); no nonlinear warp, so every pixel is an exact
 *   scaled/rotated copy of the seed frame. The fixed point 0 is repelling
 *   (|a| = 1.045 > 1), so every orbit reaches the seed border: full coverage.
 * - Because the border seed is a square (D4), the visible rotation is
 *   psi mod 90 deg = 47.5 deg: odd/even generations form two interleaved
 *   square tunnels with a residual 5 deg twist per pair.
 * - Color = SpatialWave: the per-generation hue ramp across screen x turns
 *   generation index into a rainbow depth code.
 * - Motion: a tiny Zn2Im detune of the divergence angle makes the visible
 *   parastichy families hand over, as in real phyllotactic transitions.
 */
inline Pattern spatial_wave_minimal_golden_whirl() {
  constexpr float GOLDEN_ANGLE = 2.39996323f; // 2*pi/phi^2
  constexpr float LAMBDA = 1.045f;            // plastochrone expansion ratio

  Pattern p;
  p.name = "spatial_wave_minimal_golden_whirl";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {LAMBDA * std::cos(GOLDEN_ANGLE), LAMBDA * std::sin(GOLDEN_ANGLE)};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.006f;
  p.color.sat_scale = 1.10f;
  p.color.fade = 0.991f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.50f;   // hue x-slope per generation
  p.color.spiral_radius_coeff = 1.0f;   // wave1 spatial freq
  p.color.chroma_breath_amp = 0.10f;    // wave1 amplitude
  p.color.chroma_breath_freq = 1.618f;  // wave2 spatial freq = phi
  p.color.chroma_breath_phase = 0.06f;  // wave2 amplitude
  p.color.hue_velocity_base = 0.012f;
  p.color.hue_velocity_breath = 0.008f;
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.16f;     // saturation breath amplitude
  p.color.luma_shape_center = 2.0f;     // saturation breath spatial freq

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.035f;
  p.seed.hue = 0.08f;
  p.seed.hue_speed = 0.08f;
  p.seed.alpha = 0.94f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.12f;
  p.speed = 0.5f;

  p.motion
      .add("divergence_angle_detune",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.031f,
                            .amplitude = 0.022f,
                            .offset = 0.0f},
           control::ModTarget::Zn2Im)
      .add("parastichy_hue_pitch",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.047f,
                            .amplitude = 0.12f,
                            .offset = 0.50f},
           control::ModTarget::SpiralAngleCoeff);

  return p;
}

} // namespace epi::presets
