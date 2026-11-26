#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh Basel–Euler abyss preset.
 *
 * MATHEMATICS: THE BASEL PROBLEM — EULER'S π²/6 TRIUMPH
 *
 * In 1735, the 28-year-old Leonhard Euler solved the BASEL PROBLEM,
 * one of the great challenges of 18th-century mathematics:
 *
 *   1 + 1/4 + 1/9 + 1/16 + 1/25 + ... = Σ_{n=1}^∞ 1/n² = π²/6
 *
 * Posed by Pietro Mengoli in 1650 and attempted unsuccessfully by the
 * Bernoulli brothers for decades, this result was a SENSATION.  That a
 * sum of reciprocal SQUARES of integers equals a rational multiple of
 * π² — connecting discrete arithmetic to continuous geometry — was
 * deeply unexpected and established Euler's reputation.
 *
 * The result connects to:
 *   - The RIEMANN ZETA FUNCTION: ζ(2) = π²/6 (the simplest even ζ value)
 *   - FOURIER ANALYSIS: Parseval's theorem applied to f(x) = x gives ζ(2)
 *   - PROBABILITY: P(two random integers are coprime) = 6/π² = 1/ζ(2)
 *   - The SPECTRAL THEORY of the Laplacian on the flat torus
 *
 * ENGINE MAPPING TO THE BASEL SUM:
 *
 *   THE FRACTAL WEB = Tanh inner function
 *     tanh(z) creates fractal filaments from its ±1 saturation walls.
 *     With |zn0| = 1.06 > 1, each iteration pushes past the walls,
 *     creating the expand → saturate → fold cycle that builds the dense
 *     fractal web.  The web's branching structure IS a visualization of
 *     how the Basel sum converges: each depth layer adds one more term,
 *     and the web's density shows how 1/n² contributes less and less
 *     (but the total keeps growing toward π²/6).
 *
 *   THE INFINITE TUNNEL = Torus reduction
 *     Torus wraps the plane seamlessly, creating an infinite tunnel.
 *     Each depth layer is one more term of the Basel sum.  Looking into
 *     the tunnel IS looking along the sequence of partial sums
 *     S_n = Σ_{k=1}^n 1/k² → π²/6.
 *
 *   THE NEAR-RIGHT-ANGLE = π²/6 ≈ 94.2° pre-rotation
 *     π²/6 ≈ 1.6449 radians ≈ 94.2°.  This is TANTALIZINGLY CLOSE to
 *     π/2 = 90° — just 4.2° past a right angle.  This near-miss creates
 *     QUASI-4-FOLD SYMMETRY with a slow irrational drift:
 *
 *       4 × 94.2° = 376.8° = 360° + 16.8°
 *
 *     Every 4 iterations, the pattern overshoots one full revolution by
 *     16.8°.  After 360/16.8 ≈ 21.4 groups of 4 (≈ 86 iterations), the
 *     extra rotation completes one full turn.  This creates a LONG-PERIOD
 *     QUASI-PERIODICITY: the 4-fold pattern slowly rotates, never exactly
 *     repeating, producing a hypnotic spiraling drift.
 *
 *     The near-miss with π/2 is the VISUAL ANALOGUE of the Basel surprise:
 *     π²/6 is "almost" π/2 × π/3, but not exactly — the connection between
 *     discrete sums and π is close but transcendental.
 *
 *   THE TRANSCENDENTAL DEPTH = -ln(π) ≈ -65.6° post-rotation
 *     ln(π) ≈ 1.1447 — whether this number is IRRATIONAL is an OPEN
 *     PROBLEM in number theory.  It is unknown whether ln(π) is even
 *     irrational, let alone transcendental.  (By contrast, π itself is
 *     transcendental, and e^π = Gelfond's constant is transcendental by
 *     Gelfond–Schneider.)
 *
 *     As a rotation: 360°/65.6° ≈ 5.49 → quasi-5.5-fold depth twist.
 *     This falls exactly between 5-fold and 6-fold — maximally
 *     incommensurate with both, producing rich interference with the
 *     quasi-4-fold azimuthal pattern.
 *
 *     The mathematical irony: we use an angle whose irrationality is
 *     UNKNOWN to create a visual pattern whose quasi-periodicity depends
 *     on that very irrationality.  If ln(π) were rational, the depth
 *     pattern would eventually repeat exactly.  That we don't know
 *     means WE DON'T KNOW if the tunnel ever repeats.
 *
 *   CLASSIC MODE = eternal mathematical truth
 *     Mode 0 presents the fractal web without temporal effects — fitting
 *     for a theorem that is eternally true.  ζ(2) = π²/6 was true before
 *     Euler proved it and will remain true forever.  The slowly drifting
 *     color shows the quasi-4-fold structure, and the deep persistence
 *     reveals hundreds of fractal generations — the sum converging.
 *
 * WHAT TO WATCH:
 *   1. QUASI-4-FOLD: near-right-angle rotation creates 4-fold near-symmetry
 *   2. SLOW DRIFT: the 4.2° excess accumulates, rotating the pattern
 *   3. DEPTH: fade=0.991 + product=0.912 → ~100+ visible generations
 *   4. WEB DENSITY: tanh ±1 folding creates dense branching filaments
 *   5. TUNNEL: Torus seamless wrapping → infinite depth perspective
 *   6. PURE GEOMETRY: Classic mode = the eternal theorem, unmodulated
 *   7. OPEN QUESTION: is the depth pattern truly aperiodic? (ln(π)!)
 *
 * Pipeline: transform -> reduce -> sample -> color(mode0) -> seed -> blend
 */
