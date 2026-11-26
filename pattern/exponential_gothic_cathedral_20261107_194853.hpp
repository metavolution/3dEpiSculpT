#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Exponential Gothic cathedral preset.
 *
 * Mathematical foundation:
 * - The exponential map e^z = e^x * (cos(y) + i*sin(y)) sends the entire
 *   complex plane into a conformal map of C* (the punctured plane). Under
 *   iteration with Grid reduction, vertical grid lines become concentric
 *   ARCHES (because e^z maps vertical lines → circles), while horizontal
 *   grid lines become radial COLUMNS (horizontal → rays). The effect is
 *   looking up into a fractal Gothic cathedral vault.
 *
 * - Pre-rotation at pi/4 (45 deg, octagonal, 8-fold) creates the
 *   characteristic ROSE WINDOW symmetry of Gothic cathedrals. The great
 *   west rose windows of Chartres and Notre-Dame have 8-fold or 12-fold
 *   structure; octagonal is the most common.
 *
 * - Grid reduction IS the structural skeleton: it creates the sharp
 *   "ribs" and "vaults" from tile boundaries. Without grid, the exp
 *   map produces smooth spirals; WITH grid, the tile edges become the
 *   load-bearing ribs of the vaulted ceiling.
 *
 * - Post-rotation at sqrt(2) radians (irrational, ≈81 deg) ensures
 *   successive vault levels never align — like the flying buttresses
 *   that step at non-repeating intervals around a cathedral exterior.
 *   sqrt(2) is pi-incommensurate, preventing periodic vault stacking.
 *
 * - Spatial wave color (mode 4) creates colored "stained glass" bands
 *   across the cathedral structure. The left→right hue gradient mimics
 *   the directional light through a row of clerestory windows.
 *   Dual-wave frequencies at phi and 1/phi create self-similar
 *   coloring — like the nested geometric tracery in real stained glass.
 *
 * - High fade (0.986) preserves many arch generations simultaneously,
 *   creating the infinite-recession illusion of a nave stretching
 *   toward a vanishing point.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode4) -> seed -> blend
 */
inline Pattern exponential_gothic_cathedral() {
  // pi/4 (45 deg): octagonal = Gothic rose window symmetry
  constexpr float C4  = 0.7071067812f;  // cos(pi/4) = sqrt(2)/2
  constexpr float S4  = 0.7071067812f;  // sin(pi/4) = sqrt(2)/2
  // sqrt(2) radians ≈ 81.0 deg: irrational, pi-incommensurate
  constexpr float CR2 = 0.1556174932f;  // cos(sqrt(2))
  constexpr float SR2 = 0.9877659460f;  // sin(sqrt(2))

  constexpr float PRE  = 1.1964f;  // moderate: exp stays within visual range
  constexpr float POST = 0.9331f;  // mild contraction for vault stability

  Pattern p;
  p.name = "exponential_gothic_cathedral";
  p.inner = InnerFunction::Exp;          // exp maps → arches + columns
  p.reduction = ReductionType::Grid;     // sharp ribs and vaults

  p.zn.zn[0] = {C4 * PRE, S4 * PRE};
  p.zn.zn[1] = {0.0347f, -0.0249f};          // tiny imaginary offset → slight asymmetry
  p.zn.zn[2] = {CR2 * POST, SR2 * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.011f;
  p.color.sat_scale = 1.16f;            // vivid stained glass
  p.color.fade = 0.986f;                // high: deep vault recession
  p.color.color_mode = as_int(ColorMode::SpatialWave);               // Spatial wave: stained glass bands
  // Directional light gradient (left→right clerestory windows)
  p.color.spiral_angle_coeff = 0.50f;   // hue_x_slope: moderate gradient
  // Self-similar tracery: phi and 1/phi frequencies
  p.color.spiral_radius_coeff = 0.32f;  // wave1 spatial freq (phi-based)
  p.color.chroma_breath_amp = 0.16f;    // wave1 amplitude
  p.color.chroma_breath_freq = 0.20f;   // wave2 spatial freq (1/phi)
  p.color.chroma_breath_phase = 0.09f;  // wave2 amplitude (subtle)
  p.color.hue_velocity_base = 0.010f;   // slow temporal drift
  p.color.hue_velocity_breath = 0.007f; // gentle breath
  p.color.hue_velocity_drift = 0.003f;  // glacial base rotation
  // Luminance depth: alternating bright (vault crown) and dark (rib shadow).
  p.color.luma_shape_depth = 0.24f;
  p.color.luma_shape_center = 1.7f;

  p.seed.border_width = 0.089f;         // moderate: feeds structural energy
  p.seed.hue_speed = 0.093f;
  p.seed.alpha = 0.92f;
  p.speed = 0.52f;                      // slow: contemplative, cathedral-like
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.115f;
  p.zn.zn0_anim_amp = 0.041f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.131f;
  p.zn.zn1_anim_amp = 0.046f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.138f;
  p.seed.pulse_amp = 0.20f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.055f,
                            .amplitude = 0.120f,
                            .offset = 0.320f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.122f,
                            .amplitude = 0.096f,
                            .offset = 0.160f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.071f,
                            .amplitude = 0.033f,
                            .offset = 0.093f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
