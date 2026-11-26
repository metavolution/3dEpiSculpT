#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * F9 Stuart–Landau Limit Cycle — proof-of-feature for the named
 * `InnerFunction::F9` enum path (no `extended_inner_id` override).
 *
 * MATHEMATICS: HOPF BIFURCATION → AMPLITUDE-SATURATED LIMIT CYCLE
 *
 * The Stuart–Landau equation governs amplitude near a Hopf bifurcation:
 *
 *   dA/dt = (μ + iω)·A − (1 + iα)·|A|²·A
 *
 * For μ > 0 the trivial fixed point becomes unstable and a stable limit
 * cycle of radius √μ appears — the canonical onset of self-sustained
 * oscillation in fluid layers, lasers, BZ chemistry, neural circuits.
 *
 * The map  f(z) = 0.8·z·tanh(z)  is a tractable amplitude proxy:
 *   small |z|:   tanh(z) ≈ z   →  f(z) ≈ 0.8·z²   (cubic-class growth,
 *                                                  matching Stuart-Landau's
 *                                                  near-origin |A|²·A term)
 *   large |Re z|: tanh(z) → ±1 →  f(z) ≈ ±0.8·z  (linear contraction —
 *                                                  the saturation walls
 *                                                  enforce limit-cycle
 *                                                  amplitude bounding)
 *
 * The crossover is the limit cycle. Iterating from any non-zero start
 * point spirals onto a quasi-circular orbit set by the tanh saturation
 * radius — the engine's living proxy for the Hopf normal form.
 *
 * Distinct from f9_zth_basin_pulse (which emphasizes the quadratic core
 * as a *basin*); this preset emphasizes the *limit cycle* — the orbit
 * reached after transients decay.
 *
 * COLOR MODE 3 — ALIVE: velocity-coupled hue exposes the limit cycle as
 * a band of constant kinetic energy ringing the origin.
 *
 * AFFINE GEOMETRY:
 *
 * zn0 = (0.84, π/12): pre-rotation near 15° — the slow phase advance ω
 * of the Stuart–Landau angular drift. Magnitude 0.84 is the criticality
 * dial: high enough to keep the orbit visible, low enough to avoid
 * runaway near the tanh saturation walls.
 *
 * zn2 = (0.92, 0): post-contraction with no extra rotation — the limit
 * cycle's natural angular velocity dominates.
 *
 * Pipeline: transform → reduce(torus) → sample → color(mode3) → seed → blend
 */
inline Pattern f9_stuart_landau_limit_cycle() {
  Pattern p;
  p.name = "f9_stuart_landau_limit_cycle";
  // Named-enum path: exercises the new InnerFunction::F9 ↔ id 29 mapping
  // through gpu_id_mapping::inner_to_gpu (no extended_inner_id override).
  p.inner = InnerFunction::F9;
  p.extended_inner_id = -1;
  p.reduction = ReductionType::Torus;

  // Pre-affine: |zn0|=0.84 (criticality), arg=π/12 ≈ 15° (Hopf angular drift)
  // Components: 0.84·cos(π/12)≈0.81138, 0.84·sin(π/12)≈0.21741
  p.zn.zn[0] = {1.6015f, 0.4291f};
  p.zn.zn[1] = {0.2567f, -0.2024f};
  // Post-affine: gentle contraction, no extra rotation
  p.zn.zn[2] = {1.4466f, -0.2039f};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 3 (Alive) — velocity hue exposes the limit-cycle band ──
  p.color.hue_rotation        = 0.004f;
  p.color.sat_scale           = 1.16f;
  p.color.fade                = 0.992f;  // long trails: limit cycle persistence
  p.color.color_mode          = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff  = 1.05f;
  p.color.spiral_radius_coeff = 0.42f;
  p.color.chroma_breath_amp   = 0.32f;
  p.color.chroma_breath_freq  = 0.38f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base   = 0.20f;
  p.color.hue_velocity_breath = 0.12f;
  p.color.hue_velocity_drift  = 0.65f;
  p.color.luma_shape_depth    = 0.18f;
  p.color.luma_shape_center   = 1.6f;

  // ── Seed: narrow border — limit cycle is interior, edges secondary ──
  p.seed.border_width = 0.080f;
  p.seed.hue          = 0.06f;  // warm seed contrasts with cool velocity hues
  p.seed.hue_speed    = 0.117f;
  p.seed.alpha        = 0.92f;

  p.speed = 0.32f;

  // ── Motion: velocity-coupled hue drift — the limit cycle rings in colour ──
  // slow_color_drift oscillates hue_rot ±0.006 around 0.004 (36 s cycle).
  // saturation_pulse breathes ±0.05 around 1.16 (22 s cycle).
  // 36/22 ≈ 1.636 (golden ratio); the combined cycle never repeats.
  p.motion = epi::control::PatternMotion::slow_color_drift(0.004f, 0.006f, 36.0f);
  p.motion.add("sat_breath",
               epi::control::ModLane{.wave         = epi::control::Waveform::Triangle,
                                     .frequency_hz = 1.0f / 22.0f,
                                     .amplitude    = 0.050f,
                                     .offset       = 1.16f},
               epi::control::ModTarget::SatScale);
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.136f;
  p.zn.zn0_anim_amp = 0.049f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.118f;
  p.zn.zn1_anim_amp = 0.041f;
  p.zn.zn1_anim_k = 2.0f;
  p.seed.pulse_freq = 0.107f;
  p.seed.pulse_amp = 0.15f;


  return p;
}

} // namespace epi::presets
