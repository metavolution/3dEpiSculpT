#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Op-art isoluminant vibration preset (perceptual color overload).
 *
 * Scientific basis:
 * - The human visual system has separate pathways for luminance (magnocellular, "where")
 *   and chrominance (parvocellular, "what").
 * - Isoluminance (constant brightness, varying hue) causes the motion and depth
 *   processing centers to fail while the color centers fire, producing a strong
 *   illusory vibration or "swimming" effect on static high-contrast patterns.
 *
 * Mapping to canonical pipeline:
 * transform -> reduce -> sample -> color(mode0 classic) -> seed -> blend
 */
inline Pattern op_art_isoluminant_vibration() {
  Pattern p;
  p.name = "op_art_isoluminant_vibration";
  p.inner = InnerFunction::Tan; // Tan creates sharp dense pole structures (stripes)
  p.reduction = ReductionType::Torus;

  // Scale and rotate to pack the stripes tightly for maximum visual noise.
  p.zn.zn[0] = {1.1206f, 0.2490f};
  p.zn.zn[1] = {0.0562f, -0.0387f};
  p.zn.zn[2] = {1.0655f, -0.3552f};
  p.zn.zn[3] = {0.0f, 0.0f};

  // Jump across the color wheel (e.g. Red <-> Cyan/Green) at high frequency
  p.color.hue_rotation = 0.5f; 
  p.color.sat_scale = 0.95f; // Perfect saturation preservation
  p.color.fade = 0.988f; // No fade! We want 100% luminance maintained.
  p.color.color_mode = presets::as_int(presets::ColorMode::IsoluminantVibrate); // Isoluminant Vibration mode
  
  // Use high-frequency spatial waves to drive the vibration
  p.color.spiral_angle_coeff = 0.5f; // Hue jump size (0.5 = opposite side of color wheel)
  p.color.spiral_radius_coeff = 10.0f; // High frequency log-r waves
  p.color.chroma_breath_freq = 25.0f; // High frequency u (horizontal) waves
  p.color.hue_velocity_base = 0.1f; // Fast time oscillation for the waves
  
  // Disable any luma shaping to keep luminance perfectly flat.
  p.color.luma_shape_depth = 0.0f; 
  p.color.chroma_breath_amp = 0.0f;

  p.seed.border_width = 0.082f;
  p.seed.hue_speed = 0.090f; // Static seed hue
  p.seed.hue = 0.0f; // Red
  p.seed.alpha = 0.92f; // Solid injection
  
  p.speed = 0.1f; // Slow movement allows the eye to "lock on" and the vibration to maximize
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.129f;
  p.zn.zn0_anim_amp = 0.038f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.166f;
  p.zn.zn1_anim_amp = 0.039f;
  p.zn.zn1_anim_k = 8.0f;
  p.seed.pulse_freq = 0.135f;
  p.seed.pulse_amp = 0.14f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.076f,
                            .amplitude = 2.800f,
                            .offset = 10.000f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.092f,
                            .amplitude = 0.050f,
                            .offset = 0.000f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.059f,
                            .amplitude = 0.032f,
                            .offset = 0.090f},
           control::ModTarget::SeedHueSpeed);


  
  return p;
}

} // namespace epi::presets
