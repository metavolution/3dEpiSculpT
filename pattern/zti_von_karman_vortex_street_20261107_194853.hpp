#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * ZTI Von Kármán Vortex Street — showcase for GPU inner ID 36.
 *
 *      f(z) = 2z / tan(iz) = 2z · cot(iz) = −2iz · coth(z)
 *
 * MATHEMATICS: VON KÁRMÁN VORTEX STREET — PERIODIC VORTEX SHEDDING
 *
 * Theodor von Kármán (1912, with the stability calculation) showed that
 * when viscous fluid flows past a cylindrical obstacle, the wake does not
 * remain smooth.  Above a critical Reynolds number Re ≈ 40, the shear
 * layers alternately roll up into discrete vortices that shed periodically
 * from each side.  The resulting VORTEX STREET has:
 *   • Two parallel rows of vortices, one per side of the obstacle
 *   • Vortices on opposite rows have opposite circulation (±Γ)
 *   • The street propagates downstream at slightly less than the free stream
 *   • The shedding frequency obeys the STROUHAL NUMBER St = fD/U ≈ 0.2
 *
 * THE STABILITY CRITERION (von Kármán 1912):
 *   For a double infinite row of point vortices in ideal 2D flow, the
 *   only linearly STABLE configuration is:
 *
 *       h/d = (1/π) · arccosh(√2) = arcsinh(1)/π = ln(1+√2)/π ≈ 0.2806
 *
 *   where h is the lateral spacing between rows and d is the downstream
 *   spacing within a row.  arccosh(√2) = arcsinh(1) = ln(1+√2) ≈ 0.8814.
 *   This is the same constant as the SILVER RATIO threshold (arcsinh(1) =
 *   ln(δ_S) where δ_S = 1+√2) that appeared in the calcium-wave preset —
 *   a remarkable appearance of the same constant in two different systems.
 *
 * COMPLEX POTENTIAL THEORY:
 *
 *   In 2D ideal flow, every irrotational flow is the real part of an
 *   analytic function W(z) = φ + iψ (complex velocity potential).
 *   The complex velocity is dW/dz = u − iv.
 *
 *   A SINGLE POINT VORTEX of circulation Γ at position z₀ has:
 *     W(z) = (iΓ/2π)·ln(z − z₀)
 *
 *   An INFINITE ROW of vortices at z₀ + nd (n ∈ ℤ) has complex potential:
 *     W(z) = (iΓ/2π)·ln[sin(π(z−z₀)/d)] ∝ cot or tan functions
 *
 * THE ZTI MAPPING — COTANGENT VORTEX LATTICE:
 *
 *   zti = 2z/tan(iz) = 2z·cot(iz).
 *   Since tan(iz) = i·tanh(z), we get:
 *
 *     zti = 2z/(i·tanh(z)) = −2iz·coth(z)
 *
 *   POLES of coth(z): where sinh(z) = 0 → z = nπi (n ∈ ℤ, n ≠ 0).
 *   These poles form a LATTICE ALONG THE IMAGINARY AXIS:
 *     z = ±πi,  ±2πi,  ±3πi, ...
 *   — exactly the positions of a vortex row!  Each pole corresponds to
 *   one vortex in the street, and the alternating ± sign of coth's
 *   residues (residue = ±1 at each pole) corresponds to the alternating
 *   circulation of real vortex streets.
 *
 *   For large |Re(z)|: coth(z) → 1, so zti → −2iz — a pure rotation.
 *   For small |z|: coth(z) ≈ 1/z, so zti → −2iz·(1/z) = −2i — constant.
 *   This GLOBAL BOUNDEDNESS on the real axis makes zti uniquely stable
 *   among extended inner functions (unlike exp or sinh which blow up).
 *
 * STROUHAL NUMBER AND SHEDDING FREQUENCY:
 *
 *   The vortex street's temporal periodicity (Strouhal St ≈ 0.2) appears
 *   as the DRIVING FREQUENCY in the mode's color animation.  The pattern
 *   "sheds" new vortex cores periodically just as the real fluid does.
 *
 * AFFINE ANGLES — HEPTAGONAL SYMMETRY AND VON KÁRMÁN STABILITY ANGLE:
 *
 *   PRE-ROTATION = 2π/7 ≈ 51.43° (heptagonal)
 *     cos(2π/7) ≈ 0.6235,  sin(2π/7) ≈ 0.7818
 *     7 is prime.  Real vortex streets have 2-fold symmetry (two rows);
 *     2×7 = 14-fold quasi-symmetry creates 14 depth layers before near-
 *     repetition, giving the maximal aperiodic complexity for prime × 2.
 *     7 also appears in the HEPTAGONAL NUMBERS and in the 7-beat rhythm
 *     of the vortex street's characteristic sound — the Aeolian tone.
 *
 *   POST-ROTATION = −arcsinh(1) ≈ −50.49° (von Kármán stability angle)
 *     arcsinh(1) = ln(1+√2) ≈ 0.8814 rad
 *     cos(arcsinh(1)) ≈ 0.6368,  sin(−arcsinh(1)) ≈ −0.7710
 *     This is the EXACT angle from the von Kármán stability criterion:
 *     the angular opening arcsinh(1) is the half-angle of the stable
 *     vortex lattice.  By encoding this angle in the post-rotation, every
 *     depth layer is separated by the critical spacing that makes the
 *     real vortex street stable.  The post-rotation literally implements
 *     von Kármán's 1912 stability calculation as a geometric transformation.
 *
 *   CONTRACTION: PRE=0.90, POST=0.88 — net 0.792.
 *     zti is bounded on the real axis but has poles at z = nπi.
 *     The strong contraction keeps iterates away from the pole lattice.
 *
 * COLOR MODE 4 — SPATIAL WAVE:
 *
 *   h_spatial = hue_time + wave_phase(z) = t·vel + a·angle(z) + r·log|z|
 *   The spatial wave maps each pixel's position to a phase in the vortex
 *   street's cycle.  The result: each vortex core acquires a characteristic
 *   color band, and the inter-vortex boundaries become sharp phase-transition
 *   lines — the visual equivalent of the real fluid's vorticity sheets.
 *
 * Pipeline: transform → reduce(torus) → sample → color(mode4) → seed → blend
 */
