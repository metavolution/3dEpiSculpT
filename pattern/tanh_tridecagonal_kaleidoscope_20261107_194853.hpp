#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh tridecagonal kaleidoscope preset.
 *
 * MATHEMATICS: 13-fold quasicrystalline fractal web with spiral coloring
 *
 * 13 is a prime number with extraordinary properties in rotational symmetry:
 *   - No lattice in ANY dimension has 13-fold rotational symmetry
 *   - 13-fold patterns are MAXIMALLY QUASICRYSTALLINE: they can never
 *     reduce to a periodic tiling, no matter how large the unit cell
 *   - The conflict between 13-fold rotation (2π/13 ≈ 27.69°) and the
 *     4-fold Grid boundary creates an IRREDUCIBLE MOIRE: two lattices
 *     whose symmetries share no common divisor (gcd(13,4)=1) producing
 *     perpetual interference at every scale
 *
 * The resulting visual is a fractal web: a dense network of branching
 * filaments that divide the plane into 13 primary sectors, each sector
 * containing a recursively nested copy of the entire pattern.  The
 * spiral color mode maps each sector to a distinct hue, creating a
 * 13-color kaleidoscope where you can COUNT the sectors and trace how
 * each one nests into the next depth level.
 *
 * ENGINE MAPPING:
 *
 *   13-FOLD QUASICRYSTAL = 2π/13 pre-rotation + tanh saturation
 *     The 27.69° rotation means it takes 13 iterations to complete one
 *     revolution.  But each iteration also FOLDS through tanh's ±1 walls,
 *     creating a new generation of structure.  After 13 steps: one full
 *     revolution + 13 generations of fractal folding = a complete
 *     tridecagonal mandala with fractal depth in every sector.
 *
 *     The prime nature of 13 is KEY: since 13 has no divisors, the
 *     13-fold pattern cannot decompose into simpler sub-symmetries.
 *     Compare: 12-fold = 3×4 = 4×3 = 6×2 (many decompositions).
 *     13-fold = 13×1 ONLY.  This means the full complexity of all 13
 *     sectors is present at every scale — no simplification occurs.
 *
 *   FRACTAL WEB = Tanh (|zn0| = 1.12) + Grid
 *     Strong expansion pushes structure past tanh's saturation walls.
 *     Grid boundaries provide a second set of walls.  The interaction:
 *       - tanh walls at Re(z) = ±1 create N-S barriers
 *       - Grid boundaries at z = ±1 create E-W barriers
 *       - The 27.69° rotation CROSSES these barriers at oblique angles
 *     Each barrier crossing creates a BRANCH POINT in the fractal web.
 *     After N iterations: 2^N × 13 branch points, forming a dense
 *     network of filaments — the visual "web" structure.
 *
 *   TRANSCENDENTAL COUNTER-ROTATION = -e ≈ -155.7°
 *     Euler's number e = 2.71828... as a radian angle is TRANSCENDENTAL
 *     (by Lindemann's theorem: if α is algebraic and nonzero, e^α is
 *     transcendental; since i is algebraic, e^(ie) = cos(e)+i·sin(e)
 *     is transcendental).  Combined with the rational pre-rotation
 *     2π/13, the depth stacking is maximally aperiodic: the ratio
 *     (2π/13)/e is irrational and TRANSCENDENTAL, guaranteeing that
 *     no two depth layers ever align.
 *
 *   KALEIDOSCOPIC COLOR = Mode 1 (Spiral)
 *     h_spiral = spiral_a × angle + spiral_r × log(r)
 *     With spiral_angle_coeff = 1.50, the 13 sectors each receive a
 *     distinct hue band, creating a 13-color mandala.  The log(r) term
 *     adds concentric depth rings WITHIN each sector — like the rings
 *     of a target viewed through a kaleidoscope.  The combination
 *     creates a spiral staircase of color descending into the fractal web.
 *
 *     Mode 1 is STATIC (no time-dependent velocity terms), which lets
 *     the fractal geometry be the visual star.  The slowly rotating hue
 *     (hue_rotation=0.007) provides gentle animation without competing
 *     with the structural complexity.
 *
 * - |zn0| = 1.12: STRONG expansion.  Tanh bounds it safely, but the
 *   expand→saturate→fold cycle creates dense internal structure.
 *   This is stronger than tanh_honeycomb (1.04) or tanh_crystal (1.06),
 *   approaching tanh_infinity_mirror (1.08) territory — but with Grid
 *   instead of Torus, so the density manifests as FACETED CELLS rather
 *   than smooth reflections.
 *
 * - |zn2| = 0.80: STRONG contraction.  Net: 1.12 × 0.80 = 0.896 < 1.0.
 *   The aggressive contraction creates rapid depth recession — each
 *   layer is 80% the size of the previous, giving ~30 visible generations
 *   before structure falls below pixel resolution.
 *
 * WHAT TO WATCH:
 *   1. COUNT the 13 sectors — each has a distinct hue (mode 1 angle coloring)
 *   2. Trace a sector INWARD: it contains a smaller 13-fold mandala
 *   3. The web branches at EVERY grid boundary × tanh wall crossing
 *   4. Depth rings create concentric bands within each sector
 *   5. The whole pattern SLOWLY rotates (hue_rotation) but the 13-fold
 *      structure is always visible because spiral color tracks it
 *
 * Pipeline: transform -> reduce -> sample -> color(mode1) -> seed -> blend
 */
