#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Minimal 3.5D): Two Chladni Plates at 45 Degrees, Sum in Light
 * and Difference in Colour.
 *
 * Mathematics:
 * - T(z) = e^(i pi / 4) z, a rotation of order 8, exact on the inscribed
 *   disc (78.5 % of the frame).
 * - The Chladni plate is fourfold symmetric in modulus (a quarter turn
 *   only flips the sign inside the bars), so the generations alternate
 *   between exactly two plates: the plate itself (even passes) and the
 *   plate turned by 45 degrees (odd passes). Two square patterns at 45
 *   degrees make an eightfold moire, the pattern behind the Ammann-Beenker
 *   tiling.
 * - Opponent colour is a rotation in the chroma plane, linear in RGB. With
 *   a step of exactly 1/2 it reverses chroma on every pass, so with
 *   rho = fade (1 - seed alpha) the settled chroma is proportional to
 *   rho P0 - P45 and the light to rho P0 + P45 (P0 and P45 the two
 *   plates), up to the [0, 1] clamp. Where the plates agree the picture is
 *   bright and grey; where only one has sand it takes that plate's hue or
 *   the opposite one; where neither has sand it is dark.
 *
 * Pipeline: Identity with zn0 = e^(i pi / 4); Clamp (the corners land on the
 * border, where the plate is masked to zero); Chladni seed, hue 0.58 (blue),
 * so the two plates come up blue and its opponent, amber; Opponent with step
 * 1/2. The plate's modes drift, so the moire and its colouring morph
 * slowly.
 *
 * Fidelity:   identity
 * Cell:       Identity / Clamp / ChladniCymatics / Opponent
 * Nearest:    identity_minimal_pentagonal_chladni_colour_symmetry (five
 *             orientations, one hue each); magnetic_dipole_octagonal_
 *             complementary_lakes uses the same chroma reversal on a
 *             2-cycle of a rational map.
 * Height:     age = passes; parity of the pass is the colour.
 * Hue clock:  bounded; step 1/2 +- 0.01.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; the sum and difference
 *             form is the geometric series of the linear update. Not built,
 *             rendered or measured.
 * @assert p_N >= 0.75
 */
inline Pattern identity_minimal_octagonal_chladni_opponent_moire() {
  Pattern p;
  p.name = "identity_minimal_octagonal_chladni_opponent_moire";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.70710678f, 0.70710678f}; // e^(i pi / 4)
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.5f; // exact chroma reversal: colour = difference
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.04f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0004f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.58f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.5f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("opposition_breath",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.005f,
                            .amplitude = 0.01f,
                            .offset = 0.5f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.02f,
                            .offset = 0.04f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
