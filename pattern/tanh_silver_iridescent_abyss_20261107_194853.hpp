#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh silver iridescent abyss preset.
 *
 * MATHEMATICS: Silver ratio quasi-symmetric fractal tunnel with spatial wave coloring
 *
 * The SILVER RATIO δ_S = 1 + √2 ≈ 2.4142 is the second metallic mean,
 * sister to the golden ratio φ = (1+√5)/2.  Where φ governs pentagons
 * and Fibonacci spirals, δ_S governs octagons and Pell sequences.
 * The silver ratio's defining property: δ_S = 2 + 1/δ_S, so its
 * continued fraction is [2; 2, 2, 2, ...] — the simplest possible
 * continued fraction after [1; 1, 1, 1, ...] = φ.
 *
 * This preset uses 2π(√2 - 1) ≈ 2.603 rad (149.1°) as pre-rotation.
 * √2 - 1 = 1/δ_S is the silver ratio's reciprocal.  As a fraction
 * of a full turn, it is maximally irrational (after the golden angle
 * 2π/φ²), meaning successive rotations fill the circle with near-
 * optimal uniformity — the mathematical basis for phyllotaxis patterns.
 *
 * ENGINE MAPPING:
 *
 *   SILVER QUASI-SYMMETRY = 2π(√2-1) pre-rotation + Tanh + Torus
 *     The 149.1° rotation is close to 150° (= 5π/6, which would give
 *     12-fold sub-symmetry), but the irrational deviation guarantees
 *     NO EXACT SYMMETRY.  The pattern looks almost-12-fold but with
 *     a perpetual asymmetric shimmer — a visual analogue of the silver
 *     ratio's "almost rational but never quite" nature.
 *
 *     Torus reduction wraps seamlessly (no Grid boundaries), so the
 *     tanh saturation walls are the ONLY source of structure.  Each
 *     ±1 wall creates a fractal fold, and the quasi-12-fold rotation
 *     stacks these folds in a spiral that never quite repeats.
 *
 *   HEPTAGONAL COUNTER-ROTATION = -π/7 ≈ -25.71°
 *     π/7 = 25.71° is the internal angle of a regular heptagon.
 *     7 is prime and NOT a Fermat prime, so the regular heptagon is
 *     NOT constructible with compass and straightedge (proven by
 *     Gauss/Wantzel).  The combination of silver-ratio pre-rotation
 *     (quasi-12-fold) with heptagonal post-rotation (7-fold) creates
 *     gcd(~12, 7) = 1 → MAXIMALLY IRREDUCIBLE depth stacking.
 *
 *     The small 25.71° counter-rotation creates gentle spiral descent:
 *     each depth layer is twisted slightly, producing a smooth tunnel
 *     effect rather than the dramatic flips of large counter-rotations.
 *
 *   SPATIAL WAVE IRIDESCENCE = Mode 4 (Spatial Wave)
 *     The spatial wave mode creates a quasi-periodic hue field that
 *     sweeps left-to-right across the frame.  With carefully chosen
 *     frequencies based on the GOLDEN RATIO:
 *       freq1 = φ ≈ 1.618 (primary wave)
 *       freq2 = 1/φ ≈ 0.618 (secondary wave)
 *     The dual-frequency interference creates IRIDESCENCE: hue bands
 *     that shift and shimmer like oil on water or mother-of-pearl.
 *     The ratio freq1/freq2 = φ² ≈ 2.618 is irrational, so the
 *     interference pattern NEVER repeats across the frame.
 *
 *     This fills the Tanh+Torus+mode4 gap — the FIRST tanh tunnel
 *     preset with spatial wave coloring.  Previous Tanh+Torus presets
 *     used modes 1-3; this adds the mesmerizing rainbow-sweep effect
 *     to the infinite fractal tunnel aesthetic.
 *
 * - |zn0| = 1.06: MODERATE expansion for tanh.  Safely bounded by
 *   tanh's ±1 saturation, but strong enough to create dense folding.
 *   This sits between tanh_honeycomb (1.04) and tanh_infinity_mirror
 *   (1.08) — the proven sweet spot for rich fractal structure.
 *
 * - |zn2| = 0.85: STRONG contraction.  Net: 1.06 × 0.85 = 0.901.
 *   Near-critical (close to 1.0): each depth layer retains 90.1% of
 *   its predecessor's scale, creating slow, graceful depth recession
 *   with many visible generations before structure fades.
 *
 * WHAT TO WATCH:
 *   1. IRIDESCENCE: rainbow hue bands sweep across the frame (mode 4)
 *   2. The TUNNEL: Torus wrapping creates seamless infinite descent
 *   3. QUASI-12-FOLD: looks almost symmetric but shimmers irregularly
 *   4. GOLDEN INTERFERENCE: dual-φ waves create oil-slick color patterns
 *   5. DEPTH RECESSION: slow 90.1% shrinkage → many visible layers
 *   6. SILVER vs GOLDEN: compare with golden_ratio preset (φ-based)
 *
 * Pipeline: transform -> reduce -> sample -> color(mode4) -> seed -> blend
 */
