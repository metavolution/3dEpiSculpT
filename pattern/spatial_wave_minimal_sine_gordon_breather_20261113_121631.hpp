#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Minimalist 5): Sine-Gordon Breather.
 *
 * Mathematical foundation:
 * - The sine-Gordon equation phi_tt - phi_xx + sin(phi) = 0 is integrable and
 *   carries kinks, antikinks and their bound state, the breather
 *   phi = 4*atan( sin(w*t) / (w' * cosh(x/w')) ): a localized lump that
 *   oscillates forever without radiating.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Sin: the restoring nonlinearity of the equation itself. With a
 *   purely real pre-scale k = 1.20, Re T = c*sin(kx)*cosh(ky) and
 *   Im T = c*cos(kx)*sinh(ky): the picture keeps exact x -> -x / y -> -y mirror
 *   symmetry (a clean, minimal composition).
 * - Gain c*k = 1.38 at the centre makes the origin repelling; on the real axis
 *   c*sin(k) = 1.07 > 1, so the left/right edges map straight onto the seed
 *   border and there is no interior attracting fixed point (no black basin).
 * - Color = SpatialWave with a 1:2 wave pair (fundamental + second harmonic).
 * - Motion: Zn2Re breathes the gain c, the breather's amplitude oscillation.
 */
inline Pattern spatial_wave_minimal_sine_gordon_breather() {
  Pattern p;
  p.name = "spatial_wave_minimal_sine_gordon_breather";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.20f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.15f, 0.03f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.12f;
  p.zn.zn1_anim_amp = 0.03f;
  p.zn.zn1_anim_k = 2.0f;

  p.color.hue_rotation = 0.007f;
  p.color.sat_scale = 1.06f;
  p.color.fade = 0.988f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.45f;
  p.color.spiral_radius_coeff = 0.5f;   // wave1: fundamental
  p.color.chroma_breath_amp = 0.16f;
  p.color.chroma_breath_freq = 1.0f;    // wave2: second harmonic
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.014f;
  p.color.hue_velocity_breath = -0.010f; // counter-propagating: kink vs antikink
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.14f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.07f;
  p.seed.hue = 0.58f;
  p.seed.hue_speed = 0.07f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.06f;
  p.seed.pulse_amp = 0.14f;
  p.speed = 0.66f;

  p.motion
      .add("breather_amplitude",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.05f,
                            .amplitude = 0.06f,
                            .offset = 1.15f},
           control::ModTarget::Zn2Re)
      .add("kink_antikink_pitch",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.037f,
                            .amplitude = 0.12f,
                            .offset = 0.5f},
           control::ModTarget::SpiralRadiusCoeff);

  return p;
}

} // namespace epi::presets
