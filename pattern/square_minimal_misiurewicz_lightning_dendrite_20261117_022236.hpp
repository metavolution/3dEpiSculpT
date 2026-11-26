#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Square (Minimal 3.5D): the Misiurewicz Dendrite of c = i, a Turning Bolt
 * of Lightning.
 *
 * Mathematics:
 * - T(z) = 2 z^2 + i/2 is w -> w^2 + i in w = 2z. The critical orbit is
 *   0 -> i -> -1 + i -> -i -> -1 + i -> ...: it lands after two steps on a
 *   repelling 2-cycle (multiplier 4 (1 + i), modulus 5.66). c = i is a
 *   Misiurewicz point: the filled Julia set has no interior; it is a
 *   dendrite, a tree of arcs branching without end, like a lightning bolt.
 * - Exactness: K lies in |w| <= (1 + sqrt 5)/2, i.e. |z| <= 0.809, inside
 *   the Simplex band; border points (|w| >= 2) escape monotonically and stay
 *   clamped. Colour is exact escape time: terraces of equipotentials.
 * - Turntable: zn0 e^(-i psi), zn2 e^(i psi), zn3 e^(i psi) conjugates T by
 *   a rotation, so the Julia set turns rigidly by psi. The terraces near
 *   the set turn with it; near the square border they follow the square.
 *
 * Pipeline: Square, zn1 = 0; rest slots zn0 = 2, zn2 = 1/2, zn3 = i/2, all
 * three on quadrature lanes at 0.002 Hz (one turn in 500 s). Clamp; Simplex
 * border; OKLab step 1/7; fade 0.93.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Square / Clamp / Simplex / OKLabColorRotation
 * Nearest:    square_minimal_feigenbaum_point_edge_of_chaos (the other
 *             kind of interior-free Julia set: infinitely renormalizable,
 *             not preperiodic).
 * Height:     age = passes before exit.
 * Hue clock:  bounded; step 1/7.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; orbit and bound
 *             evaluated arithmetically. Not built, rendered or measured.
 * @assert sing[0].escapes == 0
 */
inline Pattern square_minimal_misiurewicz_lightning_dendrite() {
  constexpr float TURN_HZ = 0.002f;
  Pattern p;
  p.name = "square_minimal_misiurewicz_lightning_dendrite";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.5f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.5f}; // c/2 with c = i

  p.color.hue_rotation = 0.14285714f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.93f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.06f;
  p.seed.hue = 0.66f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("turntable_zn0_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 2.0f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn0Re)
      .add("turntable_zn0_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 2.0f,
                            .offset = 0.0f,
                            .phase = 0.5f},
           control::ModTarget::Zn0Im)
      .add("turntable_zn2_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 0.5f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Re)
      .add("turntable_zn2_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 0.5f,
                            .offset = 0.0f,
                            .phase = 0.0f},
           control::ModTarget::Zn2Im)
      .add("turntable_zn3_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 0.5f,
                            .offset = 0.0f,
                            .phase = 0.5f},
           control::ModTarget::Zn3Re)
      .add("turntable_zn3_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 0.5f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.025f,
                            .offset = 0.05f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
