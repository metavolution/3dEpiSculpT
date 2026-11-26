#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * F9 z·tanh Breathing Basin — Mode-2 (Breathing) showcase for GPU inner ID 29.
 *
 *      f(z) = 0.8 · z · tanh(z)
 *
 * COMPANION TO f9_zth_basin_pulse:
 *
 * basin_pulse reveals F9's quadratic-core / sigmoid-rim structure through
 * Mode 3 (Alive) — kinetic energy → hue, so the rim glows where iterates
 * accelerate.  This preset reveals the SAME basin through Mode 2 (Breathing) —
 * a global chromatic spike envelope (chroma_breath_amp/freq/phase) ridden by
 * the basin's natural convergence rhythm.  The basin doesn't fluoresce by
 * velocity; it inhales and exhales as a whole, with the breath crest pulling
 * saturation and luma up across the entire attractor in unison.
 *
 * Why a Mode-2 partner around F9 specifically:
 *   - F9 is the only INNER_FUNCTIONS entry whose dynamic is dominated by
 *     true *attracting* basin convergence (every other extended preset
 *     diverges at the boundary).  Convergence has a natural rhythm — each
 *     iterate pulls closer in a geometric envelope.  Mode 2's global
 *     chroma_breath envelope is the perceptual analogue: a slow inhale
 *     (saturation rising) → crest → exhale (recovery), like the iterate's
 *     approach pulse rendered as breathing color.
 *   - Mode 3 (Alive) reads kinetic *contrast* (rim hot, core cool); Mode 2
 *     reads rhythmic *unison* (all of the basin breathes together).  The
 *     two presets thus expose orthogonal facets of the same attractor:
 *     spatial differentiation (Alive) and temporal coherence (Breathing).
 *
 * AFFINE GEOMETRY:
 *
 * π/11 (≈16.36°) PRE-TILT — prime-fold tilt distinct from basin_pulse's 5°.
 * 11 is prime, and a π/11 tilt under tanh's 2-fold (Re-axis) symmetry never
 * commensurates, so the tanh-pole filaments paint asymmetric arcs across
 * the basin floor.  The breath crest then highlights different filaments
 * at successive cycles, preventing the static "breathing wallpaper" look
 * that an integer-fold pre-tilt would give.
 *
 * zn0 = (PRE·cos(π/11), PRE·sin(π/11)) with PRE=0.93: same "preserve rim
 * dynamics" philosophy as basin_pulse (light contraction).  The basin self-
 * attracts; aggressive pre-contraction would collapse it into a dot.
 *
 * −1/φ² rad (≈ −0.382) POST-COUNTER-ROTATION: golden-ratio inverse-square,
 * irrational, *finer* than spiral_pinwheel's −1/φ.  Successive depth layers
 * misalign by a quasiperiodic angle dense enough that the breath's spatial-
 * phase term (φ rad) catches many distinct angular fronts → a multi-arc
 * breath wave instead of a single sweep.
 *
 * zn2 = (POST·cos(−1/φ²), POST·sin(−1/φ²)) with POST=0.95: same scalar as
 * basin_pulse, with a tiny irrational rotation appended.  Net contraction
 * 0.93·0.95 = 0.884 (basin remains attracting, not over-damped).
 *
 * COLOR MODE 2 — BREATHING (global chroma envelope + spatial phase):
 *
 *   sat ← sat · (1 + chroma_breath_amp · sin(2π · freq · t + phase · θ_spatial))
 *
 *   chroma_breath_amp   = 0.34 → strong, visible inhale/exhale across the
 *                                whole basin.  F9's attracting structure is
 *                                the perfect "vessel" for a breath to fill.
 *   chroma_breath_freq  = 0.10 → slow enough to read as a deliberate breath,
 *                                fast enough to cycle several times before
 *                                viewer attention drifts.
 *   chroma_breath_phase = φ ≈ 1.618 → spatial gradient: golden-ratio phase
 *                                offset.  Distinct from calcium_wave's 1.8
 *                                (which encodes a Ca²⁺ wave front) — here
 *                                the breath propagates along a φ-spaced
 *                                angular gradient, never quite repeating.
 *
 *   spiral_angle_coeff  = 0.95 → moderate: the π/11 pre-tilt creates ~11
 *                                soft sectors as the breath cycles; a
 *                                stronger coeff would over-saturate them.
 *   spiral_radius_coeff = 0.45 → mild concentric ring layering, same as
 *                                basin_pulse — keep the basin readable.
 *
 *   Mode 2 ignores hue_velocity_* (set to 0 to make intent explicit).
 *
 *   luma_shape_depth = 0.24 → visible breath-luma contrast: basin floor
 *   pulses brighter at crest, dims at trough.
 *
 * Pipeline: transform → reduce(torus) → sample → color(mode2) → seed → blend
 */
