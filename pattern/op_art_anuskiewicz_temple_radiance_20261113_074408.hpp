#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Richard Anuszkiewicz "Temple Radiance" Simultaneous Contrast Portals.
 *
 * Perceptual color science & op-art foundation:
 * - Richard Anuszkiewicz (Temple of Golden / Radiant Green, 1965-1978) & Josef
 *   Albers (Interaction of Color, 1963): nested geometric sanctuary portals
 *   lined with thin high-chroma complementary pinstripes trigger Von Bezold
 *   chromatic assimilation, causing identical ground regions to glow with
 *   illusory vermilion, turquoise, and aureolin light.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = F12 (0.5*(z^4 - z)) + PareidoliaCathedral dihedral mirror folding:
 *   sculpts nested 4-fold/8-fold architectural temple colonnades and vaults!
 * - Color = OKLabColorRotation (mode 17): rotates hue in Bjorn Ottosson's
 *   perceptually uniform OKLab (L, a, b) space so chroma transitions never
 *   suffer Helmholtz-Kohlrausch luminance collapse.
 */
inline Pattern op_art_anuskiewicz_temple_radiance() {
  Pattern p;
  p.name = "op_art_anuskiewicz_temple_radiance";
  p.inner = InnerFunction::F12;
  p.reduction = ReductionType::PareidoliaCathedral;

  p.zn.zn[0] = {1.36f, 0.12f};
  p.zn.zn[1] = {0.038f, 0.024f};
  p.zn.zn[2] = {1.14f, -0.07f};
  p.zn.zn[3] = {0.0f, -0.018f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.084f;
  p.zn.zn0_anim_amp = 0.038f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.106f;
  p.zn.zn1_anim_amp = 0.048f;
  p.zn.zn1_anim_k = 4.0f;

  p.color.hue_rotation = 0.016f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.986f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 2.5f;
  p.color.spiral_radius_coeff = 1.42f;
  p.color.chroma_breath_amp = 0.27f;
  p.color.chroma_breath_freq = 0.17f;
  p.color.chroma_breath_phase = 0.20f;
  p.color.hue_velocity_base = 0.025f;
  p.color.hue_velocity_breath = 0.015f;
  p.color.hue_velocity_drift = 0.006f;
  p.color.luma_shape_depth = 0.23f;
  p.color.luma_shape_center = -4.24f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.072f;
  p.seed.hue = 0.02f;
  p.seed.hue_speed = 0.11f;
  p.seed.alpha = 0.93f;
  p.seed.pulse_freq = 0.10f;
  p.seed.pulse_amp = 0.18f;

  p.speed = 0.86f;

  p.motion
      .add("sanctuary_portal_zoom",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.062f,
                            .amplitude = 0.062f,
                            .offset = 1.14f},
           control::ModTarget::Zn2Re)
      .add("bezold_pinstripe_pitch",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.098f,
                            .amplitude = 0.60f,
                            .offset = 2.5f},
           control::ModTarget::SpiralAngleCoeff)
      .add("oklab_chroma_radiance",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.077f,
                            .amplitude = 0.08f,
                            .offset = 0.27f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("complementary_hue_glide",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.045f,
                            .amplitude = 0.009f,
                            .offset = 0.016f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
