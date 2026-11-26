#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * F3 Double-Wave Vortex — showcase for GPU inner ID 23.
 *
 *      f(z) = sin(z²) · sin(iz) = i · sin(z²) · sinh(z)
 *
 * MATHEMATICS: TWO-FREQUENCY QUANTUM INTERFERENCE
 *
 * The inner function f3 is the product of two oscillating factors:
 *
 *   FACTOR 1 — sin(z²): QUADRATIC PHASE / FRESNEL OSCILLATION
 *     The argument z² gives a phase that grows QUADRATICALLY with |z|.
 *     sin(z²) has zeros at z = √(nπ) (n = 0,1,2,...), which lie on a
 *     SQUARE-ROOT SPIRAL in the complex plane: |zₙ| = √(nπ), arg(zₙ) = 0.
 *     The spacing between consecutive zeros SHRINKS as n grows: the zeros
 *     become increasingly dense at large |z|, creating finer and finer
 *     oscillation — a CHIRP SIGNAL (frequency increases with distance).
 *     This is precisely the mathematical structure of a FRESNEL ZONE PLATE:
 *     the rings at radii √(nπ) are the constructive/destructive interference
 *     fringes of a point-source diffraction pattern.
 *
 *   FACTOR 2 — sin(iz) = i·sinh(z): HYPERBOLIC GROWTH
 *     sinh(z) has zeros at z = nπi (imaginary axis), growing exponentially
 *     elsewhere.  The multiplication by i rotates the output 90° in the
 *     complex plane, coupling real and imaginary components.
 *
 *   THE PRODUCT — BEATING BETWEEN INCOMMENSURABLE MODES:
 *     sin(z²) zeros form a SPIRAL LATTICE (radii √(nπ), quadratic growth).
 *     sinh(z) zeros form a LINEAR LATTICE (at nπi, linear growth).
 *     These two zero lattices are INCOMMENSURABLE — no algebraic relation
 *     connects √(nπ) to the imaginary axis spacing π.  The beating between
 *     them creates a MOIRÉ PATTERN of combined zeros whose large-scale
 *     structure has no periodicity: every new beat produces a genuinely
 *     new zero arrangement.
 *
 * QUANTUM MECHANICS CONNECTION:
 *
 *   sin(z²) is the integrand of the FRESNEL INTEGRAL C(x) = ∫₀ˣ cos(πt²/2)dt
 *   (optical diffraction).  The zeros of sin(z²) are the half-period rings
 *   of the Fresnel zone plate that divides wavefronts into zones of
 *   constructive vs destructive interference.
 *
 *   sinh(z): The imaginary-axis zeros of sinh are the POLE POSITIONS of
 *   the BOSE-EINSTEIN distribution g(z) = 1/(e^z - 1) at z = 2πin.
 *   Under iteration, the product sin(z²)·sinh(z) creates patterns that
 *   resemble the NODAL LINES of a 2D QUANTUM ANHARMONIC OSCILLATOR —
 *   the regions where the wavefunction vanishes, separating probability-
 *   amplitude domains of opposite sign.
 *
 * AFFINE ANGLES — GOLDEN RATIO AND HERMITE CONSTANT:
 *
 *   PRE-ROTATION = arctan(φ) ≈ 58.17°,   φ = (1+√5)/2 ≈ 1.6180
 *     cos(arctan(φ)) = 1/√(1+φ²) = 1/√(2+φ) ≈ 0.5257
 *     sin(arctan(φ)) = φ/√(1+φ²) = φ/√(2+φ) ≈ 0.8507
 *     Since φ² = φ+1, we have 1+φ² = 2+φ, giving cos = 1/√(2+φ), sin = φ/√(2+φ).
 *     This angle is the arctangent of the GOLDEN RATIO — the most irrational
 *     number, optimal for aperiodic layering in the quadratic phase structure.
 *     The golden angle (2π/φ²) uses φ for angular spacing; here we use it
 *     for the SLOPE of the pre-affine contraction, maximizing the density
 *     of distinct sin(z²) zero encounters per iteration.
 *
 *   POST-ROTATION = −π/3 ≈ −60° (exact)
 *     The HERMITE CONSTANT γ₂ = 2/√3: the optimal packing density in 2D
 *     (hexagonal lattice) is π/(2√3), and the angle between nearest-neighbor
 *     directions is π/3 = 60°.  The sin(iz) zeros at nπi lie along the
 *     imaginary axis; a post-rotation of −π/3 tilts the depth layers by the
 *     hexagonal lattice angle, creating interference between the quadratic
 *     (Fresnel) spiral and the hexagonal zero lattice of the hyperbolic factor.
 *
 *   CONTRACTION: PRE=1.05, POST=0.81 — net 0.850.
 *     sin(z²) grows quadratically in some sectors; moderate expansion +
 *     strong post-contraction maintains stability.
 *
 * COLOR MODE 2 — BREATHING:
 *
 *   The breathing IS the quantum beat: the two incommensurable zero lattices
 *   create a low-frequency envelope (the beat frequency) superimposed on the
 *   high-frequency oscillation (the carrier).  The breathing amplitude reveals
 *   this envelope as a visible pulsation — the pattern literally breathes
 *   at the beat frequency of the two interfering modes.
 *
 * Pipeline: transform → reduce(torus) → sample → color(mode2) → seed → blend
 */
