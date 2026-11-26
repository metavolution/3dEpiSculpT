#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Emile Picard's Great Theorem — Essential Singularity Star (z * sin(1/z)).
 *
 * Mathematical foundation:
 * - Picard's Great Theorem (1879) states that in every punctured neighborhood
 *   0 < |z| < eps of an essential singularity, a holomorphic function assumes
 *   every complex value (with at most one exception) infinitely many times.
 * - For f(z) = z * sin(1/z), the essential singularity of sin(1/z) at z = 0
 *   is regularised in amplitude along the real axis (|x * sin(1/x)| <= |x|,
 *   the topologist's Warsaw sine curve) while exploding exponentially along
 *   the imaginary axis sinh(1/y)/y, creating twin Stokes-sector starburst plumes!
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = FractalStar (z * sin(1/z), id 40) + Torus reduction.
 * - Pre-affine keeps |zn0 * z| ~ 0.85 inside the inner Stokes annuli while
 *   post-affine |zn2| = 1.56 expands the essential-singularity shells across
 *   the Torus mirror domain.
 * - Color = SpectralPrism (mode 20): RGB cosine dispersion resolves infinite
 *   accumulating phase rings near z = 0.
 */
inline Pattern picard_essential_singularity_star() {
  Pattern p;
  p.name = "picard_essential_singularity_star";
  p.inner = InnerFunction::FractalStar;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {0.82f, 0.28f};
  p.zn.zn[1] = {0.035f, -0.024f};
  p.zn.zn[2] = {1.56f, -0.34f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.13f;
  p.zn.zn0_anim_amp = 0.045f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.16f;
  p.zn.zn1_anim_amp = 0.052f;
  p.zn.zn1_anim_k = 5.0f;

  p.color.hue_rotation = 0.018f;
  p.color.sat_scale = 1.86f;
  p.color.fade = 0.990f;
  p.color.color_mode = as_int(ColorMode::SpectralPrism);
  p.color.spiral_angle_coeff = 1.0f;
  p.color.spiral_radius_coeff = 0.62f;
  p.color.chroma_breath_amp = 0.18f;
  p.color.chroma_breath_freq = 0.15f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.00f;   // R phase offset
  p.color.hue_velocity_breath = 0.33f; // G phase offset (120 deg)
  p.color.hue_velocity_drift = 0.67f;  // B phase offset (240 deg)
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.092f;
  p.seed.hue = 0.62f;
  p.seed.hue_speed = 0.12f;
  p.seed.alpha = 0.94f;
  p.seed.pulse_freq = 0.09f;
  p.seed.pulse_amp = 0.18f;

  p.speed = 0.85f;

  p.motion
      .add("stokes_sector_expansion",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.081f,
                            .amplitude = 0.085f,
                            .offset = 1.56f},
           control::ModTarget::Zn2Re)
      .add("essential_ring_dispersion",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.053f,
                            .amplitude = 0.22f,
                            .offset = 0.62f,
                            .phase = 0.25f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("prism_angular_twist",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.068f,
                            .amplitude = 0.35f,
                            .offset = 1.0f},
           control::ModTarget::SpiralAngleCoeff)
      .add("warsaw_chroma_pulse",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.097f,
                            .amplitude = 0.06f,
                            .offset = 0.18f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
