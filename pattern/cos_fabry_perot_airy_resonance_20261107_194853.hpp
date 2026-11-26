#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Fabry-Perot Airy Resonance — thin-film interferometer
 * ================================================================
 *
 * PHYSICS / MATH  (Perot & Fabry 1899; Airy transmission function):
 *
 * A Fabry-Perot étalon is two parallel partially-reflective mirrors
 * separated by an optical path of thickness n·d. Light that enters
 * bounces back and forth, and the transmitted intensity depends on
 * the round-trip phase accumulated between successive bounces:
 *
 *     δ = (4π / λ) · n · d · cos(θ)
 *
 * The steady-state transmission through the cavity is the Airy
 * function:
 *
 *     T(δ) = 1 / (1 + F · sin²(δ/2))
 *
 * where F = 4R / (1-R)² is the coefficient of finesse (R is the
 * mirror reflectivity). The transmission shows VERY SHARP PEAKS at
 * δ = 2πm (constructive interference — all bounces in phase)
 * separated by broad valleys (destructive interference).
 *
 * The ratio of peak spacing to peak FWHM is the finesse ℱ = π√F / 2.
 * For a high-finesse cavity (R ≈ 0.95 → ℱ ≈ 62), transmission peaks
 * are 62× narrower than their spacing — a wavelength discrimination
 * that's the basis of laser cavities, gravitational-wave detectors,
 * and wavelength-division-multiplexing optics.
 *
 *
 * ENGINE MAPPING
 *
 *   PHASE DELAY = Cos inner function
 *     cos(δ/2) is the fundamental carrier of the Airy transmission's
 *     period structure. The cos inner function applied in the complex
 *     plane evaluates cos(x)·cosh(y) - i·sin(x)·sinh(y) — its real
 *     part directly encodes the standing-wave node pattern that's
 *     visually IDENTICAL to the transmission-peak comb of a Fabry-
 *     Perot cavity.
 *
 *   MIRROR-CAVITY BOUNDARIES = Grid reduction
 *     The two parallel mirrors impose strict periodic boundary
 *     conditions on the optical field. Grid reduction maps each
 *     tile boundary to exactly this mirror plane — every bounce
 *     off a grid edge is a physical reflection in the real cavity.
 *     Successive bounces accumulate the δ phase on each pass.
 *
 *   HIGH FINESSE = narrow seed border (0.012)
 *     A narrow border means the field is dominated by INTERFERENCE
 *     between successive bounces, not by fresh injection. This is
 *     exactly the high-finesse regime: R near 1 → most light stays
 *     in the cavity → narrow, sharp transmission peaks emerge.
 *
 *   FREE SPECTRAL RANGE = π/9 pre-rotation (20°)
 *     The pre-rotation creates a 9-fold angular symmetry that
 *     represents the m = ±1, ±2, ±3, ±4 transmission orders of a
 *     cavity tuned for 9 longitudinal modes. Each of the 9 angular
 *     sectors is one free spectral range.
 *
 *   IRRATIONAL POST-ROTATION = √2 rad (≈ 81°) counter-rotation
 *     Real cavities aren't perfectly aligned — mirror misalignment
 *     causes a slow incommensurate drift of the fringe pattern.
 *     Rotating by √2 rad on each iteration is maximally irrational
 *     (quadratic irrational with largest partial-quotient sum),
 *     producing the densely-filled quasiperiodic fringe stacking
 *     seen in real long-cavity experiments.
 *
 *   INTENSITY MODULATION = color mode 2 (Breathing)
 *     Fabry-Perot transmission oscillates over time as the cavity
 *     length drifts thermally (δ evolves). Mode 2's sat breathing
 *     at freq 0.10 + spatial-coupling phase 1.8 directly animates
 *     the transmission peaks with a visible "cavity breathing" —
 *     you can see the whole fringe pattern pulse in intensity as
 *     the cavity's resonance condition sweeps through exact match.
 *
 *
 * WHAT TO WATCH:
 *   1. SHARP BRIGHT PEAKS against a DARKER background — the signature
 *      of high finesse. High-finesse cavities are "comb filters".
 *   2. 9-FOLD ANGULAR REPETITION of the transmission pattern — the
 *      free-spectral-range structure of 9 longitudinal modes.
 *   3. SLOW PULSATION of the whole image — the cavity drift-breathing
 *      as thermal expansion modulates the exact resonance condition.
 *   4. INCOMMENSURATE FRINGE LAYERING — successive reflections never
 *      exactly align (√2 rad post-rotation), so the deep structure
 *      never repeats at any depth.
 *   5. Narrow border keeps the interior feedback-dominated — the
 *      physics of a high-Q optical cavity.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern cos_fabry_perot_airy_resonance() {
  // π/9 ≈ 0.3491 rad (20°) — 9 longitudinal cavity modes per 2π
  constexpr float C9 = 0.9396926208f; // cos(π/9)
  constexpr float S9 = 0.3420201433f; // sin(π/9)

  // √2 rad ≈ 81° — maximally irrational post-rotation (quasiperiodic
  // fringe stacking that mimics real long-cavity misalignment drift).
  constexpr float COS_SQRT2 = 0.1559436247f;  // cos(√2)
  constexpr float SIN_SQRT2_NEG = -0.9878662125f; // sin(-√2) counter-rotation

  // cos is UNBOUNDED along imaginary axis (cosh growth), so scale
  // conservatively per design-guide §3: |zn[0]| = 0.52–0.99 for cos.
  constexpr float PRE  = 1.3994f;  // smooth lens effect, not aggressive
  constexpr float POST = 1.2424f;  // standard contraction
  // Stability product 0.72 · 0.88 = 0.634 — well-bounded for the fine
  // fringe structure to accumulate clearly across many iterations.

  Pattern p;
  p.name = "cos_fabry_perot_airy_resonance";

  // Cos inner: cos(δ/2) IS the carrier of the Airy transmission
  p.inner = InnerFunction::Cos;
  // Grid reduction: the two cavity mirrors at ±1 in unit cell
  p.reduction = ReductionType::Grid;

  // Pre-affine: rotation by π/9 — 9 longitudinal cavity modes
  p.zn.zn[0] = {C9 * PRE, S9 * PRE};
  p.zn.zn[1] = {0.0403f, -0.0328f}; // tiny perturbation = cavity jitter

  // Post-affine: counter-rotation by -√2 rad — irrational misalignment
  p.zn.zn[2] = {COS_SQRT2 * POST, SIN_SQRT2_NEG * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 2 (Breathing) — transmission intensity oscillation ──
  p.color.hue_rotation = 0.013f;
  p.color.sat_scale = 1.10f;
  p.color.fade = 0.987f; // high fade — fringes live long (high Q)
  p.color.color_mode = as_int(ColorMode::Breathing);
  p.color.spiral_angle_coeff = 1.05f; // reveal 9 sectors as distinct hues
  p.color.spiral_radius_coeff = 0.48f; // moderate depth banding
  p.color.chroma_breath_amp = 0.27f;   // visible cavity pulsation
  p.color.chroma_breath_freq = 0.10f;  // slow breathing (thermal timescale)
  p.color.chroma_breath_phase = 1.8f;  // spatial coupling: rotating pulse
  p.color.hue_velocity_base = 0.012f;   // slight hue drift per iterate
  p.color.hue_velocity_breath = 0.008f;
  p.color.hue_velocity_drift = 0.002f;
  p.color.luma_shape_depth = 0.30f;    // strong bright/dark contrast
  p.color.luma_shape_center = 2.1f;    // peak luma at mid-depth

  // ── Seed: NARROW BORDER — high-finesse regime ──
  p.seed.border_width = 0.090f; // thin: feedback-dominated interior
  p.seed.hue_speed = 0.129f;     // slow color cycle
  p.seed.alpha = 0.92f;         // slightly transparent

  // Speed: slow-to-moderate — the eye needs time to resolve the
  // comb structure and the slow thermal-drift pulsation.
  p.speed = 0.66f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.138f;
  p.zn.zn0_anim_amp = 0.035f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.203f;
  p.zn.zn1_anim_amp = 0.051f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.125f;
  p.seed.pulse_amp = 0.16f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.096f,
                            .amplitude = 0.134f,
                            .offset = 0.480f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.085f,
                            .amplitude = 0.120f,
                            .offset = 0.270f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.084f,
                            .amplitude = 0.045f,
                            .offset = 0.129f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
