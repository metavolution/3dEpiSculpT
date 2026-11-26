#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Tanh (Advanced 3.5D): the Family mu tanh u Walks its Multiplier round the
 * Circle, and a Chladni Plate Turns with It.
 *
 * Mathematics:
 * - T(z) = tanh(zn0 z) / s with zn0 = s mu, s = 2.5: conjugate (u = s mu z)
 *   to u -> mu tanh u. 0 is fixed with multiplier mu; two lanes in
 *   quadrature on zn0 put mu = e^(2 pi i theta) and walk theta uniformly
 *   round the circle, exactly (the zn0 construction).
 * - tanh has no critical points. Its singular values are the two
 *   asymptotic values +-1 (and here +-mu), approached along the real
 *   directions, and it has poles at u = i pi (k + 1/2). It is odd, so the
 *   two singular orbits are mirror images. As for every map of finite
 *   type, the boundary of a Siegel disc and every parabolic petal cycle are
 *   tied to the singular orbits: here they are tied to asymptotic values,
 *   not to critical points.
 * - Near 0, T is the rotation by theta to first order. The Chladni plate
 *   is fourfold symmetric in modulus, so generation n shows the plate
 *   turned by n theta mod a quarter turn: at theta = p/q the plates stack
 *   onto q / gcd(q, 4) orientations, between rationals they spread.
 *
 * Pipeline: Tanh with zn1 = zn3 = 0, zn2 = 1/s, zn0 = s mu on Sine lanes in
 * quadrature at 0.0025 Hz (a full turn in 400 s) starting at the golden
 * mean, where the base slots rest. The poles of T are at |z| = pi / (2 s) =
 * 0.63, inside the frame; points near them are thrown far out and clamped
 * onto the border. Chladni seed at alpha 0.6; OKLab hue step 0.08.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Tanh / Clamp / ChladniCymatics / OKLabColorRotation
 * Nearest:    sin_multiplier_walk_rotation_number_dial (critical points,
 *             Cross seed); cube_unicritical_multiplier_walk (a polynomial).
 *             Here the singular values are asymptotic and the seed is a
 *             plate.
 * Height:     age = passes; the orientation of a plate is its generation
 *             times theta.
 * Hue clock:  bounded; step 0.08 +- 0.01.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17. The assertion below is
 *             expected for the golden rest state (the singular orbits
 *             accumulate on the Siegel boundary) but not proved here. A
 *             plate of generation n turns n times as fast as mu. Not built,
 *             rendered or measured.
 * @assert sing[0].escapes == 0
 */
inline Pattern tanh_multiplier_walk_turning_chladni_plate() {
  constexpr float S = 2.5f;
  constexpr float GOLDEN = 0.38196601f;
  constexpr float WALK_HZ = 0.0025f;
  Pattern p;
  p.name = "tanh_multiplier_walk_turning_chladni_plate";
  p.inner = InnerFunction::Tanh;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {-1.8434222f, 1.6887257f}; // s e^(2 pi i g)
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.4f, 0.0f}; // 1 / s
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.08f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
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

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.62f;
  p.seed.hue_speed = 0.01f;
  p.seed.alpha = 0.6f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("multiplier_walk_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = WALK_HZ,
                            .amplitude = S,
                            .offset = 0.0f,
                            .phase = GOLDEN + 0.25f},
           control::ModTarget::Zn0Re)
      .add("multiplier_walk_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = WALK_HZ,
                            .amplitude = S,
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
