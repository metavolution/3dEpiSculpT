#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Julia Dragon — z² + c at c = −0.123 + 0.745i
 *
 * MATHEMATICAL FOUNDATION:
 *
 * The filled Julia set J_c = {z ∈ ℂ : |f^n(z)| stays bounded} for
 * f(z) = z² + c partitions ℂ into basins of attraction.  When c lies
 * inside the Mandelbrot set (|c| < 2 and bounded orbit), J_c is a
 * connected, self-similar fractal.
 *
 * At c = −0.123 + 0.745i the Julia set lies near the main cardioid /
 * period-1 bulb boundary and takes the form of a "dragon curve" with a
 * characteristic hook-tail geometry.  It is connected but has a highly
 * filamented fractal boundary — a Cantor bouquet of dendrites radiating
 * from the central body.
 *
 * WHY THIS c?
 *   |c| ≈ 0.755, placing it well within the Mandelbrot set.
 *   The critical orbit 0 → c → c²+c → … converges to the fixed point
 *     z* = (1 ± √(1−4c))/2 ≈ (1 ± √(1.492−2.98i))/2
 *   with |f'(z*)| = |2z*| < 1 (attracting fixed point).
 *   The feedback system samples the boundary of J_c each frame, and the
 *   self-similar detail propagates across scales via the z² folding.
 *
 * ENGINE MAPPING:
 *   pre-affine zn0 ≈ 0.97 (slight contraction keeps orbit near J_c)
 *   inner = Square (the canonical z² in z²+c)
 *   post-affine zn2 ≈ 0.97 (symmetric contraction)
 *   post-translate zn3 = c = (−0.123, 0.745)
 *
 * Color mode 1 (spiral) uses atan2 angle + log|z| to paint the fractal
 * boundary in perceptually uniform colour bands matching the basin depth.
 */
inline Pattern julia_dragon() {
  Pattern p;
  p.name = "julia_dragon";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Torus;

  // Near-identity pre-affine: mild scale to keep orbit bounded.
  p.zn.zn[0] = {0.97f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  // Near-identity post-affine.
  p.zn.zn[2] = {0.97f, 0.0f};
  // Julia parameter c — "dragon" locus near period-1/cardioid boundary.
  p.zn.zn[3] = {-0.123f, 0.745f};

  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.02f;
  p.color.fade = 0.992f;

  // Mode 1 (spiral): angle + log-radius paint the basin boundary.
  p.color.color_mode = as_int(ColorMode::Spiral);
  p.color.spiral_angle_coeff = 1.1f;
  p.color.spiral_radius_coeff = 0.65f;
  p.color.chroma_breath_amp = 0.0f;
  p.color.luma_shape_depth = 0.12f;
  p.color.luma_shape_center = 1.8f;

  p.seed.border_width = 0.05f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.85f;
  p.speed = 0.45f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.178f;
  p.zn.zn0_anim_amp = 0.039f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.140f;
  p.zn.zn1_anim_amp = 0.043f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.091f;
  p.seed.pulse_amp = 0.20f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.058f,
                            .amplitude = 0.182f,
                            .offset = 0.650f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.114f,
                            .amplitude = 0.050f,
                            .offset = 0.000f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.046f,
                            .amplitude = 0.030f,
                            .offset = 0.030f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
