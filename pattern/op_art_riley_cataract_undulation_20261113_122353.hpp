#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

/**
 * Op-Art: Bridget Riley "Cataract 3" (1967): Undulating Warm/Cool Bands.
 *
 * Scientific foundation:
 * - Riley's Cataract series stacks thin sinusoidal bands whose wavelength
 *   and amplitude vary smoothly down the canvas, painted in a warm (vermilion)
 *   and a cool (turquoise) grey that converge in value. The eye cannot hold
 *   both the fine band grating and the slow undulation: accommodation and
 *   vergence hunt, and the surface shimmers and swells. Near-isoluminant
 *   warm/cool pairs add chromatic vibration (weak luminance edge, strong
 *   colour edge).
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = F6 (1.3*cos(i*sin(z)) = 1.3*cosh(sin(z))): periodic along x with
 *   period pi and smooth: on the real axis it undulates between 1.3 and
 *   2.0; off the axis it becomes cos(sinh(y)), a chirped band grating. One
 *   function supplies both of Riley's ingredients: the wave and the bands
 *   whose frequency changes across the field.
 * - Post gain 0.78 places the critical value 1.3*0.78 = 1.01 on the seed
 *   border, so the crest line of the undulation is lit.
 * - Color = PaletteMorph: the kernel interpolates between a WARM and a COOL
 *   cosine palette, sin(2*pi*f*t), Riley's two-temperature scheme, applied to
 *   feedback luminance. chroma_breath_phase = 0 keeps the morph uniform.
 * - Motion: undulation amplitude (Zn2Re), band-frequency rings, warm/cool
 *   tempo, saturation swell.
 */
inline Pattern op_art_riley_cataract_undulation() {
  constexpr float A0 = 0.08726646f;  // 5 deg
  constexpr float A2 = -0.05235988f; // -3 deg
  constexpr float PRE = 1.15f;
  constexpr float POST = 0.78f; // 1.3 * 0.78 = 1.01: critical value on the rim
  const float zn2_re = POST * std::cos(A2);

  Pattern p;
  p.name = "op_art_riley_cataract_undulation";
  p.inner = InnerFunction::F6;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {PRE * std::cos(A0), PRE * std::sin(A0)};
  p.zn.zn[1] = {0.01f, 0.004f};
  p.zn.zn[2] = {zn2_re, POST * std::sin(A2)};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.09f;
  p.zn.zn1_anim_amp = 0.03f;
  p.zn.zn1_anim_k = 2.0f;

  p.color.hue_rotation = 0.012f;
  p.color.sat_scale = 1.86f;  // palette * luma * sat: ~unity mean gain
  p.color.fade = 0.989f;
  p.color.color_mode = as_int(ColorMode::PaletteMorph);
  p.color.spiral_angle_coeff = 1.0f;
  p.color.spiral_radius_coeff = 0.70f;
  p.color.chroma_breath_amp = 0.20f;
  p.color.chroma_breath_freq = 0.10f;  // warm <-> cool period: 10 s
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.04f;
  p.color.hue_velocity_breath = 0.18f;
  p.color.hue_velocity_drift = 0.38f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.09f;
  p.seed.hue = 0.55f;
  p.seed.hue_speed = 0.08f;
  p.seed.alpha = 0.93f;
  p.seed.pulse_freq = 0.07f;
  p.seed.pulse_amp = 0.14f;
  p.speed = 0.7f;

  p.motion
      .add("undulation_amplitude",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.042f,
                            .amplitude = 0.04f,
                            .offset = zn2_re},
           control::ModTarget::Zn2Re)
      .add("band_frequency_rings",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.055f,
                            .amplitude = 0.20f,
                            .offset = 0.70f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("warm_cool_tempo",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.03f,
                            .amplitude = 0.03f,
                            .offset = 0.10f},
           control::ModTarget::ChromaBreathFreq)
      .add("cataract_saturation",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.065f,
                            .amplitude = 0.08f,
                            .offset = 1.86f},
           control::ModTarget::SatScale);

  return p;
}

} // namespace epi::presets