inline Pattern tanh_silver_iridescent_abyss() {
  // 2π(√2-1) ≈ 2.6033 rad (149.1°): silver ratio quasi-symmetry
  constexpr float CSV  = -0.8584f;           // cos(2π(√2-1))
  constexpr float SSV  =  0.5130f;           // sin(2π(√2-1))
  // -π/7 ≈ -0.4488 rad (-25.71°): heptagonal counter-rotation
  constexpr float C7   =  0.9010f;           // cos(π/7)
  constexpr float S7N  = -0.4339f;           // sin(-π/7)

  constexpr float PRE  = 1.5240f;  // moderate expansion: tanh bounds → dense folds
  constexpr float POST = 1.3053f;  // strong contraction: graceful depth recession

  Pattern p;
  p.name = "tanh_silver_iridescent_abyss";
  p.inner = InnerFunction::Tanh;             // saturation walls → fractal tunnel cells
  p.reduction = ReductionType::Torus;        // seamless wrap → infinite tunnel

  p.zn.zn[0] = {CSV * PRE, SSV * PRE};
  p.zn.zn[1] = {0.1831f, -0.2058f};           // perturbation: break exact symmetry
  p.zn.zn[2] = {C7 * POST, S7N * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 4 (Spatial Wave) — iridescent hue field ──
  // Mode 4 reuses alive_color slots via parameter mapping:
  //   spiral_angle_coeff → hue_x_slope (spatial hue gradient)
  //   spiral_radius_coeff → hue_y_slope (vertical hue tilt)
  //   chroma_breath_amp → freq1 (primary wave frequency)
  //   chroma_breath_freq → freq2 (secondary wave frequency)
  //   chroma_breath_phase → phase_offset (wave phase)
  p.color.hue_rotation = 0.006f;             // very slow: spatial wave dominates
  p.color.sat_scale = 1.15f;                 // VIVID: iridescent shimmer
  p.color.fade = 0.988f;                     // HIGH: ~83 frame persistence
  p.color.color_mode = as_int(ColorMode::SpatialWave);                    // Spatial Wave: quasi-periodic hue field
  p.color.spiral_angle_coeff = 0.40f;        // hue_x_slope: moderate spatial gradient
  p.color.spiral_radius_coeff = 0.15f;       // hue_y_slope: gentle vertical tilt
  // GOLDEN RATIO DUAL-WAVE IRIDESCENCE
  p.color.chroma_breath_amp = 1.618f;        // freq1 = φ (golden ratio primary wave)
  p.color.chroma_breath_freq = 0.618f;       // freq2 = 1/φ (golden reciprocal secondary)
  p.color.chroma_breath_phase = 1.2f;        // phase offset: iridescent band positioning
  // Hue velocity: gentle drift for temporal shimmer
  p.color.hue_velocity_base = 0.12f;         // base velocity: slow iridescent drift
  p.color.hue_velocity_breath = 0.08f;       // breathing modulation of velocity
  p.color.hue_velocity_drift = 0.04f;        // spatial drift component
  // Luminance: bright tunnel walls, dark depths → depth contrast
  p.color.luma_shape_depth = 0.20f;          // moderate: tunnel wall contrast
  p.color.luma_shape_center = 1.8f;

  // ── Seed: minimal — tunnel self-generates from tanh folding ──
  p.seed.border_width = 0.091f;              // subtle: tunnel depth dominates
  p.seed.hue_speed = 0.115f;                  // slow: contemplative
  p.seed.alpha = 0.92f;

  p.speed = 0.48f;                           // slow-moderate: iridescent shimmer pace
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.188f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.179f;
  p.zn.zn1_anim_amp = 0.040f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.084f;
  p.seed.pulse_amp = 0.16f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.089f,
                            .amplitude = 0.120f,
                            .offset = 0.150f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.069f,
                            .amplitude = 0.120f,
                            .offset = 1.618f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.059f,
                            .amplitude = 0.040f,
                            .offset = 0.115f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
