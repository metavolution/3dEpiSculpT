#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Exponential Cherenkov wave tunnel preset.
 *
 * ELECTRODYNAMICS: CHERENKOV RADIATION & ELECTROMAGNETIC WAVE PROPAGATION
 *
 * When a charged particle travels through a dielectric medium FASTER than
 * the phase velocity of light in that medium (v > c/n), it emits coherent
 * electromagnetic radiation in a CONE — Cherenkov radiation.  This is the
 * optical analogue of a sonic boom: the particle outruns its own field,
 * and the wavefronts pile up into a shock cone.
 *
 * The CHERENKOV ANGLE θ_C is determined by:
 *
 *   cos(θ_C) = c/(nv) = 1/(nβ)
 *
 * For a particle at v = 0.9c in glass (n = 1.5):
 *   cos(θ_C) = 1/(1.5 × 0.9) = 1/1.35 ≈ 0.7407
 *   θ_C = arccos(0.7407) ≈ 42.2°
 *
 * This angle determines the opening of the Mach cone.  Faster particles
 * → wider cones.  At the threshold (v = c/n): θ_C = 0° (no radiation).
 * At v → c: θ_C → arccos(1/n) ≈ 48.2° for glass (the maximum cone angle).
 *
 * The PRE-ROTATION uses the MAGIC ANGLE arctan(√2) ≈ 54.74°.  This angle
 * appears throughout electrodynamics:
 *
 *   - BREWSTER'S ANGLE for n = √3: the angle at which reflected light is
 *     perfectly polarized (p-polarization reflection coefficient = 0)
 *   - MAGIC ANGLE in NMR/MAS: the angle where (3cos²θ − 1) = 0, eliminating
 *     dipolar coupling and chemical shift anisotropy
 *   - DIPOLE RADIATION NULL: an oscillating dipole radiates zero power at
 *     arctan(√2) from its axis (the angular factor sin²θ/(1−β²cos²θ)⁵
 *     has a null here for certain β)
 *   - TETRAHEDRAL ANGLE: arctan(√2) = arccos(1/√3) is the semi-angle of
 *     the cone circumscribing a regular tetrahedron
 *
 * The exact algebraic values: cos(arctan(√2)) = 1/√3, sin(arctan(√2)) = √(2/3).
 *
 * ENGINE MAPPING TO ELECTRODYNAMICS:
 *
 *   THE EM WAVE = Exp(z) inner function
 *     e^z = e^x(cos y + i sin y) directly IS the complex EM wave:
 *     E(z,t) = E₀ e^{i(kz-ωt)} = E₀ e^{-αz} e^{i(βz-ωt)}.
 *     The real part (e^x) maps the AMPLITUDE (exponential attenuation
 *     in a lossy medium = skin effect), while the imaginary part (angle)
 *     maps the PHASE (wave oscillation).  Under iteration, Exp creates
 *     the wavefront architecture: arches = field maxima, dark regions =
 *     field nodes, the tunnel depth = propagation distance into the medium.
 *
 *   THE WAVEGUIDE = Torus reduction
 *     Torus provides seamless mirrored boundaries — the waveguide walls
 *     that confine the EM field.  Combined with Exp's radial mapping,
 *     this creates a circular waveguide (like a microwave cavity or
 *     fiber optic) where the field bounces between boundaries, creating
 *     STANDING WAVE MODES visible as the tunnel's radial structure.
 *
 *   THE MAGIC ANGLE = arctan(√2) pre-rotation (54.74°)
 *     cos = 1/√3 ≈ 0.5774 (EXACT algebraic value)
 *     sin = √(2/3) ≈ 0.8165 (EXACT algebraic value)
 *     This creates 2π/0.9553 ≈ 6.6 arch-pairs per revolution — a
 *     NON-INTEGER count that makes each revolution slightly different.
 *     The irrational step ensures the wave pattern never exactly repeats,
 *     just as in a real waveguide with incommensurate mode frequencies.
 *
 *   THE CHERENKOV CONE = -θ_C post-rotation (-42.2°)
 *     Each depth layer is rotated by the Cherenkov angle.  Looking into
 *     the tunnel, the successive layers trace out the SHOCK CONE geometry:
 *     the spiral twist IS the cone opening angle made visible.  Faster
 *     particles would create a wider spiral — you can SEE the Mach number.
 *
 *   SKIN EFFECT = fade (0.988)
 *     In a conducting medium, EM fields decay as e^{-z/δ} where δ is the
 *     skin depth.  fade = 0.988 → γ = -ln(0.988) ≈ 0.012.  The tunnel
 *     dims with depth: that IS the field attenuating as it penetrates
 *     the medium.  Q = π/0.012 ≈ 262: high-Q cavity, many visible
 *     wavefronts before the field decays to noise.
 *
 *   THREE EM TIMESCALES = Mode 3 (Alive) hue velocity
 *     base  = 0.025 → carrier wave oscillation (fast E-field cycling)
 *     breath = 0.014 → envelope modulation (wave packet shape)
 *     drift  = 0.005 → group velocity dispersion (packet spreading)
 *     You SEE: rapid hue flicker (carrier), medium pulse (envelope),
 *     slow color drift (dispersion).  This IS a dispersive wave packet
 *     propagating through the tunnel/medium.
 *
 *   WAVE POLARIZATION = chroma_breath (angular variation)
 *     breath_phase = 0.30 → polarization ellipse orientation.
 *     The brightness varies angularly at any instant: bright sectors are
 *     where E-field is maximum (polarization aligned), dark sectors are
 *     where E-field is zero (orthogonal polarization).  This IS the
 *     rotating polarization vector of an elliptically polarized wave.
 *
 * WHAT TO WATCH (EM wave propagation in action):
 *   1. WAVEFRONTS: Exp arches = field maxima of the propagating EM wave
 *   2. CHERENKOV SPIRAL: depth twist = shock cone opening angle
 *   3. SKIN DEPTH: tunnel dims with depth = exponential field attenuation
 *   4. CARRIER: fast hue cycling = E-field oscillation at frequency ω
 *   5. ENVELOPE: medium brightness pulse = wave packet shape
 *   6. DISPERSION: slow color drift = group velocity ≠ phase velocity
 *   7. POLARIZATION: angular brightness variation = E-field vector rotation
 *   8. MAGIC ANGLE: ~6.6 arches/revolution = tetrahedral/Brewster geometry
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern exp_cherenkov_wave_tunnel() {
  // arctan(√2) ≈ 0.9553 rad (54.74°): the magic angle
  // cos = 1/√3 (EXACT), sin = √(2/3) (EXACT)
  // Brewster angle, NMR magic angle, dipole null, tetrahedral semi-angle
  constexpr float CM   =  0.5774f;           // cos(arctan(√2)) = 1/√3
  constexpr float SM   =  0.8165f;           // sin(arctan(√2)) = √(2/3)
  // -θ_C ≈ -42.2° (Cherenkov angle for v=0.9c in glass n=1.5)
  // cos(θ_C) = 1/(nβ) = 1/1.35 ≈ 0.7407
  constexpr float CC   =  0.7407f;           // cos(θ_C)
  constexpr float SCN  = -0.6719f;           // sin(-θ_C)

  constexpr float PRE  = 0.86f;  // moderate: clear wavefront architecture
  constexpr float POST = 0.93f;  // gentle contraction: product 0.800

  Pattern p;
  p.name = "exp_cherenkov_wave_tunnel";
  p.inner = InnerFunction::Exp;              // e^z = amplitude × phase = EM wave
  p.reduction = ReductionType::Torus;        // waveguide: seamless cavity walls

  p.zn.zn[0] = {CM * PRE, SM * PRE};
  p.zn.zn[1] = {0.006f, -0.004f};           // perturbation: excites multiple modes
  p.zn.zn[2] = {CC * POST, SCN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 3 (Alive) — three EM timescales ──
  p.color.hue_rotation = 0.010f;             // moderate: wave interference colors
  p.color.sat_scale = 1.12f;                 // vivid: Cherenkov blue-white glow
  p.color.fade = 0.988f;                     // SKIN EFFECT: Q ≈ 262, deep penetration
  p.color.color_mode = as_int(ColorMode::Alive);                    // Alive: carrier + envelope + dispersion
  p.color.spiral_angle_coeff = 1.10f;        // angular: wavefront node sectors
  p.color.spiral_radius_coeff = 0.48f;       // radial: wave attenuation rings
  // WAVE POLARIZATION (angular brightness modulation)
  p.color.chroma_breath_amp = 0.22f;         // moderate: visible polarization
  p.color.chroma_breath_freq = 0.12f;        // polarization rotation rate
  p.color.chroma_breath_phase = 0.30f;       // polarization ellipse orientation
  // THREE EM TIMESCALES: carrier, envelope, dispersion
  p.color.hue_velocity_base = 0.025f;        // FAST: carrier wave E-field oscillation
  p.color.hue_velocity_breath = 0.014f;      // MEDIUM: wave packet envelope
  p.color.hue_velocity_drift = 0.005f;       // SLOW: group velocity dispersion
  // Luminance: wavefront contrast — bright crests, dark nodes
  p.color.luma_shape_depth = 0.24f;          // STRONG: clear wavefront visibility
  p.color.luma_shape_center = 1.8f;

  // ── Seed: EM source — radiation from accelerated charge ──
  p.seed.border_width = 0.012f;              // moderate: wavefronts dominate
  p.seed.hue_speed = 0.06f;
  p.seed.alpha = 0.88f;

  p.speed = 0.52f;                           // moderate: watch wave propagation
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.124f;
  p.zn.zn0_anim_amp = 0.049f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.221f;
  p.zn.zn1_anim_amp = 0.055f;
  p.zn.zn1_anim_k = 3.0f;
  p.seed.pulse_freq = 0.108f;
  p.seed.pulse_amp = 0.20f;

  return p;
}

} // namespace epi::presets
