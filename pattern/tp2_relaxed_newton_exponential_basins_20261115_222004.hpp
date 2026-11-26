#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Advanced 3.5D: Relaxed Newton's Method for exp(u) = 1.
 *
 * Mathematics:
 * - Newton's method for g(u) = exp(u) - 1 is N(u) = u - 1 + exp(-u). With a
 *   step length h it is u - h (1 - exp(-u)), and after the shift
 *   v = u - ln h it reads
 *       v -> v - h + exp(-v).
 *   This is the Fatou family v + w + exp(-v) at w = -h.
 * - The roots are v = -ln h + 2 pi i k, one for every integer k, each with
 *   multiplier 1 - h:
 *       0 < h < 1   damped, the approach is monotone;
 *       h = 1       Newton's method, the roots are superattracting;
 *       1 < h < 2   over-relaxed, the approach alternates sides;
 *       h = 2       multiplier -1, a period doubling (not reached here).
 * - Far to the right the map is the translation v - h: every orbit walks
 *   left at a fixed pace until it meets the row of roots. Each root owns a
 *   horizontal band. Between the bands, on the lines Im v = (2k + 1) pi,
 *   exp(-v) is real and negative and orbits run left without limit; those
 *   lines carry the hairs of the Julia set.
 * - The Julia set is the boundary of each basin, so every boundary point of
 *   one basin is a boundary point of all the others, an infinite family
 *   with one common frontier.
 *
 * Pipeline: TP2 with zn0 = -3 pi, zn1 = i pi, zn2 = -2 / (3 pi); the frame
 * coordinate is z = v / (3 pi). Roots at z = 0 and z = +-2i/3, hairs at
 * Im z = +-1/3 and along the top and bottom edges. zn3 = -h / (3 pi) + i/3,
 * so h is one lane on Zn3Re and nothing else moves. A second lane gives the
 * step a small imaginary part, which makes the multiplier complex and the
 * approach a spiral. The WaveRing seed is centred on the middle root (at
 * h = 1 exactly), so generation n shows the curves on which n steps bring
 * an orbit to a given distance from it. Left of about x = -0.24 the
 * exponential throws orbits out of the frame; Clamp pins them and that part
 * is outside the claim.
 *
 * Fidelity:   identity
 * Cell:       TP2 / Clamp / WaveRing / OKLab
 * Nearest:    tp2_minimal_fatou_baker_domain_river is the same family at
 *             w > 0, where the roots have gone to infinity and nothing
 *             attracts; here w < 0 and the subject is the basins.
 * Height:     age = number of steps taken, which grows linearly to the
 *             right at h / (3 pi) per step.
 * Hue clock:  bounded (hue_velocity_base = 0).
 * Fastest lane: 0.022 Hz
 * Status:     designed from source on 2026-11-15; not built, rendered or
 *             measured.
 * @assert sing[0].period == 1 && sing[0].rho_abs <= 0.9 && p_C >= 0.6
 */
inline Pattern tp2_relaxed_newton_exponential_basins() {
  Pattern p;
  p.name = "tp2_relaxed_newton_exponential_basins";
  p.inner = InnerFunction::TP2;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {-9.4247780f, 0.0f};        // -3 pi
  p.zn.zn[1] = {0.0f, 3.1415927f};         // i pi
  p.zn.zn[2] = {-0.21220659f, 0.0f};       // -2 / (3 pi)
  p.zn.zn[3] = {-0.12201880f, 0.33333333f}; // -h / (3 pi) + i / 3, h = 1.15

  p.color.hue_rotation = 0.06f;
  p.color.sat_scale = 0.97f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.08f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.6f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("newton_step_h", // h from 0.40 to 1.90
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.006f,
                            .amplitude = 0.07957747f,
                            .offset = -0.12201880f},
           control::ModTarget::Zn3Re)
      .add("spiral_detune",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0045f,
                            .amplitude = 0.012f,
                            .offset = 0.33333333f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.022f,
                            .amplitude = 0.03f,
                            .offset = 0.05f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
