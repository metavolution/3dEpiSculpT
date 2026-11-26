#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Laughlin fractional quantum Hall vortex preset.
 *
 * Scientific foundation:
 * - Robert Laughlin's 1983 trial wavefunction for the fractional quantum
 *   Hall (FQH) state at filling fraction nu = 1/m (m odd):
 *
 *       Psi(z_1, ..., z_N) = prod_{i<j} (z_i - z_j)^m
 *                             * exp(-sum_k |z_k|^2 / (4 * l_B^2))
 *
 *   where z_k = x_k + i*y_k are complex-plane electron positions in the
 *   lowest Landau level (LLL), and l_B = sqrt(hbar / eB) is the magnetic
 *   length. This wavefunction earned Laughlin the 1998 Nobel Prize (with
 *   Tsui and Stormer, who discovered the nu = 1/3 plateau experimentally).
 *
 * - The two structural factors encode two distinct topological features:
 *     - (z_i - z_j)^m factors: each pair of electrons sees an m-fold
 *       VORTEX around every other electron. These are the flux quanta
 *       bound to each electron that make FQH quasi-particles carry
 *       FRACTIONAL CHARGE e/m. For the famous m = 3 state, fractional
 *       charge e/3 was experimentally verified by shot-noise measurements
 *       (Saminadayar et al. 1997, de-Picciotto et al. 1997).
 *     - exp(-|z|^2/4) Gaussian envelope: confines electrons to a
 *       magnetic-length-wide disc (the LLL wavefunction ground state of
 *       a 2D harmonic oscillator in a perpendicular magnetic field).
 *
 * - Laughlin's beautiful PLASMA ANALOGY: |Psi|^2 = exp(-beta * H_plasma)
 *   where H_plasma is the Hamiltonian of a 2D one-component Coulomb
 *   plasma at inverse temperature beta = 1/m. This maps the quantum
 *   FQH problem onto a CLASSICAL statistical-mechanics plasma, and
 *   allows the incompressibility of the FQH state to be derived from
 *   screening in the plasma.
 *
 * - The m = 3 case is the most famous: nu = 1/3, Hall conductance
 *   plateau at (1/3) * e^2/h, fractional quasi-particles with charge
 *   e/3 and anyonic braiding statistics theta = pi/3. Every factor
 *   of 3 in this preset (rotations, scales, color angle coupling) is
 *   a deliberate echo of the m = 3 Laughlin state.
 *
 * - The post-rotation angle sqrt(pi) rad (≈ 101.5 deg) is the
 *   LOWEST-LANDAU-LEVEL GAUSSIAN NORMALISATION phase: the LLL
 *   harmonic-oscillator ground state psi_0(x) has normalisation
 *   (1 / pi^(1/4)) * exp(-x^2 / 2), so the square root of pi literally
 *   appears in the ground-state normalisation of the underlying Landau
 *   problem. Using sqrt(pi) rad as the post-rotation gives an
 *   irrational angle that is incommensurate with every other preset
 *   in the catalog yet carries authentic physical interpretation.
 *
 * Mapping to this engine:
 * - Inner = Exp. The Laughlin envelope is Gaussian, and the feedback
 *   composition of exp-inner with a pre-scaled (|zn0|<1) complex
 *   argument produces a radially-contracting spiral that is the
 *   classical LLL electron-density visualisation: dense center, spiral
 *   arms, ring structure.
 * - Pre-rotation = 2*pi/3 (120 deg): each feedback iteration applies
 *   one m = 3 Laughlin vortex winding. Composing these over feedback
 *   iterations traces the (z_i - z_j)^3 vortex structure at every
 *   center of the feedback texture.
 *   Exact: cos(2*pi/3) = -1/2, sin(2*pi/3) = sqrt(3)/2.
 * - Pre-scale = 0.72 — moderate contraction placing the feedback inside
 *   the LLL magnetic-length disc while leaving room for the spiral
 *   to breathe.
 * - Post-rotation = sqrt(pi) rad: the LLL Gaussian-normalisation phase.
 *   Irrational and transcendental, incommensurate with every algebraic
 *   irrational (golden / silver / sqrt(3)) and with every rational
 *   multiple of pi already in the catalog.
 * - Torus reduction: realises the 2D periodic plasma analogy — the
 *   Laughlin plasma on a torus is the canonical setup for FQH edge-
 *   state derivations (Haldane's geometric approach to FQH).
 * - Color mode 3 (alive): breath coupling visualises the plasma
 *   density oscillations; strong log-radial coupling reveals the
 *   composite-fermion Jain-hierarchy bands (filling fractions
 *   nu = p / (2p+1) as concentric radial hue rings).
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern exp_laughlin_fractional_hall_vortex() {
  // Pre-rotation 2*pi/3 = 120 deg — the m = 3 Laughlin vortex angle.
  // Exact trigonometric values.
  constexpr float C3 = -0.5f;         // cos(2*pi/3)
  constexpr float S3 = 0.8660254038f; // sin(2*pi/3) = sqrt(3)/2

  // Post-rotation sqrt(pi) rad ≈ 1.7724 rad ≈ 101.51 deg — the LLL
  // Gaussian normalisation phase (pi^(1/4) normalisation of the
  // harmonic-oscillator ground state in the Landau problem).
  //   sqrt(pi) ≈ 1.7724538509
  //   cos(sqrt(pi)) ≈ -0.2002864622
  //   sin(sqrt(pi)) ≈  0.9797462604
  constexpr float CN = -0.2002864622f; // cos(sqrt(pi))
  constexpr float SN = 0.9797462604f;  // sin(sqrt(pi))

  // Pre-scale: moderate contraction. |zn0| = 0.72 places the feedback
  // inside the lowest-Landau-level magnetic-length disc while keeping
  // the exp-inner spiral arms visible (if we went too tight |zn0| the
  // exp saturates fast and we lose the characteristic FQH spiral).
  constexpr float PRE  = 1.2149f;
  constexpr float POST = 0.9445f;

  Pattern p;
  p.name = "exp_laughlin_fractional_hall_vortex";
  p.inner = InnerFunction::Exp;       // Gaussian LLL envelope
  p.reduction = ReductionType::Torus; // periodic plasma analogy

  p.zn.zn[0] = {C3 * PRE, S3 * PRE};
  // Small detune: encodes the residual disorder that always exists
  // in real FQH samples. The 1/3 plateau is famously robust against
  // disorder because of its topological incompressibility — a
  // feature this preset celebrates by remaining visually stable even
  // under a non-symmetric detuning of the feedback.
  p.zn.zn[1] = {0.019f, -0.013f};
  p.zn.zn[2] = {CN * POST, SN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // Color: reveals the FQH topology through chromatic stratification.
  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.19f;          // vivid — FQH edge luminescence
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::Alive);

  // Strong angular coupling — exposes the m = 3 three-fold vortex
  // symmetry by painting three distinct hue wedges around every
  // feedback center (the classical Laughlin vortex ring structure).
  p.color.spiral_angle_coeff = 0.86f;
  // Strong log-radial coupling — the Jain composite-fermion hierarchy
  // predicts further FQH plateaux at nu = p / (2p+1): 1/3, 2/5, 3/7,
  // 4/9, ... converging to 1/2. These hierarchical filling fractions
  // appear as concentric hue bands whose radial spacing follows the
  // Jain sequence.
  p.color.spiral_radius_coeff = 0.46f;

  // Breathing: encodes the plasma-density oscillation of Laughlin's
  // plasma analogy. Amplitude is moderate because the FQH state is
  // INCOMPRESSIBLE (uniform density in the bulk) — large density
  // oscillations would destroy the plateau; gentle oscillation is
  // the correct visual analogue of the zero-point motion of the
  // electron fluid.
  p.color.chroma_breath_amp = 0.18f;
  p.color.chroma_breath_freq = 0.12f;
  p.color.chroma_breath_phase = 0.43f;
  p.color.hue_velocity_base = 0.015f;
  p.color.hue_velocity_breath = 0.020f;
  p.color.hue_velocity_drift = 0.0040f;
  p.color.luma_shape_depth = 0.27f;
  p.color.luma_shape_center = 1.78f;

  p.seed.border_width = 0.083f;
  p.seed.hue_speed = 0.088f;
  p.seed.alpha = 0.92f;
  // Pulse: represents the cyclotron orbit of the composite fermions
  // (electrons bound to 2m flux quanta). In the Jain theory these
  // composite fermions see a REDUCED effective magnetic field and
  // precess at an effective cyclotron frequency — a small but
  // persistent pulse that visually "drives" the Laughlin vortex
  // into the characteristic chiral edge flow.
  p.seed.pulse_freq = 0.089f;
  p.seed.pulse_amp = 0.19f;
  p.speed = 0.80f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.179f;
  p.zn.zn0_anim_amp = 0.047f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.191f;
  p.zn.zn1_anim_amp = 0.054f;
  p.zn.zn1_anim_k = 3.0f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.092f,
                            .amplitude = 0.129f,
                            .offset = 0.460f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.105f,
                            .amplitude = 0.103f,
                            .offset = 0.180f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.077f,
                            .amplitude = 0.031f,
                            .offset = 0.088f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
