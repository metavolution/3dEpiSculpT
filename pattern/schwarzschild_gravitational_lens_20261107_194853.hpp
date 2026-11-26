#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Schwarzschild gravitational lens preset.
 *
 * Scientific foundation:
 * - In general relativity, the Schwarzschild metric describes spacetime
 *   around a non-rotating mass M. Light rays passing at impact parameter b
 *   are deflected by angle alpha = 4GM/(c^2 * b). In the weak-field limit,
 *   this deflection is PROPORTIONAL TO 1/b = 1/|z|.
 *
 * - The inner function 1/z (Invert) IS this gravitational deflection law
 *   expressed as a complex map. It is not a metaphor: the map z -> 1/z
 *   applied to the complex plane rotates and scales each point by 1/|z|^2,
 *   which is exactly the conformal lens mapping that GR prescribes for
 *   the thin-lens approximation of gravitational lensing.
 *
 * - The EINSTEIN RING appears at the critical impact parameter where
 *   1/z maps the circle |z| = 1 to itself: source, lens, and observer
 *   aligned → a perfect ring of light. With pre-contraction PRE < 1,
 *   the effective Einstein ring radius shifts to |z| = sqrt(PRE).
 *
 * - Post-translate {0.18, 0.07} offsets the "mass" from the geometric
 *   center, creating asymmetric gravitational arcs — exactly as observed
 *   in galaxy cluster lenses (Abell 2218, Abell 370) where the mass
 *   distribution is offset from the optical center.
 *
 * - Pre-rotation at 2*pi/11 (a prime p=11, 32.7 deg) creates 11-fold
 *   caustic structure. In strong lensing, the caustic network determines
 *   where multiple images form; prime-order symmetry prevents degenerate
 *   caustic crossings, maximizing the number of distinct image arcs.
 *
 * - Post-rotation at 2*pi/e (irrational) ensures the iterated lensing
 *   orbits never close, producing the dense caustic web seen in the
 *   full-wave-optics treatment of gravitational microlensing.
 *
 * - Alive color (mode 3) with strong angle→hue coupling reveals the
 *   MULTIPLE IMAGES: the same source galaxy appears at different angular
 *   positions around the lens, each at a distinct hue.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern schwarzschild_gravitational_lens() {
  // 2*pi/11: prime-order caustic structure (p=11)
  constexpr float C11 = 0.8412535328f;  // cos(2*pi/11)
  constexpr float S11 = 0.5406408170f;  // sin(2*pi/11)
  // 2*pi/e: irrational, dense orbit → microlensing caustic web
  constexpr float CE  = -0.9117339148f; // cos(2*pi/e)
  constexpr float SE  = -0.4107812905f; // sin(2*pi/e)

  // PRE = 0.52: strong contraction keeps arguments near unit circle
  // where 1/z acts as genuine conformal inversion (not pole blowup).
  // Einstein ring at |z| = sqrt(0.52) ≈ 0.72.
  constexpr float PRE  = 0.52f;
  constexpr float POST = 0.86f;

  Pattern p;
  p.name = "schwarzschild_gravitational_lens";
  p.inner = InnerFunction::Invert;       // 1/z IS gravitational deflection
  p.reduction = ReductionType::Torus;    // seamless: light bends smoothly

  p.zn.zn[0] = {C11 * PRE, S11 * PRE};
  p.zn.zn[1] = {0.0f, 0.0f};            // no pre-translate: pure lens
  p.zn.zn[2] = {CE * POST, SE * POST};
  // Off-center mass → asymmetric gravitational arcs.
  // Galaxy cluster lenses always have offset mass centroids.
  p.zn.zn[3] = {0.18f, 0.07f};

  p.color.hue_rotation = 0.013f;
  p.color.sat_scale = 1.14f;
  p.color.fade = 0.982f;
  p.color.color_mode = as_int(ColorMode::Alive);               // Alive: reveals multiple image topology
  // Strong angle→hue: each gravitational image (same source, different
  // angle around the lens) gets a distinct color → visible arc structure.
  p.color.spiral_angle_coeff = 0.90f;
  // Moderate radius→hue: Einstein ring appears as concentric color band.
  p.color.spiral_radius_coeff = 0.42f;
  p.color.chroma_breath_amp = 0.16f;    // breathing envelope
  p.color.chroma_breath_freq = 0.12f;   // slow: gravitational timescale
  p.color.chroma_breath_phase = 0.40f;
  p.color.hue_velocity_base = 0.012f;
  p.color.hue_velocity_breath = 0.019f; // breath-dominant → pulsing arcs
  p.color.hue_velocity_drift = 0.003f;
  // Luminance shaping reveals the critical curve (Einstein ring boundary).
  p.color.luma_shape_depth = 0.24f;
  p.color.luma_shape_center = 2.1f;

  p.seed.border_width = 0.010f;         // narrow: let lensing geometry dominate
  p.seed.hue_speed = 0.055f;
  p.seed.alpha = 0.89f;
  p.speed = 0.62f;                      // unhurried: cosmic timescale
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.113f;
  p.zn.zn0_anim_amp = 0.049f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.187f;
  p.zn.zn1_anim_amp = 0.051f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.096f;
  p.seed.pulse_amp = 0.19f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.068f,
                            .amplitude = 0.120f,
                            .offset = 0.420f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.110f,
                            .amplitude = 0.096f,
                            .offset = 0.160f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.079f,
                            .amplitude = 0.030f,
                            .offset = 0.055f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
