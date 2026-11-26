#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh heptadecagonal breathing web preset.
 *
 * MATHEMATICS: 17-fold Gauss-constructible fractal lattice with breathing
 *
 * In 1796, the 19-year-old Carl Friedrich Gauss proved that the regular
 * heptadecagon (17-gon) is constructible with compass and straightedge —
 * the first new constructible polygon discovered in over 2000 years.
 * The key: 17 = 2^(2^2) + 1 is a FERMAT PRIME (F₂), and Gauss showed
 * that only Fermat-prime-gons are constructible.  This preset is a
 * visual monument to that proof.
 *
 * 17-fold symmetry creates an EXTREMELY DENSE kaleidoscopic pattern:
 * each 2π/17 ≈ 21.18° rotation step is small enough that adjacent
 * sectors nearly overlap, but prime enough that the pattern never
 * simplifies into sub-symmetries.  Combined with tanh's expansion-
 * saturation-fold cycle, this generates one of the densest possible
 * fractal webs: 17 interlocking sector-copies, each containing
 * recursively nested 17-fold sub-patterns.
 *
 * ENGINE MAPPING:
 *
 *   GAUSS CONSTRUCTIBILITY = 2π/17 pre-rotation + tanh + Grid
 *     The rotation angle 2π/17 ≈ 21.18° creates 17 sectors per
 *     revolution.  With |zn0| = 1.10 (strongly expanding), each
 *     iteration pushes structure past tanh's ±1 walls.  Grid tile
 *     boundaries provide a second set of barriers.
 *
 *     The interaction creates a DENSE WEB:
 *       - 17 rotation steps × tanh folding = 17 branches per generation
 *       - Grid 4-fold × 17-fold = gcd(4,17) = 1 → irreducible moire
 *       - After N iterations: up to 17^N web intersection points
 *
 *     Why 17 is special for density: the angular step 21.18° is small
 *     enough that consecutive sectors ALMOST overlap (unlike 5-fold at
 *     72° or 7-fold at 51.4°), creating an interference zone between
 *     each pair of adjacent sectors.  With 17 sectors, there are 17
 *     such interference zones, producing maximum visual complexity.
 *
 *   TRANSCENDENTAL DEPTH UNWINDING = -π²/4 ≈ -141.3° post-rotation
 *     π²/4 = ζ(2)/1.5 is deeply connected to the Basel problem
 *     (∑1/n² = π²/6), placing this angle in the realm of analytic
 *     number theory.  It is TRANSCENDENTAL (by Lindemann's theorem:
 *     if α is algebraic and nonzero, e^(iα) is transcendental; since
 *     π²/4 is irrational but algebraic-over-π, the resulting rotation
 *     is maximally aperiodic with respect to 2π/17).
 *
 *     The 141.3° counter-rotation creates DRAMATIC depth spiral:
 *     each layer appears to flip nearly halfway around before the
 *     next generation of web structure appears.
 *
 *   BREATHING CRYSTAL = Mode 2 (Breathing)
 *     breath = 1 + amp × sin(freq × 2πt + phase × angle)
 *     With strong amplitude (0.35) and slow frequency (0.07), the
 *     entire 17-fold web appears to PULSE: all 17 sectors brighten
 *     and dim together, but with angular phase offset (phase=3.0)
 *     creating a cascading wave of brightness that sweeps through
 *     the sectors like a lighthouse beam.
 *
 *     This fills the Tanh+Grid+mode2 gap — the only breathing-mode
 *     preset with tanh saturation walls visible as Grid facets.
 *     The result: a living crystal lattice that visibly breathes.
 *
 * - |zn0| = 1.10: STRONG expansion.  This is the same expansion as
 *   tanh_honeycomb and kelvin_helmholtz — proven to create rich
 *   fractal depth without stability issues (tanh bounds the growth).
 *
 * - |zn2| = 0.82: STRONG contraction.  Net: 1.10 × 0.82 = 0.902.
 *   The high product means the system is near-critical: each layer
 *   retains 90.2% of its predecessor's scale, creating slow graceful
 *   recession into infinite depth.
 *
 * WHAT TO WATCH:
 *   1. The DENSITY: 17 fine sectors create an extremely intricate web
 *   2. The BREATHING: watch the web pulse — bright → dim → bright
 *   3. The SWEEP: breathing cascades angularly through the sectors
 *   4. The MOIRE: 17-fold × 4-fold Grid creates irreducible interference
 *   5. DEPTH RECESSION: each layer rotated ~141° (π²/4) from the last
 *   6. GAUSS'S GHOST: the 17-fold symmetry itself is the theorem made visible
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern tanh_heptadecagonal_breathing_web() {
  // 2π/17 ≈ 0.3696 rad (21.18°): Fermat prime heptadecagonal symmetry
  constexpr float C17  =  0.9325f;           // cos(2π/17)
  constexpr float S17  =  0.3613f;           // sin(2π/17)
  // -π²/4 ≈ -2.4674 rad (-141.3°): Basel-connected transcendental depth
  constexpr float CPQ  = -0.7812f;           // cos(π²/4)
  constexpr float SPQN = -0.6243f;           // sin(-π²/4)

  constexpr float PRE  = 1.4827f;  // STRONG expansion: tanh bounds → dense web
  constexpr float POST = 1.3500f;  // STRONG contraction: graceful depth recession

  Pattern p;
  p.name = "tanh_heptadecagonal_breathing_web";
  p.inner = InnerFunction::Tanh;            // saturation walls → fractal web cells
  p.reduction = ReductionType::Grid;        // tile boundaries = web intersection walls

  p.zn.zn[0] = {C17 * PRE, S17 * PRE};
  p.zn.zn[1] = {0.2737f, -0.1567f};          // perturbation: break perfect 17-fold
  p.zn.zn[2] = {CPQ * POST, SPQN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 2 (Breathing) — pulsing crystal lattice ──
  p.color.hue_rotation = 0.008f;            // very slow: web structure dominates
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.988f;                    // HIGH: ~83 frame persistence, deep web
  p.color.color_mode = as_int(ColorMode::Breathing);                   // Breathing: living crystal pulsation
  p.color.spiral_angle_coeff = 0.95f;       // strong: reveals heptadecagonal sectors
  p.color.spiral_radius_coeff = 0.45f;      // moderate: depth rings through web layers
  // BREATHING WEB (slow, strong, spatially cascading)
  p.color.chroma_breath_amp = 0.35f;        // STRONG: dramatic web pulsation
  p.color.chroma_breath_freq = 0.07f;       // SLOW: mesmerizing breath cycle
  p.color.chroma_breath_phase = 3.0f;       // STRONG coupling: brightness cascades
  // Mode 2 does not use hue velocity params
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: bright web nodes, dark web gaps → crystal contrast
  p.color.luma_shape_depth = 0.26f;         // strong: web branch luminance contrast
  p.color.luma_shape_center = 2.2f;

  // ── Seed: minimal — web self-generates from tanh folding ──
  p.seed.border_width = 0.079f;             // minimal: fractal web dominates
  p.seed.hue_speed = 0.118f;                 // very slow: contemplative
  p.seed.alpha = 0.92f;

  p.speed = 0.40f;                          // very slow: meditative crystal descent
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.141f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.133f;
  p.zn.zn1_anim_amp = 0.055f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.100f;
  p.seed.pulse_amp = 0.17f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.087f,
                            .amplitude = 0.126f,
                            .offset = 0.450f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.072f,
                            .amplitude = 0.120f,
                            .offset = 0.350f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.058f,
                            .amplitude = 0.041f,
                            .offset = 0.118f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
