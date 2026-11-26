#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Nonlinear Dynamics: Devaney's Cantor Bouquet in the Sine Family.
 *
 * Scientific foundation:
 * - Devaney and Tangerman (1986): for entire maps such as lambda sin z the
 *   escaping set is a "Cantor bouquet", uncountably many disjoint curves
 *   (hairs) running to infinity, with the points that do not escape forming
 *   basins between them. McMullen (1987) showed that in the sine family this
 *   bouquet has positive area, unlike the exponential family's.
 * - For real 1 < lambda < 2.26 the map has attracting fixed points +-x* on
 *   the real axis (x* = lambda sin x*). Their basins hug the axis; the
 *   imaginary axis and the lines Re z = k pi escape, since there sin z is
 *   purely imaginary and grows like sinh.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Sin, zn0 = pi, zn2 = lambda / pi: in the coordinate w = pi z the
 *   loop iterates w -> lambda sin w on the window [-pi, pi]^2, two periods.
 * - Reduce = Clamp with the Simplex border as trap: a pixel is coloured at
 *   the step its orbit first leaves the window. This is the standard way to
 *   draw these sets (bail out on large |Im w|). The window is small, so the
 *   hairs are drawn as fingers of finite width rather than curves; the
 *   central strip |Im w| < 1.6, where lambda |sin w| < pi, is exact.
 * - The basins of +-x* never leave: they stay black, four lens-shaped lakes
 *   on the axis with the fingers of the bouquet reaching in from above and
 *   below at every multiple of pi.
 * - Color = DepthEcho. The alpha channel counts frames since the seed was
 *   written (32/255 per pass), so hue and saturation follow the exit time
 *   directly for the first eight generations.
 * - Motion: lambda moves between 1.10 and 1.80. As it rises the fixed points
 *   slide apart and the lakes swell, squeezing the fingers.
 */
inline Pattern devaney_sine_cantor_bouquet() {
  constexpr float LAMBDA_OVER_PI = 0.46154934f; // lambda = 1.45

  Pattern p;
  p.name = "devaney_sine_cantor_bouquet";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {3.14159265f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {LAMBDA_OVER_PI, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.05f;
  p.color.sat_scale = 1.05f;
  p.color.fade = 0.975f;
  p.color.color_mode = as_int(ColorMode::DepthEcho);
  p.color.spiral_angle_coeff = 0.35f; // hue per unit of depth
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.20f;  // saturation rises with depth
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;   // also the depth luma lift: off
  p.color.hue_velocity_breath = 0.002f;
  p.color.hue_velocity_drift = 0.0f;  // weight 0 spatial, 1 depth
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.55f;
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("sine_family_lambda",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.021f,
                            .amplitude = 0.11f,
                            .offset = LAMBDA_OVER_PI},
           control::ModTarget::Zn2Re)
      .add("exit_time_hue_step",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.015f,
                            .amplitude = 0.025f,
                            .offset = 0.05f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
