#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh superconductor vortex abyss preset.
 *
 * CONDENSED MATTER ELECTRODYNAMICS: SUPERCONDUCTIVITY & THE ABRIKOSOV LATTICE
 *
 * In a TYPE II SUPERCONDUCTOR, magnetic flux penetrates the material in
 * QUANTIZED VORTICES — thin tubes of normal metal threading through the
 * superconducting bulk.  Each vortex carries exactly one FLUX QUANTUM:
 *
 *   Phi_0 = h/(2e) ≈ 2.068 × 10^{-15} Wb
 *
 * The factor of 2e (not e) reflects the COOPER PAIR: two electrons bound
 * together by phonon-mediated attraction.  This pairing was explained by
 * Bardeen, Cooper, and Schrieffer (BCS theory, Nobel Prize 1972).
 *
 * THE ABRIKOSOV VORTEX LATTICE (Nobel Prize 2003):
 * Above the lower critical field H_c1, vortices penetrate and arrange
 * themselves in a TRIANGULAR LATTICE — the minimum-energy configuration.
 * This is observable directly with scanning tunneling microscopy and
 * Bitter decoration: a perfect hexagonal array of flux tubes, each one
 * a tiny tornado of supercurrent circulating around a normal-metal core.
 *
 * THE BCS UNIVERSAL RATIO:
 * At T=0, the superconducting gap Δ₀ and critical temperature T_c
 * are related by a UNIVERSAL constant:
 *
 *   2Δ₀/(k_B × T_c) = 2π/e^γ ≈ 3.528
 *
 * where γ ≈ 0.5772 is the EULER-MASCHERONI CONSTANT — the additive
 * correction to the harmonic series: H_n = Σ(1/k, k=1..n) ≈ ln(n) + γ.
 * This universal ratio holds for ALL weakly-coupled superconductors
 * (Al, Sn, In, V) to remarkable precision.
 *
 * ENGINE MAPPING TO THE VORTEX LATTICE:
 *
 *   THE NORMAL/SUPERCONDUCTING SWITCH = Tanh inner function
 *     Inside each vortex core (~coherence length ξ wide), the material
 *     is NORMAL (resistive).  Outside, it is SUPERCONDUCTING (zero
 *     resistance).  This binary state is described by the Ginzburg-Landau
 *     ORDER PARAMETER ψ(r): |ψ|² = 0 (normal) or |ψ|² = |ψ_∞|²
 *     (superconducting).  The transition is SIGMOIDAL — tanh profiles
 *     appear explicitly in the GL vortex solution:
 *
 *       |ψ(r)| ∝ tanh(r/(√2 × ξ))
 *
 *     tanh's ±1 saturation IS the order parameter: -1 = normal core,
 *     +1 = superconducting bulk.  Under iteration, the dense tanh web
 *     IS the Abrikosov lattice — a periodic array of normal cores
 *     (tanh → 0 at centers) surrounded by superconducting bulk (tanh → ±1).
 *
 *   THE VORTEX LATTICE = Grid reduction
 *     Grid tiles are the WIGNER-SEITZ CELLS of the Abrikosov lattice.
 *     Each tile contains exactly one flux quantum Φ₀.  The tile boundary
 *     is where the supercurrent from adjacent vortices cancels — the
 *     DOMAIN WALL between neighboring vortex cells.  Grid's visible
 *     tile boundaries ARE these domain walls.
 *
 *   THE DIVERGENCE CONSTANT = γ/π ≈ 0.1837 rad (10.52°) pre-rotation
 *     γ ≈ 0.5772: the Euler-Mascheroni constant — the universal constant
 *     of LOGARITHMIC DIVERGENCE.  It appears in:
 *       - The harmonic series: H_n ≈ ln(n) + γ
 *       - The BCS gap: Δ₀ = (2ω_D/e^γ) × exp(-1/N₀V)
 *       - Renormalization in QED: divergent loop integrals → γ terms
 *       - The digamma function: ψ(1) = -γ
 *       - Mertens' theorem: lim Π(1-1/p) × ln(N) = e^{-γ}
 *
 *     γ/π: the divergence constant measured in HALF-TURNS — how far
 *     from periodicity the logarithmic singularity reaches.
 *
 *     cos(γ/π) ≈ 0.9832, sin(γ/π) ≈ 0.1827
 *     360°/10.52° ≈ 34.2 → quasi-34-fold symmetry.
 *     Each sector represents one vortex in the Abrikosov lattice.
 *     34 = Fibonacci number (F₉), connecting crystallography to biology.
 *
 *   THE BCS GAP = -arctan(e^γ) ≈ -60.67° post-rotation
 *     e^γ ≈ 1.7811: the exponential of the Euler-Mascheroni constant.
 *     This universal factor appears in the BCS gap equation:
 *
 *       Δ₀/(k_B T_c) = π/e^γ ≈ 1.764
 *
 *     e^γ also appears in Mertens' third theorem (prime number theory):
 *     the product over primes Π(1-1/p) ≈ e^{-γ}/ln(N).
 *
 *     cos(arctan(e^γ)) ≈ 0.4896
 *     sin(-arctan(e^γ)) ≈ -0.8719
 *     360°/60.67° ≈ 5.93 → quasi-6-fold depth symmetry.
 *     6-fold: the hexagonal symmetry of the Abrikosov TRIANGULAR lattice!
 *     A triangular lattice has 6-fold rotational symmetry.
 *
 *     Combined: 34-fold × 6-fold = ~202 interference cells.
 *
 *   THE SUPERCURRENT SPIRAL = Mode 1 (Spiral)
 *     Around each vortex core, the supercurrent circulates:
 *     J_s = (n_s e*²/m*)(ℏ∇φ - 2eA) where φ is the GL phase.
 *     Mode 1's spiral coloring traces these circulating supercurrents:
 *     the hue wraps around each vortex core, visualizing the 2π phase
 *     winding.  The log(r) component shows the radial decay of the
 *     supercurrent density away from the core.
 *
 *     MAXIMUM CALIBRATION: matching the densest preset (ramanujan):
 *     fade=0.993, sat_scale=1.18, luma_shape_depth=0.28, speed=0.32
 *
 * WHAT TO WATCH:
 *   1. 34-FOLD LATTICE: Abrikosov vortex array (count the cores!)
 *   2. FLUX QUANTA: each Grid tile = one Φ₀ = h/(2e)
 *   3. SUPERCURRENT SPIRALS: Mode 1 traces phase winding around cores
 *   4. 6-FOLD DEPTH: hexagonal triangular-lattice symmetry in stacking
 *   5. DOMAIN WALLS: tile boundaries = cancellation of adjacent currents
 *   6. THE BCS RATIO: π/e^γ ≈ 1.764 encoded in the depth rotation
 *   7. tanh PROFILE: |ψ| ∝ tanh(r/ξ) IS the vortex core shape
 *
 * Pipeline: transform -> reduce -> sample -> color(mode1) -> seed -> blend
 */
