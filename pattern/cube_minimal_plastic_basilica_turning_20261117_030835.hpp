#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Cube (Minimal 3.5D): the Cubic Basilica of w^3 + i, Bounded by the
 * Plastic Number.
 *
 * Mathematics:
 * - T(z) = (1.5 z)^3 / 1.5 + i/1.5, i.e. w -> w^3 + i in w = 1.5 z. The
 *   double critical point 0 is in a superattracting 2-cycle: 0 -> i ->
 *   -i + i = 0 (c^3 + c = 0 gives the cubic period-2 centres c = +-i).
 * - (omega w)^3 = w^3 for omega^3 = 1, so the Julia set is threefold
 *   symmetric: dark basins round 0 and i and their preimages, budding in
 *   threes.
 * - Escape: |w^3 + i| > |w| once |w|^3 - |w| - 1 > 0, i.e. |w| > 1.3247, the
 *   plastic number (the real root of x^3 = x + 1). So K lies in
 *   |z| <= 0.883, inside the Simplex band, and the border (|w| >= 1.5)
 *   stays clamped. Colour is exact escape time.
 * - Turntable: zn0 e^(-i psi), zn2 e^(i psi), zn3 e^(i psi) turns the Julia
 *   set rigidly, once in 500 s.
 *
 * Pipeline: Cube, zn0 = 1.5, zn1 = 0, zn2 = 1/1.5, zn3 = i/1.5; Clamp;
 * Simplex border; fade 0.93; OKLab step 1/6.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Cube / Clamp / Simplex / OKLabColorRotation
 * Nearest:    quartic_minimal_four_fold_basilica_turning (w^4 - 1, fourfold);
 *             cube_unicritical_multiplier_walk (the cubic's neutral fixed
 *             point, not a superattracting cycle).
 * Height:     age = passes before exit.
 * Hue clock:  bounded; step 1/6.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; orbit and escape radius
 *             evaluated arithmetically. Not built, rendered or measured.
 * @assert sing[0].escapes == 0 && sing[0].period == 2
 */
inline Pattern cube_minimal_plastic_basilica_turning() {
  constexpr float TURN_HZ = 0.002f;
  Pattern p;
  p.name = "cube_minimal_plastic_basilica_turning";
  p.inner = InnerFunction::Cube;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.5f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.66666667f, 0.0f}; // 1/1.5
  p.zn.zn[3] = {0.0f, 0.66666667f}; // i/1.5

  p.color.hue_rotation = 0.16666667f;
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
  p.seed.hue = 0.25f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("turntable_zn0_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 1.5f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn0Re)
      .add("turntable_zn0_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 1.5f,
                            .offset = 0.0f,
                            .phase = 0.5f},
           control::ModTarget::Zn0Im)
      .add("turntable_zn2_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 0.66666667f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Re)
      .add("turntable_zn2_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 0.66666667f,
                            .offset = 0.0f,
                            .phase = 0.0f},
           control::ModTarget::Zn2Im)
      .add("turntable_zn3_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 0.66666667f,
                            .offset = 0.0f,
                            .phase = 0.5f},
           control::ModTarget::Zn3Re)
      .add("turntable_zn3_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 0.66666667f,
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
