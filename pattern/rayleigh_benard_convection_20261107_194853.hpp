#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Rayleigh-Benard convection roll preset.
 *
 * Scientific foundation:
 * - Rayleigh-Benard convection is the canonical pattern-formation instability
 *   in fluid dynamics. A thin layer of fluid heated from below becomes
 *   unstable above the critical Rayleigh number Ra_c = 27*pi^4/4 ≈ 657.5,
 *   spontaneously organizing into regular convection cells.
 *
 * - The temperature profile between the hot bottom plate (T_hot) and the
 *   cold top plate (T_cold) follows T(y) ∝ cosh(y) in the conductive
 *   (pre-instability) state. cosh(z) IS this temperature profile:
 *   symmetric, with a dome peak at cosh(0) = 1 (the midplane), growing
 *   exponentially toward both plates.
 *
 * - When Ra > Ra_c, the cosh profile destabilizes into counter-rotating
 *   convection ROLLS. The iterated feedback of cosh(z) with grid reduction
 *   produces exactly this: cosh's symmetric expansion creates the roll
 *   amplitude profile, while grid reduction's sharp tile boundaries create
 *   the visible CELL WALLS — the polygonal boundaries between adjacent
 *   convection cells.
 *
 * - In real Benard experiments, cells are HEXAGONAL (Benard-Marangoni cells
 *   in a dish of heated silicone oil show perfect hexagons). Post-rotation
 *   at pi/3 (60 deg = hexagonal) encodes this 6-fold symmetry.
 *
 * - Pre-rotation at 2*pi/sqrt(Ra_c/100) ≈ 2*pi/2.564 ≈ 2.451 rad encodes
 *   the critical Rayleigh number. This irrational angle ensures the roll
 *   pattern is quasiperiodic — like real convection in finite containers
 *   where boundary effects prevent perfect periodicity.
 *
 * - Breathing color (mode 2) simulates the thermal oscillation of
 *   convection rolls: intensity waxes and wanes as plumes rise and fall.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode2) -> seed -> blend
 */
inline Pattern rayleigh_benard_convection() {
  // 2*pi/sqrt(Ra_c/100): Rayleigh-derived irrational angle
  // Ra_c = 657.5, sqrt(Ra_c/100) = sqrt(6.575) ≈ 2.5641
  // 2*pi/2.5641 ≈ 2.4507 rad
  constexpr float CRA = -0.7751405506f; // cos(2.4507)
  constexpr float SRA =  0.6318148090f; // sin(2.4507)
  // pi/3 (60 deg): hexagonal symmetry — Benard cells
  constexpr float C3  =  0.5f;          // cos(pi/3)
  constexpr float S3  =  0.8660254038f; // sin(pi/3)

  constexpr float PRE  = 0.84f;  // moderate: cosh(0)=1 dome stays bounded
  constexpr float POST = 0.88f;

  Pattern p;
  p.name = "rayleigh_benard_convection";
  p.inner = InnerFunction::Cosh;         // cosh IS the conductive T profile
  p.reduction = ReductionType::Grid;     // sharp cell boundaries

  p.zn.zn[0] = {CRA * PRE, SRA * PRE};
  // Small asymmetric perturbation: in real convection, the onset of rolls
  // requires a finite-amplitude perturbation to break symmetry.
  p.zn.zn[1] = {0.012f, -0.008f};
  p.zn.zn[2] = {C3 * POST, S3 * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.978f;                // convection dissipates: moderate fade
  p.color.color_mode = as_int(ColorMode::Breathing);               // Breathing: thermal oscillation
  // Breathing envelope simulates plume rise/fall cycle.
  // In BZ we used mode 3; here mode 2's simpler breathing better matches
  // the single-frequency thermal oscillation of Benard rolls.
  p.color.spiral_angle_coeff = 0.55f;   // moderate angle→hue (hexagonal cells)
  p.color.spiral_radius_coeff = 0.35f;  // radial temperature gradient
  p.color.chroma_breath_amp = 0.28f;    // strong breathing: plume pulsation
  p.color.chroma_breath_freq = 0.20f;   // thermal oscillation frequency
  p.color.chroma_breath_phase = 0.45f;  // spatial phase (cell-to-cell offset)
  p.color.hue_velocity_base = 0.014f;
  p.color.hue_velocity_breath = 0.010f;
  p.color.hue_velocity_drift = 0.004f;
  // Luminance shaping: hot upwelling (bright) vs cold downwelling (dark).
  p.color.luma_shape_depth = 0.22f;
  p.color.luma_shape_center = 1.9f;

  p.seed.border_width = 0.022f;         // moderate: convection needs energy
  p.seed.hue_speed = 0.075f;
  p.seed.alpha = 0.90f;
  p.speed = 0.80f;                      // brisk: convection is dynamic
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.108f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.195f;
  p.zn.zn1_anim_amp = 0.044f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.137f;
  p.seed.pulse_amp = 0.20f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.056f,
                            .amplitude = 0.120f,
                            .offset = 0.350f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.103f,
                            .amplitude = 0.120f,
                            .offset = 0.280f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.051f,
                            .amplitude = 0.030f,
                            .offset = 0.075f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