inline Pattern f9_zth_breathing_basin() {
  // π/11 (≈16.36°) pre-tilt: prime-fold, never commensurates with tanh's 2-fold
  constexpr float C11 = 0.9594929736f; // cos(π/11)
  constexpr float S11 = 0.2817325568f; // sin(π/11)

  // -1/φ² ≈ -0.3820 rad: golden-ratio inverse-square counter-rotation
  constexpr float CR_INVPHI2  =  0.9279386812f; // cos(-1/φ²) =  cos(1/φ²)
  constexpr float SR_INVPHI2N = -0.3727282745f; // sin(-1/φ²) = -sin(1/φ²)

  constexpr float PRE  = 1.6081f; // light: preserve rim dynamics (proven in basin_pulse)
  constexpr float POST = 1.4330f; // mild: same scalar as basin_pulse, plus tiny rotation

  Pattern p;
  p.name = "f9_zth_breathing_basin";
  // Named-enum path: InnerFunction::F9 ↔ id 29 (0.8·z·tanh(z))
  p.inner = InnerFunction::F9;
  p.extended_inner_id = -1;
  p.reduction = ReductionType::Torus; // smooth wrap → no boundary on the breathing basin

  p.zn.zn[0] = {C11 * PRE, S11 * PRE};
  p.zn.zn[1] = {0.2472f, -0.1649f}; // small off-origin translate: asymmetric basin lighting
  p.zn.zn[2] = {CR_INVPHI2 * POST, SR_INVPHI2N * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 2 (Breathing) — global chroma envelope, basin-wide unison ──
  p.color.hue_rotation        = 0.005f; // slow base hue drift
  p.color.sat_scale           = 1.12f;  // vivid but not over-saturated
  p.color.fade                = 0.990f; // deep: breath crests linger as afterglow
  p.color.color_mode          = as_int(ColorMode::Breathing); // Breathing: chroma envelope drives the basin
  p.color.spiral_angle_coeff  = 0.95f;  // moderate: ~11 soft sectors from π/11 tilt
  p.color.spiral_radius_coeff = 0.45f;  // mild radial: keep basin readable
  p.color.chroma_breath_amp   = 0.34f;  // strong: visible inhale/exhale
  p.color.chroma_breath_freq  = 0.10f;  // slow: deliberate breath, ~10s period
  p.color.chroma_breath_phase = 1.618f; // golden phase: propagating breath wave gradient
  // Mode 2 ignores these — set to 0 to make intent explicit
  p.color.hue_velocity_base   = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift  = 0.0f;
  p.color.luma_shape_depth    = 0.24f;  // breath-luma contrast: basin pulses bright/dim
  p.color.luma_shape_center   = 1.8f;

  // ── Seed: narrow border — basin self-organizes, breath dominates ──
  p.seed.border_width = 0.094f;
  p.seed.hue          = 0.18f;  // amber: warm contrast against cool breath crests
  p.seed.hue_speed    = 0.118f; // slow drift: doesn't compete with breath rhythm
  p.seed.alpha        = 0.92f;

  p.speed = 0.32f; // medium-slow: breath needs time to rise & fall visibly

  // ── Motion: hue drift + zoom breath — makes the basin literally breathe ──
  // The name is "breathing_basin"; the motion layer makes good on that promise.
  //   hue_drift: hue_rot oscillates ±0.008 around 0.005 on a 28 s cycle.
  //   zoom_pulse: Zn0Re (= C11·PRE = 0.892) breathes ±0.018 on a 16 s cycle.
  //     The 16/28 ratio is irrational (≈ 4/7): the two oscillations de-phase
  //     over time so their combined effect never repeats on a human timescale.
  using namespace epi::control;
  p.motion = PatternMotion::slow_color_drift(0.005f, 0.008f, 28.0f);
  p.motion.add("zoom_pulse",
               ModLane{.wave         = Waveform::Sine,
                       .frequency_hz = 1.0f / 16.0f,
                       .amplitude    = 0.018f,
                       .offset       = C11 * PRE},  // resting Zn0Re
               ModTarget::Zn0Re);
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.120f;
  p.zn.zn0_anim_amp = 0.039f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.170f;
  p.zn.zn1_anim_amp = 0.036f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.115f;
  p.seed.pulse_amp = 0.16f;


  return p;
}

} // namespace epi::presets
