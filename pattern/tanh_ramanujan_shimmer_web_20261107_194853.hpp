#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh Ramanujan shimmer web preset.
 *
 * MATHEMATICS: Heegner-number quasi-31-fold fractal web with Apéry depth —
 *              the densest prime symmetry ever attempted
 *
 * 163 is the LARGEST HEEGNER NUMBER — one of exactly nine special integers
 * {1, 2, 3, 7, 11, 19, 43, 67, 163} for which the imaginary quadratic field
 * Q(√-d) has class number 1 (unique factorization).  Its fame comes from
 * the RAMANUJAN CONSTANT:
 *
 *   e^(π√163) = 262537412640768743.999999999999250...
 *
 * This number is almost an integer TO TWELVE DECIMAL PLACES — one of the
 * most astonishing near-misses in all of mathematics.  The near-integrality
 * is not coincidence but a consequence of the j-invariant of the elliptic
 * curve with complex multiplication by Q(√-163), connecting modular forms,
 * class field theory, and the monster group.
 *
 * √163 ≈ 12.7671 radians.  Reduced mod 2π: √163 - 2(2π) ≈ 0.2007 rad
 * ≈ 11.50°.  This creates QUASI-31-FOLD SYMMETRY: 360°/11.50° ≈ 31.3
 * iterations per revolution.  31 is a MERSENNE PRIME (2⁵ - 1), connecting
 * this preset to perfect numbers, error-correcting codes, and the Lucas-
 * Lehmer primality test.  The irrational deviation from exact 31-fold
 * means the pattern SHIMMERS — quasi-periodic, never exactly repeating,
 * with 31 barely-distinguishable ultra-fine sectors creating a dense
 * fractal web of unprecedented fineness.
 *
 * ENGINE MAPPING:
 *
 *   HEEGNER WEB = √163 pre-rotation + Tanh + Grid
 *     The 11.50° step is the SMALLEST angular rotation of any preset.
 *     It creates the MOST SECTORS per revolution: ~31 ultra-fine wedges.
 *     Each wedge is only 11.5° wide — at the edge of visual distinguishability,
 *     creating maximum tension between order and chaos.
 *
 *     With Tanh folding (|zn0| = 1.14, the HIGHEST of any Tanh preset),
 *     each iteration pushes structure aggressively past tanh's ±1 walls.
 *     Combined with Grid's tile boundaries, each iteration creates ~62
 *     branch points (31 angular × 2 wall types).  After just 3 iterations:
 *     ~238,000 total branch points.  This is EXPONENTIAL DENSITY — a
 *     fractal web so fine that individual filaments approach pixel scale.
 *
 *     The interaction: 31-fold rotation × 4-fold Grid = gcd(31,4) = 1
 *     (since 31 is prime and not ≡ 0 mod 4).  This creates a MAXIMALLY
 *     IRREDUCIBLE MOIRÉ: the two lattice symmetries share absolutely no
 *     common structure, producing perpetual interference at every scale.
 *
 *   APÉRY COUNTER-ROTATION = -ζ(3) ≈ -68.9°
 *     APÉRY'S CONSTANT ζ(3) = 1 + 1/8 + 1/27 + 1/64 + ... ≈ 1.2021
 *     is the sum of reciprocal cubes.  In 1978, Roger Apéry SHOCKED
 *     the mathematical world by proving ζ(3) is IRRATIONAL.  The proof
 *     was so unexpected that the audience at the Journées Arithmétiques
 *     initially didn't believe it — Henri Cohen had to verify the
 *     identities by computer before the result was accepted.
 *
 *     ζ(3) connects to: Feynman diagrams in quantum field theory,
 *     volumes of hyperbolic 3-manifolds, and the K-theory of integers.
 *     As a rotation angle (68.9°), it creates roughly 5.2 iterations per
 *     depth revolution — creating a QUASI-5-FOLD depth twist that echoes
 *     Penrose tiling symmetry.
 *
 *     The combination: √163 pre-rotation (quasi-31-fold, algebraic) ×
 *     ζ(3) post-rotation (quasi-5-fold, NEITHER ALGEBRAIC NOR KNOWN TO
 *     BE TRANSCENDENTAL — its transcendence is an open problem in number
 *     theory).  This connects two of mathematics' deepest mysteries:
 *     the class number problem (Heegner numbers) and the nature of
 *     zeta values at odd integers.
 *
 *   SPIRAL DIFFRACTIVE SHIMMER = Mode 1 (Spiral)
 *     h_spiral = spiral_a × angle + spiral_r × log(r)
 *     With 31 sectors and moderate spiral_angle_coeff (0.80), the hue
 *     cycles through ~4 complete rainbows per revolution.  Each of the
 *     31 sectors receives a slightly different hue, creating DIFFRACTIVE
 *     SHIMMER: the visual analogue of white light passing through a fine
 *     diffraction grating.  The strong spiral_radius_coeff (0.60)
 *     creates bold depth rings that reveal the near-critical dynamics.
 *
 *     The rainbow-cycling effect is deliberate: with 31 sectors, giving
 *     each a UNIQUE hue would require 31 distinguishable colors (beyond
 *     human perception).  Instead, the 4-cycle repetition creates
 *     INTERFERENCE BANDS where the angular hue periodicity interacts
 *     with the 31-fold structural periodicity, producing visible beats.
 *
 * - |zn0| = 1.14: HIGHEST OF ANY TANH PRESET.  This is the mathematical
 *   frontier: maximum expansion before visual coherence breaks down.
 *   With tanh bounding to ±1, the expand→saturate→fold cycle runs at
 *   peak intensity, creating the densest possible internal branching.
 *
 * - |zn2| = 0.84: STRONG contraction.  Net: 1.14 × 0.84 = 0.958.
 *   NEAR-CRITICAL DYNAMICS: the system retains 95.8% of its energy per
 *   iteration.  Combined with fade = 0.993 (143-frame persistence),
 *   visible structure extends to ~100+ depth generations — an abyss of
 *   fractal detail that approaches the theoretical maximum.
 *
 * WHAT TO WATCH:
 *   1. The SHIMMER: 31 ultra-fine sectors create diffractive iridescence
 *   2. WEB DENSITY: finer filaments than ANY other preset (31-fold × tanh)
 *   3. DEPTH: near-critical dynamics → ~100 visible generations
 *   4. RAINBOW BEATS: 4 hue cycles × 31 sectors = visible interference
 *   5. The web is ALIVE: quasi-31-fold means it perpetually shimmers
 *   6. MOIRÉ: 31-fold × 4-fold Grid creates irreducible interference bands
 *   7. TWO OPEN PROBLEMS: is ζ(3) transcendental? Are there more Heegner numbers?
 *
 * Pipeline: transform -> reduce -> sample -> color(mode1) -> seed -> blend
 */
