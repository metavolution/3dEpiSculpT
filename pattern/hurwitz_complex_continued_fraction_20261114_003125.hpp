#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Geometry & Number: Hurwitz Complex Continued Fractions (the Gauss Map of
 * the Gaussian Integers).
 *
 * Scientific foundation:
 * - Hurwitz (1887) expanded a complex number as a continued fraction whose
 *   partial quotients are Gaussian integers: G(w) = 1/w - [1/w], with [.] the
 *   nearest Gaussian integer, acting on the square |Re w|, |Im w| <= 1/2. It
 *   is the complex analogue of the Gauss map x -> {1/x}; it is expanding and
 *   ergodic with an absolutely continuous invariant measure, and the cells
 *   on which the first digit is constant are bounded by circles through the
 *   origin (images of lattice lines under inversion).
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - This is the map the pipeline computes with no approximation:
 *   Inner = Invert, zn2 = 4, Reduce = Grid. Grid subtracts the nearest
 *   element of 2Z[i], so Reduce(4 / z) = 2 G(z / 2).
 * - Seed = Simplex border, the boundary of the fundamental square. Generation
 *   n shows its n-th preimage: the boundaries of the rank-n cylinder sets of
 *   the expansion, families of mutually tangent circles nesting towards the
 *   origin and, recursively, towards every Gaussian rational.
 * - Seamlessness: Grid is discontinuous, but the map commutes with z -> -z
 *   and with conjugation, and so does the seed. The image is therefore
 *   symmetric and the wrap from one edge to the opposite one is invisible,
 *   provided the colour stage is symmetric too. That is why Color =
 *   PaletteMorph with no angular term (hue from log|T| only) and zn3 = 0.
 * - |T'| = 4 / |z|^2 >= 2: at least a doubling per generation, so Fade is
 *   0.92 and the palette is driven by luminance (one brightness step per
 *   digit).
 * - Motion: the numerator moves between 3.7 and 4.3. Symmetry and seamless
 *   wrapping survive; the digit cells breathe around the classical map.
 */
inline Pattern hurwitz_complex_continued_fraction() {
  constexpr float NUMERATOR = 4.0f;

  Pattern p;
  p.name = "hurwitz_complex_continued_fraction";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {NUMERATOR, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.86f; // palette modes: rgb = palette * luma * sat
  p.color.fade = 0.92f;
  p.color.color_mode = as_int(ColorMode::PaletteMorph);
  p.color.spiral_angle_coeff = 0.0f;  // symmetric: no angular hue term
  p.color.spiral_radius_coeff = 0.5f; // palette position from log|4/z|
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.05f; // warm/cool palette morph rate
  p.color.chroma_breath_phase = 0.0f; // symmetric: no angular morph
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.004f;
  p.color.hue_velocity_drift = 0.001f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.11f;
  p.seed.hue = 0.10f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.95f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("partial_quotient_scale",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.017f,
                            .amplitude = 0.3f,
                            .offset = NUMERATOR},
           control::ModTarget::Zn2Re)
      .add("palette_cycle",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.011f,
                            .amplitude = 0.15f,
                            .offset = 0.0f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
