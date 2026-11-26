#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Lord Rayleigh's Whispering-Gallery Microresonator Caustic (z * cos(z)).
 *
 * Scientific foundation:
 * - Lord Rayleigh (1910, Phil. Mag. "The problem of the whispering gallery"):
 *   acoustic waves in St. Paul's Cathedral dome and optical whispering-gallery
 *   modes (WGMs) in high-Q silica microtoroids are governed by high-order Bessel
 *   functions J_m(k*r) * exp(i*m*theta), trapping wave amplitude in a luminous
 *   annular gallery outside an inner turning-point caustic r_c = m / k!
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = TP5 (z * cos(z), id 18): radial growth |z| weights periodic wave
 *   crests cos(z), concentrating energy outward toward the rim caustic while
 *   leaving a quiet evanescent inner eye!
 * - Color = PhantomFractal (mode 16): quantizes azimuth into 12 whispering-gallery
 *   angular sectors (m = 12) pulsing with radial log_r wave resonances.
 */
inline Pattern bessel_whispering_gallery_caustic() {
  Pattern p;
  p.name = "bessel_whispering_gallery_caustic";
  p.inner = InnerFunction::TP5;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.46f, 0.42f};
  p.zn.zn[1] = {0.02f, -0.015f};
  p.zn.zn[2] = {1.08f, -0.31f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.14f;
  p.zn.zn0_anim_amp = 0.044f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.18f;
  p.zn.zn1_anim_amp = 0.048f;
  p.zn.zn1_anim_k = 6.0f;

  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.980f;
  p.color.color_mode = as_int(ColorMode::PhantomFractal);
  p.color.spiral_angle_coeff = 2.0f;
  p.color.spiral_radius_coeff = 0.72f;
  p.color.chroma_breath_amp = 0.24f;
  p.color.chroma_breath_freq = 0.16f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.025f;
  p.color.hue_velocity_breath = 0.012f;
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.14f; // subtle WGM radial brightness pulse
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.06f;
  p.seed.hue = 0.12f;
  p.seed.hue_speed = 0.11f;
  p.seed.alpha = 0.93f;
  p.seed.pulse_freq = 0.10f;
  p.seed.pulse_amp = 0.18f;

  p.speed = 0.90f;

  p.motion
      .add("wgm_azimuthal_mode_m",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.078f,
                            .amplitude = 0.55f,
                            .offset = 2.0f},
           control::ModTarget::SpiralAngleCoeff)
      .add("caustic_turning_radius",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.049f,
                            .amplitude = 0.24f,
                            .offset = 0.72f,
                            .phase = 0.25f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("microtoroid_coupling_detune",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.064f,
                            .amplitude = 0.085f,
                            .offset = -0.31f},
           control::ModTarget::Zn2Im)
      .add("evanescent_rim_glow",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.092f,
                            .amplitude = 0.06f,
                            .offset = 0.14f},
           control::ModTarget::LumaShapeDepth);

  return p;
}

} // namespace epi::presets
