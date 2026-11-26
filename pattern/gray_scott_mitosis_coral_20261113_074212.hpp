#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Pearson Gray-Scott Autocatalytic Mitosis & Coral Dendrite Morphogenesis.
 *
 * Scientific foundation:
 * - J. E. Pearson (1993, Science 261, "Complex Patterns in a Simple System"):
 *   in the autocatalytic reaction-diffusion system U + 2V -> 3V near feed/kill
 *   rates (F, k) = (0.037, 0.060), localized activator spots spontaneously
 *   pinch and divide like mitotic cells before elongating into space-filling
 *   abyssal coral labyrinths.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = TP0 (sin(z) + tanh(z)): couples short-wavelength Turing wave
 *   splitting (sin) with saturating bistable reaction fronts (tanh).
 * - Color = BioluminescentPulse (mode 18): deep-pelagic luciferin cyan/emerald/
 *   violet bursts peaking along high-curvature dividing membranes.
 */
inline Pattern gray_scott_mitosis_coral() {
  Pattern p;
  p.name = "gray_scott_mitosis_coral";
  p.inner = InnerFunction::TP0;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.26f, 0.19f};
  p.zn.zn[1] = {0.035f, -0.028f};
  p.zn.zn[2] = {1.14f, -0.11f};
  p.zn.zn[3] = {-0.018f, 0.022f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.105f;
  p.zn.zn0_anim_amp = 0.048f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.135f;
  p.zn.zn1_anim_amp = 0.062f;
  p.zn.zn1_anim_k = 4.0f;

  p.color.hue_rotation = 0.012f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::BioluminescentPulse);
  p.color.spiral_angle_coeff = 2.2f;
  p.color.spiral_radius_coeff = 1.35f;
  p.color.chroma_breath_amp = 0.30f;
  p.color.chroma_breath_freq = 0.22f;
  p.color.chroma_breath_phase = 0.18f;
  p.color.hue_velocity_base = 0.024f;
  p.color.hue_velocity_breath = 0.018f;
  p.color.hue_velocity_drift = 0.007f;
  p.color.luma_shape_depth = 0.22f;
  p.color.luma_shape_center = -4.20f;

  p.seed.type = SeedType::OrganicNoise;
  p.seed.border_width = 0.078f;
  p.seed.hue = 0.48f;
  p.seed.hue_speed = 0.11f;
  p.seed.alpha = 0.94f;
  p.seed.pulse_freq = 0.14f;
  p.seed.pulse_amp = 0.22f;

  p.speed = 0.88f;

  p.motion
      .add("feed_rate_dilation",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.074f,
                            .amplitude = 0.065f,
                            .offset = 1.14f},
           control::ModTarget::Zn2Re)
      .add("kill_rate_wavelength",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.119f,
                            .amplitude = 0.38f,
                            .offset = 1.35f,
                            .phase = 0.33f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("luciferin_flash",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.092f,
                            .amplitude = 0.09f,
                            .offset = 0.30f},
           control::ModTarget::ChromaBreathAmp)
      .add("membrane_seed_flux",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.058f,
                            .amplitude = 0.020f,
                            .offset = 0.078f},
           control::ModTarget::SeedWidth);

  return p;
}

} // namespace epi::presets
