#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Exponential prismatic depth cascade preset.
 *
 * MATHEMATICS: Conformal triangular prism with infinite fractal recession
 *
 * The exponential map e^z = e^x · (cos(y) + i·sin(y)) is the universal
 * covering map of the punctured plane C*.  Under iteration with Grid
 * reduction, it creates infinite-depth tile cascades where:
 *   - Vertical grid lines → CONCENTRIC RINGS (e^z maps Im-lines to circles)
 *   - Horizontal grid lines → RADIAL SPOKES (e^z maps Re-lines to rays)
 *
 * DESIGN:
 *
 *   TRIANGULAR PRISM = 2π/3 (120°) pre-rotation
 *     Pre-rotation at exactly 120° creates strong 3-fold rotational symmetry.
 *     After e^z warps the domain, each Grid tile is seen from three
 *     equidistant angular perspectives simultaneously — like looking down
 *     the axis of a triangular prism.  The 3-fold interacts with Grid's
 *     4-fold (rectangular tile boundaries) to create 12-fold local
 *     micro-symmetries at tile intersections.
 *
 *   INFINITE FRACTAL RECESSION = exp + Grid + high fade
 *     Each iteration pushes structure exponentially outward, but Grid
 *     wraps it back.  The result: each tile contains a miniature copy of
 *     the entire pattern, each copy containing its own miniature copies,
 *     ad infinitum.  With fade = 0.987, approximately 75 generations of
 *     recursive structure remain visible simultaneously.
 *
 *   IRRATIONAL DEPTH STACKING = -√5 radians post-rotation
 *     √5 is algebraic but π-incommensurate.  This means successive depth
 *     layers rotate by an amount that never repeats.  The golden ratio φ
 *     contains √5 (φ = (1+√5)/2), so √5 connects to the densest possible
 *     irrational rotation — each depth layer is maximally misaligned with
 *     every previous layer, filling the angular space uniformly.
 *
 *   PRISMATIC COLOR = Alive mode 3
 *     Mode 3's three-velocity hue model (base + 0.08Hz breath + 0.003Hz drift)
 *     creates the effect of white light splitting through a prism: different
 *     depth layers shift hue at different rates, producing spectral
 *     separation.  The luma shaping creates alternating bright/dark depth
 *     bands — the intensity variation of dispersed light.
 *
 * - |zn0| = 0.80: moderate (Exp already amplifies exponentially, so the
 *   pre-scale must be conservative to keep structure visible).
 * - |zn2| = 0.84: contraction for depth stability.  Net: 0.80 × 0.84 = 0.672.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern exp_prismatic_depth_cascade() {
  // 2π/3 (120 deg): triangular prism symmetry
  constexpr float C23 = -0.5f;           // cos(2π/3)
  constexpr float S23 =  0.8660254038f;  // sin(2π/3) = √3/2
  // -√5 ≈ -2.2361 rad: Fibonacci-related irrational counter-rotation
  constexpr float CR5  = -0.6172815f;    // cos(√5) = -cos(π - √5)
  constexpr float SR5N = -0.7867698f;    // sin(-√5)

  constexpr float PRE  = 0.80f;  // conservative: exp amplifies exponentially
  constexpr float POST = 0.84f;  // contraction for cascade stability

  Pattern p;
  p.name = "exp_prismatic_depth_cascade";
  p.inner = InnerFunction::Exp;          // conformal covering map → depth cascade
  p.reduction = ReductionType::Grid;     // visible tile boundaries = prism facets

  p.zn.zn[0] = {C23 * PRE, S23 * PRE};
  p.zn.zn[1] = {0.0f, 0.005f};          // tiny imaginary offset (breaks degeneracy)
  p.zn.zn[2] = {CR5 * POST, SR5N * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 3 (Alive) — prismatic spectral dispersion ──
  p.color.hue_rotation = 0.013f;
  p.color.sat_scale = 1.12f;            // vivid: prismatic spectral purity
  p.color.fade = 0.987f;                // HIGH: ~75 depth generations visible
  p.color.color_mode = as_int(ColorMode::Alive);               // Alive: 3-velocity prismatic dispersion
  p.color.spiral_angle_coeff = 0.95f;   // strong: reveals 3-fold angular sectors
  p.color.spiral_radius_coeff = 0.55f;  // strong: depth rings through cascade layers
  p.color.chroma_breath_amp = 0.18f;
  p.color.chroma_breath_freq = 0.11f;
  p.color.chroma_breath_phase = 0.32f;
  p.color.hue_velocity_base = 0.016f;   // base dispersion velocity
  p.color.hue_velocity_breath = 0.011f; // spectral breathing
  p.color.hue_velocity_drift = 0.004f;  // slow prismatic rotation
  p.color.luma_shape_depth = 0.22f;     // depth-alternating bright/dark bands
  p.color.luma_shape_center = 2.0f;

  // ── Seed: minimal — depth cascade self-generates ──
  p.seed.border_width = 0.010f;         // narrow: cascade generates its own structure
  p.seed.hue_speed = 0.06f;
  p.seed.alpha = 0.87f;
  p.speed = 0.44f;                      // very slow: contemplate infinite recession
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.144f;
  p.zn.zn0_anim_amp = 0.050f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.136f;
  p.zn.zn1_anim_amp = 0.055f;
  p.zn.zn1_anim_k = 6.0f;
  p.seed.pulse_freq = 0.080f;
  p.seed.pulse_amp = 0.18f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.080f,
                            .amplitude = 0.154f,
                            .offset = 0.550f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.098f,
                            .amplitude = 0.103f,
                            .offset = 0.180f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.087f,
                            .amplitude = 0.030f,
                            .offset = 0.060f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
