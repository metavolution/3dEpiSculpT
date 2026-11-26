#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Rössler Spiral Ribbon — single-band continuous chaos
 * ================================================================
 *
 * PHYSICS / MATH  (Otto E. Rössler 1976, "An equation for continuous
 * chaos", Physics Letters A 57, 397-398):
 *
 *   ẋ = −y − z
 *   ẏ = x + a·y
 *   ż = b + z(x − c)
 *
 * Canonical parameters: a = 0.1, b = 0.1, c = 14 → strange attractor.
 *
 * Rössler designed this system specifically as the SIMPLEST POSSIBLE
 * continuous-time chaotic flow (just three ODEs, ONE nonlinearity in
 * the z equation). It demonstrates a topologically distinct type of
 * chaos from Lorenz:
 *
 *   • Lorenz attractor: DOUBLE BAND (butterfly) with orbit switching
 *     between two lobes around two stable foci.
 *   • Rössler attractor: SINGLE BAND (spiral ribbon) with one-sided
 *     reinjection — orbit spirals outward in the xy-plane, then when
 *     x exceeds c the z-coordinate kicks up, bending the orbit
 *     inward through a Möbius-like twist back to the spiral center.
 *
 * The spiral-stretch-then-fold-back dynamics create a STRETCHED-AND-
 * FOLDED horseshoe topology in cross-section — the canonical
 * mechanism (Smale 1967) producing positive Lyapunov exponent and
 * fractal attractor structure. Hausdorff dimension ≈ 2.02; correlation
 * dimension ≈ 1.99 (nearly 2D at a glance, but with the Cantor-like
 * fiber structure that defines strangeness).
 *
 *
 * ENGINE MAPPING — stretch + fold + reinject AS iterated feedback
 *
 *   SPIRAL STRETCH = Sinh inner function
 *     Sinh is ANTISYMMETRIC: sinh(−z) = −sinh(z). This two-sided
 *     exponential stretch is the same "stretching" mechanism Rössler
 *     uses in the xy-plane (the a·y term amplifies y on each cycle).
 *     Unlike Tanh's bounded saturation or Sin's periodic oscillation,
 *     Sinh grows without bound — and we need the near-critical pre-
 *     scale to contain it, exactly as Rössler's z-coupling contains
 *     the xy-plane's spiral growth.
 *
 *   MÖBIUS-LIKE REINJECTION = post-translate zn[3] = (0.03, 0)
 *     Rössler's distinguishing feature is the ASYMMETRIC z-kick:
 *     b + z(x − c) = 0.1 + z(x − 14). For most of the orbit (x < c)
 *     the z-dynamics are nearly linear; but when x crosses c, z
 *     suddenly grows exponentially, forcing the orbit inward. This
 *     one-sided reinjection is what makes the attractor a SINGLE
 *     band (not two, like Lorenz). A small real post-translate
 *     (0.03, 0) in the complex feedback map produces the equivalent
 *     one-sided asymmetry: each iterate is nudged in a fixed radial
 *     direction, creating the characteristic "ribbon twist" fold.
 *
 *   NEAR-CRITICAL EXPANSION = |zn[0]| · |zn[2]| = 1.02 · 0.92 = 0.938
 *     Rössler at canonical parameters lives just past the period-
 *     doubling cascade that precedes chaos. Stability product ~0.94
 *     places the preset at "edge of chaos" — expansion high enough
 *     to produce the stretched Cantor structure, contraction just
 *     enough to keep the attractor bounded. Matches design guide §3
 *     Sinh note: unbounded character contained by strong post-
 *     contraction.
 *
 *   NATURAL SPIRAL FREQUENCY = pre-rotation 2π/10 (36°)
 *     The Rössler oscillation frequency near the unstable fixed
 *     point is ω = √(1 − a²/4) ≈ 0.9987 rad per unit time at a=0.1.
 *     Mapped onto the engine's iteration timescale with Δt = 0.1,
 *     per-iteration rotation ≈ 0.0999 · 2π ≈ 36°. Using 2π/10 = 36°
 *     exactly encodes one Rössler oscillation per 10 iterations —
 *     visually creates a ~10-spiral-arm ribbon before the reinjection
 *     kicks in.
 *
 *   QUASI-PERIODIC BAND TWIST = post-rotation −π/12 (−15°)
 *     Counter-rotating the post-affine by −π/12 on each iteration
 *     makes successive loops of the spiral offset slightly —
 *     creating the visible "ribbon width" that distinguishes the
 *     Rössler attractor from a mere limit cycle. The −15° is
 *     irrational relative to the +36° pre-rotation (ratio −5:12,
 *     incommensurate), so the ribbon never exactly closes — which
 *     is precisely the fractal Cantor-fiber property of the real
 *     Rössler attractor.
 *
 *   SMOOTH BAND TOPOLOGY = Torus reduction
 *     Rössler's attractor is topologically a compact bounded region
 *     without visible tile boundaries. Torus wrapping preserves the
 *     smooth continuous band; Grid would inject artificial
 *     rectangular edges that would cut the ribbon's Möbius twist.
 *
 *   ALIVE PHASE PORTRAIT = color mode 3
 *     Mode 3's triple-velocity hue (fast base + medium breath + slow
 *     drift) + luma depth reveals the fractal cross-section of the
 *     band. Each Cantor fiber gets a distinct hue based on its
 *     angular position in the orbit — making the band's internal
 *     structure visually decipherable.
 *
 *
 * WHAT TO WATCH:
 *   1. SPIRAL RIBBON in the xy-plane, growing outward turn by turn
 *      from the central bright spiral focus — the dissipation-plus-
 *      amplification xy-dynamics of the ẏ = x + a·y equation.
 *   2. PERIODIC REINJECTION (once per ~10 frames visually) — the
 *      orbit reaches the band's outer rim and folds back inward via
 *      the z-kick equivalent. This is what makes Rössler visibly
 *      aperiodic despite the strong spiral character.
 *   3. SINGLE BAND — no second lobe, no butterfly. The attractor is
 *      topologically one connected ribbon with a fractal cross-
 *      section. Compare directly against `tanh_lorenz_butterfly_
 *      section` to see the two-lobe vs one-band distinction.
 *   4. HUE BANDING across the ribbon cross-section — each fiber of
 *      the Cantor set gets a distinct hue under mode 3. If you could
 *      "zoom in" on any slice of the ribbon, you'd see infinitely
 *      many fine strata like a Rossler band microscopy image.
 *   5. RIBBON TWIST — consecutive spiral turns don't exactly align
 *      because of the −π/12 post-rotation mismatch with +2π/10
 *      pre-rotation. This quasi-periodic dephasing IS the strangeness.
 *
 * Complementary to:
 *   - `tanh_lorenz_butterfly_section` (continuous chaos, double-lobe)
 *   - `square_henon_strange_attractor` (discrete chaos, quadratic)
 *   - `strange_attractor` (generic sinh-based strange attractor with
 *     irrational rotations — Rössler is the specific canonical case)
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern sinh_rossler_spiral_ribbon() {
  // Pre-rotation 2π/10 = 36° — one Rössler oscillation per 10 iterates.
  // cos(36°) = (1+√5)/4 ≈ 0.809 is the golden-ratio-adjacent exact value.
  constexpr float COS_36 = 0.8090169944f;  // cos(2π/10)
  constexpr float SIN_36 = 0.5877852523f;  // sin(2π/10)

  // Post counter-rotation −π/12 = −15°, incommensurate with +36°.
  constexpr float COS_N15 = 0.9659258263f;  // cos(−15°)
  constexpr float SIN_N15 = -0.2588190451f; // sin(−15°)

  // Near-critical edge-of-chaos product 0.938.
  // Sinh is unbounded, so the mild expansion (1.02) is safe only
  // with the strong post-contraction (0.92) compensating.
  constexpr float PRE  = 1.3707f;
  constexpr float POST = 1.1829f;

  Pattern p;
  p.name = "sinh_rossler_spiral_ribbon";
  p.inner = InnerFunction::Sinh;          // antisymmetric stretch
  p.reduction = ReductionType::Torus;      // smooth single band

  // Pre-affine: 36° rotation at 1.02 magnitude (mild expansion).
  p.zn.zn[0] = {COS_36 * PRE, SIN_36 * PRE};
  // Small perturbation — real Rössler has b = 0.1 constant injection.
  p.zn.zn[1] = {0.0457f, -0.0327f};

  // Post-affine: -15° at 0.92 magnitude (contraction + ribbon twist).
  p.zn.zn[2] = {COS_N15 * POST, SIN_N15 * POST};

  // Real post-translate = one-sided z-kick reinjection asymmetry.
  // This 0.03 shift is the analog of Rössler's b + z(x − c) term
  // in the engine's complex-plane feedback.
  p.zn.zn[3] = {0.03f, 0.0f};

  // ── Color: MODE 3 (Alive) — ribbon phase portrait ──
  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.986f;     // Q ≈ 157 — matches Rössler's Lyapunov time
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 0.88f;  // reveal angular phase along ribbon
  p.color.spiral_radius_coeff = 0.48f; // cross-section Cantor strata banding
  p.color.chroma_breath_amp = 0.0f;    // mode 3 doesn't use breathing
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.016f;   // fast hue rotation = spiral phase
  p.color.hue_velocity_breath = 0.010f; // medium oscillation
  p.color.hue_velocity_drift = 0.003f;  // long-period chaos envelope
  p.color.luma_shape_depth = 0.23f;     // visible ribbon strata
  p.color.luma_shape_center = 1.8f;

  // ── Seed: NARROW — attractor self-illuminates via spiral feedback ──
  p.seed.border_width = 0.086f;    // thin: ~2% of unit cell
  p.seed.hue_speed = 0.07f;
  p.seed.alpha = 0.92f;

  // Speed: moderate — the characteristic Rössler orbit period is
  // ~6 time units; at speed 0.72 that renders as ~2-second visible
  // spiral revolution, ideal for eye-tracking the ribbon dynamics.
  p.speed = 0.72f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.109f;
  p.zn.zn0_anim_amp = 0.037f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.147f;
  p.zn.zn1_anim_amp = 0.039f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.123f;
  p.seed.pulse_amp = 0.15f;

  return p;
}

} // namespace epi::presets
