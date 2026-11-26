#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Bressloff-Cowan V1 Retinotopic Cortical Hallucination Form Constants.
 *
 * Scientific foundation:
 * - Bressloff, Cowan, Golubitsky, Thomas & Wiener (2001, Phil. Trans. R. Soc. B):
 *   spontaneous symmetry-breaking Turing instabilities in primary visual cortex
 *   (V1) orientation hypercolumns produce planar stripe/lattice eigenstates in
 *   cortical coordinates w = (u, v). Mapped back to retinal visual space via
 *   Schwartz's complex logarithmic retinotopy z = exp(w), cortical stripes
 *   become Kluver's four universal geometric form constants: tunnels, funnels,
 *   spirals, and cobwebs.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Exp (z -> e^z) + Grid wrap directly implements the periodic V1
 *   cortical sheet mapped through the exponential inverse retinotopic chart.
 * - Color = LogPolarColorSpiral (mode 11) + multi-parameter PatternMotion
 *   sweeping cortical orientation angle (SpiralAngleCoeff) and hypercolumn
 *   wavelength (SpiralRadiusCoeff) across all four Kluver form regimes.
 */
inline Pattern bressloff_cowan_visual_hallucination() {
  Pattern p;
  p.name = "bressloff_cowan_visual_hallucination";
  p.inner = InnerFunction::Exp;
  p.reduction = ReductionType::Grid;

  p.zn.zn[0] = {1.68f, 0.74f};
  p.zn.zn[1] = {0.14f, -0.09f};
  p.zn.zn[2] = {1.12f, -0.18f};
  p.zn.zn[3] = {0.02f, 0.03f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.095f;
  p.zn.zn0_anim_amp = 0.055f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.125f;
  p.zn.zn1_anim_amp = 0.068f;
  p.zn.zn1_anim_k = 8.0f;

  p.color.hue_rotation = 0.016f;
  p.color.sat_scale = 1.09f;
  p.color.fade = 0.985f;
  p.color.color_mode = as_int(ColorMode::LogPolarColorSpiral);
  p.color.spiral_angle_coeff = 3.0f;
  p.color.spiral_radius_coeff = 1.618f;
  p.color.chroma_breath_amp = 0.24f;
  p.color.chroma_breath_freq = 0.19f;
  p.color.chroma_breath_phase = 0.12f;
  p.color.hue_velocity_base = 0.028f;
  p.color.hue_velocity_breath = 0.016f;
  p.color.hue_velocity_drift = 0.006f;
  p.color.luma_shape_depth = 0.20f;
  p.color.luma_shape_center = -4.15f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.072f;
  p.seed.hue = 0.76f;
  p.seed.hue_speed = 0.14f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.11f;
  p.seed.pulse_amp = 0.18f;

  p.speed = 0.92f;

  p.motion
      .add("cortical_column_tilt",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.083f,
                            .amplitude = 0.85f,
                            .offset = 3.0f},
           control::ModTarget::SpiralAngleCoeff)
      .add("hypercolumn_pitch",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.051f,
                            .amplitude = 0.45f,
                            .offset = 1.618f,
                            .phase = 0.25f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("retinotopic_shear",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.067f,
                            .amplitude = 0.095f,
                            .offset = -0.18f},
           control::ModTarget::Zn2Im)
      .add("chroma_excitation",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.108f,
                            .amplitude = 0.075f,
                            .offset = 0.24f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
