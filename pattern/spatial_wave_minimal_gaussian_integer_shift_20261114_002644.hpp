#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimal): Gaussian-Integer Shift, the Torus Endomorphism
 * z -> (1 + i) z.
 *
 * Scientific foundation:
 * - Multiplication by a Gaussian integer a maps the lattice 2Z[i] into itself,
 *   so z -> a z is a well-defined, continuous, |a|^2-to-one endomorphism of
 *   the torus C / 2Z[i]. For a = 1 + i it is the two-dimensional Bernoulli
 *   shift in the complex base 1 + i (the numeration system whose fundamental
 *   tile is the twindragon). It is uniformly expanding by sqrt(2), mixing, and
 *   it is the torus side of the degree-2 Lattes map (see
 *   lattes_torus_sphere_shadow, which is this map seen through Weierstrass P).
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Identity, zn0 = (1, 1): T(z) = (1 + i) z + b, nothing else.
 * - Reduce = Grid. Grid is the only discontinuous reduction, but for a
 *   Gaussian-integer multiplier the wrapped map is continuous on the torus,
 *   so the feedback image has no seams. This is the reason the multiplier is
 *   exactly (1, 1) and is never modulated.
 * - Seed = Simplex border. The border of the fundamental square is the pair
 *   of lattice lines; frame n shows their n-th preimage, a square grid scaled
 *   by 2^(-n/2) and turned by 45 degrees each generation.
 * - Fade = 0.90, far below the catalogue norm. The steady image is the
 *   Neumann series sum_n fade^n (seed o F^n); a sqrt(2)-expanding map reaches
 *   pixel scale in about 20 generations, so the series must converge faster
 *   than the usual 0.985 or it aliases into noise.
 * - Color = SpatialWave with integer wave numbers (1 and 2 cycles across the
 *   cell) so the hue field is itself lattice-periodic.
 * - Motion: a Sawtooth of amplitude exactly 1 on Zn3Re is a seamless glide,
 *   because a translation by 2 is the identity on the torus.
 */
inline Pattern spatial_wave_minimal_gaussian_integer_shift() {
  Pattern p;
  p.name = "spatial_wave_minimal_gaussian_integer_shift";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {1.0f, 1.0f}; // the Gaussian integer 1 + i, exact
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.085f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.90f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;   // no hue ramp: keeps lattice periodicity
  p.color.spiral_radius_coeff = 1.0f;  // wave 1: one cycle per cell
  p.color.chroma_breath_amp = 0.06f;   // wave 1 amplitude
  p.color.chroma_breath_freq = 2.0f;   // wave 2: two cycles per cell
  p.color.chroma_breath_phase = 0.04f; // wave 2 amplitude
  p.color.hue_velocity_base = 0.012f;
  p.color.hue_velocity_breath = -0.008f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.10f;
  p.seed.hue = 0.58f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.95f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f; // opaque lattice lines
  p.speed = 0.8f;

  p.motion
      .add("torus_glide",
           control::ModLane{.wave = control::Waveform::Sawtooth,
                            .frequency_hz = 0.011f,
                            .amplitude = 1.0f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.019f,
                            .amplitude = 0.03f,
                            .offset = 0.085f},
           control::ModTarget::HueRot)
      .add("wave_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.027f,
                            .amplitude = 0.035f,
                            .offset = 0.06f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
