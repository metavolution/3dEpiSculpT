#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Hofstadter butterfly / Harper magnetic-Bloch spectrum preset.
 *
 * Scientific foundation:
 * - Douglas Hofstadter's 1976 paper "Energy levels and wave functions of
 *   Bloch electrons in rational and irrational magnetic fields" derived the
 *   single most famous fractal of solid-state physics: the energy spectrum
 *   of a tight-binding electron on a 2D square lattice in a perpendicular
 *   magnetic field, plotted against the dimensionless flux ratio
 *       alpha = Phi / Phi_0
 *   (magnetic flux per plaquette divided by the flux quantum h/e).
 *
 * - The underlying equation is Harper's equation (Harper 1955):
 *       psi(n+1) + psi(n-1) + 2*cos(2*pi*alpha*n - k) * psi(n) = E * psi(n)
 *   The `cos(2*pi*alpha*n - k)` term is the Peierls-substitution phase the
 *   magnetic flux injects into the tight-binding hopping — IT IS LITERALLY
 *   a cosine inner function applied to an affine-stepped phase, which is
 *   EXACTLY the structure this engine's transform stage produces.
 *
 * - When alpha is a rational p/q, the spectrum has exactly q bands.
 *   When alpha is irrational, the spectrum is a Cantor-like fractal. The
 *   plot (alpha vs E) reveals nested miniature butterflies at every scale
 *   — the canonical demonstration of self-similarity in quantum mechanics.
 *
 * - We pick alpha = sqrt(2) - 1 = 0.41421356..., the "silver-mean
 *   conjugate": the second-most-irrational number after the golden
 *   conjugate (by the theory of Markov numbers / continued fractions).
 *   Silver-mean flux reveals the butterfly's distinctive four-fold
 *   nesting structure — complementary to the golden-mean structure
 *   already explored by `chirikov_kam_island_chain`. Using silver here
 *   keeps this preset visually and mathematically distinct.
 *
 * Mapping to this engine:
 * - Inner = Cos (Harper's equation hopping term is literally cos).
 * - Pre-affine: magnitude ~ alpha_silver with rotation 2*pi*alpha_silver
 *   encodes the Peierls-phase winding per lattice step.
 * - Post-affine: rotation at 2*pi/3 is the Dirac K-point of a honeycomb
 *   Brillouin zone — a second fractal "flavor" coupling that breaks the
 *   square-lattice 4-fold symmetry into the 6-fold motif characteristic
 *   of magnetic Bloch bands near topological band crossings.
 * - Grid reduction realises the T^2 Brillouin zone (periodic k-space).
 * - Color mode 3 (alive) + moderate spatial phase reveals the nested
 *   self-similar band hierarchy — different "butterfly levels" pick up
 *   different hues via the log-radius + angle coupling.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern hofstadter_butterfly_spectrum() {
  // Silver-mean flux per plaquette: alpha = sqrt(2) - 1 = 0.41421356...
  // The second-most-irrational real (after golden conjugate). In
  // Hofstadter's (alpha vs E) plot, silver-mean alpha produces a
  // distinct four-fold-nested butterfly pattern.
  // constexpr float ALPHA_SILVER = 0.41421356237f;

  // Pre-rotation: 2*pi*alpha_silver = 2.60185073... rad (≈ 149.06°).
  // This is the Peierls phase a tight-binding electron accumulates
  // hopping one lattice step under the silver-mean magnetic flux.
  constexpr float CP = -0.85749292581f; // cos(2*pi*(sqrt(2)-1))
  constexpr float SP = 0.51449575542f;  // sin(2*pi*(sqrt(2)-1))

  // Post-rotation: 2*pi/3 = 120° — the honeycomb-lattice Dirac K-point.
  // Adding this chirality-breaking rotation on top of square-lattice
  // magnetic hopping probes the transition between square and
  // triangular band topologies, where the butterfly gains its richest
  // fine structure.
  constexpr float CD = -0.5f;           // cos(2*pi/3)
  constexpr float SD = 0.86602540378f;  // sin(2*pi/3)

  // Pre-scale: moderate contraction. Harper's raw 2*cos amplitude would
  // diverge the feedback loop (|zn0| must be < 1), so we renormalize to
  // a stable fraction while preserving the characteristic spectral
  // width ratio.
  constexpr float PRE  = 0.68f;
  constexpr float POST = 0.88f;

  Pattern p;
  p.name = "hofstadter_butterfly_spectrum";
  p.inner = InnerFunction::Cos;         // Harper's cosine hopping term
  p.reduction = ReductionType::Grid;    // T^2 Brillouin zone (periodic k)

  p.zn.zn[0] = {CP * PRE, SP * PRE};
  // Small imaginary offset is the Bloch wavevector k: it nudges the
  // spectrum through the fractal slice at this flux. Non-zero k breaks
  // E-reflection symmetry and asymmetrises the butterfly's wings.
  p.zn.zn[1] = {0.021f, -0.014f};
  p.zn.zn[2] = {CD * POST, SD * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // Color: highlight the self-similar band hierarchy.
  p.color.hue_rotation = 0.013f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.983f;
  p.color.color_mode = as_int(ColorMode::Alive);               // Alive: oscillator triple-velocity
  // Strong angular coupling separates adjacent Hofstadter bands by hue,
  // so that bands at different rational approximants of alpha (the p/q
  // staircase converging to the silver mean) are visually distinct.
  p.color.spiral_angle_coeff = 0.78f;
  // Moderate log-radial coupling turns the nested-miniature-butterfly
  // self-similarity into concentric hue rings — each ring corresponds
  // to one recursion level of the Cantor-like spectrum.
  p.color.spiral_radius_coeff = 0.44f;
  // Breathing: simulates the spectral-gap opening/closing as alpha
  // drifts along the silver-mean's continued-fraction convergents.
  p.color.chroma_breath_amp = 0.18f;
  p.color.chroma_breath_freq = 0.11f;
  p.color.chroma_breath_phase = 0.47f;
  p.color.hue_velocity_base = 0.014f;
  p.color.hue_velocity_breath = 0.019f; // breath-dominant → gap breathing
  p.color.hue_velocity_drift = 0.0035f;
  p.color.luma_shape_depth = 0.24f;
  p.color.luma_shape_center = 1.7f;

  p.seed.border_width = 0.019f;
  p.seed.hue_speed = 0.074f;
  p.seed.alpha = 0.90f;
  // Gentle pulse: echoes the topological "cycling" of Chern numbers as
  // the system sweeps through flux rationals.
  p.seed.pulse_freq = 0.085f;
  p.seed.pulse_amp = 0.18f;
  p.speed = 0.78f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.135f;
  p.zn.zn0_anim_amp = 0.041f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.171f;
  p.zn.zn1_anim_amp = 0.046f;
  p.zn.zn1_anim_k = 7.0f;

  return p;
}

} // namespace epi::presets
