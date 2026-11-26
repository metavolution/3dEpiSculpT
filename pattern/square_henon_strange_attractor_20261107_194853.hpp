#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Hénon Strange Attractor — discrete 2D chaos
 * ================================================================
 *
 * PHYSICS / MATH  (Hénon 1976, "A two-dimensional mapping with
 * a strange attractor", Comm. Math. Phys. 50, 69-77):
 *
 *   x_{n+1} = 1 - a·x_n² + y_n
 *   y_{n+1} = b·x_n
 *
 * Canonical parameters: a = 1.4, b = 0.3.
 *
 * Hénon constructed this as the simplest 2D diffeomorphism that
 * reproduces the essential qualitative features of the Lorenz
 * system: a SADDLE FIXED POINT, a quadratic nonlinearity, and a
 * contracting piecewise action that folds phase space into a
 * measure-zero fractal attractor with the topology of a Cantor set
 * cross an interval. The Hausdorff dimension is approximately 1.261.
 *
 * Successive iterates trace out the characteristic "banana" shape
 * whose fine cross-section reveals infinitely nested Cantor strata
 * — the self-similar signature of a strange attractor. This is the
 * discrete-map counterpart of the Lorenz butterfly.
 *
 *
 * ENGINE MAPPING
 *
 *   QUADRATIC NONLINEARITY = Square inner function (z → z²)
 *     The -a·x² core of the Hénon map IS the real part of z². The
 *     Square inner function applied in the complex plane produces
 *     exactly the folding + stretching structure that creates the
 *     Hénon attractor's characteristic curves.
 *
 *   SADDLE STABILITY = near-critical product |zn[0]|·|zn[2]|
 *     Hénon lives on the edge of chaos. Product = 0.95·0.89 = 0.846
 *     — marginally contracting so orbits converge to the attractor
 *     without spiraling to a fixed point. This is the SAME regime
 *     as mandelbrot_ghost (|zn[0]|=1.0, |zn[2]|=1.0, cardioid cusp).
 *
 *   b · x COUPLING  = post-rotation by arctan(b) = arctan(0.3)
 *     The y' = b·x step of the Hénon map is a rotation + scaling
 *     that maps the x-axis into the y-axis at strength b. In the
 *     complex-plane analog, this is a post-rotation by arctan(0.3)
 *     ≈ 0.2915 rad (16.7°), which shears successive iterates in
 *     the right direction to form the banana curve.
 *
 *   SADDLE FIXED POINT SHIFT = post-translate zn[3] ≈ (0.18, 0)
 *     Hénon's fixed points live at x* = ((b-1) ± √((1-b)²+4a))/(2a).
 *     For canonical (a,b), x* ≈ 0.63 or -1.13. The positive fixed
 *     point sits inside the attractor — we shift the post-affine
 *     origin there so the accumulated orbit visibly nests around
 *     the saddle, emphasizing the strange-attractor curves rather
 *     than drifting asymptotically.
 *
 *   PHASE PORTRAIT = color mode 3 (Alive)
 *     Mode 3's triple-velocity hue evolution + radial luma depth
 *     is the purpose-built visualization for phase-space portraits
 *     of dynamical systems. Each point on the attractor gets a
 *     distinct hue based on its angular position and log-radius,
 *     revealing the banded Cantor-set cross-section as hue banding.
 *
 *   GLOBAL BOUNDEDNESS = Torus reduction
 *     Hénon's attractor has compact support (bounded diameter ≈ 4).
 *     Torus wrapping preserves this boundedness without introducing
 *     artificial grid-tile walls that would cut the fractal.
 *
 *
 * WHAT TO WATCH:
 *   1. The characteristic "banana" curve forms after a few seconds
 *      of iteration — this is the attractor's topology.
 *   2. Zooming in (conceptually — by inspecting fine structure)
 *      reveals nested parallel strands at every scale: the Cantor
 *      set cross-section.
 *   3. Hue bands along the curve expose the period-doubled orbit
 *      cascade: some regions are traversed more densely (periodic
 *      islands) than others (fully chaotic).
 *   4. The seed border is narrow (0.018) — the attractor is energy-
 *      poor and should be lit primarily by feedback from its own
 *      recursive folding, not by external injection.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern square_henon_strange_attractor() {
  // arctan(0.3) ≈ 0.29146 rad (16.7°) — the Hénon b-coupling angle.
  constexpr float COS_ATAN_B = 0.9578263f; // cos(arctan(0.3)) = 1/√1.09
  constexpr float SIN_ATAN_B = 0.2873479f; // sin(arctan(0.3)) = 0.3/√1.09

  // Near-critical product 0.95 · 0.89 = 0.846 — marginally stable,
  // same family as mandelbrot_ghost (product = 1.0, cardioid cusp).
  constexpr float PRE  = 1.3781f;
  constexpr float POST = 1.2871f;

  Pattern p;
  p.name = "square_henon_strange_attractor";

  // Square inner: the quadratic z² IS the -a·x² core of the map.
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Torus; // bounded, smooth — no grid tiles

  // Pre-affine: identity magnitude, zero rotation — Hénon applies
  // its nonlinearity with no pre-mixing; the Square inner then
  // executes the full quadratic fold.
  p.zn.zn[0] = {PRE, 0.0f};
  p.zn.zn[1] = {0.2417f, -0.1705f};

  // Post-affine: rotation by arctan(b) scaled by POST contraction.
  // This rotation is the Hénon b·x → y coupling rendered as a
  // complex-plane shear.
  p.zn.zn[2] = {COS_ATAN_B * POST, SIN_ATAN_B * POST};

  // Post-translate: shift the origin to near the positive fixed
  // point (x* ≈ 0.63 in Hénon coords; 0.18 matches the attractor's
  // geometric centroid in the unit-cell engine frame).
  p.zn.zn[3] = {0.18f, 0.0f};

  // ── Color: MODE 3 (Alive) — phase-portrait visualization ──
  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.985f; // moderate — Q≈209, long trails show orbit history
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 0.72f;  // reveal angular banding of the banana
  p.color.spiral_radius_coeff = 0.46f; // strong radial depth = Cantor strata
  p.color.chroma_breath_amp = 0.0f;    // mode 3 does not use breathing
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.014f;    // slow base rotation = orbit drift
  p.color.hue_velocity_breath = 0.010f;  // secondary oscillation
  p.color.hue_velocity_drift = 0.003f;   // long-period chaos envelope
  p.color.luma_shape_depth = 0.22f;      // clear light/dark strata
  p.color.luma_shape_center = 1.7f;

  // ── Seed: NARROW — attractor lights itself via recursion ──
  p.seed.border_width = 0.088f;  // thin: ~2% of unit cell
  p.seed.hue_speed = 0.07f;
  p.seed.alpha = 0.92f;

  // Speed: contemplative — Hénon's banana structure rewards
  // extended viewing; the fine Cantor strata need iterations to
  // accumulate clearly.
  p.speed = 0.62f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.109f;
  p.zn.zn0_anim_amp = 0.051f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.211f;
  p.zn.zn1_anim_amp = 0.051f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.084f;
  p.seed.pulse_amp = 0.16f;

  return p;
}

} // namespace epi::presets
