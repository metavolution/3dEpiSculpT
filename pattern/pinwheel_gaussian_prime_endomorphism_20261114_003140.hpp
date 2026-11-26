#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Geometry & Number: Pinwheel Orientations from the Gaussian Prime 2 + i.
 *
 * Scientific foundation:
 * - 2 + i is a Gaussian prime of norm 5. Multiplication by it is a 5-to-1
 *   endomorphism of the torus C / 2Z[i] that expands by sqrt(5) and turns by
 *   arctan(1/2), an angle that is an irrational multiple of pi (Niven).
 * - The Conway-Radin pinwheel tiling (1994) has exactly this inflation: its
 *   triangles are expanded by sqrt(5) and turned by arctan(1/2), and because
 *   the angle is irrational the tiles occur in infinitely many orientations,
 *   equidistributed on the circle. It was the first substitution tiling with
 *   statistical rotation invariance.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Identity, zn0 = (2, 1), Reduce = Grid. As for 1 + i (see
 *   spatial_wave_minimal_gaussian_integer_shift) a Gaussian-integer
 *   multiplier makes the wrapped map continuous on the torus, so the
 *   feedback is seamless.
 * - Seed = Cross: the two coordinate axes. Generation n is their n-th
 *   preimage, a square line lattice of mesh 2 / 5^(n/2) turned by
 *   -n arctan(1/2). The frame holds every generation at once: line lattices
 *   at angles 0, 26.57, 53.13, 79.70 ... degrees that never repeat. This is
 *   the orientation set of the pinwheel, not the tiling itself: the pipeline
 *   has no triangle substitution, only the linear part of it.
 * - Fade = 0.72: the map multiplies detail by 2.24 per generation, so six
 *   generations reach pixel scale and the series must be cut short.
 * - Color = Classic with hue step 1/5, one hue per power of the prime.
 * - Motion: two Sawtooth lanes of amplitude 1 on zn3 (seamless on the torus)
 *   glide the lattices through each other at incommensurate rates.
 */
inline Pattern pinwheel_gaussian_prime_endomorphism() {
  Pattern p;
  p.name = "pinwheel_gaussian_prime_endomorphism";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {2.0f, 1.0f}; // the Gaussian prime 2 + i, exact
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.20f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.72f;
  p.color.color_mode = as_int(ColorMode::Classic);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f; // bounded hue clock
  p.color.hue_velocity_breath = 0.002f;
  p.color.hue_velocity_drift = 0.0006f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.08f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.04f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("torus_glide_x",
           control::ModLane{.wave = control::Waveform::Sawtooth,
                            .frequency_hz = 0.007f,
                            .amplitude = 1.0f,
                            .offset = 0.0f,
                            .phase = 0.5f},
           control::ModTarget::Zn3Re)
      .add("torus_glide_y",
           control::ModLane{.wave = control::Waveform::Sawtooth,
                            .frequency_hz = 0.0113f,
                            .amplitude = 1.0f,
                            .offset = 0.0f,
                            .phase = 0.5f},
           control::ModTarget::Zn3Im)
      .add("prime_power_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.015f,
                            .amplitude = 0.05f,
                            .offset = 0.20f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
