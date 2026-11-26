#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Fresnel Edge Diffraction — Cornu-spiral optical fringes
 * ================================================================
 *
 * PHYSICS / MATH  (Augustin-Jean Fresnel 1815/1819; see Born &
 * Wolf, Principles of Optics §8.7):
 *
 * When a plane wave strikes an opaque half-plane (a straight edge),
 * the amplitude just past the geometric shadow boundary is given by
 * the Fresnel integrals:
 *
 *     C(x) = ∫₀ˣ cos(π t²/2) dt
 *     S(x) = ∫₀ˣ sin(π t²/2) dt
 *
 * The complex Fresnel integral F(x) = C(x) + i·S(x) traces out the
 * CORNU SPIRAL — two tightly-wound spirals converging to the
 * asymptotic points (½, ½) and (−½, −½) as x → ±∞. This spiral
 * IS the hodograph of edge diffraction.
 *
 * The transmitted amplitude at transverse position x past a sharp
 * edge is:
 *
 *     U(x) = (1+i)/2 · [F(x) + (1+i)/2]
 *
 * and the intensity I(x) = |U(x)|² oscillates with the characteristic
 * FRESNEL FRINGES: bright-dark bands that compress closer to the
 * geometric shadow, asymptoting to the unobstructed intensity
 * I₀ in the illuminated region and to zero deep in the shadow.
 *
 * The defining mathematical feature: a QUADRATIC PHASE inside the
 * integral, π·t²/2. This is the hallmark of Fresnel diffraction (as
 * opposed to Fraunhofer far-field, which has a linear phase) — it
 * captures the near-field geometry where curvature of the wavefront
 * is still significant.
 *
 * The Cornu spiral has appeared in physics wherever a quadratic
 * phase integral shows up: edge diffraction, transmission-line
 * wave packets, optical fiber dispersion, and (most famously) the
 * impulse response of a quadratic-phase dispersive medium.
 *
 *
 * ENGINE MAPPING — quadratic-phase integrand AS iterated feedback
 *
 *   QUADRATIC PHASE = Square inner function
 *     The core mathematical object of Fresnel diffraction is t²
 *     inside the integrand's phase. Square (z → z²) is the canonical
 *     complex-plane realization of quadratic dependence. Iterated
 *     under feedback, the Square-inner dynamics accumulate phase
 *     contributions that echo the Fresnel integral's structure in
 *     the same way a Julia-set iteration does — a natural match.
 *
 *   CORNU SPIRAL CONVERGENCE = zn[3] = (0.5, 0.5)
 *     The Cornu spiral's asymptotic limit as x → +∞ is EXACTLY the
 *     complex number (½, ½). Encoding this as the post-translate
 *     makes the attractor geometrically center around the Cornu
 *     limit point — so the feedback loop drifts toward the exact
 *     spot where the Fresnel integral converges.
 *
 *   SHARP EDGE SHADOW = Grid reduction
 *     An opaque half-plane creates a RECTILINEAR edge in the
 *     transmitted field. Grid reduction's tile-boundary discontinuity
 *     (unlike Torus's smooth wrap) preserves this edge-like character:
 *     each tile boundary acts as a new diffracting edge, producing
 *     the compounded interference pattern seen in multi-edge
 *     arrangements like a periodic grating of slits.
 *
 *   FRINGE INTERFERENCE = color mode 4 (Spatial Wave)
 *     Mode 4's dual-sine interference with irrational spatial
 *     frequencies (√2, √3) is purpose-built for quasi-periodic
 *     bright-dark banding. We use √2 and √3 as wave1 and wave2
 *     spatial frequencies — their incommensurate ratio produces
 *     the non-repeating Fresnel-fringe compression pattern
 *     observed near real edge shadows (fringes get finer closer
 *     to the geometric shadow boundary).
 *
 *   NEAR-CRITICAL SQUARE DYNAMICS = |zn[0]| · |zn[2]| = 0.85 · 0.93
 *     Product 0.790 — well-bounded. Design guide §3 places Square's
 *     sweet spot at |zn[0]| = 0.92–1.00 for richest Julia-family
 *     fractal dynamics; we go slightly below (0.85) to emphasize
 *     the SPATIAL wave fringes over the self-similar fractal
 *     substructure, which would dominate at the Mandelbrot cusp.
 *     The result is a Fresnel-fringe-first aesthetic with a faint
 *     Julia-like textural undertone.
 *
 *   SLIGHT EDGE TILT = pre-rotation π/12 (15°)
 *     Real Fresnel edges aren't always perpendicular to the
 *     propagation direction. A 15° pre-rotation models a slightly
 *     tilted edge — producing the asymmetric fringe pattern seen
 *     in real knife-edge diffraction experiments when the knife
 *     is at a small angle.
 *
 *   IRRATIONAL POST-ROTATION = −π/8 (−22.5°)
 *     Counter-rotating by −π/8 produces a slow precession of the
 *     fringe orientation across iterations, modeling the gradual
 *     rotation seen in a time-varying edge-diffraction experiment
 *     (e.g., a rotating knife edge). The ratio with the 15° pre
 *     is incommensurate, so the fringe pattern never exactly
 *     self-repeats at any depth.
 *
 *
 * WHAT TO WATCH:
 *   1. BRIGHT-DARK FRINGES compressing toward a characteristic
 *      shadow boundary — the classic Fresnel diffraction pattern.
 *      Fringe SPACING decreases geometrically as you approach the
 *      geometric shadow (this is the √2/√3 irrational-ratio
 *      contribution of mode 4).
 *   2. CORNU-SPIRAL-LIKE ATTRACTOR CENTER — the visible bright
 *      point around which the bulk of structure organizes is
 *      offset toward (½, ½), exactly where the complex Fresnel
 *      integral converges asymptotically.
 *   3. QUASI-PERIODIC FRINGE ORIENTATION — the fringe pattern
 *      precesses slowly as iterations progress, driven by the
 *      incommensurate pre/post rotation pair. Never repeats
 *      exactly.
 *   4. SHARP GRID-EDGE ARTIFACTS at unit cell boundaries — these
 *      ARE the diffracting edges in this model, not artifacts to
 *      hide. Each boundary contributes its own fringe field,
 *      compounded with neighbors via the feedback loop. This
 *      matches real multi-edge diffraction (e.g., Young's double
 *      slit) where each slit contributes its own Fresnel fringes.
 *   5. JULIA-LIKE TEXTURAL UNDERTONE at mid-depth — a subtle
 *      fractal quality beneath the primary fringe pattern, from
 *      the Square inner's Mandelbrot-family dynamics at |zn[0]|
 *      below the critical threshold. Look for mirror-lobed
 *      substructure at around log_r = 2.
 *
 * Complementary to:
 *   - `cos_fabry_perot_airy_resonance` (optics — cavity resonance
 *     fringes, different physics: Fabry-Perot is MULTIPLE-BEAM
 *     interferometry, Fresnel is near-field edge diffraction)
 *
 * Pipeline: transform -> reduce -> sample -> color(mode4) -> seed -> blend
 */
inline Pattern square_fresnel_edge_diffraction() {
  // Pre-rotation π/12 = 15° — slight edge tilt.
  constexpr float COS_15 = 0.9659258263f;  // cos(π/12)
  constexpr float SIN_15 = 0.2588190451f;  // sin(π/12)

  // Post counter-rotation −π/8 = −22.5° — fringe precession.
  constexpr float COS_N22_5 = 0.9238795325f;   // cos(−π/8)
  constexpr float SIN_N22_5 = -0.3826834324f;  // sin(−π/8)

  // Near-critical Square dynamics (below the Mandelbrot cusp |zn[0]|=1
  // so fringe aesthetic dominates over self-similar fractal substructure).
  constexpr float PRE  = 0.85f;
  constexpr float POST = 0.93f;
  // Stability product: 0.85 · 0.93 = 0.790 (well-bounded).

  // Incommensurate spatial-wave frequencies for quasi-periodic fringe
  // compression matching the Fresnel √(λz)-geometric-scaling character.
  constexpr float SQRT2 = 1.4142135624f;
  constexpr float SQRT3 = 1.7320508076f;

  Pattern p;
  p.name = "square_fresnel_edge_diffraction";
  p.inner = InnerFunction::Square;        // z² = quadratic-phase core
  p.reduction = ReductionType::Grid;      // rectilinear edge shadows

  // Pre-affine: 15° edge tilt at 0.85 magnitude
  p.zn.zn[0] = {COS_15 * PRE, SIN_15 * PRE};
  p.zn.zn[1] = {0.006f, -0.004f};         // small wavefront irregularity

  // Post-affine: -22.5° fringe-precession rotation at 0.93 magnitude
  p.zn.zn[2] = {COS_N22_5 * POST, SIN_N22_5 * POST};

  // Cornu spiral asymptotic limit point: exact literal encoding.
  // The Fresnel integral F(∞) = (½, ½) = lim C(∞) + i·S(∞).
  p.zn.zn[3] = {0.5f, 0.5f};

  // ── Color: MODE 4 (Spatial Wave) — fringe interference pattern ──
  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.07f;
  p.color.fade = 0.982f;                  // moderate-high — fringes linger
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.55f;     // hue x-slope: moderate
  p.color.spiral_radius_coeff = SQRT2;    // wave1 spatial freq: √2
  p.color.chroma_breath_amp = 0.24f;      // wave1 amplitude
  p.color.chroma_breath_freq = SQRT3;     // wave2 spatial freq: √3
  p.color.chroma_breath_phase = 0.14f;    // wave2 amplitude
  p.color.hue_velocity_base = 0.018f;     // wave1 temporal (faster = near-field)
  p.color.hue_velocity_breath = 0.011f;   // wave2 temporal (slower = far-field)
  p.color.hue_velocity_drift = 0.005f;    // base rotation
  p.color.luma_shape_depth = 0.28f;       // strong bright-dark fringe contrast
  p.color.luma_shape_center = 2.8f;

  // ── Seed: EDGE = BORDER ──
  p.seed.border_width = 0.030f;           // visible boundary IS the edge
  p.seed.hue_speed = 0.06f;
  p.seed.alpha = 0.90f;
  p.seed.pulse_freq = 0.08f;              // slow illumination variation
  p.seed.pulse_amp = 0.20f;

  // Speed: moderate — fringes need time to stabilize before precession
  // becomes visible; 0.72 matches the contemplative-aesthetic cluster
  // used by other fine-structure optical presets (op_art_moire_quasiflux).
  p.speed = 0.72f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.120f;
  p.zn.zn0_anim_amp = 0.049f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.127f;
  p.zn.zn1_anim_amp = 0.039f;
  p.zn.zn1_anim_k = 6.0f;

  return p;
}

} // namespace epi::presets
