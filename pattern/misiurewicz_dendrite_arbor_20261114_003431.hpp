#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Growth by Inverse Iteration: Misiurewicz Dendrite Arbor.
 *
 * Scientific foundation:
 * - For c = i the critical orbit of Z -> Z^2 + c is 0 -> i -> -1 + i -> -i
 *   -> -1 + i: it lands on a repelling 2-cycle and never returns. c is a
 *   Misiurewicz point. The Julia set is then a dendrite: connected, with no
 *   interior and zero area, branching at a dense set of points. Every point
 *   off it escapes.
 * - A Julia set is the closure of the backward orbit of any of its points.
 *   Each point has two preimages, so the set grows as a binary tree, one
 *   generation of branches per inverse step, the same bifurcating rule as a
 *   neuronal arbor or a river network ordered by Horton and Strahler.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Square, zn2 = 1.7, zn3 = i / 1.7: Z -> Z^2 + i in the coordinate
 *   Z = 1.7 z.
 * - Reduce = Clamp, Seed = Simplex. After a reset, frame n holds the n-th
 *   preimage of the frame border. Each frame adds one generation of
 *   branches. The growth on screen is the actual inverse iteration, in the
 *   order the mathematics performs it, not a picture of its result.
 * - Because the dendrite has no interior the whole frame is lit; the exit
 *   bands crowd onto the tree and outline it at every scale.
 * - Color = Opponent rotation: one chroma step per generation, with a small
 *   log|T| term so each band is shaded by the escape potential.
 * - This is a growth process, but not morphogenesis in the reaction-
 *   diffusion sense: there is no coupling between neighbouring pixels. The
 *   pipeline has no Laplacian, and no preset can supply one.
 * - Motion: c moves +-0.02 across the Misiurewicz point. On one side the
 *   tree shatters into dust, on the other it thickens: a dendrite exists
 *   only at the exact parameter.
 */
inline Pattern misiurewicz_dendrite_arbor() {
  constexpr float C_IM = 0.58823529f; // 1 / 1.7

  Pattern p;
  p.name = "misiurewicz_dendrite_arbor";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.7f, 0.0f};
  p.zn.zn[3] = {0.0f, C_IM};

  p.color.hue_rotation = 0.07f;
  p.color.sat_scale = 1.02f;
  p.color.fade = 0.97f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.03f;
  p.color.chroma_breath_amp = 0.08f;
  p.color.chroma_breath_freq = 0.09f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.002f;
  p.color.hue_velocity_drift = 0.0006f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.30f;
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("misiurewicz_crossing",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.017f,
                            .amplitude = 0.012f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("generation_chroma_step",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.013f,
                            .amplitude = 0.025f,
                            .offset = 0.07f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
