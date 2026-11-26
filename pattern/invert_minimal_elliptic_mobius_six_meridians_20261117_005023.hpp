#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Minimal 3.5D): a Sixth Turn of the Riemann Sphere, Drawn as
 * Meridians.
 *
 * Mathematics:
 * - Let p = 0.85 i and h(z) = (z - p) / (z + p). The Mobius map M with
 *   h(M z) = e^(i pi / 3) h(z) fixes the two poles +-p and turns about them
 *   by 60 degrees: under stereographic projection it is a rigid rotation of
 *   the sphere by a sixth of a turn, so M^6 = id exactly.
 * - Written in the slots: zn0 = 1 - L, zn1 = p (1 + L), zn2 = -4 L p^2 /
 *   (1 - L), zn3 = p (1 + L) / (1 - L), L = e^(i pi / 3). The pole of M and
 *   the image of infinity are the real points +-p cot(pi/6) = +-1.47,
 *   outside the frame.
 * - Circles through both poles (meridians) are permuted, turned by 60
 *   degrees each pass; circles of Apollonius about the poles (parallels)
 *   are each mapped onto themselves.
 *
 * Pipeline: Invert with the slots above; Clamp; Cross seed; OKLab hue step
 * 1/6. The imaginary axis passes through both poles, so it is a meridian:
 * its images are three circles through +-p meeting there at 60 degrees,
 * six half-meridians, the n-th in hue n/6, so every full circle has two
 * complementary halves. The real axis is the parallel halfway between the
 * poles, so it is mapped onto itself and stays lit. Frame points whose
 * image leaves the square are clamped onto the border.
 *
 * Fidelity:   identity
 * Cell:       Invert / Clamp / Cross / OKLabColorRotation
 * Nearest:    spatial_wave_riemann_sphere_golden_rotation turns the sphere
 *             by an irrational angle; spatial_wave_mobius_horocycle_bouquet
 *             is parabolic. Here the rotation has order 6 and the seed is
 *             chosen to draw meridians.
 * Height:     age = passes; the hue of a meridian is its generation.
 * Hue clock:  bounded; step 1/6 +- 0.01.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. The slot values were
 *             checked arithmetically: M(+-p) = +-p and M^6 = id to 2e-15,
 *             M'(p) = e^(i pi / 3). Not built, rendered or measured.
 * @assert p_C <= 0.05
 */
inline Pattern invert_minimal_elliptic_mobius_six_meridians() {
  Pattern p;
  p.name = "invert_minimal_elliptic_mobius_six_meridians";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.5f, -0.86602540f};       // 1 - L
  p.zn.zn[1] = {-0.73612159f, 1.275f};     // p (1 + L)
  p.zn.zn[2] = {-1.445f, 2.50281342f};     // -4 L p^2 / (1 - L)
  p.zn.zn[3] = {-1.47224319f, 0.0f};       // p (1 + L) / (1 - L)

  p.color.hue_rotation = 0.16666667f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0006f;
  p.color.hue_velocity_drift = 0.0002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.52f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("meridian_hue_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.006f,
                            .amplitude = 0.01f,
                            .offset = 0.16666667f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.025f,
                            .offset = 0.05f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
