#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Quartic F12 (Minimal 3.5D): the Four-fold Basilica of w^4 - 1, Turning.
 *
 * Mathematics:
 * - The F12 inner is 0.7 (u - 1)(u + 1)(u - i)(u + i) = 0.7 (u^4 - 1)
 *   (inner_registry.hpp id 32). With zn0 = 1.4, zn2 = 1/(0.7 x 1.4),
 *   zn1 = zn3 = 0: T(z) = ((1.4 z)^4 - 1)/1.4, i.e. w -> w^4 - 1 in w = 1.4 z.
 * - The critical point 0 (of multiplicity 3) is in a superattracting
 *   2-cycle 0 -> -1 -> 0: the quartic basilica. Since (i w)^4 = w^4 the
 *   Julia set has the square's own symmetry, a quarter turn: dark basins
 *   about 0 and -1 and their preimages, budding in fours.
 * - Exactness: |w^4 - 1| > |w| once |w| > 1.2207 (root of r^4 - r - 1), so
 *   K lies in |z| <= 0.872, inside the Simplex band; the border has
 *   |w| >= 1.4 and stays clamped. Colour is exact escape time.
 * - Turntable: zn0 e^(-i psi), zn2 e^(i psi) (zn3 = 0 stays 0) turns the
 *   Julia set rigidly; after a quarter turn (125 s) it is itself again,
 *   while the square border's terraces are not.
 *
 * Pipeline: F12; Clamp; Simplex border; OKLab step 1/8; fade 0.93; four
 * turntable lanes, one turn in 500 s.
 *
 * Fidelity:   identity for every lane value
 * Cell:       F12 / Clamp / Simplex / OKLabColorRotation
 * Nearest:    quadratic basilica rows (w^2 - 1, twofold); here fourfold,
 *             matching the frame.
 * Height:     age = passes before exit.
 * Hue clock:  bounded; step 1/8.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; inner formula read from
 *             inner_registry.hpp, escape radius evaluated arithmetically.
 *             Not built, rendered or measured.
 * @assert sing[0].escapes == 0 && sing[0].period == 2
 */
inline Pattern quartic_minimal_four_fold_basilica_turning() {
  constexpr float TURN_HZ = 0.002f;
  Pattern p;
  p.name = "quartic_minimal_four_fold_basilica_turning";
  p.inner = InnerFunction::F12;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.4f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.02040816f, 0.0f}; // 1/(0.7 x 1.4)
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.125f;
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
  p.seed.hue = 0.78f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("turntable_zn0_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 1.4f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn0Re)
      .add("turntable_zn0_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 1.4f,
                            .offset = 0.0f,
                            .phase = 0.5f},
           control::ModTarget::Zn0Im)
      .add("turntable_zn2_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 1.02040816f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Re)
      .add("turntable_zn2_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 1.02040816f,
                            .offset = 0.0f,
                            .phase = 0.0f},
           control::ModTarget::Zn2Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.025f,
                            .offset = 0.05f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
