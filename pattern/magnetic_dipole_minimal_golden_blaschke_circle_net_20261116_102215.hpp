#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Magnetic Dipole (Minimal 3.5D): the Golden Blaschke Product and its
 * Circle Net.
 *
 * Mathematics:
 * - B(u) = (u^2 + a) / (a u^2 + 1) with a = sin 18 deg = (sqrt 5 - 1) / 4
 *   is a Blaschke product of degree 2. It maps the unit circle onto
 *   itself two to one, the disc into the disc, the outside into the
 *   outside, and it commutes with the inversion u -> 1 / conj(u).
 * - Its fixed points solve (u - 1)(a u^2 + (a - 1) u + a) = 0. For this a
 *   they are 1, 1 / phi and phi exactly (phi the golden ratio). The two
 *   golden ones are sinks with multiplier 2 / sqrt 5 = 0.894, one for the
 *   disc and one for the outside; 1 repels with B'(1) = 1.056. The Julia
 *   set is the unit circle.
 * - B is real and even, so B(u) is real exactly when u^2 is real: the
 *   preimage of the real axis is the real axis together with the
 *   imaginary axis. The Cross seed is therefore generation 0 and
 *   generation 1 of one family, and its pullbacks B^-k(axis) are curves
 *   that cross four at a time at the 2^k preimages of the critical point
 *   0 and crowd onto the circle from both sides. Inside the disc B
 *   contracts the Poincare metric, so the lines keep a roughly constant
 *   hyperbolic width and thin toward the rim as in a Circle Limit; the
 *   outside is the same net turned inside out.
 *
 * Pipeline: MagneticDipole with zn0 = 1.75 sqrt a, zn2 = A / 1.75,
 * zn3 = D / 1.75, where A = (1/a - a) / 2 and D = (1/a + a) / 2, gives
 * T(z) = B(1.75 z) / 1.75. The circle has radius 0.571 in the frame, the
 * sinks sit on the horizontal seed line at 0.353 and 0.925, and the poles
 * +-1.80 i are just outside. Every orbit ends on that seed line, so every
 * pixel is lit; a new curve of the net is lit by the vertical line one
 * pass earlier, in the complementary hue. OKLab turns the hue 0.04 per
 * pass: depth toward the circle reads as a slow rainbow.
 *
 * Fidelity:   identity
 * Cell:       MagneticDipole / Clamp / Cross / OKLabColorRotation
 * Nearest:    spatial_wave_minimal_poincare_disc_five_sources is an
 *             isometry of the disc (order 5, nothing converges);
 *             tanh_hyperbolic_tessellation_abyss tiles by a strip map.
 *             Here the map is a 2-to-1 contraction of the disc and the
 *             circle is its Julia set.
 * Height:     age = passes to reach the real axis (median 19 in the check).
 * Hue clock:  bounded; step 0.04 +- 0.015 from the lane.
 * Fastest lane: 0.018 Hz
 * Status:     designed from source on 2026-11-16. The slot arithmetic was
 *             checked against B at 50 points (error 1e-15) and the fixed
 *             points and multiplier numerically; the seed reached opacity
 *             above 0.9 at every pixel of a 300 x 300 grid. Not built,
 *             rendered or measured.
 * @assert sing[0].period == 1 && sing[0].rho_abs within 1 % of 0.8944
 */
inline Pattern magnetic_dipole_minimal_golden_blaschke_circle_net() {
  Pattern p;
  p.name = "magnetic_dipole_minimal_golden_blaschke_circle_net";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.97281270f, 0.0f}; // 1.75 * sqrt(sin 18 deg)
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.83630028f, 0.0f}; // (1/a - a) / 2 / 1.75
  p.zn.zn[3] = {1.01288142f, 0.0f}; // (1/a + a) / 2 / 1.75

  p.color.hue_rotation = 0.04f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.97f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.015f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0005f;
  p.color.hue_velocity_drift = 0.0002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.58f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("rainbow_per_generation",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.007f,
                            .amplitude = 0.015f,
                            .offset = 0.04f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.018f,
                            .amplitude = 0.01f,
                            .offset = 0.015f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
