#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * TP3 Meromorphic Filaments — showcase for GPU inner ID 16
 *
 *      f(z) = 0.5·(sin(z) + tan(z))
 *
 * MATHEMATICS: ENTIRE + MEROMORPHIC SUM
 *
 * sin(z) is entire (no singularities); tan(z) = sin(z)/cos(z) is meromorphic
 * with simple poles at z = π/2 + nπ along the real axis.  Their sum
 * inherits both behaviours, but the structural character is dominated by
 * the pole strip:
 *
 *   Near z = 0:   sin(z) ≈ z,  tan(z) ≈ z + z³/3
 *                 → f(z) ≈ z + z³/6   (multiplier 1 → marginally neutral)
 *                 → long-period spiral orbits emanate from origin.
 *
 *   On Im(z) → ∞: sin(iy) = i·sinh(y) (unbounded), tan(iy) = i·tanh(y) → i (bounded)
 *                 → tan saturates while sin diverges along the imaginary
 *                   axis; vertical compression in the rendered output comes
 *                   from the affine envelope (zn0..zn3 contraction), not
 *                   from the inner map itself.
 *
 *   Approaching Re(z) = ±π/2 + nπ: tan blows up
 *                 → orbits passing the pole strip get launched along
 *                   nearly-radial trajectories: FILAMENTS that radiate
 *                   from the poles outward into the field.
 *
 * The interplay produces a distinctive composition: a calm vertical "spine"
 * at Re(z) = 0 where sin and tan partially cancel, periodic pole strips at
 * each Re(z) = π/2 + nπ that emit filament rays, and bounded wandering
 * elsewhere.  Unlike the entire-function presets (TP1, TP6), this map's
 * dynamic palette is *defined by its singularities*.
 *
 * COLOR MODE 1 — SPIRAL:
 *
 *   hue_shift = hue_time + hv_drift × (spiral_a × angle + spiral_r × log_r)
 *
 * Mode 1's angle-driven hue sweep is ideally matched to radiating filaments:
 * each filament's direction maps to a hue ray, so the field reads as a
 * many-colored fan emanating from each pole.  The log_r contribution adds
 * concentric color rings that interfere with the radial filament hues at
 * the poles — pole regions become high-frequency color shimmers, far-field
 * regions read as smooth angular sweeps.
 *
 * AFFINE GEOMETRY:
 *
 * zn0 = (0.65, 0.20): moderate contraction with ~17° tilt.  Vertical
 * boundedness is automatic (sinh/tanh in imaginary direction); the
 * contraction's job is purely to control horizontal magnitude — keep
 * iterates from straying into the divergent exterior past the second pole
 * strip (Re(z) > 3π/2).  The tilt rotates the pole strip slightly off the
 * real axis, breaking the otherwise stark horizontal symmetry.
 *
 * zn1 = (0.04, 0.0): small rightward translate parks the central pole
 * (originally at +π/2) within the visible field, making its filament
 * radiation a foreground feature rather than an off-screen attractor.
 *
 * zn2 = (0.88, −0.04): post-contraction with mild counter-rotation —
 * preserves filament length without snowballing angular drift.
 *
 * Pipeline: transform → reduce(torus) → sample → color(mode1) → seed → blend
 */
inline Pattern tp3_meromorphic_filaments() {
  Pattern p;
  p.name = "tp3_meromorphic_filaments";
  // Named-enum path: InnerFunction::TP3 ↔ GPU id 16 ((sin(z)+tan(z))/2)
  p.inner = InnerFunction::TP3;
  p.extended_inner_id = -1;
  p.reduction = ReductionType::Torus;

  // Moderate contraction + 17° tilt: control horizontal blow-up at poles
  p.zn.zn[0] = {0.65f,  0.20f};
  // Rightward translate: pulls the +π/2 pole into the visible field
  p.zn.zn[1] = {0.04f,  0.0f};
  // Post-contraction with mild counter-rotation: tames angular accumulation
  p.zn.zn[2] = {0.88f, -0.04f};
  p.zn.zn[3] = {0.0f,   0.0f};

  // ── Color: MODE 1 (Spiral) — angle hue maps onto filament rays ──
  p.color.hue_rotation      = 0.005f;   // slow base hue drift
  p.color.sat_scale         = 1.16f;    // vivid filaments
  p.color.fade              = 0.991f;   // medium persistence: filaments streak
  p.color.color_mode        = as_int(ColorMode::Spiral);        // Spiral
  p.color.spiral_angle_coeff  = 2.10f;  // strong angular sweep — filament rays paint
  p.color.spiral_radius_coeff = 0.85f;  // log_r rings interfere at poles
  p.color.chroma_breath_amp   = 0.26f;
  p.color.chroma_breath_freq  = 0.90f;  // faster breath: pole shimmer pulses
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base   = 0.32f;
  p.color.hue_velocity_breath = 0.14f;
  p.color.hue_velocity_drift  = 0.62f;  // 62% spatial — filament directionality leads
  p.color.luma_shape_depth    = 0.20f;
  p.color.luma_shape_center   = 1.55f;

  // ── Seed: medium border — filaments reach edges, refresh feeds them ──
  p.seed.border_width = 0.015f;
  p.seed.hue          = 0.78f;   // violet seed: orthogonal to filament hue palette
  p.seed.hue_speed    = 0.015f;
  p.seed.alpha        = 0.78f;

  p.speed = 0.33f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.124f;
  p.zn.zn0_anim_amp = 0.038f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.124f;
  p.zn.zn1_anim_amp = 0.047f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.098f;
  p.seed.pulse_amp = 0.19f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.094f,
                            .amplitude = 0.238f,
                            .offset = 0.850f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.115f,
                            .amplitude = 0.120f,
                            .offset = 0.260f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.044f,
                            .amplitude = 0.030f,
                            .offset = 0.015f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
