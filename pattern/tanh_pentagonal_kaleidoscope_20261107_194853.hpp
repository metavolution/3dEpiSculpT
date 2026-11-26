#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh pentagonal kaleidoscope preset.
 *
 * Mathematical foundation:
 * - Five-fold symmetry is one of the rarest in nature: it appears in
 *   echinoderms (starfish, sea urchins), certain flowers (morning glory,
 *   hibiscus), quasicrystals (Penrose tilings), and the icosahedron.
 *   It is FORBIDDEN in periodic crystallography (no 2D lattice has
 *   5-fold symmetry) -- which is exactly why it creates such visually
 *   striking non-repeating patterns under iteration.
 *
 * - tanh(z) saturates at +/-1, creating bounded "mirror walls."  With
 *   expanding pre-scale (|zn0| = 1.09 > 1.0), each iteration pushes
 *   structure past the tanh walls, which fold it back into [-1,1].
 *   This creates exponentially increasing density of internal structure:
 *   a fractal kaleidoscope where each layer contains finer detail.
 *
 * - Pre-rotation at 2*pi/5 (72 deg, pentagonal) creates 5-fold rotational
 *   symmetry.  After 5 iterations the pattern completes one revolution,
 *   but each iteration also folds via tanh, so the accumulated structure
 *   forms a PENTAGONAL MANDALA: 5 petals/sectors, each containing a
 *   recursively nested version of the whole pattern.
 *
 * - Post-rotation at the golden angle (2*pi*(2-phi) ~ 137.5 deg) is the
 *   MOST IRRATIONAL angle (farthest from any rational approximation).
 *   It produces the densest possible orbit -- maximizing the visual
 *   richness of each depth layer.  The golden angle is deeply connected
 *   to pentagons: phi = 2*cos(pi/5), the diagonal/side ratio of a
 *   regular pentagon.  This makes golden post-rotation the NATURAL
 *   companion to pentagonal pre-rotation.
 *
 * - Torus reduction creates seamless, smooth kaleidoscopic reflections
 *   (like a mirrored kaleidoscope tube) rather than sharp grid tiles.
 *   The continuous mirroring of torus + the dense golden-angle layering
 *   produces a mandala-like visual of infinite recursive depth.
 *
 * - Color mode 1 (Spiral) uses angle-dependent hue to make the 5-fold
 *   symmetry dramatically visible: each of the 5 sectors gets a distinct
 *   hue band.  The log-distance term creates concentric depth rings
 *   within each sector.  Luma shaping adds bright peaks at high
 *   concentration and dark valleys, creating jewel-like facets.
 *   Mode 1 is under-explored (only 1 preset) -- this fills that gap.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode1) -> seed -> blend
 */
inline Pattern tanh_pentagonal_kaleidoscope() {
  // 2*pi/5 (72 deg): pentagonal symmetry
  constexpr float C5  = 0.3090169944f;   // cos(2*pi/5)
  constexpr float S5  = 0.9510565163f;   // sin(2*pi/5)
  // Golden angle: 2*pi*(2-phi) ~ 137.5 deg -- densest layering
  constexpr float CGA = -0.7373688781f;  // cos(golden angle)
  constexpr float SGA = 0.6754902943f;   // sin(golden angle)

  constexpr float PRE  = 1.09f;  // EXPANDING: fractal density increase
  constexpr float POST = 0.74f;  // strong contraction: 1.09*0.74 = 0.807

  Pattern p;
  p.name = "tanh_pentagonal_kaleidoscope";
  p.inner = InnerFunction::Tanh;         // bounded expansion -> infinite density
  p.reduction = ReductionType::Torus;    // smooth kaleidoscopic reflections

  p.zn.zn[0] = {C5 * PRE, S5 * PRE};
  p.zn.zn[1] = {0.003f, -0.005f};       // tiny perturbation
  p.zn.zn[2] = {CGA * POST, SGA * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.010f;        // slow: let spiral color do the work
  p.color.sat_scale = 1.08f;            // boosted for mandala vibrancy
  p.color.fade = 0.989f;                // high: deep mandala persistence
  p.color.color_mode = as_int(ColorMode::Spiral);               // Spiral: angular hue bands + depth rings
  p.color.spiral_angle_coeff = 1.40f;   // very strong: makes 5 sectors dramatic
  p.color.spiral_radius_coeff = 0.55f;  // moderate: concentric depth layers
  // Mode 1 ignores breathing and velocity params
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.24f;     // strong: jewel-like luminance facets
  p.color.luma_shape_center = 1.6f;     // ring position

  p.seed.border_width = 0.009f;         // minimal: kaleidoscope depth dominates
  p.seed.hue_speed = 0.05f;             // slow: meditative
  p.seed.alpha = 0.86f;
  p.speed = 0.48f;                      // slow: contemplative mandala rotation
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.119f;
  p.zn.zn0_anim_amp = 0.041f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.215f;
  p.zn.zn1_anim_amp = 0.055f;
  p.zn.zn1_anim_k = 8.0f;
  p.seed.pulse_freq = 0.111f;
  p.seed.pulse_amp = 0.19f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.068f,
                            .amplitude = 0.154f,
                            .offset = 0.550f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.093f,
                            .amplitude = 0.050f,
                            .offset = 0.000f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.053f,
                            .amplitude = 0.030f,
                            .offset = 0.050f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
