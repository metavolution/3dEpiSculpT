#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh Ising domain abyss preset.
 *
 * ELECTRODYNAMICS / STATISTICAL MECHANICS: THE ISING MODEL & MAGNETIC DOMAINS
 *
 * The 2D ISING MODEL is the simplest system that exhibits a genuine PHASE
 * TRANSITION — the spontaneous emergence of long-range order from local
 * interactions.  Spins σᵢ = ±1 sit on a square lattice, interacting with
 * nearest neighbors via Hamiltonian:
 *
 *   H = -J Σ_{<i,j>} σᵢσⱼ - h Σᵢ σᵢ
 *
 * Lars Onsager's EXACT SOLUTION (1944) — one of the greatest achievements
 * in theoretical physics — gives the critical temperature:
 *
 *   k_B T_C / J = 2 / ln(1 + √2) ≈ 2.2692
 *
 * Below T_C: spins align → FERROMAGNETIC ORDER (magnetic domains)
 * Above T_C: spins randomize → PARAMAGNETIC DISORDER (no domains)
 * At T_C: CRITICAL POINT — fractal domain walls, power-law correlations,
 *         conformal invariance, and universality.
 *
 * THE DEEP CONNECTION: tanh IS the magnetization function.
 *
 * For a spin-1/2 particle in a magnetic field, the mean magnetization is
 * given by the BRILLOUIN FUNCTION B_{1/2}(x) = tanh(x).  The mean-field
 * self-consistency equation for the Ising model is:
 *
 *   m = tanh(βJzm + βh)    where β = 1/(k_BT), z = coordination number
 *
 * This equation determines the spontaneous magnetization: at T < T_C,
 * it has nonzero solutions m ≠ 0 (ordered phase); at T > T_C, only m = 0
 * (disordered phase).  The BIFURCATION at T = T_C is visible in the preset:
 * the tanh iteration map IS the self-consistency equation being iterated
 * to find fixed points.  The fractal web structure shows ALL possible
 * magnetization states simultaneously.
 *
 * ENGINE MAPPING TO ISING / ELECTRODYNAMICS:
 *
 *   MAGNETIZATION CURVE = Tanh inner function
 *     tanh(z) IS the Brillouin function B_{1/2}(z) for spin-1/2 systems.
 *     Under iteration with |zn0| = 1.10 > 1:
 *       - EXPANSION: pushes past ±1 → spins try to over-align (strong field)
 *       - SATURATION: tanh folds to [-1,1] → magnetization can't exceed ±1
 *       - CONTRACTION: |zn2| = 0.83 → thermal fluctuations reduce order
 *     This expand-saturate-contract cycle IS the competition between
 *     magnetic ordering (exchange J) and thermal disorder (k_BT) that
 *     governs domain formation.
 *
 *   DOMAIN WALLS = Grid reduction
 *     Grid tile boundaries ARE domain walls — the interfaces between
 *     regions of opposite magnetization.  In real ferromagnets, domain
 *     walls have FINITE WIDTH proportional to √(J/K) where K is the
 *     anisotropy.  The Grid boundaries show this: tanh folding creates
 *     a smooth transition zone at each tile edge, not a sharp discontinuity.
 *     The web filaments branching from these walls ARE the fractal
 *     domain wall structure seen at criticality.
 *
 *   CRITICAL TEMPERATURE = k_BT_C/J pre-rotation ≈ 130.0°
 *     2/ln(1+√2) ≈ 2.2692 radians.  The Onsager critical temperature
 *     (EXACT, not approximate!) as a rotation angle creates:
 *       360°/130.0° ≈ 2.77 iterations per revolution
 *     This near-3-fold quasi-symmetry with strong irrational deviation
 *     ensures the domain pattern never repeats — critical fluctuations
 *     at ALL length scales, the hallmark of a phase transition.
 *
 *   ISING COUPLING = -ln(1+√2) post-rotation ≈ -50.5°
 *     At the critical point, the dimensionless coupling is:
 *       βJ = J/(k_BT_C) = ln(1+√2)/2 ≈ 0.4407
 *     But ln(1+√2) ≈ 0.8814 is the FUNDAMENTAL Ising constant —
 *     it appears in the partition function, correlation length, and
 *     spontaneous magnetization.  Using it as the post-rotation creates
 *     a depth twist that encodes the coupling strength geometrically.
 *
 *     The remarkable identity: sinh(ln(1+√2)/2) × cosh(ln(1+√2)/2) =
 *     1/2 × sinh(ln(1+√2)) = 1/2 × √2 = 1/√2.  At the critical point,
 *     the hyperbolic functions evaluate to EXACT algebraic values.
 *
 *   SPATIAL MAGNETIZATION = Mode 4 (Spatial Wave)
 *     The dual-sine spatial hue field maps the LOCAL MAGNETIZATION
 *     direction across the visual plane:
 *       freq1 = √2 ≈ 1.414 — the irrational ratio in sinh(2J/k_BT_C) = 1
 *       freq2 = 1/√2 ≈ 0.707 — its reciprocal (T_C equation symmetry)
 *       phase = ln(1+√2) ≈ 0.881 — the critical coupling constant
 *     The QUASIPERIODIC interference between √2 and 1/√2 creates a
 *     spatial pattern with no translational period — analogous to the
 *     fractal domain structure at criticality.
 *
 *   CORRELATION LENGTH = fade (0.990)
 *     Near T_C, the correlation length ξ diverges as |T-T_C|^{-ν} with
 *     ν = 1 (2D Ising exact).  fade = 0.990 → persistence ~100 frames.
 *     The slow decay means correlations extend across the entire visible
 *     field — long-range order IS emerging.  You can SEE domains growing
 *     to span the system: that's the divergent correlation length.
 *
 * WHAT TO WATCH (the Ising phase transition in action):
 *   1. DOMAINS: large regions of similar hue = aligned spin domains
 *   2. DOMAIN WALLS: Grid boundaries + tanh folds = wall structure
 *   3. FRACTAL WALLS: web filaments at boundaries = critical fluctuations
 *   4. SPATIAL MAGNETIZATION: hue varies across space = m(x,y) field
 *   5. CRITICAL SLOWING: fade=0.990 → domains persist = divergent ξ
 *   6. QUASI-3-FOLD: ~2.77 sectors = near-critical angular symmetry
 *   7. tanh SATURATION: ±1 walls = magnetization bounds |m| ≤ 1
 *   8. TWO LENGTH SCALES: √2 and 1/√2 spatial waves = domain + wall
 *
 * Pipeline: transform -> reduce -> sample -> color(mode4) -> seed -> blend
 */
