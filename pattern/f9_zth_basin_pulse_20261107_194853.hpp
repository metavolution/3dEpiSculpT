#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * F9 z·tanh Basin Pulse — showcase for GPU inner ID 29 (0.8·z·tanh(z)).
 *
 * MATHEMATICS: QUADRATIC CORE × SIGMOID ENVELOPE
 *
 * The map f(z) = 0.8·z·tanh(z) factors the dynamics into two regimes joined
 * by a smooth sigmoid transition:
 *
 *   Near z = 0:  tanh(z) ≈ z − z³/3 + ... → f(z) ≈ 0.8·z²
 *                  → quadratic basin attractor with origin as super-stable
 *                    fixed point (multiplier = 0).
 *
 *   For large |Re(z)|:  |tanh(z)| → 1 with sign(Re(z))
 *                  → f(z) ≈ ±0.8·z   → linear contraction toward origin.
 *
 *   On Im(z) = π/2 + nπ:  tanh has poles → singular spikes that radiate
 *                          long-range filaments through the field.
 *
 * The combination yields STRONG BASIN STRUCTURE: a deep quadratic well at
 * the origin, surrounded by a smoothly transitioning linear-flow region,
 * punctured by tanh-pole filaments at periodic imaginary altitudes.  The
 * basin is the cleanest *attracting* feature in the catalog — every other
 * landed extended preset diverges at the boundary; this one converges.
 *
 * COLOR MODE 3 — ALIVE (velocity-modulated hue):
 *
 *   hue_shift ← hue_shift + |dz/dt| · velocity_drift
 *
 * The Alive mode reads kinetic energy as hue.  Inside the quadratic core,
 * iterates slow dramatically (super-stable fixed point → near-zero velocity);
 * the basin floor reads as a *quiet* color.  At the basin rim, where tanh
 * transitions from quadratic to linear, velocity peaks → the rim glows in
 * a hot velocity hue.  Beyond the rim, velocity decays linearly → cooler
 * trailing colors.  The result is a single living organism: a calm core,
 * a bright rim, and shimmering filaments at the tanh poles.
 *
 * AFFINE GEOMETRY:
 *
 * zn0 = (0.92, 0.08): light contraction with 5° tilt.  The basin is
 * naturally attracting, so heavy contraction would collapse the visible
 * structure into a single dark dot — we want to *preserve* the rim
 * dynamics, not suppress them.
 *
 * zn1 = (0.022, 0.014): small off-origin translate displaces the basin
 * slightly into the upper-right quadrant, so the rim catches asymmetric
 * lighting from color_mode's spatial gradient.
 *
 * zn2 = (0.95, 0.0): minimal post-affine — let the natural basin
 * convergence be the dominant motion.
 *
 * Pipeline: transform → reduce(torus) → sample → color(mode3) → seed → blend
 */
inline Pattern f9_zth_basin_pulse() {
  Pattern p;
  p.name = "f9_zth_basin_pulse";
  // Named-enum path: InnerFunction::F9 ↔ id 29 (0.8·z·tanh(z))
  p.inner = InnerFunction::F9;
  p.extended_inner_id = -1;
  p.reduction = ReductionType::Torus;

  // Light contraction + 5° tilt: preserve rim dynamics, gentle drift
  p.zn.zn[0] = {1.5987f, 0.1390f};
  // Off-origin translate: displaces basin → asymmetric rim lighting
  p.zn.zn[1] = {0.2055f, -0.1416f};
  // Minimal post-affine: don't fight the natural basin convergence
  p.zn.zn[2] = {1.4556f, -0.2051f};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 3 (Alive) — velocity-driven basin / rim / filament hues ──
  p.color.hue_rotation      = 0.003f;   // very slow base: let velocity dominate
  p.color.sat_scale         = 1.18f;    // vivid rim
  p.color.fade              = 0.986f;   // high persistence: filaments linger
  p.color.color_mode        = as_int(ColorMode::Alive);        // Alive
  p.color.spiral_angle_coeff  = 1.10f;
  p.color.spiral_radius_coeff = 0.45f;  // gentle radial term — basin reads cleanly
  p.color.chroma_breath_amp   = 0.40f;  // moderate luma pulse: basin "breathes"
  p.color.chroma_breath_freq  = 0.45f;  // slow basin breathing
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base   = 0.18f;
  p.color.hue_velocity_breath = 0.14f;
  p.color.hue_velocity_drift  = 0.78f;  // 78% velocity-driven: alive mode signature
  p.color.luma_shape_depth    = 0.20f;
  p.color.luma_shape_center   = 1.8f;

  // ── Seed: very narrow border — basin is interior-driven, edges secondary ──
  p.seed.border_width = 0.082f;
  p.seed.hue          = 0.92f;   // magenta seed: pops against velocity-warm rim
  p.seed.hue_speed    = 0.124f;
  p.seed.alpha        = 0.92f;

  p.speed = 0.28f;  // medium-slow: velocity contrast needs time to establish
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.096f;
  p.zn.zn0_anim_amp = 0.047f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.125f;
  p.zn.zn1_anim_amp = 0.044f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.084f;
  p.seed.pulse_amp = 0.16f;

  return p;
}

} // namespace epi::presets
