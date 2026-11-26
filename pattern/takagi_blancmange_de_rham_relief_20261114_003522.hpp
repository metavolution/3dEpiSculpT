#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Recursive Memory: Takagi Blancmange, a de Rham Functional Equation Solved
 * by Feedback.
 *
 * Scientific foundation:
 * - Takagi (1903): tau(x) = sum_n 2^-n dist(2^n x, Z) is continuous and
 *   nowhere differentiable. The Takagi-Landsberg family replaces 2^-n by
 *   w^n; for 1/2 < w < 1 the graph has box dimension 2 + log2(w).
 * - De Rham (1957) characterised such functions by a self-referential
 *   equation f(x) = g(x) + r f(2x mod 1): the whole equals a smooth piece
 *   plus a scaled copy of itself read through the doubling map.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Any steady state of the loop satisfies
 *       I(z) = a(F z) S(F z) + (1 - a(F z)) * fade * color(I(F z)),
 *   which is a de Rham equation with g = a S, r = (1 - a) fade and F in the
 *   place of the doubling map. This preset chooses the three so that it is
 *   Takagi's.
 * - Inner = Identity, zn0 = 2, Reduce = Torus: the triangle fold of 2z is
 *   the tent map, the same map as in Takagi's sum.
 * - Seed = Simplex with border_width = 1. The border ramp
 *   max(0, (|x| + w - 1)/w) becomes |x| itself: the tent function
 *   dist(x, Z) up to scale, over the whole square.
 * - Fade is the Takagi-Landsberg weight w. With seed alpha 0.5 the ratio r
 *   runs from 0.4 to 0.8 across the cell, mostly above 1/2: the luminance is
 *   a continuous surface with no tangent plane anywhere, a blancmange in two
 *   variables.
 * - Color = DynamicsDriven: hue is a smooth function of |T| and position,
 *   deliberately featureless, so every crease in the picture belongs to the
 *   luminance.
 * - Motion: the Fade lane moves w between 0.68 and 0.92, so the roughness
 *   (the Holder exponent -log2 r) changes continuously; Zn3 slides the
 *   octaves across each other.
 */
inline Pattern takagi_blancmange_de_rham_relief() {
  constexpr float WEIGHT = 0.80f; // Takagi-Landsberg w

  Pattern p;
  p.name = "takagi_blancmange_de_rham_relief";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {2.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.55f;
  p.color.sat_scale = 1.05f;
  p.color.fade = WEIGHT;
  p.color.color_mode = as_int(ColorMode::DynamicsDriven);
  p.color.spiral_angle_coeff = 0.3f;   // hue across x
  p.color.spiral_radius_coeff = 30.0f; // hue per unit of 0.01 |T|
  p.color.chroma_breath_amp = 0.2f;
  p.color.chroma_breath_freq = 60.0f;  // tanh slope of the saturation edge
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.010f;
  p.color.hue_velocity_breath = 0.004f;
  p.color.hue_velocity_drift = 0.001f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.012f;  // radius of the saturation edge

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 1.0f; // the ramp fills the square: seed = tent
  p.seed.hue = 0.55f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.50f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("takagi_landsberg_weight",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.013f,
                            .amplitude = 0.12f,
                            .offset = WEIGHT},
           control::ModTarget::Fade)
      .add("octave_slide_x",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.009f,
                            .amplitude = 0.5f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("octave_slide_y",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0123f,
                            .amplitude = 0.5f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Im);

  return p;
}

} // namespace epi::presets
