#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Geometry & Number: Lattes Map, the Shadow of a Torus Endomorphism on the
 * Sphere.
 *
 * Scientific foundation:
 * - Lattes (1918) gave the first rational map whose Julia set is the whole
 *   Riemann sphere. Take the torus C / Z[i], multiply by 1 + i, and push the
 *   result down to the sphere with the Weierstrass function P (which forgets
 *   the sign of u). The quotient map is the degree-2 rational map
 *       L(z) = (z^2 - 1) / (2 i z).
 *   It has no attracting cycle, no Fatou set, expands by sqrt(2) in the flat
 *   orbifold metric, and its four postcritical points are the images of the
 *   torus' 2-torsion points.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Conjugating L by the Cayley transform (z - i)/(z + i) moves one critical
 *   point to infinity and gives exactly
 *       T(v) = i * ((w^2 - 1)/(w^2 + 1)),   w = e^{-i pi/4} v,
 *   that is Inner = MagneticDipole with zn0 = e^{-i pi/4} and zn2 = i. Check:
 *   0 -> -i -> -1 -> 1, infinity -> i -> -1 -> 1, and 1 is fixed with
 *   multiplier 2i = (1 + i)^2, as the theory requires.
 * - The view is widened by s = 1.8 (zn0 = s e^{-i pi/4}, zn2 = i / s), a
 *   conjugation that changes nothing but the window.
 * - Reduce = Clamp, Seed = Simplex. Since the Julia set is everything, there
 *   is no basin and nothing truly escapes; the window edge is an artificial
 *   trap. Generation n is the set of points first leaving the window at step
 *   n: small discs round the two poles, then their preimages, and so on,
 *   dense in the frame at every scale and nowhere bounding an interior.
 * - It is the same dynamics as spatial_wave_minimal_gaussian_integer_shift,
 *   seen through P: that preset is the torus, this one is its shadow.
 * - Fade = 0.93 for a sqrt(2)-expanding map.
 * - Motion: Lattes maps are not structurally stable. A 2 percent change of
 *   |zn2| leaves the Lattes locus, and small Fatou basins may open and close.
 */
inline Pattern lattes_torus_sphere_shadow() {
  constexpr float PRE = 1.27279221f;  // 1.8 * cos(pi/4)
  constexpr float POST = 0.55555556f; // 1 / 1.8

  Pattern p;
  p.name = "lattes_torus_sphere_shadow";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {PRE, -PRE};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.0f, POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.05f;
  p.color.sat_scale = 1.02f;
  p.color.fade = 0.93f;
  p.color.color_mode = as_int(ColorMode::LivingPaletteAgent);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.003f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.06f;
  p.seed.hue = 0.72f;
  p.seed.hue_speed = 0.04f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("leave_lattes_locus",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.009f,
                            .amplitude = 0.012f,
                            .offset = POST},
           control::ModTarget::Zn2Im)
      .add("trap_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.021f,
                            .amplitude = 0.12f,
                            .offset = 0.72f},
           control::ModTarget::SeedHue);

  return p;
}

} // namespace epi::presets
