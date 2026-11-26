#pragma once

#include "base.hpp"
#include <cmath>

namespace epi::presets {

/**
 * Feigenbaum cascade — period-doubling at the onset of chaos.
 *
 * MATHEMATICAL FOUNDATION:
 *
 * The logistic map x → r·x·(1−x) undergoes a period-doubling cascade
 * as r increases from 3 to r∞ ≈ 3.56995.  At each doubling threshold
 * the ratio of successive widths converges to the Feigenbaum constant
 * δ ≈ 4.6692 — a universal constant appearing in ALL one-dimensional
 * unimodal maps approaching chaos, regardless of their specific form.
 *
 * The complex-plane analog of the logistic map is f(z) = λz(1−z).
 * Under the substitution w = λ/2 − λz this becomes w → w² + c where
 * c = λ/2 − λ²/4.  So the Mandelbrot set IS the complex logistic map.
 *
 * At λ = 2 + √5 ≈ 4.236 (real), the logistic map is deeply chaotic.
 * The "edge of chaos" sits at r∞ ≈ 3.5699 → λ∞ ≈ 3.5699.
 *
 * We map this into the engine's affine framework:
 *   z → λ·z·(1−z) = λz − λz²
 *   As composed map: pre-affine scale = −λ, inner = Square, post = identity,
 *   post-translate = +λ/2 (re-centers the parabola).
 *
 * But the engine's flow is zn0·z + zn1 → inner → zn2·result + zn3.
 * For z² with c: set zn0 = (1,0), inner = Square, zn2 = (1,0),
 * zn3 = c where c corresponds to r∞ in the logistic family.
 * The Mandelbrot set value for r∞ is c∞ ≈ −1.401155 + 0i.
 * BUT: at exactly c∞ the orbit is marginally chaotic and the visual
 * pattern is maximally intricate — the "edge" where structure lives.
 *
 * This preset uses c just INSIDE the cascade at c = −1.38 + 0.02i
 * (slightly off the real axis for richer 2D structure), with a gentle
 * spiral rotation to continuously reveal new detail.
 */
inline Pattern feigenbaum_cascade() {
  Pattern p;
  p.name = "feigenbaum_cascade";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Torus;

  // Pre-affine: near-identity with very slow rotation
  constexpr float THETA = 0.015f;
  p.zn.zn[0] = {std::cos(THETA) * 0.98f, std::sin(THETA) * 0.98f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  // c = −1.38 + 0.02i  — just inside the period-doubling cascade,
  // slightly off the real axis for 2D spiral structure.
  p.zn.zn[3] = {-1.38f, 0.02f};

  p.color.hue_rotation = 0.009f;
  p.color.sat_scale = 1.012f;
  p.color.fade = 0.993f;  // Very slow decay — Feigenbaum structure persists

  // Mode 3 (alive): non-uniform hue velocity reveals period-doubling
  // bands through colour differentiation.
  p.color.color_mode = as_int(ColorMode::Alive);
  p.color.spiral_angle_coeff = 0.6f;
  p.color.spiral_radius_coeff = 0.35f;
  p.color.chroma_breath_amp = 0.10f;
  p.color.chroma_breath_freq = 0.07f;
  p.color.chroma_breath_phase = 0.35f;
  p.color.hue_velocity_base = 0.011f;
  p.color.hue_velocity_breath = 0.006f;
  p.color.hue_velocity_drift = 0.0018f;
  p.color.luma_shape_depth = 0.16f;
  p.color.luma_shape_center = 2.4f;

  p.seed.border_width = 0.045f;
  p.seed.hue_speed = 0.025f;
  p.seed.alpha = 0.82f;
  p.speed = 0.4f;
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.126f;
  p.zn.zn0_anim_amp = 0.046f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.158f;
  p.zn.zn1_anim_amp = 0.045f;
  p.zn.zn1_anim_k = 8.0f;
  p.seed.pulse_freq = 0.105f;
  p.seed.pulse_amp = 0.16f;
  p.motion
      .add("radial_pitch_wave",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.059f,
                            .amplitude = 0.120f,
                            .offset = 0.350f},
           control::ModTarget::SpiralRadiusCoeff)
      .add("chroma_diaphragm",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.071f,
                            .amplitude = 0.075f,
                            .offset = 0.100f,
                            .phase = 0.25f},
           control::ModTarget::ChromaBreathAmp)
      .add("seed_spectrum_tide",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.053f,
                            .amplitude = 0.030f,
                            .offset = 0.025f},
           control::ModTarget::SeedHueSpeed);


  return p;
}

} // namespace epi::presets
