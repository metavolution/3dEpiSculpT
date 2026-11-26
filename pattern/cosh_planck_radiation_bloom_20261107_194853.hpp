#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Cosh Planck radiation bloom preset.
 *
 * QUANTUM STATISTICAL MECHANICS: THE PLANCK BLACKBODY SPECTRUM
 *
 * On December 14, 1900, Max Planck presented the formula that launched
 * quantum mechanics.  The spectral radiance of a blackbody:
 *
 *   B(ν,T) = (2hν³/c²) × 1/(exp(hν/(kT)) - 1)
 *
 * This deceptively simple equation resolved the "ultraviolet catastrophe"
 * and introduced the quantum hypothesis: energy is emitted in discrete
 * packets E = hν.  The denominator (exp(x) - 1)⁻¹ is the BOSE-EINSTEIN
 * DISTRIBUTION — the average number of photons in a mode at temperature T.
 *
 * THE COSH CONNECTION:
 * The partition function of a quantum harmonic oscillator is:
 *
 *   Z = 1/(2 × sinh(βℏω/2))
 *
 * The mean energy involves COTH = cosh/sinh:
 *
 *   <E> = (ℏω/2) × coth(βℏω/2) = (ℏω/2)(e^x + e^{-x})/(e^x - e^{-x})
 *
 * Cosh IS the thermal equilibrium: it represents the balance between
 * the exponentially growing occupation of high-energy modes (e^x) and
 * the exponentially decaying contribution of low-energy modes (e^{-x}).
 * The Planck spectrum IS cosh/sinh = coth, evaluated at β = 1/(kT).
 *
 * ENGINE MAPPING TO BLACKBODY RADIATION:
 *
 *   THE THERMAL KERNEL = Cosh inner function
 *     cosh(z) = (e^z + e^{-z})/2: the EVEN part of the exponential.
 *     It represents THERMAL SYMMETRY — the equal probability of
 *     excitation (e^z) and de-excitation (e^{-z}).  Under iteration,
 *     Cosh creates bright ridges (high thermal occupation) separated
 *     by dark valleys (low occupation) — the SPECTRAL MODES of the
 *     radiation field.  Each ridge IS one standing wave mode in the
 *     electromagnetic cavity.
 *
 *   THE MODE CAVITY = Grid reduction
 *     A blackbody is an ideal electromagnetic CAVITY — a box with
 *     perfectly reflecting walls.  Inside, standing waves form at
 *     discrete frequencies.  Grid tiles ARE the cavity mode cells:
 *     each tile contains one standing wave mode.  The tile boundaries
 *     are the NODAL PLANES where the electromagnetic field is zero.
 *
 *   THE LEECH DIMENSION = π/12 = 15° pre-rotation
 *     360°/15° = 24 → EXACT 24-fold symmetry.
 *
 *     cos(π/12) = (√6 + √2)/4 ≈ 0.9659
 *     sin(π/12) = (√6 - √2)/4 ≈ 0.2588
 *
 *     These are among the most BEAUTIFUL exact trigonometric values:
 *     NESTED RADICALS involving both √6 and √2 in symmetric sum and
 *     difference.  The identity cos(15°) = cos(45° - 30°) gives these
 *     through the angle subtraction formula.
 *
 *     24 is one of the deepest numbers in mathematics:
 *       - The LEECH LATTICE lives in 24 dimensions — the densest
 *         lattice sphere packing known in ANY dimension
 *       - The bosonic string has 24 transverse dimensions (D=26-2)
 *       - The Dedekind eta: η(τ) = q^{1/24} Π(1-q^n) — 1/24 is the
 *         CASIMIR ENERGY of the string
 *       - 24 = 4! (factorial): the symmetric group S₄ has 24 elements
 *       - 24 is the only n where 1²+2²+...+n² is a perfect square
 *         (70² = 4900 = 24×25×49/6)
 *       - The BINARY GOLAY CODE corrects up to 3 errors in 24 bits
 *
 *     Each of the 24 sectors IS one cavity mode — 24 standing waves
 *     forming the thermal radiation spectrum.
 *
 *   THE GAUSSIAN DEPTH = -arctan(√(π-1)) ≈ -55.62° post-rotation
 *     cos(arctan(√(π-1))) = 1/√π ≈ 0.5642
 *     sin(-arctan(√(π-1))) = -√(π-1)/√π ≈ -0.8257
 *     Verification: 1/π + (π-1)/π = π/π = 1 ✓ (EXACT)
 *
 *     1/√π: the GAUSSIAN NORMALIZATION constant!
 *     The probability density of the standard normal distribution is
 *     f(x) = (1/√(2π)) × exp(-x²/2), and 1/√π appears in the Gamma
 *     function Γ(1/2) = √π, the Wallis product, and the Fresnel
 *     integrals of wave optics.
 *
 *     In the Planck spectrum: the Wien displacement law gives the peak
 *     wavelength λ_max × T = b ≈ 2.898 mm·K.  The SHAPE of the Planck
 *     curve near its peak is approximately Gaussian — and 1/√π is the
 *     normalization that ensures the total radiated power equals σT⁴
 *     (Stefan-Boltzmann law).
 *
 *     360°/55.62° ≈ 6.47 → quasi-6.5-fold depth symmetry.
 *     Combined: 24 × 6.5 ≈ 156 interference cells.
 *
 *   THERMAL OSCILLATION = Mode 2 (Breathing)
 *     The breathing IS the thermal fluctuation of the radiation field:
 *     at finite temperature, photons are randomly emitted and absorbed,
 *     causing the field intensity to oscillate.  The variance of photon
 *     number fluctuations is <(Δn)²> = <n>(<n>+1) — the Bose-Einstein
 *     bunching that distinguishes quantum from classical radiation.
 *
 * WHAT TO WATCH:
 *   1. 24-FOLD MODE LATTICE: cavity standing wave modes in hexagonal array
 *   2. NESTED RADICALS: (√6±√2)/4 — the most elegant trig values
 *   3. THERMAL RIDGES: cosh bright peaks = high photon occupation
 *   4. BREATHING: thermal fluctuation → Bose-Einstein photon bunching
 *   5. GAUSSIAN DEPTH: 1/√π normalizes each cavity layer
 *   6. DARK VALLEYS: inter-ridge gaps = mode nodes (zero electromagnetic field)
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern cosh_planck_radiation_bloom() {
  // π/12 = 15° (0.2618 rad): exact 24-fold — Leech lattice dimensions
  // cos = (√6+√2)/4, sin = (√6-√2)/4 — NESTED RADICALS (rare exact beauty)
  // 24 = bosonic string transverse modes, |S₄|, unique perfect-square sum
  constexpr float CP   =  0.9659f;           // cos(π/12) = (√6+√2)/4
  constexpr float SP   =  0.2588f;           // sin(π/12) = (√6-√2)/4
  // -arctan(√(π-1)) ≈ -0.9710 rad (-55.62°): Gaussian normalization depth
  // cos = 1/√π ≈ 0.5642 (THE normalization constant of probability theory)
  // sin = -√(π-1)/√π ≈ -0.8257; check: 1/π + (π-1)/π = 1 EXACT
  constexpr float CGN  =  0.5642f;           // cos(arctan(√(π-1))) = 1/√π
  constexpr float SGNN = -0.8257f;           // sin(-arctan(√(π-1))) = -√(π-1)/√π

  constexpr float PRE  = 0.78f;  // conservative: Cosh grows fast (cosh ≥ 1)
  constexpr float POST = 0.88f;  // moderate contraction: product 0.686

  Pattern p;
  p.name = "cosh_planck_radiation_bloom";
  p.inner = InnerFunction::Cosh;             // thermal kernel: (e^z + e^{-z})/2
  p.reduction = ReductionType::Grid;         // cavity mode cells: standing wave lattice

  p.zn.zn[0] = {CP * PRE, SP * PRE};
  p.zn.zn[1] = {0.003f, -0.003f};           // perturbation: zero-point fluctuation
  p.zn.zn[2] = {CGN * POST, SGNN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 2 (Breathing) — thermal photon fluctuation ──
  p.color.hue_rotation = 0.010f;             // moderate: spectral temperature drift
  p.color.sat_scale = 1.14f;                 // vivid: thermal radiation glow
  p.color.fade = 0.988f;                     // deep: ~83 frame persistence
  p.color.color_mode = as_int(ColorMode::Breathing);                    // Breathing: Bose-Einstein thermal oscillation
  p.color.spiral_angle_coeff = 0.85f;        // STRONG: shows 24 cavity mode sectors
  p.color.spiral_radius_coeff = 0.50f;       // strong: mode frequency depth rings
  // THERMAL FLUCTUATION: photon bunching oscillation
  p.color.chroma_breath_amp = 0.30f;         // strong: visible thermal pulsation
  p.color.chroma_breath_freq = 0.08f;        // thermal oscillation rate
  p.color.chroma_breath_phase = 1.5f;        // spatial temperature gradient
  // Mode 2 does not use hue velocity params
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: strong thermal ridge contrast (bright cosh peaks, dark nodes)
  p.color.luma_shape_depth = 0.24f;          // strong: crisp cavity mode ridges
  p.color.luma_shape_center = 2.0f;

  // ── Seed: moderate — injects thermal photon seed ──
  p.seed.border_width = 0.010f;              // moderate: cavity wall visible
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 0.86f;

  p.speed = 0.42f;                           // moderate: watch thermal equilibrium form
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.141f;
  p.zn.zn0_anim_amp = 0.050f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.186f;
  p.zn.zn1_anim_amp = 0.037f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.110f;
  p.seed.pulse_amp = 0.19f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.078f,
                            .amplitude = 0.140f,
                            .offset = 0.500f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.085f,
                            .amplitude = 0.120f,
                            .offset = 0.300f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.056f,
                            .amplitude = 0.030f,
                            .offset = 0.050f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
