#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Recursive Memory: Riesz Product, a Multiplicative Cascade in Chroma.
 *
 * Scientific foundation:
 * - F. Riesz (1918): the lacunary products
 *       P_n(t) = prod_{k<n} (1 + a cos(3^k t)),   |a| <= 1,
 *   are non-negative with mean 1, yet converge to a measure that is singular
 *   to Lebesgue measure: almost everywhere the product tends to zero while
 *   its mass piles onto a set of zero length. It is the simplest multifractal
 *   and the template for multiplicative cascades in turbulence (Mandelbrot
 *   1974; Kahane and Peyriere 1976).
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Cube: z -> 1.15 z^3 triples the argument, so the orbit of a
 *   pixel at angle t visits the angles t, 3t, 9t, 27t ...
 * - Color = Opponent with chroma_breath_phase = 2 pi. On each pass the
 *   chroma vector is multiplied by
 *       sat_scale * (1 + A sin(angle + phase)),
 *   a factor that depends on the angle of the pixel the orbit is at. After n
 *   passes the chroma has been multiplied by the Riesz product P_n(t). (2 pi
 *   makes the factor single-valued in the angle; any other value would cut a
 *   seam along the negative real axis.)
 * - sat_scale = 1.11 cancels the mean logarithm of the factor at A = 0.6
 *   (log((1 + sqrt(1 - A^2))/2) = -0.105), so typical chroma neither grows
 *   nor dies; only the fluctuations remain. Those are the cascade.
 * - Seed = WaveRing at the centre, where every interior orbit ends. The
 *   n-th annulus counted from the centre shows P_n: 1, 3, 9, 27, 81 lobes
 *   of colour, each a modulation of the last, sharpening into isolated
 *   bursts on a grey ground as the Julia circle (radius 0.93) is approached.
 * - Reduce = Clamp: the exterior goes to the border and stays dark.
 * - Motion: the cascade amplitude A swings between 0.35 and 0.85, from
 *   nearly uniform to strongly intermittent.
 */
inline Pattern riesz_product_lacunary_multifractal() {
  constexpr float CASCADE_AMP = 0.60f;

  Pattern p;
  p.name = "riesz_product_lacunary_multifractal";
  p.inner = InnerFunction::Cube;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.15f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.04f;
  p.color.sat_scale = 1.11f; // cancels the mean log of the cascade factor
  p.color.fade = 0.97f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = CASCADE_AMP;
  p.color.chroma_breath_freq = 0.03f;       // slow rotation of the cascade
  p.color.chroma_breath_phase = 6.2831853f; // 2 pi: one lobe per turn
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.002f;
  p.color.hue_velocity_drift = 0.0006f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.07f;
  p.seed.hue = 0.10f;
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 0.90f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("cascade_intermittency",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.017f,
                            .amplitude = 0.25f,
                            .offset = CASCADE_AMP},
           control::ModTarget::ChromaBreathAmp)
      .add("julia_circle_parameter",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.011f,
                            .amplitude = 0.03f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re);

  return p;
}

} // namespace epi::presets
