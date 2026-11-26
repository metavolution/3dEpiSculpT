#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Minimal 3.5D): a Five-fold Rosette of the Poincare Disc whose
 * Centre Orbits.
 *
 * Mathematics:
 * - T is the elliptic Mobius map with fixed points a = 0.45 and 1/a and
 *   multiplier e^(2 pi i / 5) at a: in zeta = (z - a)/(z - 1/a) it is
 *   zeta -> e^(2 pi i / 5) zeta. Its fixed points are mirror images in the
 *   unit circle, so T maps the unit disc onto itself: a hyperbolic rotation
 *   of the Poincare disc by a fifth of a turn about a, with T^5 = id.
 * - Exactness: the Chladni seed is masked to the disc (zero for r >= 1).
 *   T keeps the inside and the outside of the circle, and Clamp only moves
 *   outside points onto the square's border, where r >= 1. So every orbit
 *   from outside the disc sees seed zero forever, every orbit from inside
 *   stays inside without being clamped, and the picture is exact.
 * - Generation n is the plate moved by the isometry T^-n. Five copies of
 *   one plate, isometric in the hyperbolic metric, overlap: the copies on
 *   the rim side of a look compressed, as in Escher's Circle Limit prints,
 *   but are congruent. With hue step 1/5 the colour of a copy is its
 *   element of the group Z/5.
 * - Turntable: T_psi(z) = e^(i psi) T(e^(-i psi) z) rotates zn0 by -psi and
 *   zn2, zn3 by +psi, with zn1 fixed. It is the same rotation about
 *   e^(i psi) a, so six quadrature lanes carry the centre round the circle
 *   |a| = 0.45 exactly, while the plate stays put.
 *
 * Pipeline: Invert with the slots below; Clamp; Chladni at alpha 0.5;
 * fade 0.97 so the five copies weigh within 12 % of each other; OKLab step
 * 1/5. The orbit of the centre takes 500 s.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Invert / Clamp / ChladniCymatics / OKLabColorRotation
 * Nearest:    identity_minimal_pentagonal_chladni_colour_symmetry is the
 *             Euclidean rotation about 0; this is its hyperbolic version
 *             about a moving centre.
 * Height:     age = passes; the five generations are the five copies.
 * Hue clock:  bounded; step 1/5.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; slots and lane phases
 *             evaluated arithmetically (T(a) = a and T^5 = id to 4e-16, the
 *             unit circle kept to 1e-15). Not built, rendered or measured.
 * @assert p_N >= 0.75
 */
inline Pattern invert_minimal_poincare_pentagonal_rosette_orbit() {
  constexpr float ORBIT_HZ = 0.002f;
  Pattern p;
  p.name = "invert_minimal_poincare_pentagonal_rosette_orbit";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.69098301f, -0.95105652f}; // 1 - L, L = e^(2 pi i / 5)
  p.zn.zn[1] = {-2.08316457f, 0.42797543f}; // L a - 1/a
  p.zn.zn[2] = {1.57038580f, -2.16145063f};
  p.zn.zn[3] = {1.33611111f, -1.21962731f}; // T(infinity)

  p.color.hue_rotation = 0.2f; // colour = element of Z/5
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.97f;
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
  p.seed.hue = 0.55f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.5f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  // Turntable: zn0 e^(-i psi), zn2 e^(i psi), zn3 e^(i psi).
  p.motion
      .add("turntable_zn0_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 1.17557050f,
                            .offset = 0.0f,
                            .phase = 0.4f},
           control::ModTarget::Zn0Re)
      .add("turntable_zn0_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 1.17557050f,
                            .offset = 0.0f,
                            .phase = 0.65f},
           control::ModTarget::Zn0Im)
      .add("turntable_zn2_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 2.67169990f,
                            .offset = 0.0f,
                            .phase = 0.1f},
           control::ModTarget::Zn2Re)
      .add("turntable_zn2_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 2.67169990f,
                            .offset = 0.0f,
                            .phase = 0.85f},
           control::ModTarget::Zn2Im)
      .add("turntable_zn3_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 1.80905602f,
                            .offset = 0.0f,
                            .phase = 0.13224884f},
           control::ModTarget::Zn3Re)
      .add("turntable_zn3_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = ORBIT_HZ,
                            .amplitude = 1.80905602f,
                            .offset = 0.0f,
                            .phase = 0.88224884f},
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
