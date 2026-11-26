#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Exponential Poynting flux tunnel preset.
 *
 * ELECTRODYNAMICS: THE POYNTING VECTOR & ELECTROMAGNETIC ENERGY FLUX
 *
 * The POYNTING VECTOR S = E x H = (1/mu_0)(E x B) describes the
 * directional energy FLUX of an electromagnetic field — the rate at
 * which electromagnetic energy flows through space, measured in watts
 * per square meter.  Named after John Henry Poynting (1884), it is the
 * fundamental quantity of electromagnetic energy transport.
 *
 * For a monochromatic plane wave propagating in the z-direction:
 *
 *   S(t) = (E_0^2 / Z_0) * cos^2(kz - wt) * z_hat
 *        = (E_0^2 / 2Z_0)(1 + cos(2wt - 2kz)) * z_hat
 *
 * Key insight: the energy flux oscillates at TWICE the wave frequency.
 * The cos^2 → (1 + cos(2wt))/2 identity means the Poynting vector
 * has a DC component (time-averaged flux) PLUS an oscillating component
 * at 2w.  This double-frequency oscillation is why fluorescent lights
 * at 50/60 Hz flicker at 100/120 Hz.
 *
 * The IMPEDANCE OF FREE SPACE: Z_0 = sqrt(mu_0/epsilon_0) ≈ 376.73 ohm
 * ≈ 120*pi ohm.  This fundamental constant sets the ratio of electric
 * to magnetic field strength in vacuum: E/H = Z_0 for a plane wave.
 *
 * ENGINE MAPPING TO ELECTROMAGNETIC ENERGY FLUX:
 *
 *   THE RADIATION FIELD = Exp inner function
 *     Exp maps Cartesian to polar: straight lines become concentric
 *     circles.  This IS the radiation field of a point source: spherical
 *     wavefronts expanding outward, with the Poynting vector pointing
 *     radially.  Each concentric ring is a wavefront of electromagnetic
 *     energy, and the exponential divergence models the 1/r^2 intensity
 *     falloff (in the feedback domain, successive iterations CREATE the
 *     expanding wavefronts).
 *
 *   THE ENERGY TUBE = Torus reduction
 *     Torus provides seamless wrapping — a tube of electromagnetic
 *     flux.  The tunnel view IS looking along a Poynting flux tube:
 *     the path that electromagnetic energy follows through space.
 *     In a waveguide, this IS the guided mode propagating down the tube.
 *
 *   THE FERMAT LATTICE = arctan(1/4) ≈ 14.04° pre-rotation
 *     tan(theta) = 1/4: the ratio of the unit to the FOUR fundamental
 *     Maxwell equations that govern all electromagnetic phenomena:
 *
 *       (1) div E = rho/eps_0      (Gauss's law for E)
 *       (2) div B = 0              (Gauss's law for B — no monopoles)
 *       (3) curl E = -dB/dt        (Faraday's law of induction)
 *       (4) curl B = mu_0*J + mu_0*eps_0*dE/dt  (Ampere-Maxwell law)
 *
 *     cos(arctan(1/4)) = 4/sqrt(17) ≈ 0.9701 (EXACT)
 *     sin(arctan(1/4)) = 1/sqrt(17) ≈ 0.2425 (EXACT)
 *     1^2 + 4^2 = 17: a FERMAT PRIME (F_2 = 2^{2^2} + 1 = 17).
 *
 *     17 is the number of WALLPAPER GROUPS — the exactly 17 distinct
 *     planar symmetry groups that classify ALL possible 2D periodic
 *     patterns.  This connects to EM waves in photonic crystals:
 *     the crystallographic symmetries of 2D periodic media determine
 *     which electromagnetic modes can propagate (photonic band gaps).
 *
 *     360°/14.04° ≈ 25.6 → quasi-26-fold symmetry.
 *     Each sector is one flux channel through the lattice.
 *
 *   THE SPECTRAL DEPTH = -arctan(sqrt(7)) ≈ -69.30° post-rotation
 *     cos(arctan(sqrt(7))) = 1/sqrt(8) = sqrt(2)/4 ≈ 0.3536 (EXACT)
 *     sin(-arctan(sqrt(7))) = -sqrt(7)/sqrt(8) = -sqrt(14)/4 ≈ -0.9354 (EXACT)
 *     Verification: (sqrt(2)/4)^2 + (sqrt(14)/4)^2 = 2/16 + 14/16 = 1 ✓
 *
 *     7 = the traditional number of electromagnetic spectrum bands
 *     (radio, microwave, infrared, visible, UV, X-ray, gamma-ray).
 *     sqrt(7) connects to the EM spectrum's logarithmic spanning of
 *     ~15 orders of magnitude in frequency.
 *     The denominators (4) connect to the 4 Maxwell equations above.
 *
 *     360°/69.30° ≈ 5.19 → quasi-5-fold depth symmetry.
 *     Combined with 26-fold azimuthal: 26 × 5.2 ≈ 135 interference cells.
 *
 *   DOUBLE-FREQUENCY OSCILLATION = Mode 2 (Breathing)
 *     The breathing IS the Poynting vector's double-frequency oscillation:
 *     S(t) = S_avg × (1 + cos(2wt)).  In Mode 2:
 *       sat *= 1 + amp * sin(freq * 2*pi*t + phase * angle)
 *     The amplitude oscillation represents the cos^2 intensity pattern
 *     of the electromagnetic wave — energy pulsing at twice the carrier
 *     frequency.  The phase gradient across the image shows the spatial
 *     component 2kz: the wave propagating through space.
 *
 * WHAT TO WATCH:
 *   1. 26-FOLD FLUX: electromagnetic energy channels through the lattice
 *   2. WAVEFRONTS: Exp concentric rings = expanding spherical wavefronts
 *   3. BREATHING: cos^2 intensity oscillation at double frequency
 *   4. TUNNEL: looking along a Poynting flux tube (guided wave)
 *   5. FERMAT STRUCTURE: 17 connects Maxwell to crystallographic symmetry
 *   6. SPECTRAL DEPTH: sqrt(7) layers = EM spectrum band stacking
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern exp_poynting_flux_tunnel() {
  // arctan(1/4) ≈ 0.2450 rad (14.04°): 4 Maxwell equations
  // cos = 4/sqrt(17), sin = 1/sqrt(17) — EXACT algebraic (17 = Fermat prime)
  // 17 = number of wallpaper groups: crystallographic EM symmetry
  constexpr float CM   =  0.9701f;           // cos(arctan(1/4)) = 4/√17
  constexpr float SM   =  0.2425f;           // sin(arctan(1/4)) = 1/√17
  // -arctan(√7) ≈ -1.2094 rad (-69.30°): EM spectrum depth
  // cos = √2/4 ≈ 0.3536, sin = -√14/4 ≈ -0.9354 — EXACT algebraic
  // 7 bands of the EM spectrum; denominators = 4 (Maxwell equations)
  constexpr float CSD  =  0.3536f;           // cos(arctan(√7)) = √2/4
  constexpr float SSDN = -0.9354f;           // sin(-arctan(√7)) = -√14/4

  constexpr float PRE  = 1.1492f;  // moderate: stable Exp expansion
  constexpr float POST = 0.9269f;  // mild contraction: product 0.771

  Pattern p;
  p.name = "exp_poynting_flux_tunnel";
  p.inner = InnerFunction::Exp;              // radiation field: spherical wavefronts
  p.reduction = ReductionType::Torus;        // flux tube: guided EM wave propagation

  p.zn.zn[0] = {CM * PRE, SM * PRE};
  p.zn.zn[1] = {0.0417f, -0.0429f};           // perturbation: field noise
  p.zn.zn[2] = {CSD * POST, SSDN * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  // ── Color: MODE 2 (Breathing) — double-frequency Poynting oscillation ──
  p.color.hue_rotation = 0.010f;             // moderate: spectral evolution
  p.color.sat_scale = 1.14f;                 // vivid: electromagnetic energy glow
  p.color.fade = 0.990f;                     // deep: ~100 frame persistence
  p.color.color_mode = as_int(ColorMode::Breathing);                    // Breathing: cos²(wt) energy oscillation
  p.color.spiral_angle_coeff = 0.90f;        // STRONG: shows 26 flux channels
  p.color.spiral_radius_coeff = 0.50f;       // strong: wavefront depth rings
  // THE POYNTING PULSE: energy flux oscillation at 2w
  p.color.chroma_breath_amp = 0.30f;         // strong: visible intensity pulsation
  p.color.chroma_breath_freq = 0.09f;        // EM oscillation rate
  p.color.chroma_breath_phase = 2.0f;        // spatial propagation phase (2kz term)
  // Mode 2 does not use hue velocity params
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  // Luminance: strong wavefront contrast (bright crests, dark troughs)
  p.color.luma_shape_depth = 0.24f;          // strong: crisp electromagnetic wavefronts
  p.color.luma_shape_center = 2.0f;

  // ── Seed: moderate — injects initial EM perturbation ──
  p.seed.border_width = 0.083f;              // moderate: waveguide wall visible
  p.seed.hue_speed = 0.129f;
  p.seed.alpha = 0.92f;

  p.speed = 0.44f;                           // moderate: watch EM waves propagate
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.126f;
  p.zn.zn0_anim_amp = 0.040f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.223f;
  p.zn.zn1_anim_amp = 0.042f;
  p.zn.zn1_anim_k = 2.0f;
  p.seed.pulse_freq = 0.109f;
  p.seed.pulse_amp = 0.17f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.060f,
                            .amplitude = 0.140f,
                            .offset = 0.500f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.078f,
                            .amplitude = 0.120f,
                            .offset = 0.300f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.072f,
                            .amplitude = 0.045f,
                            .offset = 0.129f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
