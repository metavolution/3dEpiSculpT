#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Exponential Larmor radiation tunnel preset.
 *
 * ELECTRODYNAMICS: LARMOR RADIATION & CYCLOTRON MOTION
 *
 * When a charged particle accelerates, it radiates electromagnetic energy.
 * The LARMOR FORMULA gives the total radiated power:
 *
 *   P = q²a² / (6πε₀c³)
 *
 * This is the MOST FUNDAMENTAL radiation formula in electrodynamics — every
 * antenna, every synchrotron, every thermal emitter obeys it.  Accelerating
 * charge → radiation.  Period.
 *
 * In a uniform magnetic field B, a charged particle undergoes CYCLOTRON
 * MOTION — circular orbits in the plane perpendicular to B:
 *
 *   ω_c = |q|B/m  (cyclotron frequency)
 *   r_L = mv_⊥/(|q|B)  (Larmor radius)
 *
 * The particle traces a HELIX: circular motion ⊥ B, free streaming ∥ B.
 * This helix IS the tunnel — the Exp inner function generates the spiral
 * wavefronts of the cyclotron orbit, and the Torus wraps them into the
 * infinite helical trajectory.
 *
 * For RELATIVISTIC particles (γ >> 1):
 *   - Cyclotron frequency drops: ω_c → ω_c/γ
 *   - Radiation is BEAMED forward into a cone of half-angle 1/γ
 *   - Power scales as γ⁴: P ∝ γ⁴ω_c² r_L²
 *   - The spectrum shifts from monochromatic (cyclotron) to broadband
 *     (SYNCHROTRON radiation) — the basis of particle accelerator light
 *     sources used in crystallography, materials science, and medicine
 *
 * ENGINE MAPPING TO CYCLOTRON PHYSICS:
 *
 *   THE SPIRAL ORBIT = Exp inner function
 *     e^z = e^x(cos y + i sin y) naturally generates SPIRAL WAVEFRONTS.
 *     The radial part (e^x) maps the energy of the orbit: high energy
 *     particles spiral outward (wider Larmor radius), low energy spiral
 *     inward.  The angular part IS the cyclotron phase ω_c·t.
 *
 *   THE MAGNETIC BOTTLE = Torus reduction
 *     Torus wraps the plane into seamless mirror boundaries — the magnetic
 *     mirror points of a MAGNETIC BOTTLE.  Charged particles bounce between
 *     mirror points, creating an INFINITE HELIX: the tunnel IS the
 *     particle's trajectory viewed head-on.
 *
 *   QUASI-23-FOLD SYMMETRY = e/π² ≈ 15.78° pre-rotation
 *     The ratio e/π² ≈ 0.2754 combines the two fundamental transcendentals
 *     in a way that connects exponential growth to circular geometry — the
 *     exact duality of the cyclotron orbit (exponential energy × circular
 *     phase).  Whether e/π² is irrational is an OPEN PROBLEM, closely
 *     related to the unsolved question of whether e and π are algebraically
 *     independent.  If e/π² were rational, it would imply e = rπ² for some
 *     rational r — connecting exponential and circular constants in a way
 *     that would revolutionize number theory.
 *
 *     cos(e/π²) ≈ 0.9624, sin(e/π²) ≈ 0.2715
 *     360°/15.78° ≈ 22.8 → quasi-23-fold symmetry.  23 orbit sectors
 *     spiral into the tunnel, each slightly offset from the others by the
 *     irrational(?) drift.  23 is a Sophie Germain prime (2×23+1 = 47 is
 *     also prime) — one of the building blocks of cryptographic security.
 *
 *   THE SKIN DEPTH ANGLE = -arctan(2√2) ≈ -70.5° post-rotation
 *     cos = 1/3 (EXACT algebraic value!)
 *     sin = -2√2/3 (EXACT algebraic value!)
 *
 *     2√2 = √8 appears in the SKIN DEPTH formula: δ = √(2/(ωμσ)).
 *     When an EM wave penetrates a conductor, the ratio of displacement
 *     current to conduction current at one skin depth IS arctan(2√2).
 *     1/3 (the cosine) is the fraction of total EM energy in each
 *     Cartesian component for an isotropic radiation field.
 *
 *   THREE RADIATION TIMESCALES = Mode 3 (Alive) hue velocity
 *     base  = 0.022 → cyclotron frequency (fast orbit cycling)
 *     breath = 0.013 → radiation damping (particle loses energy → spirals in)
 *     drift  = 0.004 → synchrotron frequency shift (ω_c/γ decreasing)
 *
 * WHAT TO WATCH:
 *   1. SPIRAL WAVEFRONTS: Exp arches = cyclotron orbit wavefronts
 *   2. 23-FOLD: quasi-23 orbit sectors spiraling into the magnetic bottle
 *   3. TUNNEL DEPTH: looking down the helical trajectory of the particle
 *   4. CARRIER: fast hue cycling = cyclotron orbit ω_c
 *   5. DAMPING: medium pulse = radiation reaction slowing the particle
 *   6. FREQUENCY SHIFT: slow drift = relativistic mass increase → ω_c/γ
 *   7. EXACT 1/3: depth contraction = isotropic energy equipartition
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern exp_larmor_radiation_tunnel() {
  // e/π² ≈ 0.2754 rad (15.78°): ratio of exponential to circular constants
  // Whether e/π² is irrational is an OPEN PROBLEM in number theory
  // 360°/15.78° ≈ 22.8 → quasi-23-fold cyclotron orbit structure
  constexpr float CE   =  0.9624f;           // cos(e/π²)
  constexpr float SE   =  0.2715f;           // sin(e/π²)
  // -arctan(2√2) ≈ -1.2310 rad (-70.5°): the skin depth angle
  // cos = 1/3 EXACTLY (algebraic!), sin = -2√2/3 EXACTLY
  // Connects to: EM energy equipartition, skin depth ratio δ = √(2/(ωμσ))
  constexpr float CSK  =  0.3333f;           // cos(arctan(2√2)) = 1/3 EXACT
  constexpr float SSKN = -0.9428f;           // sin(-arctan(2√2)) = -2√2/3 EXACT

  constexpr float PRE  = 0.84f;  // moderate: clear 23-fold wavefront architecture
  constexpr float POST = 0.92f;  // gentle contraction: product 0.773

  Pattern p;
  p.name = "exp_larmor_radiation_tunnel";
  p.inner = InnerFunction::Exp;              // e^z = cyclotron spiral orbit
  p.reduction = ReductionType::Torus;        // magnetic bottle: seamless mirror walls

  p.zn.zn[0] = {CE * PRE, SE * PRE};
  p.zn.zn[1] = {0.004f, -0.005f};           // perturbation: excites cyclotron harmonics
  p.zn.zn[2] = {CSK * POST, SSKN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 3 (Alive) — three radiation timescales ──
  p.color.hue_rotation = 0.012f;             // moderate: radiation spectrum evolution
  p.color.sat_scale = 1.14f;                 // vivid: synchrotron blue-white brilliance
  p.color.fade = 0.989f;                     // deep: Q ≈ 262, long-lived orbits
  p.color.color_mode = as_int(ColorMode::Alive);                    // Alive: cyclotron + damping + shift
  p.color.spiral_angle_coeff = 0.95f;        // STRONG: shows 23 orbit sectors
  p.color.spiral_radius_coeff = 0.50f;       // moderate: radial energy bands
  // Radiation reaction (angular brightness modulation)
  p.color.chroma_breath_amp = 0.24f;         // moderate: visible energy loss
  p.color.chroma_breath_freq = 0.11f;        // radiation damping rate
  p.color.chroma_breath_phase = 0.35f;       // phase of radiation reaction
  // THREE RADIATION TIMESCALES
  p.color.hue_velocity_base = 0.022f;        // FAST: cyclotron frequency ω_c
  p.color.hue_velocity_breath = 0.013f;      // MEDIUM: radiation damping (energy loss)
  p.color.hue_velocity_drift = 0.004f;       // SLOW: synchrotron freq shift (ω_c/γ)
  // Luminance: strong wavefront contrast
  p.color.luma_shape_depth = 0.26f;          // strong: crisp orbit wavefronts
  p.color.luma_shape_center = 1.9f;

  // ── Seed: moderate — cyclotron injection ──
  p.seed.border_width = 0.010f;              // moderate: particle injection at boundary
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 0.87f;

  p.speed = 0.48f;                           // moderate: watch orbit dynamics
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.143f;
  p.zn.zn0_anim_amp = 0.044f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.132f;
  p.zn.zn1_anim_amp = 0.042f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.107f;
  p.seed.pulse_amp = 0.17f;

  return p;
}

} // namespace epi::presets
