#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Nonlinear Dynamics: A Postcritically Finite Rational Map and its
 * Superattracting Two-Cycle.
 *
 * Scientific foundation:
 * - R(z) = (z^2 - 1)/(z^2 + 1) is the Cayley transform applied to z^2. Its
 *   two critical points are 0 and infinity, and
 *       0 -> -1 -> 0,        infinity -> 1 -> 0.
 *   So 0 and -1 form a superattracting cycle of period 2 and the second
 *   critical point falls onto it after two steps. Every critical orbit is
 *   finite: the map is hyperbolic and postcritically finite, its Fatou set
 *   is the basin of that single cycle, and every Fatou component is an
 *   eventual preimage of the two immediate basins.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = MagneticDipole with zn0 = zn2 = 1: T is R itself.
 * - The cycle point -1 lies exactly on the left edge of the frame, inside
 *   the Simplex seed band. The attractor sits on the seed, so the interior
 *   of every Fatou component is lit, at the step its orbit first comes
 *   within the band width of -1. Nothing here relies on escape.
 * - Reduce = Clamp. Points sent far to the left are pinned to (-1, 0) and
 *   points sent far to the right to (1, 0), which R maps to 0: the clamp
 *   agrees with the true dynamics at both ends of the real axis.
 * - Color = OKLab rotation with hue step 1/2. The two points of the cycle
 *   alternate, so components are two-coloured by the parity of their arrival
 *   time, in complementary hues, and Fade = 0.90 darkens them by depth.
 * - Motion: a small real shift of zn3 moves the map off the postcritically
 *   finite parameter; the cycle stays attracting and stays inside the band.
 */
inline Pattern cayley_rational_superattracting_two_cycle() {
  Pattern p;
  p.name = "cayley_rational_superattracting_two_cycle";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.5f; // parity colouring
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.90f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.08f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f; // bounded hue clock: step stays near 1/2
  p.color.hue_velocity_breath = 0.0006f;
  p.color.hue_velocity_drift = 0.0002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.09f;
  p.seed.hue = 0.60f;
  p.seed.hue_speed = 0.04f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("leave_postcritical_parameter",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.019f,
                            .amplitude = 0.03f,
                            .offset = 0.0f},
           control::ModTarget::Zn3Re)
      .add("parity_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.013f,
                            .amplitude = 0.04f,
                            .offset = 0.5f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
