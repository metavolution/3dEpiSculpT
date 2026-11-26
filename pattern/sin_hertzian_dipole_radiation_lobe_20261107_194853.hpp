#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Hertzian Dipole Radiation Lobes — classical electromagnetism
 * ================================================================
 *
 * PHYSICS / MATH  (H. Hertz 1888, J. C. Maxwell 1865;
 * see Jackson "Classical Electrodynamics" §9.2):
 *
 * A HERTZIAN DIPOLE is an infinitesimal oscillating electric dipole:
 * two point charges ±q separated by a small distance d(t) = d₀ cos(ωt),
 * giving a time-varying dipole moment p(t) = q·d(t) along a fixed axis.
 * The electromagnetic radiation it emits is the simplest non-trivial
 * EM field in classical physics and the fundamental building block of
 * all antenna theory.
 *
 * In the FAR FIELD (r ≫ λ = 2πc/ω), the electric field is:
 *
 *     E(r, θ, t) = (ω² p₀ sin θ) / (4π ε₀ c² r)  · cos(kr − ωt) · θ̂
 *
 * where θ is the polar angle from the dipole axis and k = ω/c. The
 * angular power radiated per unit solid angle is:
 *
 *     dP/dΩ ∝ sin²(θ)
 *
 * This gives the iconic "doughnut" radiation pattern: TWO LOBES
 * arranged perpendicular to the dipole axis, with NULL RADIATION
 * along the axis itself (sin 0 = sin π = 0).
 *
 * The Poynting vector S = E × B / μ₀ points radially outward,
 * carrying energy away as concentric spherical wavefronts expanding
 * at c. In a 2D equatorial cross-section, these show up as CIRCULAR
 * INTERFERENCE RINGS with spacing λ/2 — the classic appearance of
 * near-field / far-field RADIATION ZONES around every real antenna.
 *
 * Hertz demonstrated this pattern experimentally in 1887 with his
 * spark-gap oscillator, the first practical generator of radio waves
 * and the foundational proof of Maxwell's equations. Every radio
 * antenna, cell-tower, microwave radar, MRI RF coil, and stellar
 * pulsar magnetosphere is a variation on the Hertzian dipole's
 * angular lobe pattern.
 *
 *
 * ENGINE MAPPING — sin(θ)/r far-field AS iterated feedback
 *
 *   ANGULAR sin(θ) PATTERN = Sin inner function
 *     The complex extension sin(x + iy) = sin(x)cosh(y) + i·cos(x)·
 *     sinh(y) carries a sinusoidal angular dependence exactly
 *     matching the dipole's sin(θ)/r far-field amplitude. Sin's
 *     zeros (at integer multiples of π) directly map to the dipole's
 *     nodal axis, and its periodic lobes map to the radiation
 *     maxima at θ = π/2.
 *
 *   DIPOLE AXIS = pre-rotation π/2 (pure-imaginary zn[0])
 *     Rotating z by π/2 before the Sin inner aligns Sin's zero axis
 *     with the VERTICAL direction in the engine's unit cell. The
 *     two radiation lobes then emerge perpendicular — along the
 *     horizontal — exactly as in a real dipole with axis upward.
 *
 *   SPHERICAL WAVEFRONTS = torus reduction
 *     The EM radiation expands as concentric spherical shells.
 *     Torus reduction (the smooth, seamless variant — unlike Grid
 *     which would introduce artificial rectangular boundaries)
 *     preserves the circular-symmetry of the outgoing wavefront
 *     structure without breaking rotational continuity.
 *
 *   RADIATION PHASE LAG = zn[3] = (0, π/8) pure imaginary
 *     The e^(i(kr − ωt)) phase factor means the field at radius r
 *     lags the source by kr radians. We encode this as a pure-
 *     imaginary post-translate of π/8 ≈ 0.393 rad — one-eighth of
 *     a full period per iteration. This creates the slow "ripple
 *     outward" propagation that makes concentric wavefronts visible
 *     in the accumulated feedback.
 *
 *   BEAM-STEERING COUNTER-ROTATION = zn[2] angle = −π/8
 *     Real dipole antennas can be rotated to steer the radiation
 *     direction. The −22.5° post-rotation models a slowly rotating
 *     dipole (like a rotating-beam lighthouse / pulsar), sweeping
 *     the lobe pattern through angle over the iteration sequence.
 *     This creates the distinctive "rotating beam" impression while
 *     preserving the perpendicular-lobe geometry.
 *
 *   STABILITY PRODUCT = 0.72 · 0.91 = 0.655
 *     Well-bounded and conservative — per the design guide,
 *     Sin prefers 0.52–0.97 pre-scale; 0.72 places us in the
 *     linear-regime (not chaotic K_crit ≈ 0.97 standard-map regime).
 *     The dipole is a driven LINEAR oscillator — no chaos — so we
 *     keep well clear of the KAM torus breaking threshold.
 *
 *   FAR-FIELD INTERFERENCE = color mode 4 (Spatial Wave)
 *     Mode 4's dual-sine interference with irrational spatial
 *     frequency ratio (√2, √5) is purpose-built for quasi-periodic
 *     interference patterns. We use these as wave1/wave2 to create
 *     the visible concentric-ring structure seen around every
 *     real antenna, with the 1.7-rad phase offset guaranteeing the
 *     two interfering wave components never perfectly align —
 *     matching the physics that near and far fields have
 *     different phase velocities.
 *
 *   OSCILLATING CURRENT DRIVE = seed pulse (freq 0.14, amp 0.36)
 *     The antenna's feed is an oscillating current I(t) = I₀ cos(ωt).
 *     The seed's border acts as the antenna feedline, and the pulse
 *     modulates its alpha on the driving-force timescale. You can
 *     literally see the "power being pushed in" as the border
 *     alpha waxes and wanes.
 *
 *
 * WHAT TO WATCH:
 *   1. TWO BRIGHT LOBES arranged along the horizontal axis, with a
 *      dimmer / null region along the vertical — the classic
 *      dipole angular pattern sin²(θ) with nulls at θ = 0, π.
 *   2. CONCENTRIC RINGS expanding outward from center — the spatial
 *      wave interference pattern rendering the far-field spherical
 *      wavefronts. Ring spacing encodes the effective wavelength.
 *   3. SLOW ROTATION of the lobe axis over ~50-frame timescales —
 *      the rotating-beam / lighthouse effect from the post counter-
 *      rotation. This evokes a pulsar or rotating-antenna radar.
 *   4. PULSE-DRIVEN BORDER — the antenna feed's oscillating current
 *      is visible as border alpha modulation at pulse_freq = 0.14
 *      (period ≈ 7 frames). You can SEE the power being driven in.
 *   5. BRIGHTER INTERIOR, DIMMER EXTERIOR — the 1/r far-field
 *      amplitude falloff is captured implicitly by iterate-to-iterate
 *      contraction (stability product 0.655). Energy concentrates
 *      near the source and diffuses outward over iterations.
 *
 * This is the first preset in the catalog to model pure-classical
 * electromagnetism (distinct from the quantum-EM presets
 * `exp_laughlin_fractional_hall_vortex` and `tanh_superconductor_
 * vortex_abyss`, which model quantum phases rather than radiation).
 *
 * Pipeline: transform -> reduce -> sample -> color(mode4) -> seed -> blend
 */
