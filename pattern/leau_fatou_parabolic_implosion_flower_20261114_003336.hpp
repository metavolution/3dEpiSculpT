#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Nonlinear Dynamics: Leau-Fatou Flower and Parabolic Implosion.
 *
 * Scientific foundation:
 * - Leau-Fatou flower theorem: at a fixed point whose multiplier is a
 *   primitive q-th root of unity, orbits arrive through q attracting petals
 *   and leave through q repelling ones. Convergence is algebraic (distance
 *   ~ n^(-1/q)), not geometric: the textbook case of critical slowing down.
 * - Parabolic implosion (Douady, Lavaurs, Shishikura): push the multiplier a
 *   hair off the unit circle and the petals open into narrow gates. Orbits
 *   that used to converge now pass through after about 1/|eps| steps, and
 *   the Julia set changes discontinuously. Shishikura used this to prove the
 *   boundary of the Mandelbrot set has Hausdorff dimension 2.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = P3, w (1 + w) / 2. With zn0 = 2, zn1 = -1/2, zn2 = lambda and
 *   zn3 = 1/4 the loop iterates f(w) = lambda w (1 + w), w = 2 z - 1/2: the
 *   quadratic family written with its fixed point at w = 0 and multiplier
 *   lambda there. In the frame the parabolic point is at z = 0.25 and the
 *   critical point w = -1/2 at the centre.
 * - lambda = |lambda| e^{2 pi i / 5}: five petals.
 * - Reduce = Clamp, Seed = Simplex: exit-time colouring of the basin of
 *   infinity. At |lambda| = 1 the five-petalled interior never exits and is
 *   black. For |lambda| > 1 it implodes: the interior drains through the
 *   five gates, and since it takes hundreds of steps the drained region is
 *   dim (Fade 0.982) and finely banded.
 * - Motion: two lanes scale Re and Im of zn2 together, so |lambda| swings
 *   between exactly 1 and 1.024 at fixed argument. The flower closes and
 *   implodes once per cycle.
 * - Color = LivingPaletteAgent: its per-pass hue step wanders slowly, so the
 *   exit bands are recoloured continuously while their geometry holds.
 */
inline Pattern leau_fatou_parabolic_implosion_flower() {
  constexpr float ROOT_RE = 0.30901699f; // cos(2 pi / 5)
  constexpr float ROOT_IM = 0.95105652f; // sin(2 pi / 5)
  constexpr float MOD_MEAN = 1.012f;     // |lambda| = 1.012 +- 0.012
  constexpr float MOD_SWING = 0.012f;
  constexpr float IMPLOSION_HZ = 0.012f;

  Pattern p;
  p.name = "leau_fatou_parabolic_implosion_flower";
  p.inner = InnerFunction::P3;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.0f, 0.0f};
  p.zn.zn[1] = {-0.5f, 0.0f};
  p.zn.zn[2] = {ROOT_RE * MOD_MEAN, ROOT_IM * MOD_MEAN};
  p.zn.zn[3] = {0.25f, 0.0f};

  p.color.hue_rotation = 0.06f;
  p.color.sat_scale = 1.02f;
  p.color.fade = 0.982f;
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
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.85f;
  p.seed.hue_speed = 0.04f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("multiplier_modulus_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = IMPLOSION_HZ,
                            .amplitude = ROOT_RE * MOD_SWING,
                            .offset = ROOT_RE * MOD_MEAN},
           control::ModTarget::Zn2Re)
      .add("multiplier_modulus_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = IMPLOSION_HZ,
                            .amplitude = ROOT_IM * MOD_SWING,
                            .offset = ROOT_IM * MOD_MEAN},
           control::ModTarget::Zn2Im);

  return p;
}

} // namespace epi::presets
