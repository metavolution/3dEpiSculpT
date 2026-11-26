#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Op-Art: Josef Albers "Homage to the Square": Interaction of Colour.
 *
 * Scientific foundation:
 * - Albers (Interaction of Color, 1963) painted over a thousand Homages:
 *   three or four nested squares, shifted toward the bottom edge, in flat
 *   unmodulated colour. The same pigment reads as two different colours
 *   against two grounds (simultaneous contrast), and neighbouring squares
 *   seem to advance, recede or glow at their shared edge. The format is
 *   deliberately minimal so that only colour relations act.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Identity with a REAL gain 1.22 and no rotation: the recurrence is
 *   I_{n+1}(z) = I_n(1.22*z + b), an exact nest of axis-aligned squares, each
 *   1/1.22 the size of the last.
 * - zn3 = (0, 0.055): the nest converges to the affine fixed point
 *   b/(1 - a) = -0.25 instead of the centre, Albers' bottom-weighted,
 *   gravity-bearing layout.
 * - Seed width 0.18 = 1 - 1/1.22: each generation's border band exactly
 *   meets the next, so the frame is tiled by contiguous square bands.
 * - Color = OKLab rotation with zero spatial coefficients: every pass turns
 *   the hue by one constant step in a perceptually uniform space and scales
 *   chroma by sat_scale. Band n is the seed colour after n equal perceptual
 *   steps, a controlled Albers colour progression. With hue velocities zero
 *   the kernel's fallback adds time*0.01, so the step interval widens slowly
 *   and the chord of colours evolves (analogous -> triadic -> complementary).
 */
inline Pattern op_art_albers_homage_nested_square() {
  Pattern p;
  p.name = "op_art_albers_homage_nested_square";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.22f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.055f};

  p.color.hue_rotation = 0.07f;   // hue step per nested square
  p.color.sat_scale = 1.04f;
  p.color.fade = 0.978f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;   // flat colour: no angular term
  p.color.spiral_radius_coeff = 0.0f;  // flat colour: no radial term
  p.color.chroma_breath_amp = 0.12f;
  p.color.chroma_breath_freq = 0.07f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.18f;
  p.seed.hue = 0.08f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.95f;
  p.seed.pulse_freq = 0.04f;
  p.seed.pulse_amp = 0.08f;
  p.speed = 0.6f;

  p.motion
      .add("gravity_offset",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.03f,
                            .amplitude = 0.03f,
                            .offset = 0.055f},
           control::ModTarget::Zn3Im)
      .add("interaction_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.021f,
                            .amplitude = 0.035f,
                            .offset = 0.07f},
           control::ModTarget::HueRot)
      .add("nesting_ratio",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.027f,
                            .amplitude = 0.03f,
                            .offset = 1.0f},
           control::ModTarget::Zn2Re);

  return p;
}

} // namespace epi::presets