inline Pattern f3_double_wave_vortex() {
  // arctan(φ) where φ = (1+√5)/2 ≈ 1.6180 (golden ratio)
  // cos = 1/√(2+φ) ≈ 0.5257,  sin = φ/√(2+φ) ≈ 0.8507
  // 1+φ² = 2+φ (from φ²=φ+1): unique identity encoding φ in both trig values
  constexpr float CGR   =  0.5257f;  // cos(arctan(φ))
  constexpr float SGR   =  0.8507f;  // sin(arctan(φ))
  // -π/3: hexagonal lattice angle (Hermite constant dimension 2)
  // cos(π/3) = 0.5000, sin(-π/3) = -0.8660
  constexpr float CH3   =  0.5000f;  // cos(π/3) = 1/2
  constexpr float SH3N  = -0.8660f;  // -sin(π/3) = -√3/2

  constexpr float PRE  = 1.4069f;  // moderate expansion: Fresnel spiral density
  constexpr float POST = 1.2427f;  // strong contraction: tame quadratic growth

  Pattern p;
  p.name = "f3_double_wave_vortex";
  // Named-enum path: InnerFunction::F3 ↔ GPU id 23 (sin(z²)·sin(iz))
  p.inner = InnerFunction::F3;
  p.extended_inner_id = -1;
  p.reduction = ReductionType::Torus;

  // Pre-rotate by arctan(φ): golden ratio slope for maximal Fresnel zone coverage
  p.zn.zn[0] = {CGR  * PRE,  SGR  * PRE};
  p.zn.zn[1] = {0.0352f, -0.0348f};   // asymmetric perturbation: breaks z²↔-z² degeneracy
  // Post-rotate by -π/3: hexagonal lattice angle (optimal 2D packing)
  p.zn.zn[2] = {CH3  * POST, SH3N * POST};
  p.zn.zn[3] = {0.0f,         0.0f};

  // ── Color: MODE 2 (Breathing) — quantum beat visible as rhythmic pulsation ──
  p.color.hue_rotation      = 0.009f;   // moderate: beat frequency evolution
  p.color.sat_scale         = 1.08f;    // moderate: quantum interference warmth
  p.color.fade              = 0.991f;   // deep: ~111 frame zero-lattice memory
  p.color.color_mode        = as_int(ColorMode::Breathing);        // Breathing: two-frequency beat envelope
  p.color.spiral_angle_coeff  = 0.80f;  // moderate: angular Fresnel zone structure
  p.color.spiral_radius_coeff = 0.45f;  // moderate: radial zero-spiral depth
  // THE BEAT: Fresnel (quadratic phase) × hyperbolic (linear phase) envelope
  p.color.chroma_breath_amp   = 0.38f;  // strong: incommensurable mode beating
  p.color.chroma_breath_freq  = 0.10f;  // Fresnel/Bose-Einstein beat period
  p.color.chroma_breath_phase = 1.20f;  // spatial: propagating interference wave
  // Mode 2 does not use hue velocity
  p.color.hue_velocity_base   = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift  = 0.0f;
  p.color.luma_shape_depth    = 0.22f;  // moderate: nodal line contrast
  p.color.luma_shape_center   = 2.0f;

  // ── Seed: violet hue — quantum wavefunction character ──
  p.seed.border_width = 0.092f;
  p.seed.hue          = 0.68f;    // violet: quantum probability amplitude
  p.seed.hue_speed    = 0.107f;
  p.seed.alpha        = 0.92f;

  p.speed = 0.46f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.141f;
  p.zn.zn0_anim_amp = 0.047f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.221f;
  p.zn.zn1_anim_amp = 0.050f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.105f;
  p.seed.pulse_amp = 0.21f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.073f,
                            .amplitude = 0.126f,
                            .offset = 0.450f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.104f,
                            .amplitude = 0.120f,
                            .offset = 0.380f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.081f,
                            .amplitude = 0.037f,
                            .offset = 0.107f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
