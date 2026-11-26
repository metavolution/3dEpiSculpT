#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Advanced): Fresnel Secant Chirp.
 *
 * Scientific foundation:
 * - Fresnel diffraction is governed by the quadratic phase exp(i k r^2 / 2z).
 *   Its zones have radii r_n proportional to sqrt(n): the zone-plate law. The
 *   same chirp is the free Schrodinger propagator and the phase of a thin
 *   lens.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = SpiralGalaxy, exp(i w) * w / cos(w^2). The secant of the chirp
 *   cos(w^2) has simple poles at w^2 = (n + 1/2) pi, i.e. at
 *   w = +-sqrt((n + 1/2) pi) on both axes: a cross of poles whose spacing
 *   shrinks like 1/sqrt(n), the zone-plate radii.
 * - Off the axes |cos(w^2)| grows like exp(2|xy|)/2, so T(z) is exponentially
 *   small there with phase x^2 - y^2: the four quadrants are mapped next to
 *   the origin carrying a pure chirp phase. T(w) ~ w near 0, so the origin is
 *   a repelling fixed point with multiplier zn0 * zn2 (|.| ~ 1.43). An orbit
 *   starting at distance eps needs log(1/eps) / log 1.43 steps to get back out
 *   to the seed frame, and log(1/eps) ~ 2|xy|: the exit-time bands are the
 *   hyperbolas xy = const, and the direction of exit is set by the chirp
 *   phase. The quadrants fill with hyperbolic Fresnel fringes.
 * - exp(i w) breaks the up/down symmetry (|exp(i w)| = exp(-y)), as an
 *   obliquely lit zone plate would.
 * - Near each pole T leaves the square and the Torus fold wraps it into a
 *   small ring target; these mark the zone radii along the cross.
 * - Motion: exit direction (Zn2Im), zone scale (Zn0Re), wave pitch.
 */
inline Pattern spatial_wave_fresnel_secant_chirp() {
  constexpr float ZONE_SCALE = 2.30f;
  constexpr float EXIT_IM = 0.2121f; // 0.62 * sin(20 deg)

  Pattern p;
  p.name = "spatial_wave_fresnel_secant_chirp";
  p.inner = InnerFunction::SpiralGalaxy;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {ZONE_SCALE, 0.12f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.5826f, EXIT_IM}; // 0.62 * e^{i 20 deg}
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.016f;
  p.color.sat_scale = 1.10f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.25f;
  p.color.spiral_radius_coeff = 2.0f;
  p.color.chroma_breath_amp = 0.16f;
  p.color.chroma_breath_freq = 3.0f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.020f;
  p.color.hue_velocity_breath = 0.012f;
  p.color.hue_velocity_drift = 0.003f;
  p.color.luma_shape_depth = 0.10f;
  p.color.luma_shape_center = 2.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.10f;
  p.seed.hue = 0.62f;
  p.seed.hue_speed = 0.07f;
  p.seed.alpha = 0.93f;
  p.seed.pulse_freq = 0.06f;
  p.seed.pulse_amp = 0.10f;
  p.speed = 0.85f;

  p.motion
      .add("exit_direction",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.037f,
                            .amplitude = 0.07f,
                            .offset = EXIT_IM},
           control::ModTarget::Zn2Im)
      .add("zone_scale",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.023f,
                            .amplitude = 0.09f,
                            .offset = ZONE_SCALE},
           control::ModTarget::Zn0Re)
      .add("fringe_pitch",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.029f,
                            .amplitude = 0.5f,
                            .offset = 2.0f},
           control::ModTarget::SpiralRadiusCoeff);

  return p;
}

} // namespace epi::presets
