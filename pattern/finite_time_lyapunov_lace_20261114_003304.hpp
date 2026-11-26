#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Nonlinear Dynamics: Finite-Time Lyapunov Lace.
 *
 * Scientific foundation:
 * - The finite-time Lyapunov exponent of a map F along an orbit is the
 *   Birkhoff average (1/n) sum log|F'(z_k)|. Its ridges are the Lagrangian
 *   coherent structures of the flow (Haller 2001): the stable manifolds that
 *   separate regions of different fate.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Every accumulating colour mode adds its hue increment once per pass, at
 *   the pixel the orbit currently occupies. The hue a pixel finally shows is
 *   therefore a Birkhoff sum of the increment along its orbit, up to the
 *   step where it meets the seed. The pipeline is an ergodic-sum integrator.
 * - Color = Alive with spiral_radius_coeff only: the increment is
 *   c * log(0.01 |T(z)|).
 * - Inner = Square with zn3 = 0, so T(z) = zn2 w^2 with w = zn0 z + zn1, and
 *       log|T|  = log|zn2| + 2 log|w|,
 *       log|T'| = log|2 zn0 zn2| + log|w|.
 *   The increment is an affine function of log|T'|. The Torus fold is a
 *   piecewise isometry, so it does not change |F'|. The accumulated hue is
 *   2c times the finite-time Lyapunov sum of F = Reduce o T, plus a constant
 *   per step. This identity is why zn3 is kept at exactly zero: a post-
 *   translation would break the proportionality.
 * - The orbits of the conjugate map w -> zn0 zn2 w^2 + zn1 pass close to the
 *   critical point w = 0 on thin sets, where log|w| is very negative: those
 *   are the filaments of the lace.
 * - Motion: zn1 (the parameter c of the quadratic family) moves on a small
 *   circle, so the manifolds reconnect continuously.
 */
inline Pattern finite_time_lyapunov_lace() {
  constexpr float C_RE = 0.31f;
  constexpr float C_IM = -0.17f;

  Pattern p;
  p.name = "finite_time_lyapunov_lace";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.18f, 0.22f};
  p.zn.zn[1] = {C_RE, C_IM};
  p.zn.zn[2] = {1.12f, -0.30f};
  p.zn.zn[3] = {0.0f, 0.0f}; // must stay zero: see the identity above

  p.color.hue_rotation = 0.0f; // unused by Alive
  p.color.sat_scale = 1.10f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.05f; // c: hue per unit of log|T|
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.005f;
  p.color.hue_velocity_breath = 0.003f;
  p.color.hue_velocity_drift = 0.001f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.09f;
  p.seed.hue = 0.40f;
  p.seed.hue_speed = 0.06f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.06f;
  p.seed.pulse_amp = 0.10f;
  p.speed = 0.85f;

  p.motion
      .add("quadratic_parameter_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.023f,
                            .amplitude = 0.08f,
                            .offset = C_RE},
           control::ModTarget::Zn1Re)
      .add("quadratic_parameter_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.023f,
                            .amplitude = 0.08f,
                            .offset = C_IM,
                            .phase = 0.25f},
           control::ModTarget::Zn1Im)
      .add("exponent_to_hue_gain",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.017f,
                            .amplitude = 0.03f,
                            .offset = 0.05f},
           control::ModTarget::SpiralRadiusCoeff);

  return p;
}

} // namespace epi::presets