inline Pattern tanh_ramanujan_shimmer_web() {
  // √163 ≈ 12.7671 rad → mod 2π → 0.2007 rad (11.50°): Heegner quasi-31-fold
  // 163 is the largest Heegner number; e^(π√163) ≈ integer to 12 places
  constexpr float CH   =  0.9799f;           // cos(√163)
  constexpr float SH   =  0.1993f;           // sin(√163)
  // -ζ(3) ≈ -1.2021 rad (-68.9°): Apéry's constant (sum of reciprocal cubes)
  // Proved irrational 1978; transcendence remains an open problem
  constexpr float CAP  =  0.3604f;           // cos(ζ(3))
  constexpr float SAPN = -0.9328f;           // sin(-ζ(3))

  constexpr float PRE  = 1.14f;  // HIGHEST tanh expansion: maximum fractal density
  constexpr float POST = 0.84f;  // strong contraction: near-critical (product 0.958)

  Pattern p;
  p.name = "tanh_ramanujan_shimmer_web";
  p.inner = InnerFunction::Tanh;             // saturation walls → ultra-dense fractal web
  p.reduction = ReductionType::Grid;         // tile boundaries = web intersection scaffold

  p.zn.zn[0] = {CH * PRE, SH * PRE};
  p.zn.zn[1] = {0.003f, -0.004f};           // perturbation: break exact symmetry
  p.zn.zn[2] = {CAP * POST, SAPN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 1 (Spiral) — diffractive shimmer across 31 sectors ──
  p.color.hue_rotation = 0.005f;             // very slow: fractal geometry dominates
  p.color.sat_scale = 1.18f;                 // HIGH: iridescent shimmer vibrancy
  p.color.fade = 0.993f;                     // EXTREME: ~143 frame persistence (deepest)
  p.color.color_mode = as_int(ColorMode::Spiral);                    // Spiral: 31 sectors → diffractive rainbows
  p.color.spiral_angle_coeff = 0.80f;        // moderate: ~4 rainbow cycles per revolution
  p.color.spiral_radius_coeff = 0.60f;       // VERY STRONG: bold depth ring visibility
  // Mode 1 does not use breathing or hue velocity params
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: maximum branch contrast — bright filaments, dark voids
  p.color.luma_shape_depth = 0.28f;          // STRONG: maximum web branch definition
  p.color.luma_shape_center = 2.3f;

  // ── Seed: ultra-minimal — the web IS the visual ──
  p.seed.border_width = 0.007f;              // very subtle: web dominates completely
  p.seed.hue_speed = 0.03f;                  // very slow: contemplative depth
  p.seed.alpha = 0.85f;

  p.speed = 0.32f;                           // VERY slow: the density demands meditation
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.187f;
  p.zn.zn0_anim_amp = 0.044f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.179f;
  p.zn.zn1_anim_amp = 0.045f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.086f;
  p.seed.pulse_amp = 0.21f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.081f,
                            .amplitude = 0.168f,
                            .offset = 0.600f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.096f,
                            .amplitude = 0.050f,
                            .offset = 0.000f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.070f,
                            .amplitude = 0.030f,
                            .offset = 0.030f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