inline Pattern tanh_basel_euler_abyss() {
  // π²/6 ≈ 1.6449 rad (94.2°): ζ(2) = the Basel problem solution
  // Just 4.2° past π/2: creates quasi-4-fold with slow irrational drift
  // cos(π/2 + x) = -sin(x), sin(π/2 + x) = cos(x), where x = 0.0741
  constexpr float CB   = -0.0741f;           // cos(π²/6) ≈ -sin(4.2°)
  constexpr float SB   =  0.9973f;           // sin(π²/6) ≈ cos(4.2°)
  // -ln(π) ≈ -1.1447 rad (-65.6°): irrationality is an OPEN PROBLEM
  // Connects to: Gelfond–Schneider theorem, Schanuel's conjecture
  constexpr float CLP  =  0.4133f;           // cos(ln(π))
  constexpr float SLPN = -0.9106f;           // sin(-ln(π))

  constexpr float PRE  = 1.38f;  // drives tanh past ±1 walls across torus folds
  constexpr float POST = 0.97f;  // preserves boundary visits while maintaining depth

  Pattern p;
  p.name = "tanh_basel_euler_abyss";
  p.inner = InnerFunction::Tanh;             // fractal web from ±1 saturation folding
  p.reduction = ReductionType::Torus;        // seamless tunnel: looking along the sum

  p.zn.zn[0] = {CB * PRE, SB * PRE};
  p.zn.zn[1] = {0.004f, -0.003f};           // perturbation: break exact symmetry
  p.zn.zn[2] = {CLP * POST, SLPN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.048f;
  p.zn.zn0_anim_amp = 0.038f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.035f;
  p.zn.zn1_anim_amp = 0.022f;
  p.zn.zn1_anim_k = 4.0f;

  // ── Color: MODE 0 (Classic) — eternal mathematical truth ──
  p.color.hue_rotation = 0.004f;             // very slow: mathematical contemplation
  p.color.sat_scale = 1.12f;                 // moderate vibrancy
  p.color.fade = 0.991f;                     // extreme: ~112 frame persistence
  p.color.color_mode = as_int(ColorMode::Classic);                    // Classic: pure fractal geometry
  p.color.spiral_angle_coeff = 0.85f;        // strong: shows quasi-4-fold structure
  p.color.spiral_radius_coeff = 0.55f;       // strong: visible depth rings
  // Mode 0 does not use breathing or hue velocity params
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: strong web branch definition
  p.color.luma_shape_depth = 0.28f;          // strong: clear filament visibility
  p.color.luma_shape_center = 2.1f;

  // ── Seed: domain rim feeding the fractal web ──
  p.seed.border_width = 0.075f;              // wide enough for tanh folds to sample
  p.seed.hue_speed = 0.03f;                  // very slow: contemplative
  p.seed.alpha = 0.92f;

  p.speed = 0.35f;                           // slow: mathematical meditation
  return p;
}

} // namespace epi::presets
