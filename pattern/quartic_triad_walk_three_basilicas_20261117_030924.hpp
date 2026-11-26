#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Quartic F12 (Advanced 3.5D): One Circle through the Three Quartic
 * Basilicas.
 *
 * Mathematics:
 * - With the F12 inner 0.7 (u^4 - 1), zn0 = 1.4, zn2 = 1/(0.7 x 1.4) and
 *   zn3 = (1 + c)/1.4, T is w -> w^4 + c in w = 1.4 z.
 * - The quartic period-2 centres solve c^3 = -1 (critical orbit
 *   0 -> c -> c^4 + c = 0): c = -1, e^(i pi/3), e^(-i pi/3), all on the unit
 *   circle. Two quadrature lanes walk c = e^(i psi) round that circle,
 *   exactly, from c = -1 (the rest state, quartic_minimal_four_fold_
 *   basilica_turning) through e^(-i pi/3) at 333 s and e^(i pi/3) at 667 s.
 * - For alpha^3 = 1, alpha^-1 ((alpha w)^4 + c) = w^4 + c/alpha, so
 *   J(c alpha^-1) is J(c) turned: the three basilicas are one set at three
 *   angles, and the walk is the same act played three times, turned by a
 *   third each time. Between them it leaves the connectedness locus: at
 *   c = 1 the critical orbit escapes at the second step and the basilica
 *   has dissolved into dust.
 * - Exactness: |c| = 1, so K lies in |w| <= 1.2207, |z| <= 0.872, inside the
 *   Simplex band; the border (|w| >= 1.4) stays clamped.
 *
 * Pipeline: F12, zn0 = 1.4, zn1 = 0, zn2 = 1.0204; zn3 = (1 + e^(i psi))/1.4
 * on two lanes at 0.001 Hz; Clamp; Simplex border; fade 0.93; OKLab step
 * 1/8.
 *
 * Fidelity:   identity for every lane value
 * Cell:       F12 / Clamp / Simplex / OKLabColorRotation
 * Nearest:    square_period_three_triad_circle (a circle through three
 *             quadratic centres of different kinds); here the three are
 *             one shape, three times.
 * Height:     age = passes before exit.
 * Hue clock:  bounded; step 1/8 +- 0.005.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; centres and escape at
 *             c = 1 checked arithmetically. Which other components the circle
 *             crosses is not computed. Not built, rendered or measured.
 * @assert sing[0].escapes == 0 && sing[0].period == 2
 */
inline Pattern quartic_triad_walk_three_basilicas() {
  constexpr float WALK_HZ = 0.001f;
  constexpr float INV_S = 0.71428571f; // 1/1.4
  Pattern p;
  p.name = "quartic_triad_walk_three_basilicas";
  p.inner = InnerFunction::F12;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.4f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.02040816f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f}; // (1 + c)/1.4 at c = -1

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
  p.seed.hue = 0.3f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("triad_walk_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = WALK_HZ,
                            .amplitude = INV_S,
                            .offset = INV_S,
                            .phase = 0.75f}, // psi = pi: c = -1
           control::ModTarget::Zn3Re)
      .add("triad_walk_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = WALK_HZ,
                            .amplitude = INV_S,
                            .offset = 0.0f,
                            .phase = 0.5f},
           control::ModTarget::Zn3Im)
      .add("basilica_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
                            .amplitude = 0.005f,
                            .offset = 0.125f},
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
