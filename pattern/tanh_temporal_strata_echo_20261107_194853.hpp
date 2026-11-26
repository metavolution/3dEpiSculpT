#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh Temporal Strata Echo — showcase for Color Mode 5 (Depth Echo).
 *
 * MATHEMATICS: TEMPORAL STRATIFICATION IN ITERATED COMPLEX MAPS
 *
 * The hyperbolic tangent tanh(z) has a global attractor at z = 0:
 * every finite z converges under repeated application, but the RATE
 * of convergence varies dramatically across the complex plane.
 *
 *   Points near 0                 — converge immediately (thin alpha layer)
 *   Points near |Im(z)| ≈ π/2   — linger near the poles (thick alpha layer)
 *   Points in the "slow manifold" — accumulate over many frames
 *
 * This differential rate creates TEMPORAL STRATA: geological-scale layers
 * of iteration history encoded in the alpha channel of the feedback buffer.
 *
 * COLOR MODE 5 — DEPTH ECHO:
 *
 * The per-pixel alpha c.w accumulates as DEPTH_INC = 32/255 ≈ 0.125
 * each frame the pixel is sampled.  After N frames in a region, c.w ≈ N/8.
 *
 * The Depth Echo mode maps this directly to chromatic stratification:
 *
 *   h_spatial = hv_drift × (spiral_a × angle + spiral_r × log_r)
 *   h_depth   = (1 − hv_drift) × spiral_a × c.w
 *   hue_shift = hue_time + h_spatial + h_depth + len
 *   brightness = 1 + breath_amp × c.w
 *   saturation += hv_base × c.w
 *
 * With hv_drift = 0.25 (75% depth-driven), old pixels glow with
 * distinctly shifted hue and boosted saturation — the past visible
 * through the present.  The spatial component provides structural
 * anchor, the depth component provides the chromatic strata effect.
 *
 * AFFINE GEOMETRY:
 *
 * zn0 = (1.0, 0.12): unit-magnitude rotation of ~6.8° introduces helical
 * asymmetry, breaking the real-axis mirror symmetry of tanh.  This causes
 * the attractor basins to spiral rather than collapse straight inward —
 * creating the characteristic swept-strata visual.
 *
 * zn2 = (0.95, 0.08): |zn2| ≈ 0.953, arg ≈ 4.8° — gentle inward spiral
 * of the post-affine stage ensures long-run stability (avoids expansion).
 *
 * Pipeline: transform → reduce(torus) → sample → color(mode5) → seed → blend
 */
inline Pattern tanh_temporal_strata_echo() {
  Pattern p;
  p.name = "tanh_temporal_strata_echo";
  p.inner = InnerFunction::Tanh;
  p.reduction = ReductionType::Torus;

  // Helical pre-affine: supercritical scale + helical rotation wraps fresh depth strata
  p.zn.zn[0] = {1.32f,  0.22f};
  // Slight pre-translate: breaks left-right symmetry → asymmetric depth gradient
  p.zn.zn[1] = {0.018f, -0.011f};
  // Post-rotation balances torus recirculation so shallow/mid/deep layers coexist
  p.zn.zn[2] = {0.98f,  0.12f};
  p.zn.zn[3] = {0.0f,   0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.14f;
  p.zn.zn0_anim_amp = 0.07f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.09f;
  p.zn.zn1_anim_amp = 0.016f;
  p.zn.zn1_anim_k = 3.0f;

  // ── Color: MODE 5 (Depth Echo) — temporal chromatic stratification ──
  p.color.hue_rotation      = 0.007f;   // slow base hue drift
  p.color.sat_scale         = 0.96f;    // keeps HSV saturation <= 1 under FP16 depth accumulation
  p.color.fade              = 0.984f;   // natural depth attenuation: fresh = bright, deep = rich
  p.color.color_mode        = as_int(ColorMode::DepthEcho);  // Depth Echo
  p.color.spiral_angle_coeff  = 0.85f;  // angle + depth chromatic layering
  p.color.spiral_radius_coeff = 0.45f;  // log_r rings anchor spatial structure
  p.color.chroma_breath_amp   = 0.015f; // gentle depth-driven saturation boost in [0, 1]
  p.color.chroma_breath_freq  = 0.0f;   // not oscillatory in mode 5
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base   = 0.0f;   // zero additive luma pump avoids unbounded .w whiteout
  p.color.hue_velocity_breath = 0.0f;   // unused in mode 5
  p.color.hue_velocity_drift  = 0.35f;  // 35% spatial / 65% depth-driven hue
  // Luma shaping is gated on color_mode < 4 in both kernels; mode 5 has its
  // own brightness path (hv_base above). These fields are intentionally zero.
  p.color.luma_shape_depth    = 0.0f;
  p.color.luma_shape_center   = 0.0f;

  // ── Seed: wide injection refreshes .w depth layers cleanly ──
  p.seed.border_width = 0.080f;
  p.seed.hue          = 0.52f;
  p.seed.hue_speed    = 0.035f;
  p.seed.alpha        = 0.92f;

  p.speed = 0.65f;
  return p;
}

} // namespace epi::presets
