#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Poincare hyperbolic vortex preset.
 *
 * Scientific inspiration:
 * - The Poincare disk model represents the hyperbolic plane inside the unit
 *   disk, where geodesics are circular arcs meeting the boundary at right
 *   angles. Isometries of the disk are Mobius transformations, of which
 *   z -> 1/z (conformal inversion) is the archetype.
 * - Invert inner (1/z) is the ONLY Mobius transformation in the inner
 *   catalog: it maps circles to circles and swaps interior/exterior of the
 *   unit circle, creating the characteristic self-similar nesting of
 *   hyperbolic tessellations.
 * - Pre-rotation at 2*pi/7 (51.43 deg) generates 7-fold rotational symmetry
 *   referencing the {7,3} hyperbolic tiling (regular heptagons, 3 per vertex),
 *   which has the smallest area ratio in its symmetry class.
 * - Post-rotation at 2*pi/sqrt(7) is irrational and incommensurate with
 *   2*pi/7, ensuring the orbit never closes — analogous to the dense geodesic
 *   flow on a hyperbolic surface of genus >= 2.
 * - Pre-contraction (0.54) keeps arguments near the unit circle where 1/z
 *   acts as a genuine inversion; post-translate offsets from the pole at z=0.
 * - Alive color (mode 3) with 3-velocity hue reveals the phase portrait of
 *   the inversive dynamics: spiraling whorls that evoke Escher's Circle Limit.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern hyperbolic_evolved_v2() {
  // 2*pi/7: heptagonal symmetry ({7,3} tiling)
  [[maybe_unused]] constexpr float C7  = 0.6234898019f;  // cos(2*pi/7)
  [[maybe_unused]] constexpr float S7  = 0.7818314825f;  // sin(2*pi/7)
  // 2*pi/sqrt(7): irrational, incommensurate with 2*pi/7
  constexpr float CQ  = -0.7193398003f; // cos(2*pi/sqrt(7))
  constexpr float SQ  = 0.6946583705f;  // sin(2*pi/sqrt(7))

  [[maybe_unused]] constexpr float PRE  = 0.54f;  // strong contraction (like conformal_wing's 0.56)
  constexpr float POST = 0.84f;

  Pattern p;
  p.name = "hyperbolic_evolved_v2";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = { 1.11f, 0.06f }; // Mutated
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {CQ * POST, SQ * POST};
  // Post-translate offsets from the 1/z pole; asymmetric for visual interest
  p.zn.zn[3] = {0.19f, -0.11f};

  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.15f;
  p.color.fade = 0.981f;
  p.color.color_mode = as_int(ColorMode::Alive);               // Alive: 3-velocity phase dynamics
  p.color.spiral_angle_coeff = 0.92f;   // strong angle → hue (reveals 7-fold)
  p.color.spiral_radius_coeff = 0.44f;  // radial ring structure
  p.color.chroma_breath_amp = 0.18f;    // visible breathing envelope
  p.color.chroma_breath_freq = 0.14f;   // moderate breath frequency
  p.color.chroma_breath_phase = 0.37f;  // spatial offset
  p.color.hue_velocity_base = 0.013f;
  p.color.hue_velocity_breath = 0.021f; // dominant breath → pulsing vortex
  p.color.hue_velocity_drift = 0.0035f;
  p.color.luma_shape_depth = 0.25f;
  p.color.luma_shape_center = 2.4f;

  p.seed.border_width = 0.012f;         // narrow: let inversive geometry dominate
  p.seed.hue_speed = 0.065f;
  p.seed.alpha = 0.90f;
  p.speed = 0.68f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.101f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.153f;
  p.zn.zn1_anim_amp = 0.043f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.081f;
  p.seed.pulse_amp = 0.18f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.095f,
                            .amplitude = 0.123f,
                            .offset = 0.440f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.069f,
                            .amplitude = 0.103f,
                            .offset = 0.180f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.048f,
                            .amplitude = 0.030f,
                            .offset = 0.065f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
