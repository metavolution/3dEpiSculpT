#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Growth by Inverse Iteration: Radial Cleavage and the Angle-Doubling Map.
 *
 * Scientific foundation:
 * - z -> z^2 doubles the argument: on the circle it is the Bernoulli shift
 *   theta -> 2 theta, the model chaotic map. Its Julia set is the circle;
 *   inside, orbits fall to 0, outside they escape. The Bottcher coordinate
 *   is the identity here, so internal and external rays are straight, and
 *   the rays that land after n steps on a fixed pair of axes are the dyadic
 *   angles k pi / 2^(n+1).
 * - Radial holoblastic cleavage (sea urchin, amphibian) divides an egg along
 *   meridional planes into 2, 4, 8, 16 ... blastomeres. Seen from the pole
 *   the furrows are spokes whose number doubles each division. The doubling
 *   law is shared; the mechanics of a cell cortex is not modelled.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = Square with zn2 = 1.25: T(z) = 1.25 z^2, Julia circle of radius
 *   0.8.
 * - Seed = Cross, the two coordinate axes, as an orbit trap. Generation n
 *   lights the points whose n-th image lies on an axis: 2^(n+2) spokes.
 *   Inside the circle each generation also falls further toward the centre,
 *   where the two arms overlap and the trap is solid, so generation n fills
 *   an annulus ending at radius (0.06 / 1.25^(2^n - 1))^(1/2^n). The disc is
 *   a stack of rings carrying 4, 8, 16, 32 ... spokes, crowding onto the
 *   circle.
 * - Reduce = Clamp. Outside the circle orbits are pinned to the border and
 *   continue along it; they are lit when they pass an axis end, so the
 *   exterior carries the external rays at dyadic angles.
 * - Color = Alive: the hue advance per pass is uniform in space, so each
 *   division is tinted as a whole.
 * - Motion: c = zn3 circles the origin at radius 0.04. The Julia circle
 *   becomes a quasi-circle and the spokes bend into its true rays.
 */
inline Pattern radial_cleavage_angle_doubling_rays() {
  Pattern p;
  p.name = "radial_cleavage_angle_doubling_rays";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.25f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0f; // unused by Alive
  p.color.sat_scale = 1.06f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.004f;
  p.color.hue_velocity_breath = 0.002f;
  p.color.hue_velocity_drift = 0.001f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.08f;
  p.seed.hue = 0.08f;
  p.seed.hue_speed = 0.05f;
  p.seed.alpha = 0.95f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("quasi_circle_parameter_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.015f,
                            .amplitude = 0.04f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("quasi_circle_parameter_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.015f,
                            .amplitude = 0.04f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Im)
      .add("furrow_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.011f,
                            .amplitude = 0.12f,
                            .offset = 0.08f},
           control::ModTarget::SeedHue);

  return p;
}

} // namespace epi::presets
