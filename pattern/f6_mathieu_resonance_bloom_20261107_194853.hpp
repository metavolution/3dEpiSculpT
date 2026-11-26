#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * F6 Mathieu Resonance Bloom — showcase for GPU inner ID 26.
 *
 *      f(z) = 1.3 · cos(i · sin(z)) = 1.3 · cosh(sin(z))
 *
 * MATHEMATICS: MATHIEU EQUATION — PARAMETRIC RESONANCE & ELLIPTIC DRUMHEADS
 *
 * Since cos(ix) = cosh(x), the inner function simplifies to:
 *
 *   f(z) = 1.3 · cosh(sin(z))
 *
 * For REAL z: sin(z) ∈ [−1, 1] ⟹ cosh(sin(z)) ∈ [1, cosh(1)] ≈ [1, 1.543].
 * The output is BOUNDED near the real axis — unlike most inner functions that
 * blow up or collapse, f6 stays in the narrow range [1.3, 2.011] on ℝ.
 *
 * For COMPLEX z: sin(z) can have large imaginary part, making cosh(sin(z))
 * grow exponentially in some directions.  This creates a BLOOM structure:
 * bounded "petals" near the real axis surrounded by rapidly diverging regions.
 *
 * THE MATHIEU EQUATION (Émile Léonard Mathieu, 1868):
 *
 *   d²y/dt² + [a − 2q·cos(2t)]·y = 0
 *
 * This governs phenomena that appear across physics and engineering:
 *   • ELLIPTIC DRUMHEAD MODES — the vibration frequencies of an elliptic
 *     membrane satisfy the Mathieu equation.  The eigenfunctions are
 *     Mathieu functions (ce_n, se_n), which are the "drum voice" of ellipses.
 *   • PARAMETRIC RESONANCE — a pendulum whose pivot oscillates vertically
 *     obeys the Mathieu equation.  For certain (a, q) pairs, small perturbations
 *     grow exponentially (INSTABILITY TONGUES), exactly the regions where
 *     a child pumping a swing "resonates" into large oscillations.
 *   • PAUL TRAP — the 3D ion trap (Nobel Prize in Physics 1989, Wolfgang Paul)
 *     confines charged particles via oscillating electric fields governed by
 *     Mathieu equations in all three coordinates simultaneously.
 *   • BLOCH WAVES — electrons in a periodic crystal potential satisfy the
 *     Mathieu equation in the nearly-free-electron approximation.  The
 *     BAND GAPS (forbidden energies) correspond to Mathieu instability tongues.
 *   • SATURN'S RINGS — the orbital stability of ring particles perturbed by
 *     Saturn's moons is governed by the Mathieu equation; gaps in the rings
 *     (Cassini Division, Kirkwood gaps) are Mathieu instability zones.
 *
 * CONNECTION TO f6:
 *   cosh(sin(z)) appears naturally in the GENERATING FUNCTION of Mathieu
 *   functions via the identity:
 *     exp(h·cos(θ)) = Σ Iₙ(h)·eⁱⁿᶿ  (modified Bessel expansion)
 *   where Iₙ(h) are the modified Bessel functions of the first kind.
 *   The "bloom" structure — stable petals surrounded by unstable growth —
 *   directly mirrors the STABILITY CHART of the Mathieu equation: stable
 *   (a,q) pairs (the Arnold tongues' complement) map to bounded behavior,
 *   unstable pairs map to exponential growth.
 *
 * AFFINE ANGLES — EULER'S NUMBER AND THE HALF-CIRCLE CONSTANT:
 *
 *   PRE-ROTATION = arctan(1/e) ≈ 20.26°
 *     cos(arctan(1/e)) = e/√(e²+1) ≈ 0.9384
 *     sin(arctan(1/e)) = 1/√(e²+1) ≈ 0.3453
 *     e ≈ 2.7183 is Euler's number, base of the natural logarithm.  It appears
 *     in the Taylor series of all transcendental functions involved in f6 —
 *     cosh(z) = (e^z + e^{-z})/2, sin(z) = Im(e^{iz}).  Using arctan(1/e)
 *     encodes e as the SLOPE of the pre-affine transform, creating a
 *     contraction whose frequency content mirrors the exponential structure
 *     of cosh.  This is the angle whose tangent is the reciprocal of the
 *     same constant that drives exponential growth in the outer function.
 *
 *   POST-ROTATION = −arctan(π/2) ≈ −57.52°
 *     cos(arctan(π/2)) = 2/√(4+π²) ≈ 0.5371
 *     sin(arctan(π/2)) = π/√(4+π²) ≈ 0.8437  [using 2 and π as the legs]
 *     arctan(π/2) is the angle in the RIGHT TRIANGLE with legs 2 and π.
 *     This triangle encodes the EULER-WALLIS PRODUCT π/2 = ∏(4n²/(4n²−1)) —
 *     the fundamental relation between π and the even integers.  In the
 *     Mathieu stability chart, π/2 is precisely the boundary of the first
 *     instability tongue (q = π/2 marks the onset of strong parametric
 *     resonance for zero detuning a = 0).
 *
 *   The near-unity pre-magnitude (1.00) reflects f6's bounded nature near
 *   the real axis.  Post-magnitude 0.83 maintains stability against the
 *   exponential blow-up in complex sectors.
 *
 * COLOR MODE 3 — ALIVE:
 *
 *   Mode 3 drives hue via a breathing velocity:
 *     hue += (vel_base + vel_breath·sin(freq·t)) · dt
 *   The bloom's petal structure — stable bounded regions surrounded by
 *   rapidly growing ones — creates spatial variation that the alive mode
 *   reads as differential "aging": petal cores develop slowly, edges fast.
 *   The chroma breathing IS the Mathieu parametric resonance: when the
 *   forcing frequency matches the natural frequency, amplitude grows —
 *   the visual equivalent of a child timing their leg pumps on a swing.
 *
 * Pipeline: transform → reduce(torus) → sample → color(mode3) → seed → blend
 */
inline Pattern f6_mathieu_resonance_bloom() {
  // arctan(1/e): e is the base of cosh and sin's Taylor series
  // cos = e/√(e²+1), sin = 1/√(e²+1)  [e ≈ 2.7183, e²+1 ≈ 8.389]
  constexpr float CE    =  0.9384f;  // e/√(e²+1) ≈ cos(arctan(1/e))
  constexpr float SE    =  0.3453f;  // 1/√(e²+1) ≈ sin(arctan(1/e))
  // -arctan(π/2): legs (2, π) in right triangle; Euler-Wallis / Mathieu tongue boundary
  // cos = 2/√(4+π²), sin(negated) = -π/√(4+π²)  [4+π² ≈ 13.870]
  constexpr float CPI2  =  0.5371f;  // 2/√(4+π²) = cos(arctan(π/2))
  constexpr float SPI2N = -0.8437f;  // -π/√(4+π²) = -sin(arctan(π/2))

  constexpr float PRE  = 1.00f;  // near-unity: f6 bounded near real axis
  constexpr float POST = 0.83f;  // moderate: guard against complex-sector blow-up

  Pattern p;
  p.name = "f6_mathieu_resonance_bloom";
  // Named-enum path: InnerFunction::F6 ↔ GPU id 26 (1.3·cos(i·sin(z)))
  p.inner = InnerFunction::F6;
  p.extended_inner_id = -1;
  p.reduction = ReductionType::Torus;

  // Pre-rotate by arctan(1/e): Euler's number encodes the exponential growth rate
  p.zn.zn[0] = {CE    * PRE,  SE    * PRE};
  p.zn.zn[1] = {0.001f,       0.004f};    // asymmetric seed: breaks cosh even symmetry
  // Post-rotate by -arctan(π/2): Mathieu instability tongue boundary angle
  p.zn.zn[2] = {CPI2  * POST, SPI2N * POST};
  p.zn.zn[3] = {0.0f,         0.0f};

  // ── Color: MODE 3 (Alive) — parametric resonance as visual hue velocity ──
  p.color.hue_rotation      = 0.011f;   // moderate: alive breathing carries the hue
  p.color.sat_scale         = 1.15f;    // vivid: resonance bloom warmth
  p.color.fade              = 0.987f;   // moderate: ~77 frame memory (bloom refresh)
  p.color.color_mode        = as_int(ColorMode::Alive);        // Alive: differential petal aging
  p.color.spiral_angle_coeff  = 1.05f;  // moderate: reveals angular petal structure
  p.color.spiral_radius_coeff = 0.48f;  // moderate: radial distance coding
  // PARAMETRIC RESONANCE: amplitude grows when forcing matches natural frequency
  p.color.chroma_breath_amp   = 0.28f;  // Mathieu resonance amplitude envelope
  p.color.chroma_breath_freq  = 0.072f; // near-resonance forcing frequency
  p.color.chroma_breath_phase = 0.52f;  // phase: spatial gradient across bloom
  // Alive hue velocity — petal motion differential
  p.color.hue_velocity_base   = 0.30f;  // base drift: bloom petals slowly rotating
  p.color.hue_velocity_breath = 0.24f;  // resonance modulation of rotation speed
  p.color.hue_velocity_drift  = 0.42f;  // slow drift: stability tongue wandering
  p.color.luma_shape_depth    = 0.18f;  // moderate: petal interior vs edge contrast
  p.color.luma_shape_center   = 1.80f;

  // ── Seed: amber — organic Mathieu/biological resonance warmth ──
  p.seed.border_width = 0.011f;
  p.seed.hue          = 0.24f;    // amber/orange: warm resonance, biological systems
  p.seed.hue_speed    = 0.030f;
  p.seed.alpha        = 0.88f;

  p.speed = 0.52f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.124f;
  p.zn.zn0_anim_amp = 0.043f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.156f;
  p.zn.zn1_anim_amp = 0.041f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.108f;
  p.seed.pulse_amp = 0.18f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.056f,
                            .amplitude = 0.134f,
                            .offset = 0.480f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.077f,
                            .amplitude = 0.120f,
                            .offset = 0.280f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.054f,
                            .amplitude = 0.030f,
                            .offset = 0.030f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
