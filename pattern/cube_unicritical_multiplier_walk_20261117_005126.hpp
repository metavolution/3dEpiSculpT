#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Cube (Advanced 3.5D): the Unicritical Cubic's Multiplier Walks the Unit
 * Circle.
 *
 * Mathematics:
 * - T(z) = zn2 (zn0 z + zn1)^3 + zn3 with zn1 = v0 = 1 / sqrt 3,
 *   zn2 = 1/2, zn3 = -v0^3 / 2. Then T(0) = 0, and the multiplier there is
 *   zn2 3 v0^2 zn0 = zn0 / 2: linear in zn0. With zn0 = 2 e^(2 pi i theta)
 *   the fixed point 0 is neutral with rotation number theta, and two lanes
 *   in quadrature move theta uniformly round the circle, exactly.
 * - In v = v0 + zn0 z the map is v -> e^(2 pi i theta) (v^3 - v0^3) + v0, a
 *   cubic with one double critical point (v = 0): a unicritical cubic,
 *   walking the rim of the main component of the cubic Multibrot set.
 * - At theta = p/q the fixed point is parabolic with q petals, one cycle of
 *   them, attracting the single critical point. At bounded-type theta it is
 *   the centre of a Siegel disc whose boundary is a quasicircle through the
 *   critical point (Zhang, 2011, for polynomials).
 * - |v'| >= |v|^3 - v0^3 - v0 exceeds |v| once |v| > 1.36, so the filled
 *   Julia set lies in |v| <= 1.36, i.e. |z| <= (1.36 + v0) / 2 = 0.97:
 *   inside the frame, and every escaping orbit reaches the border.
 *
 * Pipeline: Cube with the slots above; Clamp; Simplex border, so escaping
 * points are coloured by exit time in OKLab steps of 1/3; the filled Julia
 * set is dark. Lanes Zn0Re = 2 cos, Zn0Im = 2 sin, at 0.002 Hz (a full turn
 * in 500 s), starting at the golden mean, where the base slots rest.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Cube / Clamp / Simplex / OKLabColorRotation
 * Nearest:    spatial_wave_cardioid_multiplier_tour (quadratic);
 *             square_basilica_bulb_rim_resonance_walk (the period-2 rim);
 *             sin_multiplier_walk_rotation_number_dial (sine family). This
 *             is the cubic member, built with the zn0 construction.
 * Height:     age = passes before exit.
 * Hue clock:  bounded; step 1/3 +- 0.01.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; constants evaluated
 *             arithmetically. Not built, rendered or measured.
 * @assert sing[0].escapes == 0
 */
inline Pattern cube_unicritical_multiplier_walk() {
  constexpr float R = 2.0f;             // |zn0|: unit multiplier
  constexpr float GOLDEN = 0.38196601f; // rotation number at t = 0
  constexpr float WALK_HZ = 0.002f;
  Pattern p;
  p.name = "cube_unicritical_multiplier_walk";
  p.inner = InnerFunction::Cube;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {-1.4747378f, 1.3509806f}; // 2 e^(2 pi i g)
  p.zn.zn[1] = {0.57735027f, 0.0f};       // v0 = 1 / sqrt 3
  p.zn.zn[2] = {0.5f, 0.0f};
  p.zn.zn[3] = {-0.096225045f, 0.0f};     // -v0^3 / 2

  p.color.hue_rotation = 0.33333333f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.94f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0006f;
  p.color.hue_velocity_drift = 0.0002f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.06f;
  p.seed.hue = 0.12f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("multiplier_walk_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = WALK_HZ,
                            .amplitude = R,
                            .offset = 0.0f,
                            .phase = GOLDEN + 0.25f},
           control::ModTarget::Zn0Re)
      .add("multiplier_walk_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = WALK_HZ,
                            .amplitude = R,
                            .offset = 0.0f,
                            .phase = GOLDEN},
           control::ModTarget::Zn0Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.025f,
                            .offset = 0.05f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
