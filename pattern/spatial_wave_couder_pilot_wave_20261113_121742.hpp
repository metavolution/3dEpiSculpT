#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Spatial Wave (Advanced 8): Couder-Fort Walking-Droplet Pilot Wave.
 *
 * Scientific foundation:
 * - Couder & Fort (2005-2006): an oil droplet bouncing on a vertically shaken
 *   bath just below the Faraday threshold is propelled by the standing waves
 *   left by its own previous bounces. The wave field stores the trajectory
 *   ("path memory", parameter Me) and guides the walker; the system shows
 *   single-particle diffraction, orbit quantization and tunnelling analogues.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = TP5 (z*cos(z)): distance times an oscillation, the radial
 *   standing-wave footprint (Bessel-like) each bounce leaves on the bath.
 * - The feedback recurrence IS path memory: each frame re-reads the field
 *   written by all earlier frames. Fade plays the memory parameter Me and is
 *   itself modulated (short memory = plain walker, long memory = quantized).
 * - zn1 rose-curve animation moves the wave source: it is the droplet walking.
 * - Seed pulse at 0.12 Hz is the subharmonic Faraday bounce.
 * - Color = SpatialWave + HorizontalWave with golden wave pair, the standing
 *   Faraday pattern drifting under the walker.
 */
inline Pattern spatial_wave_couder_pilot_wave() {
  constexpr float A0 = 0.27925268f;  // 16 deg
  constexpr float A2 = -0.31415927f; // -18 deg
  constexpr float PRE = 1.42f;
  constexpr float POST = 1.10f;

  Pattern p;
  p.name = "spatial_wave_couder_pilot_wave";
  p.inner = InnerFunction::TP5;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {PRE * std::cos(A0), PRE * std::sin(A0)};
  p.zn.zn[1] = {0.03f, -0.02f};
  p.zn.zn[2] = {POST * std::cos(A2), POST * std::sin(A2)};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.10f;
  p.zn.zn0_anim_amp = 0.035f;
  p.zn.zn1_animate = true;       // the walker
  p.zn.zn1_anim_speed = 0.17f;
  p.zn.zn1_anim_amp = 0.06f;
  p.zn.zn1_anim_k = 3.0f;

  p.color.hue_rotation = 0.011f;
  p.color.sat_scale = 1.07f;
  p.color.fade = 0.987f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spatial_mode = ColorSpatialMode::HorizontalWave;
  p.color.spiral_angle_coeff = 0.60f;
  p.color.spiral_radius_coeff = 0.809f;  // phi/2
  p.color.chroma_breath_amp = 0.17f;
  p.color.chroma_breath_freq = 1.309f;   // phi^2/2
  p.color.chroma_breath_phase = 0.10f;
  p.color.hue_velocity_base = 0.019f;
  p.color.hue_velocity_breath = 0.013f;
  p.color.hue_velocity_drift = 0.005f;
  p.color.luma_shape_depth = 0.15f;
  p.color.luma_shape_center = 2.618f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.08f;
  p.seed.hue = 0.47f;
  p.seed.hue_speed = 0.095f;
  p.seed.alpha = 0.93f;
  p.seed.pulse_freq = 0.12f;
  p.seed.pulse_amp = 0.20f;
  p.speed = 0.88f;

  p.motion
      .add("path_memory_parameter",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.033f,
                            .amplitude = 0.003f,
                            .offset = 0.987f},
           control::ModTarget::Fade)
      .add("faraday_wavelength",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.057f,
                            .amplitude = 0.18f,
                            .offset = 0.809f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("walker_hue_slope",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.044f,
                            .amplitude = 0.13f,
                            .offset = 0.60f},
           control::ModTarget::SpiralAngleCoeff);

  return p;
}

} // namespace epi::presets
