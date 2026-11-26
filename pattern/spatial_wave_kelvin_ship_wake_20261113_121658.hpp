#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Spatial Wave (Advanced 5): Kelvin Ship-Wake Dispersion Wedge.
 *
 * Scientific foundation:
 * - Lord Kelvin (1887): deep-water gravity waves obey omega^2 = g*k, so the
 *   group velocity is exactly half the phase velocity. Stationary phase then
 *   confines every ship wake to a wedge of half-angle asin(1/3) = 19.47 deg,
 *   independent of speed, and the crests at the cusp line make
 *   atan(1/sqrt(2)) = 35.26 deg with the track.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Sinh: sinh(k*y) is the exact depth profile of a linear water wave;
 *   its antisymmetric stretch splits the field into port/starboard arms.
 * - Pre-affine rotated by the Kelvin angle, post-affine by minus the cusp
 *   angle: the two universal constants of the wake are the two rotations of
 *   the feedback map. |zn0|*|zn2| = 1.40 keeps the pullback expanding.
 * - Color = SpatialWave carrying the dispersion relation: wave2 has 4x the
 *   wavenumber of wave1 (omega ~ sqrt(k): doubling omega quadruples k) and
 *   half its time speed (group = phase / 2).
 * - Motion: Froude-number wavelength sweep, transverse/divergent mix, hull yaw.
 */
inline Pattern spatial_wave_kelvin_ship_wake() {
  constexpr float KELVIN = 0.33983691f; // asin(1/3)
  constexpr float CUSP = 0.61547971f;   // atan(1/sqrt(2))
  constexpr float PRE = 1.30f;
  constexpr float POST = 1.08f;

  Pattern p;
  p.name = "spatial_wave_kelvin_ship_wake";
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {PRE * std::cos(KELVIN), PRE * std::sin(KELVIN)};
  p.zn.zn[1] = {0.03f, -0.02f};
  p.zn.zn[2] = {POST * std::cos(-CUSP), POST * std::sin(-CUSP)};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.12f;
  p.zn.zn0_anim_amp = 0.04f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.15f;
  p.zn.zn1_anim_amp = 0.045f;
  p.zn.zn1_anim_k = 3.0f;

  p.color.hue_rotation = 0.012f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.985f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.66f;
  p.color.spiral_radius_coeff = 0.5f;   // wave1 wavenumber k1
  p.color.chroma_breath_amp = 0.18f;
  p.color.chroma_breath_freq = 2.0f;    // wave2 wavenumber k2 = 4*k1
  p.color.chroma_breath_phase = 0.10f;
  p.color.hue_velocity_base = 0.024f;   // phase speed
  p.color.hue_velocity_breath = 0.012f; // group speed = phase / 2
  p.color.hue_velocity_drift = 0.006f;
  p.color.luma_shape_depth = 0.16f;
  p.color.luma_shape_center = 3.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.085f;
  p.seed.hue = 0.55f;
  p.seed.hue_speed = 0.09f;
  p.seed.alpha = 0.93f;
  p.seed.pulse_freq = 0.08f;
  p.seed.pulse_amp = 0.16f;
  p.speed = 0.85f;

  p.motion
      .add("froude_wavelength",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.058f,
                            .amplitude = 0.14f,
                            .offset = 0.5f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("transverse_divergent_mix",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.071f,
                            .amplitude = 0.05f,
                            .offset = 0.10f},
           control::ModTarget::ChromaBreathPhase)
      .add("hull_yaw",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.049f,
                            .amplitude = 0.04f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Im)
      .add("wake_hue_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.037f,
                            .amplitude = 0.006f,
                            .offset = 0.012f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