inline Pattern tanh_tridecagonal_kaleidoscope() {
  // 2π/13 ≈ 0.4833 rad (27.69°): prime-order tridecagonal symmetry
  constexpr float C13  = 0.8855f;            // cos(2π/13)
  constexpr float S13  = 0.4647f;            // sin(2π/13)
  // -e ≈ -2.718 rad (-155.7°): Euler's transcendental counter-rotation
  constexpr float CEU  = -0.9117f;           // cos(e)
  constexpr float SEUN = -0.4107f;           // sin(-e)

  constexpr float PRE  = 1.12f;  // STRONG expansion: tanh bounds → dense web
  constexpr float POST = 0.80f;  // STRONG contraction: rapid depth recession

  Pattern p;
  p.name = "tanh_tridecagonal_kaleidoscope";
  p.inner = InnerFunction::Tanh;            // saturation walls → fractal web
  p.reduction = ReductionType::Grid;        // tile boundaries = web branch points

  p.zn.zn[0] = {C13 * PRE, S13 * PRE};
  p.zn.zn[1] = {0.005f, -0.007f};          // perturbation: break perfect 13-fold
  p.zn.zn[2] = {CEU * POST, SEUN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 1 (Spiral) — 13-sector kaleidoscopic hue mapping ──
  p.color.hue_rotation = 0.007f;            // very slow: let spiral structure dominate
  p.color.sat_scale = 1.10f;
  p.color.fade = 0.989f;                    // VERY HIGH: ~91 frame persistence, deep web
  p.color.color_mode = as_int(ColorMode::Spiral);                   // Spiral: 13 sectors → 13 distinct hues
  p.color.spiral_angle_coeff = 1.50f;       // VERY STRONG: all 13 sectors distinct
  p.color.spiral_radius_coeff = 0.52f;      // strong: concentric depth rings in each sector
  // Mode 1 does not use breathing or hue velocity params
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: bright web nodes, dark web gaps → fractal contrast
  p.color.luma_shape_depth = 0.22f;         // moderate: web branch contrast
  p.color.luma_shape_center = 1.7f;

  // ── Seed: minimal — fractal web self-generates ──
  p.seed.border_width = 0.008f;             // MINIMAL: web depth dominates
  p.seed.hue_speed = 0.04f;                 // very slow: contemplative
  p.seed.alpha = 0.86f;

  p.speed = 0.38f;                          // very slow: meditative fractal descent
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.188f;
  p.zn.zn0_anim_amp = 0.036f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.182f;
  p.zn.zn1_anim_amp = 0.046f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.079f;
  p.seed.pulse_amp = 0.20f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.095f,
                            .amplitude = 0.146f,
                            .offset = 0.520f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.091f,
                            .amplitude = 0.050f,
                            .offset = 0.000f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.084f,
                            .amplitude = 0.030f,
                            .offset = 0.040f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