inline Pattern tanh_superconductor_vortex_abyss() {
  // γ/π ≈ 0.1837 rad (10.52°): Euler-Mascheroni ÷ π
  // The universal divergence per cycle — how far ln(n) deviates from H_n
  // γ appears in BCS theory: Δ₀ = (2ω_D/e^γ)·exp(-1/N₀V)
  // quasi-34-fold: 34 Abrikosov vortices per pattern (34 = Fibonacci F₉)
  constexpr float CV   =  0.9832f;           // cos(γ/π)
  constexpr float SV   =  0.1827f;           // sin(γ/π)
  // -arctan(e^γ) ≈ -1.0595 rad (-60.67°): the BCS gap constant
  // e^γ ≈ 1.7811: BCS universal ratio Δ₀/(kT_c) = π/e^γ ≈ 1.764
  // Mertens' theorem: Π(1-1/p) → e^{-γ}/ln(N) (prime distribution)
  // quasi-6-fold depth: the HEXAGONAL symmetry of the triangular lattice!
  constexpr float CBG  =  0.4896f;           // cos(arctan(e^γ))
  constexpr float SBGN = -0.8719f;           // sin(-arctan(e^γ))

  constexpr float PRE  = 1.14f;  // MAXIMUM tanh expansion: densest vortex web
  constexpr float POST = 0.83f;  // strong contraction: near-critical (product 0.946)

  Pattern p;
  p.name = "tanh_superconductor_vortex_abyss";
  p.inner = InnerFunction::Tanh;             // order parameter: |ψ| ∝ tanh(r/ξ)
  p.reduction = ReductionType::Grid;         // Wigner-Seitz cells of the Abrikosov lattice

  p.zn.zn[0] = {CV * PRE, SV * PRE};
  p.zn.zn[1] = {0.003f, -0.004f};           // perturbation: thermal vortex creep
  p.zn.zn[2] = {CBG * POST, SBGN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 1 (Spiral) — supercurrent circulation ──
  // MAXIMUM CALIBRATION: matching the densest preset (ramanujan_shimmer)
  p.color.hue_rotation = 0.005f;             // very slow: vortex lattice geometry dominates
  p.color.sat_scale = 1.18f;                 // MAXIMUM: iridescent supercurrent brilliance
  p.color.fade = 0.993f;                     // MAXIMUM: ~143 frame persistence (deepest)
  p.color.color_mode = as_int(ColorMode::Spiral);                    // Spiral: supercurrent phase winding
  p.color.spiral_angle_coeff = 0.80f;        // strong: vortex circulation hue wrapping
  p.color.spiral_radius_coeff = 0.60f;       // VERY STRONG: bold depth ring visibility
  // Mode 1 does not use breathing or hue velocity params
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: MAXIMUM vortex core contrast — bright superconductor, dark cores
  p.color.luma_shape_depth = 0.28f;          // MAXIMUM: sharpest vortex core definition
  p.color.luma_shape_center = 2.3f;

  // ── Seed: ultra-minimal — the vortex lattice IS the visual ──
  p.seed.border_width = 0.007f;              // very subtle: lattice dominates completely
  p.seed.hue_speed = 0.03f;                  // very slow: frozen flux contemplation
  p.seed.alpha = 0.85f;

  p.speed = 0.32f;                           // VERY slow: maximum density demands meditation
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.122f;
  p.zn.zn0_anim_amp = 0.052f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.215f;
  p.zn.zn1_anim_amp = 0.036f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.126f;
  p.seed.pulse_amp = 0.19f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.070f,
                            .amplitude = 0.168f,
                            .offset = 0.600f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.109f,
                            .amplitude = 0.050f,
                            .offset = 0.000f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.045f,
                            .amplitude = 0.030f,
                            .offset = 0.030f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
