#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Advanced 3.5D: A Rose Window from z^3, with a Gear Train of Symmetries.
 *
 * Mathematics:
 * - The Chladni seed |cos(n x) cos(m y) - cos(m x) cos(n y)| has the
 *   symmetry of the square, order 4 in rotation, for every n and m.
 * - Let T(z) = e^{ia} z^3. A rotation r leaves seed(T(z)) unchanged exactly
 *   when r^3 is a symmetry of the seed, that is when r^12 = 1. Generation n
 *   is seed(T^n(z)) with T^n(z) = e^{i a (3^n - 1) / 2} z^(3^n), so its
 *   rotational order is 4 * 3^n: 4, 12, 36, 108 and so on.
 * - Inside the unit circle |T^n(z)| = |z|^(3^n), so generation n shows the
 *   plate only in a ring just inside the circle, and the rings narrow as n
 *   grows: the plate in the middle, twelvefold tracery round it,
 *   thirty-six-fold round that, a fine rim. Outside the circle orbits
 *   escape; Clamp pins them where the drum mask of the seed is zero, so the
 *   window stands on black.
 * - Turning a turns tier n by -a (3^n - 1) / (2 * 3^n): the ratios are
 *   1/3, 4/9, 13/27, 40/81, with limit 1/2. The tiers turn against one
 *   another like a gear train while each keeps its own order exactly.
 *
 * Pipeline: Cube with zn2 = e^{ia}. Two Sine lanes of amplitude 1 in
 * quadrature drive Zn2Re and Zn2Im, so zn2 stays on the unit circle and a
 * makes one turn in 250 s. The seed animates n and m by itself.
 *
 * Fidelity:   identity
 * Cell:       Cube / Clamp / ChladniCymatics / Opponent
 * Nearest:    radial_cleavage_angle_doubling_rays stacks 4, 8, 16 spokes
 *             from z^2 and the Cross seed, with no relative rotation; here
 *             the orders are 4 * 3^n from a plate with square symmetry and
 *             the subject is the differential rotation of the tiers.
 * Height:     age = tier index n; tier n lies near radius 0.5^(1 / 3^n).
 * Hue clock:  bounded (hue_velocity_base = 0).
 * Fastest lane: 0.024 Hz
 * Status:     designed from source on 2026-11-15; not built, rendered or
 *             measured.
 * @assert winding == 3 && sing[0].period == 1 && p_C >= 0.75
 */
inline Pattern cube_chladni_rose_window_gear_train() {
  Pattern p;
  p.name = "cube_chladni_rose_window_gear_train";
  p.inner = InnerFunction::Cube;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.11f;
  p.color.sat_scale = 0.97f;
  p.color.fade = 0.92f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.06f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.03f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.1f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 0.75f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("gear_angle_cos",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 1.0f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Re)
      .add("gear_angle_sin",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 1.0f,
                            .offset = 0.0f},
           control::ModTarget::Zn2Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.024f,
                            .amplitude = 0.03f,
                            .offset = 0.06f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
