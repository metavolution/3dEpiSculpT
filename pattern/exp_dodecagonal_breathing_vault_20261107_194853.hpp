#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Exponential dodecagonal breathing vault preset.
 *
 * MATHEMATICS: 12-fold architectural tunnel with breathing luminance pulsation
 *
 * 12 is the MOST DIVISIBLE small number: 12 = 2² × 3.  Its divisors
 * {1,2,3,4,6,12} mean that a 12-fold pattern contains simultaneously
 * visible 2-fold, 3-fold, 4-fold, and 6-fold sub-symmetries.  This is
 * the mathematical reason 12 dominates human design: clock faces, zodiac,
 * music (12 semitones), architecture (12-ribbed vaults in Gothic cathedrals).
 *
 * The complex exponential e^z maps the Cartesian grid to polar coordinates:
 *   Re(z) → radial distance (concentric circles)
 *   Im(z) → angle (radial rays)
 * With Grid reduction, these become visible ARCHES (circle-arc segments at
 * tile boundaries) and COLUMNS (ray segments).  The 12-fold pre-rotation
 * creates 12 such arch-column pairs per revolution, producing a complete
 * Gothic vault rosette — but one that BREATHES.
 *
 * ENGINE MAPPING:
 *
 *   12-FOLD VAULT = π/6 pre-rotation + Exp + Grid
 *     π/6 = 30° exactly.  This is the ONLY preset using a rational multiple
 *     of π as pre-rotation angle — it creates EXACT 12-fold symmetry.
 *     After 12 iterations: exactly 2π rotation + 12 generations of
 *     exponential folding = a perfect dodecagonal vault mandala.
 *
 *     The rational angle is deliberate: combined with the transcendental
 *     post-rotation (1/e), the depth stacking is aperiodic (30°×N mod 360°
 *     repeats every 12 steps, but the post-rotation 1/e never aligns with
 *     any rational multiple of π).  The result: perfect 12-fold sectors
 *     at each depth level, but each level ROTATED by an irrational angle
 *     relative to the previous one.
 *
 *   RECIPROCAL EULER COUNTER-ROTATION = -1/e ≈ -21.08°
 *     1/e = 0.3679... is the reciprocal of Euler's number.  It appears
 *     in probability theory as the derangement limit: the probability
 *     that a random permutation has NO fixed point converges to 1/e.
 *     As a radian angle, 1/e is transcendental (by Lindemann: e^(i/e)
 *     is transcendental since 1/e is algebraic-over-e, hence transcendental).
 *
 *     The small counter-rotation (21°) creates GENTLE DEPTH TWIST:
 *     each successive vault layer appears only slightly rotated from
 *     the previous, giving the impression of looking down a slowly
 *     spiraling Gothic staircase.  Compare with hendecagonal_vault's
 *     99° near-perpendicular twist — this preset is its GENTLE counterpart.
 *
 *   BREATHING VAULT = Mode 2 (Breathing)
 *     breath = 1 + amp × sin(freq × 2πt + phase × angle)
 *     With moderate amplitude (0.28) and slow frequency (0.08), the
 *     12 vault arches brighten and dim in unison.  The angular phase
 *     coupling (phase=2.0) creates a 12-sector LIGHTHOUSE effect:
 *     brightness sweeps through the sectors like a rotating beam.
 *
 *     This fills the Exp+Grid+mode2 gap — the FIRST exponential vault
 *     preset with visible breathing pulsation.  The architectural ribs
 *     (Grid boundaries) pulse between bright stone and dark shadow,
 *     creating a living cathedral effect.
 *
 * - |zn0| = 0.74: MODERATE for Exp.  Lower than gothic_cathedral (0.82)
 *   and hendecagonal_vault (0.76) — the arches are SHARPER because less
 *   exponential blow-up means Grid boundaries remain more defined.
 *
 * - |zn2| = 0.87: MODERATE contraction.  Net: 0.74 × 0.87 = 0.644.
 *   The low product creates DEEP accumulation: many visible vault
 *   generations before structure falls below pixel threshold.
 *
 * WHAT TO WATCH:
 *   1. COUNT 12 vault sectors — each arch-column pair is visible
 *   2. The BREATHING: watch arches pulse bright → dim → bright
 *   3. The LIGHTHOUSE: brightness sweeps angularly through sectors
 *   4. GENTLE SPIRAL: each depth layer twisted just ~21° (1/e rad)
 *   5. SUB-SYMMETRIES: look for 2-fold, 3-fold, 4-fold, 6-fold echoes
 *   6. ARCH DEFINITION: sharper than gothic_cathedral due to lower |zn0|
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern exp_dodecagonal_breathing_vault() {
  // π/6 = 0.5236 rad (30°): exact dodecagonal symmetry
  constexpr float C12  =  0.8660f;           // cos(π/6) = √3/2
  constexpr float S12  =  0.5000f;           // sin(π/6) = 1/2
  // -1/e ≈ -0.3679 rad (-21.08°): reciprocal Euler transcendental twist
  constexpr float CRE  =  0.9330f;           // cos(1/e)
  constexpr float SREN = -0.3597f;           // sin(-1/e)

  constexpr float PRE  = 0.74f;  // moderate: sharp vault arches
  constexpr float POST = 0.87f;  // moderate contraction: deep accumulation

  Pattern p;
  p.name = "exp_dodecagonal_breathing_vault";
  p.inner = InnerFunction::Exp;              // Cartesian→polar: arches + columns
  p.reduction = ReductionType::Grid;         // architectural ribs at tile boundaries

  p.zn.zn[0] = {C12 * PRE, S12 * PRE};
  p.zn.zn[1] = {0.0f, 0.005f};              // tiny imaginary offset (break degeneracy)
  p.zn.zn[2] = {CRE * POST, SREN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 2 (Breathing) — pulsing vault arches ──
  p.color.hue_rotation = 0.009f;             // slow: vault geometry dominates
  p.color.sat_scale = 1.12f;                 // vivid: stained glass warmth
  p.color.fade = 0.987f;                     // HIGH: ~77 frame persistence, deep vault
  p.color.color_mode = as_int(ColorMode::Breathing);                    // Breathing: living cathedral pulsation
  p.color.spiral_angle_coeff = 1.10f;        // strong: 12 sectors visually distinct
  p.color.spiral_radius_coeff = 0.45f;       // moderate: depth rings through vault layers
  // BREATHING VAULT (slow, moderate, spatially sweeping)
  p.color.chroma_breath_amp = 0.28f;         // MODERATE: visible but not overwhelming
  p.color.chroma_breath_freq = 0.08f;        // SLOW: meditative breathing cycle
  p.color.chroma_breath_phase = 2.0f;        // MODERATE: brightness sweeps through sectors
  // Mode 2 does not use hue velocity params
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: bright arch peaks, dark rib shadows → vault depth
  p.color.luma_shape_depth = 0.24f;          // moderate: arch/shadow contrast
  p.color.luma_shape_center = 2.0f;

  // ── Seed: minimal — vault self-generates from exponential feedback ──
  p.seed.border_width = 0.012f;              // subtle: vault depth dominates
  p.seed.hue_speed = 0.05f;                  // slow: contemplative
  p.seed.alpha = 0.88f;

  p.speed = 0.46f;                           // slow: architectural meditation
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.167f;
  p.zn.zn0_anim_amp = 0.051f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.192f;
  p.zn.zn1_anim_amp = 0.043f;
  p.zn.zn1_anim_k = 8.0f;
  p.seed.pulse_freq = 0.118f;
  p.seed.pulse_amp = 0.17f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.060f,
                            .amplitude = 0.126f,
                            .offset = 0.450f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.074f,
                            .amplitude = 0.120f,
                            .offset = 0.280f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.062f,
                            .amplitude = 0.030f,
                            .offset = 0.050f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
