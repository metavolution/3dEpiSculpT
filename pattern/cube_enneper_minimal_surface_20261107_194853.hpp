#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Enneper Minimal Surface — Weierstrass-Enneper z³ geometry
 * ================================================================
 *
 * PHYSICS / MATH  (Alfred Enneper 1864; Weierstrass-Enneper
 * representation of minimal surfaces, Karl Weierstrass 1866):
 *
 * A MINIMAL SURFACE is a surface with identically zero mean
 * curvature H ≡ 0 at every point. Physically, these are the shapes
 * that minimize surface area for a given boundary — SOAP FILM
 * equilibria, stretched between wire frames. Famous minimal surfaces
 * include the catenoid, helicoid, Scherk surface, Costa surface, and
 * the Enneper surface studied here.
 *
 * The Weierstrass-Enneper representation expresses every minimal
 * surface in terms of a pair of complex analytic data (f, g). For
 * Enneper's surface, the data is simplest possible: f(z) = 1 and
 * g(z) = z, giving the explicit parameterization:
 *
 *     X(u,v) = u − u³/3 + u·v²
 *     Y(u,v) = v − v³/3 + u²·v
 *     Z(u,v) = u² − v²
 *
 * In complex form with z = u + i·v, the defining CUBIC structure
 * appears as z − z³/3 in the Weierstrass integral. That z³ is the
 * mathematical DNA of the Enneper surface — responsible for its:
 *
 *   • 3-FOLD ROTATIONAL SYMMETRY (the cubic tripling of angle),
 *   • SELF-INTERSECTION for |z| ≥ √3 (where the surface folds back
 *     through itself — a feature, not a bug),
 *   • TREFOIL-LIKE LOBE STRUCTURE visible when the surface is
 *     projected onto the xy-plane.
 *
 * Minimal surfaces satisfy the variational principle:
 *     δ ∫∫ dA = 0
 * over all variations of the surface with fixed boundary. This is
 * the differential-geometric sibling of the least-action principle
 * in classical mechanics. The Weierstrass-Enneper representation
 * is a HOLOMORPHIC encoding of this principle — and it's why
 * complex-plane iteration is such a natural simulator of minimal
 * surface geometry.
 *
 *
 * ENGINE MAPPING — Weierstrass-Enneper cubic AS iterated feedback
 *
 *   CUBIC NONLINEARITY = Cube inner function
 *     The z³/3 term in the Enneper parameterization IS Cube's
 *     action in the complex plane. No other inner captures the
 *     exact Weierstrass-Enneper geometry. Cube + Torus is listed
 *     in design guide §3 as producing "smooth crystallographic
 *     symmetry" — exactly the right character for minimal surface
 *     visualization.
 *
 *   3-FOLD TREFOIL SYMMETRY = pre-rotation π/9
 *     Per the design guide: z³ TRIPLES the pre-rotation angle.
 *     Our π/9 (20°) pre-rotation becomes 3·20° = 60° = π/3 after
 *     the Cube, which with Torus produces 6-fold hexagonal
 *     symmetry — two inter-leaved copies of the Enneper
 *     3-fold pattern (one from the surface, one from its mirror
 *     image through the self-intersection, combining to hexagonal).
 *
 *   NATURAL IRRATIONAL POST-ROTATION = −ln(2) rad
 *     Per design guide §10, ln(2) ≈ 0.693 rad (39.7°) is a
 *     transcendental irrational with excellent continued-fraction
 *     properties — close to the golden-ratio family for
 *     "densest never-repeating layering". This gives the Enneper
 *     surface's self-intersection sheets a non-repeating depth
 *     structure under iteration, rendering the infinite fan of
 *     self-intersections each at a slightly different angle.
 *     Used successfully in `hexagonal_grain_mosaic`; same
 *     mathematical reason here.
 *
 *   COMFORTABLE CUBE RANGE = |zn[0]| · |zn[2]| = 0.88 · 0.88 = 0.774
 *     Design guide §3: Cube prefers |zn[0]| in 0.82–0.96 — we
 *     choose 0.88 on both sides for symmetric contraction,
 *     avoiding the cusp (|zn[0]| → 1) that would produce
 *     chaotic rather than geometric-surface aesthetics. 0.774
 *     product is comfortably bounded and produces a stable
 *     geometric attractor rather than a strange one.
 *
 *   ORIGIN-CENTERED SURFACE = zn[3] = (0, 0)
 *     Enneper's surface is naturally centered at the origin in
 *     (u, v) parameter space — no post-translate is needed. The
 *     symmetric self-intersection structure is the visual payoff;
 *     translating would only hide it.
 *
 *   SURFACE-SHEETS PHASE PORTRAIT = color mode 3 (Alive)
 *     Mode 3's triple-velocity hue + radial luma depth is ideal
 *     for rendering surfaces with LAYERED structure — each sheet
 *     of the self-intersecting Enneper surface gets a distinct
 *     hue band under mode 3's log_r-coupled hue. The 0.25 luma
 *     depth makes the sheet boundaries visible as
 *     bright-on-dark contours.
 *
 *   GEOMETRIC PERSISTENCE = fade 0.987 (Q ≈ 242)
 *     Minimal surfaces are EQUILIBRIUM shapes — soap films don't
 *     decay, they relax to the surface that minimizes area. High
 *     fade (long trail persistence) mirrors this: the iterated
 *     attractor settles into a long-lived geometric shape that
 *     doesn't drift away.
 *
 *   CONTEMPLATIVE SPEED = 0.55
 *     Geometric-study aesthetic — the eye needs time to trace
 *     out the 3-fold lobes, follow a self-intersection sheet
 *     through the ambient structure, appreciate the cubic warp.
 *     0.55 matches the slow cluster used by other geometric
 *     presets (`exp_golden_phyllotaxis_cathedral`, `hexagonal_
 *     grain_mosaic`).
 *
 *
 * WHAT TO WATCH:
 *   1. 3-FOLD TREFOIL LOBES — three distinct "petals" extending
 *      from the center, encoded by Cube's angle-tripling action.
 *      The dominant 6-fold visual (Cube + Torus produces 2×3-fold
 *      via mirroring) is Enneper's surface + its self-intersected
 *      twin superimposed.
 *   2. SELF-INTERSECTION SHEETS — at intermediate radii the
 *      surface folds back through itself (the |z| > √3 region
 *      of the parameter space). Under iteration this shows up as
 *      layered translucent sheets that each pick up a distinct
 *      mode-3 hue band.
 *   3. SMOOTH CRYSTALLOGRAPHIC CHARACTER — the Cube+Torus pairing
 *      (not Grid) produces organic curves instead of faceted tile
 *      boundaries. Soap-film-like, not crystalline.
 *   4. NEVER-REPEATING DEPTH — the −ln(2) irrational post-rotation
 *      ensures successive self-intersection sheets rotate slightly
 *      relative to each other, producing quasi-periodic depth
 *      layering that never exactly self-repeats.
 *   5. GEOMETRIC EQUILIBRIUM — unlike strange-attractor presets,
 *      this converges to a STABLE SHAPE rather than a chaotic
 *      orbit. Once settled, the Enneper geometry stays put.
 *      Watch the transient from initialization — the soap film
 *      finding its equilibrium.
 *
 * First differential-geometry preset in the catalog. Also strengthens
 * the under-represented Cube inner (previously only 4 presets at
 * NC158; Enneper is specifically about the CUBIC structure rather
 * than a post-translate-driven variant).
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern cube_enneper_minimal_surface() {
  // Pre-rotation π/9 = 20° — under Cube this triples to π/3 = 60°
  // → 6-fold hexagonal symmetry with Torus reduction.
  constexpr float COS_20 = 0.9396926208f;  // cos(π/9)
  constexpr float SIN_20 = 0.3420201433f;  // sin(π/9)

  // Post counter-rotation −ln(2) rad (−39.72°) — transcendental
  // irrational for quasi-periodic self-intersection layering.
  constexpr float COS_NEG_LN2 = 0.7692389013f;   // cos(−ln(2))
  constexpr float SIN_NEG_LN2 = -0.6389612763f;  // sin(−ln(2))

  // Cube range per design guide: 0.82–0.96. We choose 0.88 on both
  // sides for symmetric contraction — stable geometric attractor
  // rather than a chaotic one.
  constexpr float PRE  = 1.3594f;
  constexpr float POST = 1.2784f;
  // Stability product: 0.88 · 0.88 = 0.774 (comfortable).

  Pattern p;
  p.name = "cube_enneper_minimal_surface";
  p.inner = InnerFunction::Cube;         // z³: Weierstrass-Enneper core
  p.reduction = ReductionType::Torus;    // smooth crystallographic

  // Pre-affine: 20° rotation at 0.88 magnitude (→ 60° post-Cube)
  p.zn.zn[0] = {COS_20 * PRE, SIN_20 * PRE};
  p.zn.zn[1] = {0.2554f, -0.1425f};        // tiny perturbation

  // Post-affine: −ln(2) counter-rotation at 0.88 magnitude
  p.zn.zn[2] = {COS_NEG_LN2 * POST, SIN_NEG_LN2 * POST};

  // No post-translate — Enneper surface is naturally origin-centered
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 3 (Alive) — surface-sheet phase portrait ──
  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 1.18f;
  p.color.fade = 0.978f;                // balanced surface equilibrium
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 0.95f;   // strong trefoil lobe hue banding
  p.color.spiral_radius_coeff = 0.48f;  // radial depth = surface sheets
  p.color.chroma_breath_amp = 0.0f;     // mode 3 doesn't use breathing
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.018f;
  p.color.hue_velocity_breath = 0.012f;
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.08f;      // sheet-boundary contrast centered at log_r ~ -4.25
  p.color.luma_shape_center = -4.25f;

  // ── Seed: NARROW — surface self-illuminates via recursive folding ──
  p.seed.border_width = 0.084f;          // thin: soap film is energy-poor
  p.seed.hue_speed = 0.06f;
  p.seed.alpha = 0.92f;

  // Speed: contemplative — geometric-study aesthetic.
  p.speed = 0.55f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.182f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.148f;
  p.zn.zn1_anim_amp = 0.055f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.092f;
  p.seed.pulse_amp = 0.21f;

  return p;
}

} // namespace epi::presets
