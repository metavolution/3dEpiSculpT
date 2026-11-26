#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Exponential hendecagonal vault abyss preset.
 *
 * MATHEMATICS: 11-fold architectural tunnel with kaleidoscopic depth coloring
 *
 * The complex exponential e^z maps vertical lines to concentric circles
 * and horizontal lines to radial rays.  With Grid reduction, these become
 * visible ARCHES (circle segments at tile boundaries) and COLUMNS (ray
 * segments).  The combination creates an infinite architectural vault
 * that recedes into fractal depth — proven by gothic_cathedral.
 *
 * This preset pushes the cathedral concept further by using 11-FOLD
 * SYMMETRY (hendecagonal), which creates a vault with 11 visible spokes.
 * 11 is prime: it has NO sub-symmetries, so the vault cannot decompose
 * into simpler repeating units.  The 11 sectors remain individually
 * distinct at every depth level, creating a mandala-within-mandala
 * structure that is inherently MORE complex than any composite-order
 * symmetry (6, 8, 10, 12, ...).
 *
 * ENGINE MAPPING:
 *
 *   11-FOLD VAULT = 2π/11 pre-rotation + Exp + Grid
 *     Each iteration rotates the exponential map by 32.73°, and Grid
 *     cuts the result into discrete arches.  After 11 iterations: one
 *     full revolution + 11 generations of exponential folding = a
 *     complete hendecagonal vault mandala.
 *
 *     11 is the smallest prime NOT related to crystallographic lattices
 *     (2,3,4,6 are lattice-compatible; 5,7 appear in quasicrystals).
 *     11-fold patterns occupy a mathematical no-man's-land: too complex
 *     for quasicrystal theory, too regular for chaos.  This makes them
 *     visually UNIQUE — neither crystalline nor amorphous.
 *
 *   NEAR-PERPENDICULAR COUNTER-ROTATION = -√3 ≈ -99.2°
 *     √3 radians is ~99°, close to a right angle.  This means each
 *     successive depth layer is rotated nearly perpendicular to the
 *     previous one.  The visual effect: the vault appears to TWIST
 *     as it descends, like a spiral staircase where each landing is
 *     turned 99° from the last.
 *
 *     √3 is irrational (algebraic), so the twist NEVER exactly repeats.
 *     After N layers, the total post-rotation is N×√3 mod 2π, which
 *     densely fills [0,2π) — guaranteeing every possible orientation
 *     is eventually visited.
 *
 *   KALEIDOSCOPIC DEPTH = Mode 1 (Spiral)
 *     h_spiral = spiral_a × angle + spiral_r × log(r)
 *     With spiral_angle_coeff = 1.30, each of 11 sectors receives a
 *     distinct hue — creating an 11-COLOR KALEIDOSCOPE.  The log(r)
 *     term adds concentric DEPTH RINGS within each sector: bright
 *     rings where structure accumulates, dark rings in the gaps.
 *
 *     Mode 1 is STATIC (no time-dependent velocity), letting the
 *     architectural geometry be the visual star.  The slow hue_rotation
 *     provides gentle temporal animation.
 *
 *     This fills the Exp+Grid+mode1 gap — no previous preset combines
 *     exponential vaults with spiral kaleidoscopic coloring.
 *
 * - |zn0| = 0.76: moderate for Exp.  Lower than cathedral (0.82) for
 *   deeper structural definition — the vault ribs are sharper because
 *   the exponential doesn't overwhelm the Grid boundaries.
 *
 * - |zn2| = 0.86: mild contraction.  Net: 0.76 × 0.86 = 0.654.
 *   The low product creates VERY DEEP accumulation — structure builds
 *   slowly over many frames, each layer adding new 11-fold detail.
 *
 * WHAT TO WATCH:
 *   1. COUNT the 11 colored vault sectors (each a distinct hue)
 *   2. The ARCHES: Grid boundaries become visible circular ribs
 *   3. The TWIST: each depth layer is rotated ~99° (√3 rad) from the last
 *   4. DEPTH RINGS: concentric hue bands within each sector (log_r)
 *   5. The vault NEVER repeats — 11-fold × √3 = maximal complexity
 *
 * Pipeline: transform -> reduce -> sample -> color(mode1) -> seed -> blend
 */
inline Pattern exp_hendecagonal_vault_abyss() {
  // 2π/11 ≈ 0.5712 rad (32.73°): prime hendecagonal symmetry
  constexpr float C11  =  0.8413f;           // cos(2π/11)
  constexpr float S11  =  0.5406f;           // sin(2π/11)
  // -√3 ≈ -1.7321 rad (-99.2°): near-perpendicular counter-rotation
  constexpr float CR3  = -0.1606f;           // cos(√3)
  constexpr float SR3N = -0.9870f;           // sin(-√3)

  constexpr float PRE  = 0.76f;  // moderate: sharp vault ribs
  constexpr float POST = 0.86f;  // mild contraction: deep accumulation

  Pattern p;
  p.name = "exp_hendecagonal_vault_abyss";
  p.inner = InnerFunction::Exp;             // vertical→arches, horizontal→columns
  p.reduction = ReductionType::Grid;        // architectural ribs at tile boundaries

  p.zn.zn[0] = {C11 * PRE, S11 * PRE};
  p.zn.zn[1] = {0.0f, 0.004f};             // tiny imaginary offset (break degeneracy)
  p.zn.zn[2] = {CR3 * POST, SR3N * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 1 (Spiral) — 11-sector kaleidoscopic vault ──
  p.color.hue_rotation = 0.008f;            // very slow: vault geometry dominates
  p.color.sat_scale = 1.14f;                // vivid: stained glass sectors
  p.color.fade = 0.988f;                    // HIGH: ~83 frame persistence, deep vault
  p.color.color_mode = as_int(ColorMode::Spiral);                   // Spiral: 11 sectors → 11 distinct hues
  p.color.spiral_angle_coeff = 1.30f;       // STRONG: all 11 sectors visually distinct
  p.color.spiral_radius_coeff = 0.50f;      // strong: concentric vault depth rings
  // Mode 1 does not use breathing or hue velocity params
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: bright arch peaks, dark rib shadows → vault depth contrast
  p.color.luma_shape_depth = 0.22f;         // moderate: arch/shadow contrast
  p.color.luma_shape_center = 1.9f;

  // ── Seed: minimal — vault self-generates from exponential feedback ──
  p.seed.border_width = 0.011f;             // minimal: vault depth dominates
  p.seed.hue_speed = 0.05f;                 // slow: contemplative
  p.seed.alpha = 0.88f;

  p.speed = 0.44f;                          // very slow: architectural meditation
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.112f;
  p.zn.zn0_anim_amp = 0.036f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.168f;
  p.zn.zn1_anim_amp = 0.050f;
  p.zn.zn1_anim_k = 6.0f;
  p.seed.pulse_freq = 0.100f;
  p.seed.pulse_amp = 0.15f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.092f,
                            .amplitude = 0.140f,
                            .offset = 0.500f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.079f,
                            .amplitude = 0.050f,
                            .offset = 0.000f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.056f,
                            .amplitude = 0.030f,
                            .offset = 0.050f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
