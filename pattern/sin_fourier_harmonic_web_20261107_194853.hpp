#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Sin Fourier harmonic web preset.
 *
 * MATHEMATICS: FOURIER ANALYSIS — THE LANGUAGE OF WAVE INTERFERENCE
 *
 * In 1822, Joseph Fourier published "Théorie analytique de la chaleur,"
 * showing that ANY periodic function can be decomposed into a sum of
 * sine and cosine waves:
 *
 *   f(x) = Σ (a_n cos(nx) + b_n sin(nx))
 *
 * This was so revolutionary that Lagrange, Laplace, and Legendre debated
 * its validity for decades.  Today, Fourier analysis is the most widely
 * used mathematical tool in science and engineering — from signal
 * processing to quantum mechanics, from image compression to cosmology.
 *
 * THE FOURIER UNCERTAINTY PRINCIPLE:
 * A function cannot be simultaneously localized in both position and
 * frequency:
 *
 *   Δx × Δk ≥ 1/(4π)
 *
 * This is the mathematical foundation of Heisenberg's uncertainty
 * principle: Δx × Δp ≥ ℏ/2 (since p = ℏk).  The sine function IS
 * the eigenfunction of the Fourier transform — it has PERFECTLY DEFINED
 * frequency (Δk = 0) but INFINITE spatial extent (Δx = ∞).
 *
 * MULTI-SLIT DIFFRACTION:
 * When light passes through N narrow slits, the intensity pattern is:
 *
 *   I(θ) = I₀ × (sin(N δ/2) / sin(δ/2))²
 *
 * where δ = (2πd/λ)sin(θ).  This is a SIN-over-SIN pattern that creates
 * N-1 secondary maxima between each pair of principal maxima.  With 15
 * slits (from our 15-fold symmetry), there would be 14 secondary peaks
 * between each pair of bright fringes — dense interference!
 *
 * ENGINE MAPPING TO FOURIER WAVE INTERFERENCE:
 *
 *   THE HARMONIC MODE = Sin inner function
 *     sin(z) IS the fundamental Fourier basis function.  Under iteration,
 *     each step creates a new harmonic: sin(sin(sin(...))) generates
 *     increasingly complex waveforms from the simplest building block.
 *     The resulting web IS the Fourier series made visible — an infinite
 *     sum of harmonics creating a dense interference pattern.
 *
 *     For complex z = x+iy: sin(z) = sin(x)cosh(y) + i·cos(x)sinh(y).
 *     The cosh(y) factor means sin grows EXPONENTIALLY for large |Im(z)|,
 *     creating towering wave crests at the peaks.  Grid reduction
 *     contains this growth, folding the tall crests back into the
 *     fundamental domain — the visual equivalent of spectral aliasing.
 *
 *   THE SPECTRAL GRID = Grid reduction
 *     Grid tiles are FREQUENCY BINS — the discrete spectral cells of
 *     the Fourier decomposition.  Each tile contains one fundamental
 *     harmonic mode.  The tile boundaries are the NYQUIST SAMPLING
 *     POINTS — the critical frequencies where the discrete and
 *     continuous Fourier transforms meet.
 *
 *   THE GOLDEN ROOT = arctan(1/√5) ≈ 24.09° pre-rotation
 *     cos(arctan(1/√5)) = √5/√6 ≈ 0.9129 (EXACT: 5/6 + 1/6 = 1)
 *     sin(arctan(1/√5)) = 1/√6 ≈ 0.4082 (EXACT)
 *
 *     √5 = 2φ - 1 = φ + 1/φ: the algebraic core of the golden ratio.
 *     1/√6 connects to the KISSING NUMBER in 2D: 6 circles can touch
 *     a central circle, and 1/√6 is the sine of the angle between
 *     adjacent contact points projected onto the golden-ratio plane.
 *
 *     360°/24.09° ≈ 14.9 → quasi-15-fold symmetry.
 *     15 harmonic modes = 15 slits in the diffraction grating.
 *     15 = 3×5 = the product of the first two odd primes.
 *
 *   THE PI-ANGLE = -arctan(π) ≈ -72.34° post-rotation
 *     The angle whose tangent IS π — the fundamental period of the
 *     sine function embedded in its own arctangent.  This is the
 *     ultimate SELF-REFERENCE: sin has period 2π, and arctan(π)
 *     encodes π within the angular framework.
 *
 *     cos(arctan(π)) = 1/√(1+π²) ≈ 0.3033
 *     sin(-arctan(π)) = -π/√(1+π²) ≈ -0.9529
 *
 *     1 + π² ≈ 10.870: the Pythagorean hypotenuse of the 1-π right
 *     triangle.  And √(1+π²) ≈ 3.297: the "π-diagonal" that appears
 *     in the spectral theory of periodic operators.
 *
 *     360°/72.34° ≈ 4.98 → quasi-5-fold depth symmetry.
 *     Combined: 15 × 5 = 75 interference cells.
 *
 *   HARMONIC DECOMPOSITION = Mode 1 (Spiral)
 *     Mode 1 maps hue to angle + log(radius).  Each of the 15 angular
 *     sectors receives a distinct hue — visualizing 15 HARMONIC MODES
 *     in the Fourier decomposition.  The log(r) component shows the
 *     AMPLITUDE SPECTRUM: depth rings correspond to the magnitude of
 *     each Fourier coefficient.  The overall spiral IS the spectrogram
 *     of the interference pattern.
 *
 * WHAT TO WATCH:
 *   1. 15 HARMONICS: each sector = one Fourier basis function
 *   2. INTERFERENCE WEB: sin-upon-sin creates dense wave crossing pattern
 *   3. SPECTRAL GRID: tile boundaries = Nyquist sampling frequencies
 *   4. PI-ANGLE DEPTH: π itself structures the layer stacking
 *   5. GOLDEN ROOT: √5/√6 connects Fourier to golden-ratio geometry
 *   6. WAVE CRESTS: bright ridges where constructive interference peaks
 *
 * Pipeline: transform -> reduce -> sample -> color(mode1) -> seed -> blend
 */
