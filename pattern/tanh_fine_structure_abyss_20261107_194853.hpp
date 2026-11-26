#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh fine structure abyss preset.
 *
 * ELECTRODYNAMICS: THE FINE STRUCTURE CONSTANT α ≈ 1/137
 *
 * The FINE STRUCTURE CONSTANT is the most mysterious number in physics:
 *
 *   α = e² / (4πε₀ℏc) ≈ 1/137.036
 *
 * It is DIMENSIONLESS — a pure number that doesn't depend on any choice of
 * units.  It measures the STRENGTH of the electromagnetic interaction:
 * how strongly light couples to matter, how tightly electrons are bound
 * in atoms, how quickly charged particles radiate.
 *
 *   - If α were ~4% larger: carbon would be unstable, life impossible
 *   - If α were ~4% smaller: stellar fusion would fail, no heavy elements
 *   - α determines atomic fine structure: energy level splitting = O(α²)
 *   - α determines the anomalous magnetic moment: g-2 = α/π + O(α²)
 *   - α runs with energy: α(0) ≈ 1/137, α(m_Z) ≈ 1/128 (electroweak)
 *
 * Richard Feynman: "It has been a mystery ever since it was discovered
 * more than fifty years ago, and all good theoretical physicists put this
 * number up on their wall and worry about it."
 *
 * Wolfgang Pauli (allegedly his last words): "I cannot understand God's
 * purpose in choosing the number 137."
 *
 * The number 137 itself:
 *   - The 33rd prime number
 *   - A Pythagorean prime: 137 = 4² + 11² (sum of two squares)
 *   - 137 in binary: 10001001 — sparse, elegant
 *   - In Jewish gematria: "Kabbalah" (קבלה) = 137
 *
 * ENGINE MAPPING TO QUANTUM ELECTRODYNAMICS:
 *
 *   THE COUPLING WEB = Tanh inner function
 *     In QED, the photon propagator contains the vacuum polarization
 *     correction: Π(q²) involves tanh-like functions of the coupling.
 *     More deeply: tanh IS the Fermi-Dirac distribution function at
 *     half-filling, describing the electron occupation of states in
 *     a metal.  The ±1 saturation walls of tanh are the FERMI SURFACE:
 *     below = occupied states, above = empty states.  The fractal web
 *     created by tanh iteration IS the structure of quantum states in
 *     the interacting electron gas.
 *
 *   THE FEYNMAN DIAGRAM = Grid reduction
 *     Grid tiles are the SPACETIME CELLS of the path integral.  Each
 *     tile boundary is where a virtual photon is exchanged.  The web
 *     of filaments within each tile IS the sum over Feynman diagrams
 *     at each order of perturbation theory: tree level, one-loop,
 *     two-loop, etc.  The fractal branching density IS the factorial
 *     growth of diagram count: n-loop diagrams grow as (2n)!/n!.
 *
 *   THE CUBE ROOT OF COUPLING = α^(1/3) ≈ 11.11° pre-rotation
 *     α^(1/3) = (1/137.036)^(1/3) ≈ 0.1940 radians ≈ 11.11°.
 *
 *     Why the CUBE ROOT?  In QED perturbation theory:
 *       - Order α¹: tree-level (single photon exchange) — 1 diagram
 *       - Order α²: one-loop (vacuum polarization, self-energy) — 7 diagrams
 *       - Order α³: two-loop — 72 diagrams
 *     The cube root α^(1/3) decomposes each quantum of coupling into THREE
 *     equal rotation steps.  After 3 iterations: total rotation = α.
 *     After 3×137 = 411 iterations: one full revolution of coupling.
 *
 *     cos(α^(1/3)) ≈ 0.9812, sin(α^(1/3)) ≈ 0.1928
 *     360°/11.11° ≈ 32.4 → quasi-32-fold symmetry.
 *     Each of the ~32 sectors represents a FUNDAMENTAL INTERACTION
 *     VERTEX — the point where a photon couples to an electron.
 *     32 ≈ 2⁵: the number of components of a Dirac bispinor in 10D
 *     spacetime (string theory).
 *
 *   THE COUPLING SCALE = ln(137) ≈ 4.920 rad post-rotation
 *     In the RENORMALIZATION GROUP, the running of α with energy scale μ
 *     is governed by the β-function: β(α) = dα/d(ln μ).  The quantity
 *     ln(1/α) = ln(137) IS the logarithmic energy scale at which the
 *     coupling is measured.  From the GUT scale (where all forces unify)
 *     down to low energy, the EM coupling traverses ln(137) ≈ 4.920
 *     units of logarithmic energy.
 *
 *     Effective post-rotation: ~-78.1° (clockwise depth spiral)
 *     cos(ln(137)) ≈ 0.2060, sin(ln(137)) ≈ -0.9786
 *     360°/78.1° ≈ 4.61 → quasi-5-fold depth symmetry.
 *     Combined with 32-fold azimuthal: 32 × 4.6 ≈ 147 interference cells.
 *
 *   THREE QED TIMESCALES = Mode 3 (Alive) hue velocity
 *     base  = 0.028 → virtual photon exchange (fast: each vertex = one α)
 *     breath = 0.015 → vacuum polarization (medium: virtual pair bubbles)
 *     drift  = 0.005 → renormalization group running (slow: α evolves)
 *
 * WHAT TO WATCH:
 *   1. 32-FOLD WEB: ~32 coupling vertices per revolution → ultra-dense
 *   2. FILAMENT DENSITY: tanh ±1 folding × 32 sectors = finest structure
 *   3. DEPTH: fade=0.992 → ~125 frame persistence (near 137!)
 *   4. TRIPLE SHIMMER: three QED timescales visible as nested flickering
 *   5. MOIRÉ: 32-fold × Grid tiling = rich interference bands
 *   6. NEAR-CRITICAL: product 0.946 → ~95% energy retention per iteration
 *   7. 137: the most important number in physics, encoded in the geometry
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern tanh_fine_structure_abyss() {
  // α^(1/3) = (1/137.036)^(1/3) ≈ 0.1940 rad (11.11°): cube root of EM coupling
  // Three iterations = one quantum of coupling α
  // 360°/11.11° ≈ 32.4 → quasi-32-fold: 32 QED interaction vertices
  constexpr float CA   =  0.9812f;           // cos(α^(1/3))
  constexpr float SA   =  0.1928f;           // sin(α^(1/3))
  // ln(137) ≈ 4.920 rad: the renormalization group scale ln(1/α)
  // Effective twist: ~-78.1° per depth layer (clockwise)
  // The logarithmic energy range from GUT scale to low-energy QED
  constexpr float CL   =  0.2060f;           // cos(ln(137))
  constexpr float SLN  = -0.9786f;           // sin(ln(137)) [naturally clockwise]

  constexpr float PRE  = 1.52f;  // Reach tanh imaginary poles (+/-pi/2) across Grid cells
  constexpr float POST = 1.05f;  // Couple 32-fold Fermi-surface sheets across cell walls

  Pattern p;
  p.name = "tanh_fine_structure_abyss";
  p.inner = InnerFunction::Tanh;             // Fermi-Dirac ±1 walls → coupling web
  p.reduction = ReductionType::Grid;         // Feynman diagram spacetime cells

  p.zn.zn[0] = {CA * PRE, SA * PRE};
  p.zn.zn[1] = {0.014f, -0.019f};            // perturbation: break exact symmetry
  p.zn.zn[2] = {CL * POST, SLN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};
  // Renormalization-group running (0.137 Hz) + 3-loop trefoil vertex drift
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.137f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.194f;
  p.zn.zn1_anim_amp = 0.032f;
  p.zn.zn1_anim_k = 3.0f;

  // ── Color: MODE 3 (Alive) — three QED timescales ──
  p.color.hue_rotation = 0.014f;             // moderate: coupling spectrum evolution
  p.color.sat_scale = 1.10f;                 // vivid: the brilliance of α
  p.color.fade = 0.988f;                     // deep multi-loop persistence without clipping
  p.color.color_mode = as_int(ColorMode::Alive);                    // Alive: photon exchange + vacuum pol + RG
  p.color.spiral_angle_coeff = 0.90f;        // STRONG: shows 32-fold coupling structure
  p.color.spiral_radius_coeff = 0.38f;       // depth rings scaled to log_r
  // Vacuum polarization (angular brightness modulation)
  p.color.chroma_breath_amp = 0.11f;         // balanced virtual pair fluctuations
  p.color.chroma_breath_freq = 0.12f;        // polarization oscillation rate
  p.color.chroma_breath_phase = 0.25f;       // vacuum phase
  // THREE QED TIMESCALES
  p.color.hue_velocity_base = 0.028f;        // FAST: virtual photon exchange rate
  p.color.hue_velocity_breath = 0.015f;      // MEDIUM: vacuum polarization bubbles
  p.color.hue_velocity_drift = 0.005f;       // SLOW: renormalization group running
  // Luminance calibrated to canonical log_r ~ -4.35
  p.color.luma_shape_depth = 0.048f;
  p.color.luma_shape_center = -4.35f;

  // ── Seed: crisp spacetime cell boundaries that ignite all 32 QED vertices ──
  p.seed.border_width = 0.072f;
  p.seed.hue_speed = 0.045f;                 // quantum vacuum phase drift
  p.seed.alpha = 0.88f;

  p.speed = 0.56f;                           // poised QED coupling evolution
  return p;
}

} // namespace epi::presets