inline Pattern zti_von_karman_vortex_street() {
  // 2π/7: heptagonal pre-rotation (7-fold + 2-fold vortex rows = 14-fold depth)
  constexpr float C7    =  0.6235f;  // cos(2π/7)
  constexpr float S7    =  0.7818f;  // sin(2π/7)
  // -arcsinh(1) = -ln(1+√2) ≈ -0.8814 rad: von Kármán vortex stability angle
  // cos(arcsinh(1)) ≈ 0.6368, sin(-arcsinh(1)) ≈ -0.7710
  constexpr float CVKS  =  0.6368f;  // cos(arcsinh(1))
  constexpr float SVKSN = -0.7710f;  // sin(-arcsinh(1))

  constexpr float PRE  = 0.90f;  // moderate: keep clear of imaginary-axis poles
  constexpr float POST = 0.88f;  // moderate: persistent vortex energy

  Pattern p;
  p.name = "zti_von_karman_vortex_street";
  // Named-enum path: InnerFunction::ZTI ↔ GPU id 36 (2z/tan(iz))
  p.inner = InnerFunction::ZTI;
  p.extended_inner_id = -1;
  p.reduction = ReductionType::Torus;

  // Pre-rotate by 2π/7: heptagonal + 2-row vortex street = 14-fold
  p.zn.zn[0] = {C7    * PRE,  S7    * PRE};
  p.zn.zn[1] = {0.0f,        -0.003f};    // tiny perturbation: symmetry-breaking
  // Post-rotate by -arcsinh(1): the von Kármán vortex street stability angle
  p.zn.zn[2] = {CVKS  * POST, SVKSN * POST};
  p.zn.zn[3] = {0.0f,         0.0f};

  // ── Color: MODE 4 (Spatial Wave) — vortex cores as phase-coherent domains ──
  p.color.hue_rotation      = 0.004f;   // slow: spatial wave dominates hue
  p.color.sat_scale         = 1.25f;    // vivid: fluid dynamics turbulent energy
  p.color.fade              = 0.992f;   // long: ~125 frame vortex street memory
  p.color.color_mode        = as_int(ColorMode::SpatialWave);        // Spatial Wave: vortex core domain coloring
  p.color.spiral_angle_coeff  = 1.15f;  // moderate: angular vortex street structure
  p.color.spiral_radius_coeff = 0.90f;  // strong log(r): vortex energy cascade depth
  p.color.chroma_breath_amp   = 0.25f;  // Strouhal periodic shedding
  p.color.chroma_breath_freq  = 0.65f;  // St ≈ 0.2 (shedding frequency ∝ 0.65)
  p.color.chroma_breath_phase = 0.0f;   // synchronized: all vortices in phase
  // Alive hue velocity — downstream convection of vortex pattern
  p.color.hue_velocity_base   = 0.38f;  // base: street convects downstream
  p.color.hue_velocity_breath = 0.12f;  // Strouhal shedding modulation
  p.color.hue_velocity_drift  = 0.50f;  // strong: spatial wave dominant
  p.color.luma_shape_depth    = 0.28f;  // strong: vortex core darkening (low pressure)
  p.color.luma_shape_center   = 1.60f;

  // ── Seed: cyan-blue — fluid dynamics, water tunnel visualization ──
  p.seed.border_width = 0.009f;
  p.seed.hue          = 0.60f;    // cyan-blue: water tunnel, ink visualization
  p.seed.hue_speed    = 0.010f;
  p.seed.alpha        = 0.82f;

  p.speed = 0.36f;                // moderate: watch vortex street form and propagate
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.117f;
  p.zn.zn0_anim_amp = 0.052f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.199f;
  p.zn.zn1_anim_amp = 0.040f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.089f;
  p.seed.pulse_amp = 0.18f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.071f,
                            .amplitude = 0.252f,
                            .offset = 0.900f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.082f,
                            .amplitude = 0.120f,
                            .offset = 0.250f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.073f,
                            .amplitude = 0.030f,
                            .offset = 0.010f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
