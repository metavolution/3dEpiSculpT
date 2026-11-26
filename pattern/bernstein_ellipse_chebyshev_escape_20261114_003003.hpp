#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spectral & Optical: Bernstein Ellipses, the Escape Bands of Chebyshev T2.
 *
 * Scientific foundation:
 * - The Julia set of Z -> Z^2 - 2 is the segment [-2, 2]: under Z = 2 cos t
 *   the map is t -> 2t, i.e. it is the Chebyshev polynomial T2. Everything
 *   off the segment escapes, and the Green's function of the segment is
 *   G(Z) = log|Z/2 + sqrt(Z^2/4 - 1)|. Its level curves are the confocal
 *   Bernstein ellipses E_rho with foci at the ends of the segment.
 * - Bernstein (1912): a function analytic inside E_rho is approximated by
 *   degree-n polynomials with error O(rho^-n). These ellipses are the
 *   convergence regions of every Chebyshev spectral method. The invariant
 *   density on the segment is the arcsine law 1 / (pi sqrt(4 - Z^2)).
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = P2, 0.9 (1 - w^2). With zn0 = k / sqrt(2), zn2 = -2 / (0.9 k) and
 *   k = 2.3 the transform is T(z) = k z^2 - 2/k, which is Z -> Z^2 - 2 in the
 *   scaled coordinate Z = k z. The segment spans 87 percent of the frame.
 * - Reduce = Clamp + Simplex seed: a pixel is coloured when its orbit leaves
 *   the square, so generation n is the band between two Bernstein ellipses,
 *   rho_n = rho_0^(1/2^n). The bands thin geometrically onto the segment.
 *   (The exit region is the window square, not a round bailout disc, so the
 *   outermost band has the window's corners; inner bands are ellipses.)
 * - Color = OKLab rotation, one perceptually even hue step per band, plus a
 *   small log|T| term that accumulates the potential along the orbit and
 *   shades each band smoothly.
 * - Motion: Zn3Re moves the parameter through c = -2, the tip of the
 *   Mandelbrot antenna. Left of it the segment breaks into a Cantor dust;
 *   right of it the set thickens and sprouts whiskers.
 */
inline Pattern bernstein_ellipse_chebyshev_escape() {
  Pattern p;
  p.name = "bernstein_ellipse_chebyshev_escape";
  p.inner = InnerFunction::P2;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.62634560f, 0.0f};  // k / sqrt(2), k = 2.3
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {-0.96618357f, 0.0f}; // -2 / (0.9 k)
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.09f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.972f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.04f; // accumulates the escape potential
  p.color.chroma_breath_amp = 0.06f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.002f;
  p.color.hue_velocity_drift = 0.0006f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.04f;
  p.seed.hue = 0.52f;
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("antenna_tip_crossing",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.021f,
                            .amplitude = 0.03f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("band_hue_step",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.015f,
                            .amplitude = 0.03f,
                            .offset = 0.09f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
