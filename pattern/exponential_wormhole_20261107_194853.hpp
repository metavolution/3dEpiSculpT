#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Exponential wormhole vortex preset.
 *
 * Mathematical foundation:
 * - A wormhole (Einstein-Rosen bridge) connects two asymptotically flat
 *   regions of spacetime through a throat of minimal radius r_0.  The
 *   embedding diagram of the Flamm paraboloid z(r) = 2*sqrt(r_0*(r-r_0))
 *   shows spacetime curving through a tube.  In the conformal picture,
 *   the wormhole throat's topology is mapped by the exponential function:
 *   e^z sends the strip {|Im(z)| < pi} conformally onto the slit plane,
 *   which IS the doubly-connected topology of a wormhole throat.
 *
 * - exp(z) maps vertical lines to circles (the throat's circular cross-
 *   sections) and horizontal lines to radial rays (geodesics through
 *   the throat).  With Grid reduction, the vertical grid lines become
 *   visible THROAT RINGS, and the horizontal lines become radial
 *   distance markers -- directly visualizing the wormhole geometry.
 *
 * - Pre-rotation at 2*pi/5 (72 deg, pentagonal) creates 5-fold modal
 *   structure of the throat.  In quantum gravity, wormhole throats can
 *   support discrete angular modes; 5-fold is the simplest non-trivial
 *   prime-adjacent mode that creates complex interference.
 *
 * - Post-rotation at NEGATIVE sqrt(2) radians creates COUNTER-ROTATION:
 *   successive depth layers rotate in the opposite direction from the
 *   pre-rotation.  This is the visual signature of a ROTATING wormhole
 *   (Kerr metric), where frame-dragging causes infalling matter to
 *   orbit in the opposite direction when viewed from the other side.
 *
 * - |zn0| = 0.72: moderate exponential growth, keeping the throat's
 *   circle structure visible (too large and e^z overwhelms everything).
 *   |zn2| = 0.86: stability product 0.72*0.86 = 0.619, well within
 *   the stable regime for deep spacetime persistence.
 *
 * Pipeline: transform -> reduce -> sample -> color(mode3) -> seed -> blend
 */
inline Pattern exponential_wormhole() {
  // 2*pi/5 (72 deg): pentagonal throat modes
  constexpr float C5  = 0.3090169944f;   // cos(2*pi/5)
  constexpr float S5  = 0.9510565163f;   // sin(2*pi/5)
  // -sqrt(2) radians: COUNTER-rotation (frame-dragging)
  constexpr float CR2 = 0.1556174932f;   // cos(sqrt(2)) = cos(-sqrt(2))
  constexpr float SR2N = -0.9877659460f; // sin(-sqrt(2)) = -sin(sqrt(2))

  constexpr float PRE  = 1.1498f;  // moderate: controlled exponential growth
  constexpr float POST = 0.9586f;  // gentle contraction for stable throat

  Pattern p;
  p.name = "exponential_wormhole";
  p.inner = InnerFunction::Exp;          // conformal wormhole mapping
  p.reduction = ReductionType::Grid;     // visible throat rings

  p.zn.zn[0] = {C5 * PRE, S5 * PRE};
  p.zn.zn[1] = {0.0334f, -0.0380f};          // tiny imaginary offset (rotating wormhole)
  p.zn.zn[2] = {CR2 * POST, SR2N * POST};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.988f;                // high: deep spacetime persistence
  p.color.color_mode = as_int(ColorMode::Alive);               // Alive: gravitational redshift dynamics
  p.color.spiral_angle_coeff = 0.90f;   // throat angular structure
  p.color.spiral_radius_coeff = 0.60f;  // strong: concentric throat rings
  p.color.chroma_breath_amp = 0.16f;
  p.color.chroma_breath_freq = 0.11f;
  p.color.chroma_breath_phase = 0.30f;
  p.color.hue_velocity_base = 0.015f;
  p.color.hue_velocity_breath = 0.010f;
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.25f;     // strong: redshift luminance contours
  p.color.luma_shape_center = 2.2f;

  p.seed.border_width = 0.087f;         // minimal: deep space background
  p.seed.hue_speed = 0.06f;             // slow: cosmic timescale
  p.seed.alpha = 0.92f;
  p.speed = 0.46f;                      // very slow: relativistic time dilation
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.097f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.210f;
  p.zn.zn1_anim_amp = 0.037f;
  p.zn.zn1_anim_k = 5.0f;
  p.seed.pulse_freq = 0.088f;
  p.seed.pulse_amp = 0.14f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.085f,
                            .amplitude = 0.168f,
                            .offset = 0.600f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.118f,
                            .amplitude = 0.096f,
                            .offset = 0.160f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.069f,
                            .amplitude = 0.030f,
                            .offset = 0.060f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
