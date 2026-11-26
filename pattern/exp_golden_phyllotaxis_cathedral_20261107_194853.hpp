#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Exponential golden phyllotaxis cathedral preset.
 *
 * MATHEMATICS: Nature's most irrational spiral meets architecture's most
 *              irrational counter-rotation — quadratic × cubic incompatibility
 *
 * The GOLDEN ANGLE 2π/φ² ≈ 137.508° is the single most important angle in
 * nature.  It governs how sunflower seeds pack into their heads, how leaves
 * arrange around stems, how pinecone scales spiral, and how petals unfurl.
 * The reason: φ² = φ + 1 makes the golden ratio the NUMBER HARDEST TO
 * APPROXIMATE BY RATIONALS (its continued fraction [1;1,1,1,...] converges
 * the slowest of all irrationals).  As a rotation angle, this means
 * successive golden-angle steps SPREAD OUT MAXIMALLY: no two ever nearly
 * align, so each new element occupies the LARGEST REMAINING GAP.
 *
 * When iterated in the Exp feedback loop, each step creates a new
 * logarithmic arch (Exp maps vertical lines → concentric circles).  The
 * golden angle places each new arch at the position MAXIMALLY DISTANT from
 * all previous arches — filling the angular space with botanical efficiency.
 * After N iterations: N arches arranged in FIBONACCI SPIRALS.  Count the
 * visible spiral arms: they will be consecutive Fibonacci numbers (5, 8,
 * 13, 21...) depending on zoom level, exactly as in a sunflower head.
 *
 * ENGINE MAPPING:
 *
 *   FIBONACCI SPIRAL VAULT = golden angle pre-rotation + Exp + Torus
 *     The 137.508° rotation means it takes φ² ≈ 2.618 iterations per
 *     revolution — an IRRATIONAL number of steps per cycle.  This is the
 *     KEY to the pattern's density: unlike n-fold symmetry (which repeats
 *     after exactly n steps), golden-angle iteration NEVER REPEATS.  Every
 *     single iteration adds genuinely new structure to the spiral.
 *
 *     With Exp (conformal Cartesian→polar map), each iteration creates a
 *     new ARCH in the cathedral vault.  Torus reduction provides seamless
 *     wrap (no Grid facets), so the arches merge smoothly into an infinite
 *     tunnel.  The result: a FIBONACCI CATHEDRAL — an infinite spiral
 *     vault where the arches are arranged by nature's own packing algorithm.
 *
 *     The golden angle's connection to Exp is deep: e^(2πi/φ²) generates
 *     a dense orbit on the unit circle, and Exp's conformal mapping
 *     transforms this dense angular orbit into a dense set of concentric
 *     arches in the visual plane.
 *
 *   PLASTIC CONSTANT COUNTER-ROTATION = -ρ ≈ -75.9°
 *     The PLASTIC CONSTANT ρ ≈ 1.3247 is the unique real root of the
 *     SIMPLEST CUBIC EQUATION with a real root > 1:
 *
 *       ρ³ = ρ + 1    (compare: φ² = φ + 1 for the golden ratio)
 *
 *     Where φ is the simplest QUADRATIC irrational, ρ is the simplest
 *     CUBIC irrational.  It governs the Padovan sequence (1,1,1,2,2,3,4,5,
 *     7,9,12,...) — the cubic analogue of Fibonacci — and its ratio of
 *     successive terms converges to ρ just as Fibonacci converges to φ.
 *
 *     Using φ (quadratic) for the pre-rotation and ρ (cubic) for the
 *     post-rotation creates ALGEBRAICALLY INCOMPATIBLE depth stacking:
 *     the total rotation after N steps is N × golden_angle + N × ρ.
 *     Since φ satisfies x² = x + 1 and ρ satisfies x³ = x + 1, no
 *     polynomial equation relates them — the two irrational contributions
 *     to each layer's orientation are ALGEBRAICALLY INDEPENDENT.  This
 *     guarantees that depth layers are not just aperiodic but MAXIMALLY
 *     non-repeating: even modular arithmetic tricks cannot find near-
 *     alignments.
 *
 *   FIBONACCI SPIRAL COLORING = Mode 1 (Spiral)
 *     h_spiral = spiral_a × angle + spiral_r × log(r)
 *     With very high spiral_angle_coeff (1.60), the golden-angle sectors
 *     receive finely varying hues.  Because the golden angle fills angular
 *     space maximally uniformly, the hue distribution is ALSO maximally
 *     uniform — no two adjacent arches have similar colors.  The log(r)
 *     term creates DEPTH RINGS that reveal the logarithmic nature of the
 *     Exp mapping.
 *
 *     The visual effect: a Fibonacci spiral staircase of smoothly varying
 *     rainbow hues, descending into infinite depth.  Count the color
 *     spiral arms — they will be Fibonacci numbers.
 *
 * - |zn0| = 0.86: HIGH for Exp (typical range 0.72-0.82).  The aggressive
 *   expansion creates dense arch structure; Torus wrap prevents blowup.
 *   This is the highest |zn0| of any Exp+Torus preset, pushing the
 *   exponential mapping into its densest regime.
 *
 * - |zn2| = 0.94: VERY MILD contraction.  Net: 0.86 × 0.94 = 0.808.
 *   This is the HIGHEST PRODUCT of any Exp preset — each depth layer
 *   retains 80.8% of its predecessor's energy, creating exceptionally
 *   deep accumulation with many more visible arch generations than any
 *   previous exponential tunnel.
 *
 * WHAT TO WATCH:
 *   1. COUNT THE SPIRAL ARMS — they will be Fibonacci numbers (5, 8, 13...)
 *   2. The arches NEVER repeat — golden angle guarantees unique placement
 *   3. DEPTH RINGS reveal logarithmic structure within each spiral arm
 *   4. COLOR UNIFORMITY — no two adjacent arches share similar hues
 *   5. The TWIST: each depth layer rotated by ρ ≈ 75.9° (cubic irrational)
 *   6. PHYLLOTAXIS: the exact same algorithm as sunflower seed placement
 *
 * Pipeline: transform -> reduce -> sample -> color(mode1) -> seed -> blend
 */
