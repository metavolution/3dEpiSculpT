#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Exponential catenary arch vault preset.
 *
 * MATHEMATICS: THE CATENARY — WHERE GRAVITY MEETS EXPONENTIALS
 *
 * The CATENARY y = a·cosh(x/a) is the curve formed by a uniform chain
 * hanging freely under gravity.  Galileo thought it was a parabola;
 * Leibniz, Huygens, and Johann Bernoulli independently proved in 1691
 * that it is a HYPERBOLIC COSINE — an exponential curve.
 *
 * The catenary's secret: cosh(x) = (e^x + e^{-x})/2.  The hanging
 * chain IS a sum of two exponentials — one growing, one decaying.
 * This makes Exp the perfect inner function: the feedback iteration
 * directly computes the exponential that defines the catenary.
 *
 * The catenary is also the IDEAL ARCH.  An inverted catenary is in
 * PURE COMPRESSION everywhere — no bending moments, no tension.
 * Antoni Gaudí discovered this by hanging weighted chains upside down
 * to design the Sagrada Família.  The St. Louis Gateway Arch is a
 * weighted catenary: y = -A·cosh(Bx) + C.
 *
 * ENGINE MAPPING TO CLASSICAL ARCHITECTURE:
 *
 *   THE EXPONENTIAL ARCH = Exp inner function
 *     e^z maps the complex plane into radial amplitude (e^x) and angular
 *     phase (y).  This IS the catenary decomposed: the real exponential
 *     part creates the arch's shape (amplitude), and the angular part
 *     creates the arch's span (phase position).  Under iteration, Exp
 *     produces VAULTED ARCHES — the architectural realization of cosh.
 *
 *   THE VAULT RIBS = Grid reduction
 *     Grid tiles create STRUCTURAL CELLS — each one a vaulted bay.
 *     The tile boundaries are the RIBS of the vault, where the catenary
 *     arch meets its neighbor.  This is ribbed vault architecture:
 *     Gothic cathedrals, Roman baths, the Hagia Sophia.  The Grid
 *     boundaries make the structural skeleton visible.
 *
 *   THE CATENARY ANGLE = arccosh(2) = ln(2+√3) ≈ 75.5° pre-rotation
 *     arccosh(2) is the parameter at which the catenary's height equals
 *     TWICE its base unit — the "doubling height" of the arch.
 *
 *     ln(2+√3) is deeply connected to classical mathematics:
 *       - 2+√3 = tan(75°) = tan(5π/12) — a trigonometric identity
 *       - (2+√3)(2-√3) = 1 — conjugate units in the ring ℤ[√3]
 *       - This connects to the PELL EQUATION x² - 3y² = 1, whose
 *         fundamental solution is (2,1), generating all solutions via
 *         (2+√3)^n
 *       - arccosh(2) = arcsinh(√3) — hyperbolic functions unify here
 *
 *     As a rotation: 360°/75.5° ≈ 4.77 → quasi-5-fold symmetry.
 *     Not exactly pentagonal (72°), but close enough for recognizable
 *     5-fold structure with irrational drift.
 *
 *   THE DOUBLING CONSTANT = -ln(2) ≈ -39.7° post-rotation
 *     ln(2) ≈ 0.6931 is the FUNDAMENTAL CONSTANT OF EXPONENTIAL GROWTH:
 *       - The time for any exponential to double: t₂ = ln(2)/rate
 *       - 1 bit of information = log₂(2) = ln(2)/ln(2)
 *       - The alternating harmonic series: ln(2) = 1 - 1/2 + 1/3 - ...
 *       - The area under 1/x from 1 to 2
 *
 *     The pairing: arccosh(2) (the doubling HEIGHT) × ln(2) (the doubling
 *     TIME) — both are about the number 2, but one is geometric (spatial
 *     doubling) and the other is temporal (exponential doubling).
 *
 *   CLASSIC MODE = pure architectural geometry
 *     Mode 0 presents the vault without temporal modulation — no breathing,
 *     no pulsing, no spatial waves.  The beauty is in the STRUCTURE ITSELF:
 *     the eternal geometry of the catenary arch, rendered as pure fractal
 *     architecture.  Hue varies with angle (showing the quasi-5-fold
 *     symmetry) and radius (showing depth layers), drifting imperceptibly.
 *
 * WHAT TO WATCH:
 *   1. ARCHES: Exp creates catenary-profile vaults inside each grid bay
 *   2. RIBS: Grid boundaries = structural skeleton of the vault
 *   3. QUASI-5-FOLD: ~4.77 arches per revolution, slowly drifting
 *   4. DEPTH: fade=0.989 → ~91 frame persistence → deep perspective
 *   5. PURE GEOMETRY: no temporal effects — eternal architecture
 *   6. DOUBLING: ln(2) depth twist = exponential halving per layer
 *
 * Pipeline: transform -> reduce -> sample -> color(mode0) -> seed -> blend
 */
