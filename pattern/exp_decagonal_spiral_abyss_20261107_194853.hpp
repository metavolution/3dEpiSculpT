#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Exponential decagonal spiral abyss preset.
 *
 * MATHEMATICS: 10-fold depth lens with spiral orientation coloring
 *
 * The exponential function e^z conformally maps the strip {|Im(z)| < π}
 * onto the punctured plane C*.  With Torus reduction (seamless mirrored
 * wrap), the result is a smooth infinite-depth tunnel where:
 *   - Each depth level is a smooth, seamlessly-tiled copy of the pattern
 *   - No visible tile boundaries (unlike Grid) — pure smooth recursion
 *   - The depth "melts" into itself as each layer blends with the next
 *
 * DESIGN:
 *
 *   DECAGONAL SYMMETRY = π/5 (36°) pre-rotation
 *     cos(36°) = φ/2 = (1+√5)/4 — this is one of the most elegant
 *     equations in mathematics.  The 36° angle is the central angle of a
 *     regular decagon and the base angle of the "golden gnomon" triangle.
 *     Combined with Torus's mirrored reflection, the effective visual
 *     symmetry is 10-fold: each 36° sector mirrors into its neighbor,
 *     creating a mandala-like structure.
 *
 *     In Islamic geometric art, 10-fold patterns appear in the Alhambra's
 *     zellige and in Penrose's original aperiodic tilings (which use 36°
 *     and 72° rhombi).  The decagonal symmetry creates a hypnotic depth
 *     lens with no repeating units.
 *
 *   SMOOTH ABYSS = Exp + Torus
 *     Unlike the Grid variants (which create visible faceted structures),
 *     Exp + Torus produces SMOOTH depth recession.  The exponential's
 *     radial amplification creates the sense of zooming into infinity,
 *     while Torus's seamless wrap eliminates all boundaries.  The result
 *     is an endless, smooth-walled abyss with 10-fold symmetry.
 *
 *   SPIRAL DEPTH COLORING = Mode 1 (Spiral)
 *     h_spiral = spiral_a × angle + spiral_r × log(r)
 *     The angle term reveals the 10-fold sector structure as distinct
 *     hues.  The log(r) term creates concentric hue rings at each depth
 *     level — like the growth rings of a tree viewed in cross-section.
 *     Together they produce a spiral staircase of color descending into
 *     the abyss.
 *
 *   IRRATIONAL COUNTER-ROTATION = -√7 radians
 *     √7 ≈ 2.646 rad is algebraic, pi-incommensurate, and far from any
 *     simple rational multiple of π.  Its continued fraction has large
 *     partial quotients, making it "quickly irrational" — successive
 *     depth layers misalign rapidly, filling the angular space densely.
 *     This creates maximum visual depth complexity in the 10-fold sectors.
 *
 * - |zn0| = 0.74: conservative for Exp (the exponential itself provides
 *   strong amplification; conservative pre-scale keeps the depth structure
 *   coherent rather than chaotic).
 * - |zn2| = 0.89: mild contraction.  Net: 0.74 × 0.89 = 0.659.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode1) -> seed -> blend
 */
inline Pattern exp_decagonal_spiral_abyss() {
  // π/5 (36 deg): decagonal — cos(36°) = φ/2 (golden ratio connection)
  constexpr float C5  = 0.8090169944f;   // cos(π/5) = cos(36°) = φ/2
  constexpr float S5  = 0.5877852523f;   // sin(π/5) = sin(36°)
  // -√7 ≈ -2.6458 rad: algebraic, π-incommensurate counter-rotation
  constexpr float CR7  = -0.8795790f;    // cos(√7)
  constexpr float SR7N = -0.4757713f;    // sin(-√7)

  constexpr float PRE  = 0.74f;  // conservative: exp provides strong amplification
  constexpr float POST = 0.89f;  // mild contraction for abyss stability

  Pattern p;
  p.name = "exp_decagonal_spiral_abyss";
  p.inner = InnerFunction::Exp;          // conformal depth → smooth abyss
  p.reduction = ReductionType::Torus;    // seamless → no tile boundaries

  p.zn.zn[0] = {C5 * PRE, S5 * PRE};
  p.zn.zn[1] = {-0.004f, 0.006f};       // tiny perturbation (breaks perfect symmetry)
  p.zn.zn[2] = {CR7 * POST, SR7N * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 1 (Spiral) — depth rings + angular sector hues ──
  p.color.hue_rotation = 0.008f;         // very slow: let spiral color dominate
  p.color.sat_scale = 1.06f;
  p.color.fade = 0.986f;                // HIGH: many depth generations visible
  p.color.color_mode = as_int(ColorMode::Spiral);               // Spiral: 10-fold sector hues + depth rings
  p.color.spiral_angle_coeff = 1.30f;   // STRONG: reveals all 10 sectors distinctly
  p.color.spiral_radius_coeff = 0.50f;  // concentric depth ring layering
  // Mode 1 does not use breathing or hue velocity params
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.22f;     // depth luminance bands
  p.color.luma_shape_center = 1.6f;

  // ── Seed: minimal — abyss self-generates ──
  p.seed.border_width = 0.012f;         // narrow: abyss depth dominates
  p.seed.hue_speed = 0.04f;             // very slow: contemplative
  p.seed.alpha = 0.85f;
  p.speed = 0.40f;                      // very slow: deep contemplative spiral
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.159f;
  p.zn.zn0_anim_amp = 0.041f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.163f;
  p.zn.zn1_anim_amp = 0.044f;
  p.zn.zn1_anim_k = 2.0f;
  p.seed.pulse_freq = 0.116f;
  p.seed.pulse_amp = 0.20f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.095f,
                            .amplitude = 0.140f,
                            .offset = 0.500f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.072f,
                            .amplitude = 0.050f,
                            .offset = 0.000f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.075f,
                            .amplitude = 0.030f,
                            .offset = 0.040f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
