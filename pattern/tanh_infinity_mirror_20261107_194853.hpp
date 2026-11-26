#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh infinity mirror preset.
 *
 * Mathematical foundation:
 * - An infinity mirror is two parallel mirrors facing each other: light
 *   bounces between them, creating a tunnel of ever-smaller reflections.
 *   tanh(z) saturates at ±1, creating EXACTLY this: two "mirrors" at
 *   Re(z) = ±1 that bound the dynamics.
 *
 * - With |zn0| = 1.08 (strongly expanding), each iteration pushes structure
 *   toward the tanh walls, which reflect it back. The round-trip creates
 *   a new, smaller copy — exactly the physics of the infinity mirror.
 *   After N iterations, structure has been folded N times, producing
 *   2^N copies at exponentially decreasing scale.
 *
 * - Pre-rotation at the GOLDEN ANGLE (2*pi/phi^2 ≈ 137.5 deg) creates
 *   MAXIMUM DENSITY of the folded reflections. The golden angle is the
 *   angle that minimizes coherent alignment between successive copies —
 *   it produces the densest possible packing of reflected images, just
 *   as sunflower seeds use golden-angle phyllotaxis for optimal packing.
 *
 * - Post-rotation at pi/5 (36 deg, decagonal, 10-fold) creates the
 *   KALEIDOSCOPIC structure within the mirror tunnel. 10-fold symmetry
 *   combined with golden-angle rotation produces quasicrystalline
 *   patterns reminiscent of Penrose tiling.
 *
 * - Torus reduction (instead of grid) gives SEAMLESS wrap: the tunnel
 *   has no visible cell boundaries, making the infinite regression
 *   appear truly continuous — like a real infinity mirror.
 *
 * - Very high fade (0.990) preserves many reflected generations,
 *   creating the visual sensation of infinite reflected copies
 *   stretching to a vanishing point.
 *
 * - Strong angle→hue coupling (0.93) creates DISTINCT COLORS for each
 *   "reflection tier," making the kaleidoscopic depth structure visible
 *   as concentric color rings.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern tanh_infinity_mirror() {
  // Golden angle: 2*pi*(2-phi) ≈ 2.39996 rad (≈137.5 deg)
  constexpr float CGA = -0.7373688781f;  // cos(golden angle)
  constexpr float SGA =  0.6754902943f;  // sin(golden angle)
  // pi/5 (36 deg): decagonal, 10-fold kaleidoscopic symmetry
  constexpr float C5  =  0.8090169944f;  // cos(pi/5)
  constexpr float S5  =  0.5877852523f;  // sin(pi/5)

  constexpr float PRE  = 1.5490f;  // STRONGLY EXPANDING: tanh bounds it back
  constexpr float POST = 1.3044f;  // post-contraction for stability
  // |PRE|*|POST| = 0.940 < 1.0: overall contractive despite expansion

  Pattern p;
  p.name = "tanh_infinity_mirror";
  p.inner = InnerFunction::Tanh;         // ±1 saturation walls = mirrors
  p.reduction = ReductionType::Torus;    // seamless: no cell boundaries

  p.zn.zn[0] = {CGA * PRE, SGA * PRE};
  p.zn.zn[1] = {0.1843f, -0.1494f};       // tiny perturbation breaks symmetry
  p.zn.zn[2] = {C5 * POST, S5 * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.018f;        // moderate-fast hue cycling
  p.color.sat_scale = 1.18f;            // vivid kaleidoscope
  p.color.fade = 0.990f;                // VERY HIGH: many reflections persist
  p.color.color_mode = as_int(ColorMode::Alive);               // Alive: organic depth dynamics
  // Very strong angle→hue: each kaleidoscopic sector gets distinct color.
  p.color.spiral_angle_coeff = 0.93f;
  // Strong radial→hue: depth layers appear as colored rings.
  p.color.spiral_radius_coeff = 0.52f;
  p.color.chroma_breath_amp = 0.22f;    // large breath → pulsing mirror effect
  p.color.chroma_breath_freq = 0.16f;
  p.color.chroma_breath_phase = 0.55f;  // large spatial offset
  p.color.hue_velocity_base = 0.022f;   // fast → kaleidoscopic color churn
  p.color.hue_velocity_breath = 0.028f; // dominant breath → pulsing
  p.color.hue_velocity_drift = 0.005f;
  p.color.luma_shape_depth = 0.32f;     // strong luminance → deep contrast
  p.color.luma_shape_center = 2.5f;     // high freq → many luma rings

  p.seed.border_width = 0.084f;         // minimal: pure mirror tunnel
  p.seed.hue_speed = 0.090f;            // fast seed cycling
  p.seed.alpha = 0.92f;                 // somewhat transparent
  p.speed = 0.50f;                      // slow: mesmerizing infinite regress
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.177f;
  p.zn.zn0_anim_amp = 0.050f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.120f;
  p.zn.zn1_anim_amp = 0.038f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.128f;
  p.seed.pulse_amp = 0.18f;

  return p;
}

} // namespace epi::presets
