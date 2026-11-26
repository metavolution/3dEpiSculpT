#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Lorenz Butterfly Section — continuous-time chaos
 * ================================================================
 *
 * PHYSICS / MATH  (E. N. Lorenz 1963, "Deterministic non-periodic
 * flow", J. Atmospheric Sciences 20, 130-141):
 *
 *   ẋ = σ (y − x)
 *   ẏ = x (ρ − z) − y
 *   ż = x y − β z
 *
 * Canonical atmospheric-convection parameters:
 *   σ = 10    (Prandtl number: momentum/thermal diffusivity ratio)
 *   ρ = 28    (Rayleigh number, scaled: ~2× the critical threshold)
 *   β = 8/3   (aspect ratio of convection cells)
 *
 * Lorenz discovered this system while studying a truncated model of
 * atmospheric convection. It was one of the founding observations of
 * modern chaos theory — the first demonstration that a simple
 * DETERMINISTIC system of three ODEs can produce orbits that are:
 *   • globally bounded (live within a compact set),
 *   • aperiodic (never exactly repeat),
 *   • sensitive to initial conditions (Lyapunov exponent ≈ 0.905),
 *   • topologically ergodic (densely explore the strange attractor).
 *
 * The Lorenz attractor has Hausdorff dimension ≈ 2.06 and the iconic
 * BUTTERFLY shape: two lobes around the unstable foci
 * C₁,₂ = (±√(β(ρ−1)), ±√(β(ρ−1)), ρ−1) ≈ (±8.48, ±8.48, 27). Orbits
 * spiral around one focus, then aperiodically SWITCH to the other —
 * the chaotic lobe-switching is what makes the figure-eight shape.
 *
 * This preset renders the attractor's (x, y)-projection section as
 * viewed looking down the z-axis: a two-lobed butterfly where each
 * lobe is a spiraling family of curves filling a Cantor-like cross-
 * section of the attractor's stable manifold.
 *
 *
 * ENGINE MAPPING
 *
 *   BOUNDED CHAOS = Tanh inner function
 *     The Lorenz attractor is GLOBALLY BOUNDED: every trajectory
 *     stays within an ellipsoid of diameter ~50. Tanh has natural
 *     saturation walls at ±1 that mimic this boundedness. Unlike
 *     Sinh (unbounded) or Square (can diverge), Tanh GUARANTEES
 *     orbits stay bounded even with expanding pre-multiply — which
 *     is exactly what we want to mirror Lorenz's stability.
 *
 *   TWO-LOBE STRUCTURE = pre-rotation by arctan(σ/ρ) ≈ 19.65°
 *     The σ/ρ = 10/28 ratio determines the ANGLE at which orbits
 *     slant as they traverse each lobe (the angle between the
 *     unstable eigenvector at the origin and the axis to C₁,₂).
 *     Encoding arctan(10/28) = arctan(0.357) ≈ 0.343 rad as the
 *     pre-rotation angle makes the feedback map's lobe geometry
 *     match the real Lorenz section.
 *
 *   NEAR-CRITICAL EXPANSION = |zn[0]| · |zn[2]| = 1.07 · 0.92 ≈ 0.985
 *     Lorenz at ρ = 28 is just above the critical Hopf bifurcation
 *     at ρ_H = σ(σ+β+3)/(σ−β−1) ≈ 24.74 — the attractor has just
 *     turned strange. Stability product ~0.985 places the preset in
 *     the corresponding "edge of chaos" regime: dense filling of the
 *     attractor, not a simple fixed point, not divergence.
 *
 *   IRRATIONAL COUNTER-ROTATION = -1/φ rad (golden ratio reciprocal)
 *     The two lobes of the Lorenz butterfly are not visited in any
 *     periodic pattern — the lobe-switching is aperiodic. Post-
 *     rotation by -1/φ = -0.618 rad ≈ -35.4° is maximally irrational
 *     (golden-ratio reciprocal has the slowest-converging continued
 *     fraction), so successive iterates through the map never align,
 *     recreating the Lorenz system's ergodic ergodic exploration.
 *
 *   BUTTERFLY CENTROID = post-translate zn[3] = (0.0, 0.24)
 *     Lorenz's two stable foci are at (±8.48, ±8.48) in (x,y). The
 *     midpoint between them is (0, 0); the saddle at origin is
 *     unstable. Shifting the complex-plane origin by a small pure-
 *     imaginary amount (0, 0.24) breaks perfect y-symmetry just
 *     enough to make one lobe slightly dominant at any given time
 *     — matching the Lorenz system's tendency to stay in one lobe
 *     longer than the other between switches.
 *
 *   LYAPUNOV DECAY = fade = 0.975
 *     Lorenz's positive Lyapunov exponent λ₁ ≈ 0.905. The
 *     corresponding per-frame decay is exp(-λ₁ · Δt). At Δt = 0.028
 *     (16× per second at 60 fps / speed=0.7), fade = 0.975 gives a
 *     predictability horizon of ~45 frames (0.75 s) — enough to
 *     visibly trace the orbit arc through one lobe before chaos
 *     decorrelates it from the next.
 *
 *   PHASE PORTRAIT = color mode 3 (Alive)
 *     Mode 3's triple-velocity hue dynamics (fast base + medium
 *     breath + slow drift) + radial luma depth are the canonical
 *     dynamical-system visualizations. Each point on the attractor
 *     gets a hue that encodes its phase-space position, making the
 *     two lobes visibly distinct in color and revealing the Cantor
 *     cross-section as hue banding within each lobe.
 *
 *
 * WHAT TO WATCH:
 *   1. TWO LOBES — the butterfly wings. Orbits spiral outward within
 *      one lobe, reach its rim, and switch to the other lobe at
 *      apparently random times. The switching is deterministic but
 *      sensitive to initial condition.
 *   2. SPIRAL STRUCTURE within each lobe — orbits don't just sit at
 *      the focus; they spiral outward on an unstable manifold, then
 *      fall back in via the stable manifold of the OTHER lobe.
 *   3. HUE BANDING — mode 3's alive-color reveals the Cantor-set
 *      substructure of the attractor's cross-section. Each band is
 *      a different "sheet" of the infinitely-layered stable manifold.
 *   4. ASYMMETRY — the small post-translate breaks y-symmetry so one
 *      lobe is slightly more populated than the other. This is a
 *      real Lorenz-system feature (chaotic systems rarely visit
 *      symmetric regions with exactly equal measure over finite time).
 *
 * Complementary to `square_henon_strange_attractor` (discrete 2D
 * chaos) and `strange_attractor` (generic sinh-based attractor with
 * irrational rotations): this preset renders a SPECIFIC continuous-
 * time chaos system's (x,y) section, not a generic 2D attractor.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern tanh_lorenz_butterfly_section() {
  // arctan(σ/ρ) = arctan(10/28) ≈ 0.3430 rad (≈ 19.65°)
  // Pre-rotation angle = slant of the unstable eigenvector from origin.
  constexpr float COS_A_LORENZ = 0.9417419115f; // cos(arctan(10/28)) = 28/√884
  constexpr float SIN_A_LORENZ = 0.3363363423f; // sin(arctan(10/28)) = 10/√884

  // -1/φ rad (≈ -35.4°, golden-ratio reciprocal) counter-rotation.
  // Maximally irrational → never-repeating lobe traversal.
  constexpr float COS_INV_PHI = 0.8150283422f;  // cos(1/φ)
  constexpr float SIN_INV_PHI_NEG = -0.5794498725f; // sin(-1/φ)

  // Near-critical expansion (product ≈ 0.985). Tanh's saturation
  // walls guarantee boundedness even with expanding pre-multiply.
  constexpr float PRE  = 1.07f;  // expanding (safe with tanh)
  constexpr float POST = 0.92f;  // mild contraction

  Pattern p;
  p.name = "tanh_lorenz_butterfly_section";

  // Tanh: bounded chaos — mirrors Lorenz's global boundedness
  p.inner = InnerFunction::Tanh;
  p.reduction = ReductionType::Torus; // smooth continuous butterfly

  // Pre-affine: expanding rotation at the Lorenz slant angle
  p.zn.zn[0] = {COS_A_LORENZ * PRE, SIN_A_LORENZ * PRE};
  p.zn.zn[1] = {0.006f, -0.004f}; // tiny asymmetric perturbation

  // Post-affine: irrational counter-rotation (-1/φ rad)
  p.zn.zn[2] = {COS_INV_PHI * POST, SIN_INV_PHI_NEG * POST};

  // Post-translate: pure imaginary centroid shift breaks y-symmetry
  // so one lobe is slightly more populated (realistic finite-orbit
  // Lorenz section asymmetry).
  p.zn.zn[3] = {0.0f, 0.24f};

  // ── Color: MODE 3 (Alive) — Lorenz phase portrait ──
  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 1.06f;
  p.color.fade = 0.975f; // Lyapunov decay: ~45-frame predictability horizon
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 0.95f;   // reveal the two lobes as distinct hues
  p.color.spiral_radius_coeff = 0.52f;  // strong Cantor-stratum depth banding
  p.color.chroma_breath_amp = 0.0f;     // mode 3 doesn't use breathing envelope
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.016f;    // base orbit-tracking rotation
  p.color.hue_velocity_breath = 0.011f;  // medium oscillation
  p.color.hue_velocity_drift = 0.004f;   // long-period chaos envelope
  p.color.luma_shape_depth = 0.24f;      // visible strata within each lobe
  p.color.luma_shape_center = 1.9f;

  // ── Seed: MODERATE — butterfly needs some external energy to
  //    keep the orbits from collapsing onto the stable manifold ──
  p.seed.border_width = 0.024f; // narrow-medium
  p.seed.hue_speed = 0.08f;
  p.seed.alpha = 0.93f;

  // Speed: moderate — Lorenz has characteristic time scale 1/σ ≈ 0.1 s;
  // at speed 0.70 the lobe-switching is visible on the ~1 second
  // scale, which matches human eye-tracking comfort.
  p.speed = 0.70f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.184f;
  p.zn.zn0_anim_amp = 0.037f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.159f;
  p.zn.zn1_anim_amp = 0.056f;
  p.zn.zn1_anim_k = 2.0f;
  p.seed.pulse_freq = 0.119f;
  p.seed.pulse_amp = 0.19f;

  return p;
}

} // namespace epi::presets
