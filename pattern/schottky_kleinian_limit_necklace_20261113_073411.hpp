#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Schottky-Klein Circle Inversion Limit Necklace ("Indra's Pearls").
 *
 * Mathematical foundation:
 * - Friedrich Schottky (1877), Felix Klein, and Mumford-Series-Wright
 *   ("Indra's Pearls"): composing complex inversion z |-> 1/z with non-concentric
 *   affine shifts zn1, zn3 generates loxodromic Mobius transformations
 *   T(z) = zn2 / (zn0 * z + zn1) + zn3 whose recursive preimages condense onto
 *   chains of osculating hyperbolic pearls and quasi-Fuchsian Jordan curves.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Invert (1/z, id 35) + Torus reflection (providing 4 virtual mirror
 *   inversion circles around the fundamental square!).
 * - Off-origin zn1 = (0.34, -0.22) breaks trivial concentricity into a true
 *   loxodromic double-spiral necklace.
 * - Color = OKLabColorRotation (mode 17): perceptually uniform LMS cube-root
 *   OKLCH rotation preserves crisp gem-like specular highlights across scales.
 */
inline Pattern schottky_kleinian_limit_necklace() {
  Pattern p;
  p.name = "schottky_kleinian_limit_necklace";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.14f, 0.36f};
  p.zn.zn[1] = {0.34f, -0.22f};
  p.zn.zn[2] = {0.88f, 0.29f};
  p.zn.zn[3] = {-0.18f, 0.14f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.11f;
  p.zn.zn0_anim_amp = 0.048f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.14f;
  p.zn.zn1_anim_amp = 0.062f;
  p.zn.zn1_anim_k = 4.0f;

  p.color.hue_rotation = 0.016f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.984f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.85f;
  p.color.spiral_radius_coeff = 0.42f;
  p.color.chroma_breath_amp = 0.18f;
  p.color.chroma_breath_freq = 0.14f;
  p.color.chroma_breath_phase = 4.0f;
  p.color.hue_velocity_base = 0.015f;
  p.color.hue_velocity_breath = 0.010f;
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.088f;
  p.seed.hue = 0.78f;
  p.seed.hue_speed = 0.09f;
  p.seed.alpha = 0.94f;
  p.seed.pulse_freq = 0.07f;
  p.seed.pulse_amp = 0.16f;

  p.speed = 0.82f;

  p.motion
      .add("loxodromic_trace_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.069f,
                            .amplitude = 0.065f,
                            .offset = 0.88f},
           control::ModTarget::Zn2Re)
      .add("loxodromic_trace_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.043f,
                            .amplitude = 0.075f,
                            .offset = 0.29f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Im)
      .add("indra_pearl_oklab",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.058f,
                            .amplitude = 0.26f,
                            .offset = 0.85f},
           control::ModTarget::SpiralAngleCoeff)
      .add("necklace_hue_glide",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.037f,
                            .amplitude = 0.008f,
                            .offset = 0.016f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