inline Pattern exp_golden_phyllotaxis_cathedral() {
  // Golden angle = 2π(2-φ) = 2π(3-√5)/2 ≈ 2.3999 rad (137.51°)
  // The most irrational angle: maximally uniform angular filling
  constexpr float CGA  = -0.7374f;           // cos(golden angle)
  constexpr float SGA  =  0.6755f;           // sin(golden angle)
  // Plastic constant ρ ≈ 1.3247 rad (75.90°): simplest cubic irrational
  // ρ³ = ρ + 1 (the cubic analogue of φ² = φ + 1)
  constexpr float CPC  =  0.2434f;           // cos(ρ)
  constexpr float SPCN = -0.9699f;           // sin(-ρ)

  constexpr float PRE  = 0.86f;  // HIGH for Exp: dense arch structure
  constexpr float POST = 0.94f;  // mild contraction: deepest Exp accumulation

  Pattern p;
  p.name = "exp_golden_phyllotaxis_cathedral";
  p.inner = InnerFunction::Exp;              // Cartesian→polar: arches + columns
  p.reduction = ReductionType::Torus;        // seamless wrap: infinite smooth tunnel

  p.zn.zn[0] = {CGA * PRE, SGA * PRE};
  p.zn.zn[1] = {0.0f, 0.003f};              // tiny perturbation
  p.zn.zn[2] = {CPC * POST, SPCN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 1 (Spiral) — Fibonacci sector hues + depth rings ──
  p.color.hue_rotation = 0.006f;             // very slow: spiral structure dominates
  p.color.sat_scale = 1.12f;                 // vivid: botanical cathedral warmth
  p.color.fade = 0.991f;                     // VERY HIGH: ~111 frame persistence
  p.color.color_mode = as_int(ColorMode::Spiral);                    // Spiral: golden sectors → rainbow spiral
  p.color.spiral_angle_coeff = 1.60f;        // VERY HIGH: maximize Fibonacci arm color
  p.color.spiral_radius_coeff = 0.55f;       // STRONG: deep concentric depth rings
  // Mode 1 does not use breathing or hue velocity params
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: bright arch peaks, dark gaps → Fibonacci contrast
  p.color.luma_shape_depth = 0.24f;          // strong: arch/depth contrast
  p.color.luma_shape_center = 2.1f;

  // ── Seed: minimal — cathedral self-generates from Exp feedback ──
  p.seed.border_width = 0.010f;              // subtle: depth dominates
  p.seed.hue_speed = 0.04f;                  // very slow: botanical meditation
  p.seed.alpha = 0.87f;

  p.speed = 0.38f;                           // very slow: Fibonacci descent meditation
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.117f;
  p.zn.zn0_anim_amp = 0.034f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.203f;
  p.zn.zn1_anim_amp = 0.047f;
  p.zn.zn1_anim_k = 8.0f;
  p.seed.pulse_freq = 0.132f;
  p.seed.pulse_amp = 0.17f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.071f,
                            .amplitude = 0.154f,
                            .offset = 0.550f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.115f,
                            .amplitude = 0.050f,
                            .offset = 0.000f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.083f,
                            .amplitude = 0.030f,
                            .offset = 0.040f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
