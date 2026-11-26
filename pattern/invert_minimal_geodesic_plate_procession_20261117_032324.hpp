#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Minimal 3.5D): a Procession of Plates along a Geodesic of the
 * Poincare Disc.
 *
 * Mathematics:
 * - T(z) = (z + t)/(1 + t z), t = 1/2: the hyperbolic translation by
 *   d = 2 artanh(1/2) = ln 3 along the real diameter, from the rim point -1
 *   to +1. Slots zn0 = t, zn1 = 1, zn2 = t - 1/t, zn3 = 1/t.
 * - Exact: T keeps the disc and its outside, the Chladni seed is zero for
 *   r >= 1, and Clamp only moves outside points onto the border (r >= 1).
 * - Generation n is the plate moved by T^-n, a translation by n ln 3
 *   toward -1. Its centre is at -tanh(n d / 2) = -(3^n - 1)/(3^n + 1):
 *   -1/2, -4/5, -13/14, -40/41, ...; its Euclidean size shrinks about
 *   threefold per step, while in the hyperbolic metric every copy is the
 *   same size, spaced by ln 3: a procession of equals marching into the
 *   rim, as in Escher's Circle Limit prints.
 * - Turntable: zn0 e^(-i psi), zn2 e^(i psi), zn3 e^(i psi) turns the
 *   geodesic about the centre, once in 500 s; the plate stays put.
 *
 * Pipeline: Invert; Clamp; Chladni at alpha 0.6; fade 0.93; OKLab step 1/7,
 * one hue per hyperbolic step.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Invert / Clamp / ChladniCymatics / OKLabColorRotation
 * Nearest:    invert_disc_isometry_trace_walk_three_classes (passes through
 *             this map at alpha = 0, moving on at once); here it is held.
 * Height:     age = passes = hyperbolic distance / ln 3.
 * Hue clock:  bounded; step 1/7.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; slots and lane phases
 *             as checked for E7i. Every orbit tends to the rim point +1
 *             (multiplier 1/3 there), so the contracting class is expected
 *             to dominate; not proved for a finite census horizon. Not built,
 *             rendered or measured.
 * @assert p_C >= 0.5
 */
inline Pattern invert_minimal_geodesic_plate_procession() {
  constexpr float TURN_HZ = 0.002f;
  Pattern p;
  p.name = "invert_minimal_geodesic_plate_procession";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.5f, 0.0f};  // t
  p.zn.zn[1] = {1.0f, 0.0f};
  p.zn.zn[2] = {-1.5f, 0.0f}; // t - 1/t
  p.zn.zn[3] = {2.0f, 0.0f};  // 1/t

  p.color.hue_rotation = 0.14285714f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.93f;
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
  p.seed.hue = 0.12f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.6f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("turntable_zn0_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 0.5f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn0Re)
      .add("turntable_zn0_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 0.5f,
                            .offset = 0.0f,
                            .phase = 0.5f},
           control::ModTarget::Zn0Im)
      .add("turntable_zn2_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 1.5f,
                            .offset = 0.0f,
                            .phase = 0.75f},
           control::ModTarget::Zn2Re)
      .add("turntable_zn2_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 1.5f,
                            .offset = 0.0f,
                            .phase = 0.5f},
           control::ModTarget::Zn2Im)
      .add("turntable_zn3_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 2.0f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Re)
      .add("turntable_zn3_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = TURN_HZ,
                            .amplitude = 2.0f,
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
