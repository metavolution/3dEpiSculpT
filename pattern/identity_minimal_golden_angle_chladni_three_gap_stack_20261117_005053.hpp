#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Minimal 3.5D): Chladni Plates Stacked at the Golden Angle, Hue
 * Locked to Orientation.
 *
 * Mathematics:
 * - T(z) = e^(2 pi i g) z with g = (3 - sqrt 5) / 2: a turn by the golden
 *   angle, 137.5 degrees. It never repeats. It maps the inscribed disc
 *   (78.5 % of the frame) onto itself exactly.
 * - The Chladni plate is fourfold symmetric in modulus, so generation n
 *   shows it at orientation 4 n g mod 1 (in quarter turns). By the three
 *   gap theorem (Steinhaus) the first N orientations cut the quarter turn
 *   into gaps of at most three lengths, and for the golden mean these are
 *   as even as possible at every N: the anti-resonant opposite of a
 *   rational turn, where the plates would stack onto q orientations.
 * - With an OKLab hue step of g turns, generation n has hue n g mod 1, and
 *   its orientation is 4 n g mod 1 = 4 x (its hue) mod 1. Hue fixes
 *   orientation exactly (four hues per orientation): though no orientation
 *   ever recurs, each plate's colour tells which way it lies.
 *
 * Pipeline: Identity with zn0 = e^(2 pi i g); Clamp (the corners land on the
 * border, where the plate is masked to zero); Chladni seed at alpha 0.5,
 * fade 0.95, so roughly the newest dozen plates are visible, each fainter
 * than the last; OKLab hue step g. The plate's own modes drift slowly.
 *
 * Fidelity:   identity
 * Cell:       Identity / Clamp / ChladniCymatics / OKLabColorRotation
 * Nearest:    identity_minimal_pentagonal_chladni_colour_symmetry (order 5,
 *             five orientations); here the turn is irrational and the
 *             orientations never close.
 * Height:     age = passes; generation n has hue n g.
 * Hue clock:  bounded; step g = 0.381966 +- 0.005.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. Not built, rendered or
 *             measured.
 * @assert p_N >= 0.75
 */
inline Pattern identity_minimal_golden_angle_chladni_three_gap_stack() {
  Pattern p;
  p.name = "identity_minimal_golden_angle_chladni_three_gap_stack";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {-0.73736888f, 0.67549029f}; // e^(2 pi i g), the golden angle
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.38196601f; // g: hue locked to orientation
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
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.1f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.5f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("golden_hue_breath",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.005f,
                            .amplitude = 0.005f,
                            .offset = 0.38196601f},
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
