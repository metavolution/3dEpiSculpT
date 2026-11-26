#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * P2 Logistic Basin — showcase for GPU inner ID 11 (0.9·(1 − z²)).
 *
 * MATHEMATICS: COMPLEX LOGISTIC AT THE EDGE OF PERIOD-DOUBLING
 *
 * The map f(z) = 0.9·(1 − z²) is the analytic continuation of the real
 * logistic map into the complex plane.  Solving the period-1 fixed-point
 * equation z = 0.9·(1 − z²):
 *
 *     0.9·z² + z − 0.9 = 0   →   z* = (−1 ± √4.24) / 1.8
 *                            →   z₁ ≈ +0.589   (period-1)
 *                            →   z₂ ≈ −1.700   (repelling)
 *
 * The multiplier at z₁ is f′(z) = −1.8·z, so |f′(z₁)| ≈ 1.06 — *just past*
 * the period-doubling bifurcation.  The orbit is therefore attracted to the
 * 2-cycle that branches off z₁: two lobes that exchange iterate-by-iterate,
 * weaving a figure-eight basin in the complex plane.
 *
 * Near z = 0 the map is approximately the affine f(z) ≈ 0.9 (a constant
 * pull toward +0.9 + 0i), which seeds the orbit into the 2-cycle from
 * almost any starting point.  Far from origin, the −0.9·z² term dominates
 * and folds iterates back across the imaginary axis — the *folding* is the
 * mechanism that joins the two lobes into a single attractor.
 *
 * COLOR MODE 3 — ALIVE (velocity-modulated hue):
 *
 *   hue_shift ← hue_shift + |dz/dt| · velocity_drift
 *
 * On a 2-cycle, velocity is *bimodal* — fast on the long inter-lobe jumps,
 * slow at the lobe centers where iterates cluster.  Mode 3 maps these two
 * speeds onto distinct hues, so the figure-eight reads as two color-stable
 * blobs joined by a hot velocity-bridge.  The bridge pulses each frame as
 * the orbit ping-pongs.
 *
 * AFFINE GEOMETRY:
 *
 * zn0 = (0.85, 0.10): light contraction with ~6.7° tilt.  The 2-cycle is
 * already mildly chaotic; heavier contraction collapses it back to a
 * fixed point (we want the LOBES, not a dot).
 *
 * zn1 = (−0.018, 0.0): tiny leftward translate centers the figure-eight
 * on screen rather than parking it at z₁ ≈ (0.59, 0).
 *
 * zn2 = (0.92, −0.05): post-contraction with mild counter-rotation —
 * preserves the lobe-exchange dynamic without runaway angular accumulation.
 *
 * Pipeline: transform → reduce(torus) → sample → color(mode3) → seed → blend
 */
inline Pattern p2_logistic_basin() {
  Pattern p;
  p.name = "p2_logistic_basin";
  // Named-enum path: InnerFunction::P2 ↔ GPU id 11 (0.9·(1−z²))
  p.inner = InnerFunction::P2;
  p.extended_inner_id = -1;
  p.reduction = ReductionType::Torus;

  // Light contraction + 6.7° tilt: keep the 2-cycle visible, not collapsed
  p.zn.zn[0] = {0.85f,  0.10f};
  // Centering translate: parks figure-eight at frame center instead of z₁
  p.zn.zn[1] = {-0.018f, 0.0f};
  // Mild counter-rotation post-affine: tames angular drift
  p.zn.zn[2] = {0.92f, -0.05f};
  p.zn.zn[3] = {0.0f,   0.0f};

  // ── Color: MODE 3 (Alive) — bimodal velocity → two-tone figure-eight ──
  p.color.hue_rotation      = 0.004f;   // slow base; let velocity dominate
  p.color.sat_scale         = 1.14f;    // vivid lobes
  p.color.fade              = 0.994f;   // persistent: lobes accumulate texture
  p.color.color_mode        = as_int(ColorMode::Alive);        // Alive
  p.color.spiral_angle_coeff  = 0.85f;
  p.color.spiral_radius_coeff = 0.55f;  // gentle radial — basin reads cleanly
  p.color.chroma_breath_amp   = 0.32f;  // moderate luma pulse at exchange beat
  p.color.chroma_breath_freq  = 0.55f;  // matches typical 2-cycle period
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base   = 0.22f;
  p.color.hue_velocity_breath = 0.12f;
  p.color.hue_velocity_drift  = 0.72f;  // 72% velocity-driven: alive signature
  p.color.luma_shape_depth    = 0.22f;
  p.color.luma_shape_center   = 1.6f;

  // ── Seed: medium border — feeds new texture into both lobes ──
  p.seed.border_width = 0.013f;
  p.seed.hue          = 0.06f;   // warm orange seed: complements cool inter-lobe bridge
  p.seed.hue_speed    = 0.011f;
  p.seed.alpha        = 0.76f;

  p.speed = 0.30f;  // medium: 2-cycle exchange needs frames to read
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.170f;
  p.zn.zn0_anim_amp = 0.049f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.117f;
  p.zn.zn1_anim_amp = 0.049f;
  p.zn.zn1_anim_k = 8.0f;
  p.seed.pulse_freq = 0.087f;
  p.seed.pulse_amp = 0.19f;

  return p;
}

} // namespace epi::presets