inline Pattern exp_catenary_arch_vault() {
  // arccosh(2) = ln(2+√3) ≈ 1.3170 rad (75.5°): the catenary doubling height
  // 2+√3 = tan(75°); (2+√3)(2−√3) = 1 → Pell equation units in ℤ[√3]
  constexpr float CC   =  0.2513f;           // cos(ln(2+√3))
  constexpr float SC   =  0.9680f;           // sin(ln(2+√3))
  // -ln(2) ≈ -0.6931 rad (-39.7°): the fundamental doubling/halving constant
  // ln(2) = 1 - 1/2 + 1/3 - 1/4 + ... (alternating harmonic series)
  constexpr float CL2  =  0.7693f;           // cos(ln(2))
  constexpr float SL2N = -0.6389f;           // sin(-ln(2))

  constexpr float PRE  = 0.78f;  // moderate: clean arch structure
  constexpr float POST = 0.88f;  // moderate contraction: product 0.686

  Pattern p;
  p.name = "exp_catenary_arch_vault";
  p.inner = InnerFunction::Exp;              // e^z = the exponential that IS cosh
  p.reduction = ReductionType::Grid;         // vault ribs = structural skeleton

  p.zn.zn[0] = {CC * PRE, SC * PRE};
  p.zn.zn[1] = {0.005f, -0.003f};           // perturbation: break exact symmetry
  p.zn.zn[2] = {CL2 * POST, SL2N * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 0 (Classic) — pure architectural geometry ──
  p.color.hue_rotation = 0.006f;             // very slow: timeless architecture
  p.color.sat_scale = 1.08f;                 // subtle: stone-like restraint
  p.color.fade = 0.989f;                     // deep: ~91 frame persistence
  p.color.color_mode = as_int(ColorMode::Classic);                    // Classic: pure geometry, no modulation
  p.color.spiral_angle_coeff = 0.75f;        // moderate: shows quasi-5-fold symmetry
  p.color.spiral_radius_coeff = 0.50f;       // moderate: visible depth rings
  // Mode 0 does not use breathing or hue velocity params
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: strong architectural contrast — bright arches, dark voids
  p.color.luma_shape_depth = 0.30f;          // STRONG: crisp arch definition
  p.color.luma_shape_center = 2.2f;

  // ── Seed: subtle — the architecture IS the visual ──
  p.seed.border_width = 0.010f;              // subtle: arches dominate
  p.seed.hue_speed = 0.04f;                  // slow: contemplative
  p.seed.alpha = 0.85f;

  p.speed = 0.40f;                           // slow: architectural contemplation
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.119f;
  p.zn.zn0_anim_amp = 0.046f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.173f;
  p.zn.zn1_anim_amp = 0.048f;
  p.zn.zn1_anim_k = 5.0f;
  p.seed.pulse_freq = 0.088f;
  p.seed.pulse_amp = 0.15f;

  return p;
}

} // namespace epi::presets
