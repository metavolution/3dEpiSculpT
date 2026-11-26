#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * MagneticDipole (Minimal 3.5D): Boole's Map in the Disc, Everything Creeps
 * into One Parabolic Point.
 *
 * Mathematics:
 * - With zn0 = 1/sqrt 3, zn2 = 4/3, zn3 = 5/3 the dipole inner gives
 *   B(z) = (3 z^2 + 1)/(z^2 + 3), a degree-2 Blaschke product. Through the
 *   Cayley map phi(w) = (w - i)/(w + i) it is exactly Boole's map
 *   w -> w - 1/w on the upper half-plane (checked: B(phi(w)) = phi(w - 1/w)
 *   with zero error). B(1) = 1 with B'(1) = 1: a parabolic point on the rim.
 * - In the half-plane Im(w - 1/w) = Im w (1 + 1/|w|^2) > Im w: every orbit
 *   climbs to infinity, i.e. every point of the disc creeps into z = 1,
 *   slowly (parabolically) and tangentially. Boole (1857): the map keeps
 *   Lebesgue measure on the line, an infinite invariant measure; on the
 *   circle its density is 1/|1 - z|^2, piled up at the parabolic point.
 * - Exactness: B keeps the disc and its outside, and the Chladni seed is
 *   zero for r >= 1 (as for every disc-preserving row). Generation n is the
 *   plate pulled back by B^n: 2^n copies, lining the whole rim (the Julia
 *   set is the circle) and bunching toward z = 1.
 * - Turntable: zn0 e^(-i psi), zn2 e^(i psi), zn3 e^(i psi) carries the
 *   parabolic point round the rim (to e^(i psi)) in 500 s.
 *
 * Pipeline: MagneticDipole, zn1 = 0; Clamp; Chladni at alpha 0.6; fade 0.95;
 * OKLab step 1/12.
 *
 * Fidelity:   identity for every lane value
 * Cell:       MagneticDipole / Clamp / ChladniCymatics / OKLabColorRotation
 * Nearest:    magnetic_dipole_blaschke_rim_shatters_into_cantor_dust (k =
 *             0.6, rotated); this is k = -1/3, the parabolic member, held.
 * Height:     age = passes; generation n has 2^n copies.
 * Hue clock:  bounded; step 1/12.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; conjugacy, B'(1) and
 *             lane phases evaluated arithmetically. Not built, rendered or
 *             measured.
 * @assert sing[0].escapes == 0 && p_E <= 0.05
 */
inline Pattern magnetic_dipole_minimal_boole_parabolic_rim() {
  constexpr float TURN_HZ = 0.002f;
  Pattern p;
  p.name = "magnetic_dipole_minimal_boole_parabolic_rim";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.57735027f, 0.0f}; // 1/sqrt 3
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.33333333f, 0.0f}; // 4/3
  p.zn.zn[3] = {1.66666667f, 0.0f}; // 5/3

  p.color.hue_rotation = 0.08333333f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.04f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.08f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.6f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("turntable_zn0_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 0.57735027f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn0Re)
      .add("turntable_zn0_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 0.57735027f,
                            .offset = 0.0f,
                            .phase = 0.5f},
           control::ModTarget::Zn0Im)
      .add("turntable_zn2_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 1.33333333f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Re)
      .add("turntable_zn2_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 1.33333333f,
                            .offset = 0.0f,
                            .phase = 0.0f},
           control::ModTarget::Zn2Im)
      .add("turntable_zn3_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 1.66666667f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Re)
      .add("turntable_zn3_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 1.66666667f,
                            .offset = 0.0f,
                            .phase = 0.0f},
           control::ModTarget::Zn3Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.02f,
                            .offset = 0.04f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