inline Pattern sin_fourier_harmonic_web() {
  // arctan(1/√5) ≈ 0.4205 rad (24.09°): golden root harmonic angle
  // cos = √5/√6, sin = 1/√6 — EXACT algebraic (5/6 + 1/6 = 1)
  // √5 = 2φ-1: the golden ratio's algebraic core
  // quasi-15-fold: 15 Fourier harmonic modes
  constexpr float CF   =  0.9129f;           // cos(arctan(1/√5)) = √5/√6
  constexpr float SF   =  0.4082f;           // sin(arctan(1/√5)) = 1/√6
  // -arctan(π) ≈ -1.2626 rad (-72.34°): the pi-angle (tan = π itself!)
  // cos = 1/√(1+π²) ≈ 0.3033: the π-diagonal normalization
  // sin = -π/√(1+π²) ≈ -0.9529: π embedded in angular depth
  constexpr float CPA  =  0.3033f;           // cos(arctan(π)) = 1/√(1+π²)
  constexpr float SPAN = -0.9529f;           // sin(-arctan(π)) = -π/√(1+π²)

  constexpr float PRE  = 0.74f;  // conservative: sin grows via cosh(Im(z))
  constexpr float POST = 0.86f;  // moderate contraction: product 0.636

  Pattern p;
  p.name = "sin_fourier_harmonic_web";
  p.inner = InnerFunction::Sin;              // the Fourier basis function itself
  p.reduction = ReductionType::Grid;         // frequency bins: discrete spectral cells

  p.zn.zn[0] = {CF * PRE, SF * PRE};
  p.zn.zn[1] = {0.003f, -0.003f};           // perturbation: spectral leakage
  p.zn.zn[2] = {CPA * POST, SPAN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 1 (Spiral) — harmonic mode decomposition ──
  p.color.hue_rotation = 0.008f;             // moderate: spectral evolution
  p.color.sat_scale = 1.10f;                 // moderate: clean wave-like clarity
  p.color.fade = 0.988f;                     // deep: ~83 frame persistence
  p.color.color_mode = as_int(ColorMode::Spiral);                    // Spiral: 15 harmonics → spectral rainbow
  p.color.spiral_angle_coeff = 1.20f;        // STRONG: shows 15 harmonic mode sectors
  p.color.spiral_radius_coeff = 0.55f;       // strong: amplitude spectrum depth rings
  // Mode 1 does not use breathing or hue velocity params
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: moderate wave contrast (smooth sinusoidal, not sharp like tanh)
  p.color.luma_shape_depth = 0.20f;          // moderate: soft wave interference fringes
  p.color.luma_shape_center = 2.0f;

  // ── Seed: moderate — injects initial wave perturbation ──
  p.seed.border_width = 0.010f;              // moderate: diffraction slit edges
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 0.86f;

  p.speed = 0.42f;                           // moderate: watch interference build
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.161f;
  p.zn.zn0_anim_amp = 0.046f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.157f;
  p.zn.zn1_anim_amp = 0.049f;
  p.zn.zn1_anim_k = 2.0f;
  p.seed.pulse_freq = 0.097f;
  p.seed.pulse_amp = 0.21f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.067f,
                            .amplitude = 0.154f,
                            .offset = 0.550f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.111f,
                            .amplitude = 0.050f,
                            .offset = 0.000f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.045f,
                            .amplitude = 0.030f,
                            .offset = 0.050f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
