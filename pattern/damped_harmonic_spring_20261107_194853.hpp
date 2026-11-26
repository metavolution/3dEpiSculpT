#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Damped harmonic spring potential preset.
 *
 * PHYSICS: getriebener gedaempfter harmonischer Oszillator (Federmodell)
 *
 * The harmonic oscillator potential energy is V(x) = (1/2)*k*x^2.
 * Taylor-expanding cos(z) around the origin:
 *   cos(z) = 1 - z^2/2 + z^4/24 - ...
 * The leading term (1 - z^2/2) IS the harmonic potential (inverted,
 * with maximum at equilibrium z=0).  This means cos(z) maps the
 * complex plane according to the EXACT potential landscape of a
 * mass-spring system near equilibrium.
 *
 * ENGINE MAPPING:
 *
 *   POTENTIAL WELL = cos(z) inner function
 *     cos(0) = 1 means the center of the domain IS the equilibrium
 *     position (maximum of the "restoring potential").  Moving away
 *     from z=0, the cosine drops quadratically — the "potential energy"
 *     increases.  Under iterated feedback, pixels near the center stay
 *     stable (the oscillator at rest), while pixels further out get
 *     warped more strongly (the oscillator at large amplitude).
 *
 *   POTENTIAL WELLS IN A LATTICE = Grid reduction
 *     Grid reduction tiles the plane into repeating cells.  Each cell
 *     IS one independent potential well.  The tile boundaries are the
 *     CLASSICAL TURNING POINTS where kinetic energy → 0 and the
 *     oscillator reverses direction.  You can SEE the oscillating
 *     structure bouncing between the grid walls.
 *
 *   DAMPING = fade (0.975) — slightly stronger damping than the
 *     resonance preset.  Quality factor Q = pi/(-ln(0.975)) = pi/0.0253
 *     ~ 124.  The decay is visible: structure fades between seed pulses.
 *
 *   DRIVING = seed (border_width=0.048, pulse_amp=0.42)
 *     Periodic energy injection from the borders.  The wide border
 *     makes the "push" visible entering the grid cells.
 *
 *   THREE TIMESCALES = Alive color mode (mode 3)
 *     The alive color system has THREE velocity components:
 *       v(t) = base + breath*sin(0.08*2pi*t) + drift*sin(0.003*2pi*t + 1.7)
 *     These map EXACTLY to the three characteristic frequencies of a
 *     driven damped oscillator:
 *       hv_base  = 0.020 → NATURAL FREQUENCY omega_0 (the spring itself)
 *       hv_breath = 0.015 → DRIVING FREQUENCY omega (the external force)
 *       hv_drift  = 0.005 → BEAT FREQUENCY |omega_0 - omega| (the envelope)
 *     What you SEE: fast hue cycling (spring vibration) modulated by
 *     medium breathing (driving force) modulated by slow drift (resonance
 *     envelope).  This is a SPECTROGRAM made visible.
 *
 * - Pre-rotation at pi/4 (45 deg, octagonal) creates 8 potential wells
 *   arranged octagonally around the center.  This is like 8 springs
 *   attached to a central mass at 45-degree intervals — the 2D
 *   isotropic oscillator.
 *
 * - Post-rotation at -pi/e (irrational counter-rotation) makes the
 *   oscillation trajectories QUASIPERIODIC in the potential well:
 *   the orbit never exactly repeats, like a Lissajous figure with
 *   irrational frequency ratio.  This IS the 2D analogue of the
 *   driven oscillator with incommensurate frequencies.
 *
 * WHAT TO WATCH:
 *   1. Grid cells = individual potential wells (each one "springs")
 *   2. Structure oscillates WITHIN each cell (the mass on the spring)
 *   3. Between drive pulses, the oscillation DECAYS (damping)
 *   4. Drive pulse arrives → amplitude JUMPS UP (re-excitation)
 *   5. Fast hue cycling = spring vibration; slow envelope = resonance
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern damped_harmonic_spring() {
  // pi/4 (45 deg): octagonal — 8 isotropic spring directions
  constexpr float C4  = 0.7071067812f;   // cos(pi/4) = sqrt(2)/2
  constexpr float S4  = 0.7071067812f;   // sin(pi/4) = sqrt(2)/2
  // -pi/e (-66.2 deg): irrational Lissajous ratio
  constexpr float CPE  = 0.4033504085f;  // cos(pi/e)
  constexpr float SPEN = -0.9150734295f; // sin(-pi/e)

  constexpr float PRE  = 1.28f;  // drives cos potential across classical turning points
  constexpr float POST = 0.97f;  // couples grid lattice wells to seed boundary

  Pattern p;
  p.name = "damped_harmonic_spring";
  p.inner = InnerFunction::Cos;          // cos(z) ~ 1 - z^2/2 IS the harmonic potential
  p.reduction = ReductionType::Grid;     // each tile IS a potential well

  p.zn.zn[0] = {C4 * PRE, S4 * PRE};
  p.zn.zn[1] = {0.009f, -0.006f};       // perturbation: excite off-center oscillation
  p.zn.zn[2] = {CPE * POST, SPEN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.20f;
  p.zn.zn0_anim_amp = 0.075f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.13f;
  p.zn.zn1_anim_amp = 0.018f;
  p.zn.zn1_anim_k = 4.0f;

  // ── Color: MODE 3 (Alive) — three timescales of the driven oscillator ──
  p.color.hue_rotation = 0.017f;
  p.color.sat_scale = 1.10f;
  p.color.fade = 0.985f;                // DAMPING: high-Q visible decay between pulses
  p.color.color_mode = as_int(ColorMode::Alive);               // Alive: triple-velocity = triple-frequency
  p.color.spiral_angle_coeff = 0.80f;   // angular: reveals potential well sectors
  p.color.spiral_radius_coeff = 0.50f;  // radial: distance-from-equilibrium coloring
  p.color.chroma_breath_amp = 0.25f;    // strong: visible oscillation modulation
  p.color.chroma_breath_freq = 0.15f;   // spatial breathing within wells
  p.color.chroma_breath_phase = 0.40f;
  // THE THREE FREQUENCIES: omega_0 (natural), omega (driving), delta_omega (beat)
  p.color.hue_velocity_base = 0.020f;   // FAST: natural spring frequency omega_0
  p.color.hue_velocity_breath = 0.015f; // MEDIUM: driving frequency omega
  p.color.hue_velocity_drift = 0.005f;  // SLOW: beat/resonance envelope
  // Luminance contours matched to log_r = log(0.01 * |t_z|) ≈ -4.3
  p.color.luma_shape_depth = 0.055f;
  p.color.luma_shape_center = -4.3f;

  // ── Seed: THE DRIVING FORCE ──
  p.seed.border_width = 0.075f;         // WIDE: driving force enters from cell edges
  p.seed.hue_speed = 0.10f;
  p.seed.alpha = 0.94f;
  p.seed.pulse_freq = 0.22f;            // DRIVING FREQUENCY: periodic re-excitation
  p.seed.pulse_amp = 0.28f;             // STRONG DRIVE: visible amplitude jumps

  p.speed = 0.80f;
  return p;
}

} // namespace epi::presets
