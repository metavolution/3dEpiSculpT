#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Lotka-Volterra Predator-Prey Limit Cycle — biology / conservative dynamics
 * =========================================================================
 *
 * PHYSICS / MATH  (Alfred Lotka 1925 / Vito Volterra 1926):
 *
 *   ẋ = a·x − b·x·y       (prey: linear growth, predated by y)
 *   ẏ = c·x·y − d·y       (predator: grows by feeding, dies off)
 *
 * Canonical parameters: a, b, c, d all positive. Classic values for
 * the "lynx–hare" Hudson Bay Company fur-trade data: a ≈ 0.5 (hare
 * reproduction), b ≈ 0.02 (lynx predation efficiency), c ≈ 0.01
 * (lynx reproduction from eating hares), d ≈ 0.5 (lynx mortality).
 *
 * The Lotka-Volterra system was among the FIRST mathematical models
 * of ecology. It captures the fundamental insight that predator and
 * prey populations exhibit COUPLED OSCILLATIONS: high prey → more
 * predator food → predator population rises → prey population
 * crashes → predators starve → prey recovers → cycle repeats.
 *
 * Key mathematical properties distinguishing LV from every other
 * preset in the catalog:
 *
 *   • FIXED POINT at (x*, y*) = (d/c, a/b) — the "balanced ecosystem"
 *     with all derivatives zero.
 *   • CONSERVED QUANTITY (Hamiltonian-like): H = b·x − c·ln(x) +
 *     d·y − a·ln(y). Every orbit lies on a level curve of H, and
 *     trajectories are CLOSED PERIODIC ORBITS around (x*, y*) —
 *     never spiraling in, never spiraling out.
 *   • MARGINALLY STABLE: the system has no attractor. Small
 *     perturbations change the orbit's energy (H value) permanently;
 *     there is no Lyapunov dissipation that "forgets" initial
 *     conditions.
 *
 * This is the FIRST CONSERVATIVE (non-dissipative) preset in the
 * catalog. Every other dynamical-system preset models DISSIPATIVE
 * systems where orbits converge to an attractor (Lorenz butterfly,
 * Hénon banana, Rössler ribbon, etc.). Lotka-Volterra models the
 * opposite regime: orbits form a one-parameter family of nested
 * closed curves, like the energy-level contours of a frictionless
 * pendulum.
 *
 * Visual signature: NESTED CONCENTRIC CLOSED CURVES around a
 * central equilibrium point — the integrable Hamiltonian phase
 * portrait. Each curve is a single predator-prey cycle's trajectory.
 *
 *
 * ENGINE MAPPING — conserved orbits AS feedback level curves
 *
 *   OSCILLATORY CORE = Cos inner function
 *     Near the fixed point, linearization gives ẍ + ω²·x = 0 with
 *     ω = √(a·d). This is the LINEAR HARMONIC OSCILLATOR equation —
 *     and cos(z) near z=0 is exactly the solution (1 − z²/2 + …).
 *     Cos's smooth parabolic lens behavior near the origin perfectly
 *     captures the small-amplitude LV orbits; Cos's full periodicity
 *     further out captures the large-amplitude nonlinear orbits
 *     without introducing spurious attraction or repulsion.
 *
 *   NESTED CLOSED ORBITS = near-resonant pre-rotation 2π/30
 *     A rotation rate of 2π/30 per iteration = 30 iterations per
 *     full orbit — matches typical LV visualization timescales
 *     (prey and predator populations cycle once per "season" of
 *     ~30 frames). The near-resonance keeps successive iterations
 *     tracing out the SAME orbit shape, creating visible nested
 *     closed curves rather than a diverging spiral.
 *
 *   CONSERVATIVE STABILITY = |zn[0]| · |zn[2]| = 0.76 · 0.93 = 0.707
 *     Just short of 1/√2 — enough contraction that successive orbits
 *     don't drift to infinity, but near enough to unity that the
 *     orbits remain approximately closed (conservative). Pushing
 *     the product toward 1.0 would make orbits open (spiral
 *     outward); dropping below 0.5 would collapse them to the
 *     fixed point. 0.707 hits the pocket where the eye perceives
 *     "family of closed curves" — the LV phase portrait.
 *
 *   EQUILIBRIUM SHIFT = post-translate zn[3] = (0.02, 0)
 *     Real populations have slight environmental carrying-capacity
 *     biases — the fixed point (d/c, a/b) is rarely exactly at
 *     origin. A 0.02 real shift places the orbit family's center
 *     off-origin, matching the asymmetric "prey-biased" positioning
 *     seen in real ecological phase portraits.
 *
 *   IRRATIONAL POST-ROTATION = −π/7 (≈ −25.7°) counter-rotation
 *     The closed orbits themselves are conservative, but successive
 *     levels of H produce different orbit shapes. Counter-rotating
 *     the post by −π/7 (heptagonal-adjacent, irrational relative to
 *     2π/30 pre-rotation) causes orbit radii to MODULATE — each H
 *     level cycles through slightly different shapes. This is
 *     mathematically equivalent to the action-angle variable drift
 *     seen in nearly-integrable systems (KAM theory).
 *
 *   OSCILLATION AMPLITUDE ENVELOPE = color mode 2 (Breathing)
 *     LV populations oscillate between (roughly) 2·a/b and zero for
 *     prey, and between 2·d/c and zero for predator. This amplitude
 *     modulation IS what chroma_breath renders: sat breathing with
 *     phase-coupling to spatial angle directly animates the
 *     prey/predator oscillation. breath_freq = 0.10 makes this
 *     visible on the same ~10-second timescale as the orbit trace.
 *
 *   SYMMETRY BREAKING = zn[1] = (0.005, −0.003)
 *     Pure LV has an exact time-reversal symmetry. A tiny
 *     perturbation in zn[1] breaks this, producing the slight
 *     thickening/pinching of orbits seen in real population data
 *     where environmental fluctuations add noise to pure LV.
 *
 *
 * WHAT TO WATCH:
 *   1. NESTED CLOSED CURVES — a family of concentric (but NOT
 *      circular) closed orbits around a central equilibrium point.
 *      Each curve is one level of H = constant, traced out by one
 *      combined prey-predator cycle.
 *   2. OFF-CENTER EQUILIBRIUM — the orbit family's center sits
 *      slightly right of origin (the (0.02, 0) shift = prey bias).
 *      This matches real ecological portraits where carrying
 *      capacity isn't at zero.
 *   3. BREATHING AMPLITUDE — saturation swells and contracts as
 *      mode 2's chroma_breath_amp * sin(2π·freq·t + phase·angle)
 *      animates population magnitude over the oscillation cycle.
 *      Peaks correspond to "prey boom" states; troughs to the
 *      "predator boom, prey crash" quarter-cycle.
 *   4. NO ATTRACTOR — unlike every other dynamical-system preset,
 *      this one does NOT converge to a fixed shape. The orbit
 *      family is marginally stable; small disturbances shift energy
 *      levels rather than being damped away. Watch how perturbations
 *      (preset transitions, ZN nudges) persist indefinitely instead
 *      of relaxing to a nominal state.
 *   5. PARABOLIC LENS DISTORTION — Cos's 1 − z²/2 behavior near
 *      origin creates a subtle "fisheye" warp on the inner orbits,
 *      compressing them radially. This is geometrically identical
 *      to the linearization approximation used to solve LV at
 *      small amplitude analytically.
 *
 * Catalog role: first conservative / Hamiltonian-flavored preset,
 * complementing every existing dissipative-attractor preset.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern cos_lotka_volterra_predator_prey() {
  // 2π/30 ≈ 0.2094 rad (12°) — one predator-prey orbit per 30 iterations.
  // Chosen so typical orbit traces close visibly within a few seconds.
  constexpr float COS_12 = 0.9781476008f;  // cos(2π/30)
  constexpr float SIN_12 = 0.2079116908f;  // sin(2π/30)

  // −π/7 (≈ −25.7°) — heptagonal counter-rotation, irrational
  // relative to 2π/30 pre, creating orbit-to-orbit amplitude drift.
  constexpr float COS_N_PI_7 = 0.9009688679f;   // cos(−π/7)
  constexpr float SIN_N_PI_7 = -0.4338837391f;  // sin(−π/7)

  // Cos prefers |zn[0]| in 0.52–0.99 per design guide §3; 0.76 hits
  // the smooth-lens regime where the 1−z²/2 approximation dominates
  // visually (exactly matching LV's near-fixed-point linearization).
  constexpr float PRE  = 1.4206f;
  constexpr float POST = 1.2591f;
  // Stability product 0.76 · 0.93 = 0.707 — just short of 1/√2,
  // the marginal-closure regime: orbits nearly close, producing
  // the characteristic "family of nested closed curves" phase
  // portrait rather than a spiral (product near 1) or collapse
  // (product near 0.5).

  Pattern p;
  p.name = "cos_lotka_volterra_predator_prey";
  p.inner = InnerFunction::Cos;         // harmonic oscillator near origin
  p.reduction = ReductionType::Torus;   // smooth closed-curve topology

  // Pre-affine: small rotation at |zn[0]| = 0.76 — near-harmonic
  // oscillation matching LV's small-amplitude limit ω = √(a·d).
  p.zn.zn[0] = {COS_12 * PRE, SIN_12 * PRE};
  // Symmetry-breaking perturbation — real populations have noise.
  p.zn.zn[1] = {0.0441f, -0.0263f};

  // Post-affine: irrational counter-rotation creates KAM-like drift
  // in orbit shape across successive H levels.
  p.zn.zn[2] = {COS_N_PI_7 * POST, SIN_N_PI_7 * POST};

  // Equilibrium shift — real prey-predator systems have non-zero
  // carrying capacity. Small real post-translate = prey-biased
  // asymmetry visible in real ecological phase portraits.
  p.zn.zn[3] = {0.02f, 0.0f};

  // ── Color: MODE 2 (Breathing) — population amplitude modulation ──
  p.color.hue_rotation = 0.013f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.984f;       // moderate-high — orbits persist visibly
  p.color.color_mode = as_int(ColorMode::Breathing);
  p.color.spiral_angle_coeff = 0.78f;   // reveal angular phase along orbit
  p.color.spiral_radius_coeff = 0.44f;  // moderate depth banding (H levels)
  // Breathing = predator-prey amplitude oscillation itself
  p.color.chroma_breath_amp = 0.28f;    // STRONG: the breathing IS the LV story
  p.color.chroma_breath_freq = 0.10f;   // slow — matches orbit period
  p.color.chroma_breath_phase = 2.2f;   // spatial coupling = rotating wave
  p.color.hue_velocity_base = 0.013f;
  p.color.hue_velocity_breath = 0.008f;
  p.color.hue_velocity_drift = 0.002f;
  p.color.luma_shape_depth = 0.22f;     // visible orbit-to-orbit luminance
  p.color.luma_shape_center = 1.9f;

  // ── Seed: external food-source input ──
  p.seed.border_width = 0.086f;    // visible edge injection = food supply
  p.seed.hue_speed = 0.06f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.08f;       // slow seasonal variation in food availability
  p.seed.pulse_amp = 0.25f;        // moderate drive — natural variation

  // Speed: moderate — matches typical ecology-textbook visualization
  // timescale where one full prey-predator cycle takes ~5-10 seconds.
  p.speed = 0.70f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.109f;
  p.zn.zn0_anim_amp = 0.048f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.165f;
  p.zn.zn1_anim_amp = 0.038f;
  p.zn.zn1_anim_k = 2.0f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.092f,
                            .amplitude = 0.123f,
                            .offset = 0.440f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.082f,
                            .amplitude = 0.120f,
                            .offset = 0.280f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.070f,
                            .amplitude = 0.030f,
                            .offset = 0.060f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