inline Pattern tanh_ising_domain_abyss() {
  // k_BT_C/J = 2/ln(1+√2) ≈ 2.2692 rad (130.0°): Onsager exact solution
  // The critical temperature of the 2D Ising model on a square lattice
  constexpr float CI   = -0.6429f;           // cos(2/ln(1+√2))
  constexpr float SI   =  0.7660f;           // sin(2/ln(1+√2))
  // -ln(1+√2) ≈ -0.8814 rad (-50.5°): the fundamental Ising coupling
  // sinh(ln(1+√2)) = √2 — connects hyperbolic and algebraic worlds
  constexpr float CL   =  0.6360f;           // cos(ln(1+√2))
  constexpr float SLN  = -0.7718f;           // sin(-ln(1+√2))

  constexpr float PRE  = 1.48f;  // pushes past tanh walls so domain folds reach the seed rim
  constexpr float POST = 0.98f;  // near-critical scaling without trapping inside [-0.83, 0.83]

  Pattern p;
  p.name = "tanh_ising_domain_abyss";
  p.inner = InnerFunction::Tanh;             // tanh = Brillouin B_{1/2} = magnetization
  p.reduction = ReductionType::Grid;         // tile boundaries = magnetic domain walls

  p.zn.zn[0] = {CI * PRE, SI * PRE};
  p.zn.zn[1] = {0.005f, -0.003f};           // perturbation: break perfect lattice symmetry
  p.zn.zn[2] = {CL * POST, SLN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.055f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.041f;
  p.zn.zn1_anim_amp = 0.028f;
  p.zn.zn1_anim_k = 3.0f;

  // ── Color: MODE 4 (Spatial Wave) — magnetization direction field ──
  // Mode 4 param reuse: spiral_angle→hue_x_slope, chroma_breath_amp→freq1,
  // chroma_breath_freq→freq2, chroma_breath_phase→phase
  p.color.hue_rotation = 0.008f;             // slow: domain structure dominates
  p.color.sat_scale = 1.15f;                 // vivid: distinct domain colors
  p.color.fade = 0.990f;                     // CRITICAL SLOWING: ~100 frame persistence
  p.color.color_mode = as_int(ColorMode::SpatialWave);                    // Spatial Wave: m(x,y) magnetization field
  p.color.spiral_angle_coeff = 0.45f;        // mode4: hue_x_slope (magnetization gradient)
  p.color.spiral_radius_coeff = 0.20f;       // mode4: hue_y_slope (perpendicular gradient)
  // DUAL-WAVE MAGNETIZATION: √2 and 1/√2 (Ising critical ratio pair)
  p.color.chroma_breath_amp = 1.414f;        // mode4 freq1: √2 (from sinh(2βJ)=1)
  p.color.chroma_breath_freq = 0.707f;       // mode4 freq2: 1/√2 (reciprocal symmetry)
  p.color.chroma_breath_phase = 0.881f;      // mode4 phase: ln(1+√2) (critical coupling)
  // THREE TIMESCALES: spin flip, domain wall motion, critical fluctuation
  p.color.hue_velocity_base = 0.10f;         // FAST: individual spin flip rate
  p.color.hue_velocity_breath = 0.06f;       // MEDIUM: domain wall creep velocity
  p.color.hue_velocity_drift = 0.03f;        // SLOW: critical fluctuation envelope
  // Luminance: domain contrast — bright interior, dark walls
  p.color.luma_shape_depth = 0.25f;          // STRONG: visible domain/wall contrast
  p.color.luma_shape_center = 2.1f;

  // ── Seed: domain boundary excitation ──
  p.seed.border_width = 0.080f;              // wide enough for tanh folds to sample
  p.seed.hue_speed = 0.04f;                  // slow: thermal equilibrium feel
  p.seed.alpha = 0.92f;

  p.speed = 0.42f;                           // slow: watch domains form and fluctuate
  return p;
}

} // namespace epi::presets