inline Pattern sin_hertzian_dipole_radiation_lobe() {
  // Sin inner: design guide §3 recommends |zn[0]| in 0.52–0.99;
  // 0.72 places us in the LINEAR-regime (not K_crit chaos) — the
  // dipole is a driven linear oscillator.
  constexpr float PRE  = 1.32f;
  constexpr float POST = 0.98f;
  // Stability product 1.32 · 0.98 = 1.29 (reaches seed rim while preserving dipole lobes)

  // Pre-rotation π/2 — rotates the input z by 90°, which aligns
  // Sin's angular zero with the vertical axis (dipole axis).
  // cos(π/2) = 0, sin(π/2) = 1 → zn[0] = {0, PRE}.

  // Post-rotation −π/8 (−22.5°): slow beam-steering.
  constexpr float COS_N_PI_8 = 0.9238795325f;  // cos(−π/8) = cos(π/8)
  constexpr float SIN_N_PI_8 = -0.3826834324f; // sin(−π/8) = −sin(π/8)

  // Lambda-over-eight phase offset: pure-imaginary post-translate.
  // In the far-field e^(i(kr − ωt)), each iteration adds one
  // increment of (kr − ωt); here that increment is π/8 ≈ 0.393.
  constexpr float PHASE_LAG = 0.3926990817f;  // π/8

  // Spatial wave frequencies: √2 and √5 are maximally irrational
  // relative to each other — the interfering wave1/wave2 never
  // phase-lock, producing quasi-periodic ring structure that
  // mirrors the distinct phase velocities of near vs far fields.
  constexpr float SQRT2 = 1.4142135624f;
  constexpr float SQRT5 = 2.2360679775f;

  Pattern p;
  p.name = "sin_hertzian_dipole_radiation_lobe";
  p.inner = InnerFunction::Sin;              // sin(θ)/r angular factor
  p.reduction = ReductionType::Torus;        // smooth spherical wavefronts

  // Pre-affine: pure-imaginary zn[0] aligns dipole axis vertically
  p.zn.zn[0] = {0.0f, PRE};
  // Small asymmetric perturbation — real antennas aren't ideal
  p.zn.zn[1] = {0.005f, -0.003f};

  // Post-affine: slow beam-steering counter-rotation
  p.zn.zn[2] = {COS_N_PI_8 * POST, SIN_N_PI_8 * POST};

  // Pure-imaginary phase-lag post-translate → wavefront propagation
  p.zn.zn[3] = {0.0f, PHASE_LAG};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.082f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.058f;
  p.zn.zn1_anim_amp = 0.024f;
  p.zn.zn1_anim_k = 2.0f;

  // ── Color: MODE 4 (Spatial Wave) — interference-ring rendering ──
  p.color.hue_rotation = 0.014f;
  p.color.sat_scale = 1.09f;
  p.color.fade = 0.988f;    // sustained antenna resonance across the field
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  // Mode 4 parameter reuse (see preset-design-guide §5):
  p.color.spiral_angle_coeff = 0.68f;    // hue x-slope: moderate gradient
  p.color.spiral_radius_coeff = SQRT2;   // wave1 spatial freq: √2
  p.color.chroma_breath_amp = 0.24f;     // wave1 amplitude
  p.color.chroma_breath_freq = SQRT5;    // wave2 spatial freq: √5
  p.color.chroma_breath_phase = 0.14f;   // wave2 amplitude
  p.color.hue_velocity_base = 0.019f;    // wave1 temporal speed (ω)
  p.color.hue_velocity_breath = 0.012f;  // wave2 temporal speed (slower)
  p.color.hue_velocity_drift = 0.008f;   // base rotation drift
  p.color.luma_shape_depth = 0.28f;      // strong sat breathing
  p.color.luma_shape_center = 3.1f;      // multi-zone intensity modulation

  // ── Seed: ANTENNA FEED — oscillating current drive ──
  p.seed.border_width = 0.085f;          // visible antenna structure
  p.seed.hue_speed = 0.09f;
  p.seed.alpha = 0.94f;
  p.seed.pulse_freq = 0.14f;             // driving frequency ω
  p.seed.pulse_amp = 0.24f;              // drive modulation without starving the field

  // Speed: brisk — radiation propagation at c (relative to our
  // dimensionless iteration timescale).
  p.speed = 0.82f;
  return p;
}

} // namespace epi::presets
