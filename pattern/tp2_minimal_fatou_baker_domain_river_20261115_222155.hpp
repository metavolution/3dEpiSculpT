#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Minimal 3.5D: Fatou's River, a Baker Domain (u -> u + w - exp(-u)).
 *
 * Mathematics:
 * - Fatou (1926) gave f(u) = u + 1 + exp(-u) as the first map with an
 *   invariant region in which every orbit runs to infinity. Polynomials and
 *   rational maps cannot have one; it is now called a Baker domain. This
 *   preset uses the same map moved by i*pi, f(u) = u + w - exp(-u), so that
 *   one hair of the Julia set lies on the negative real axis.
 * - Far to the right exp(-u) is negligible and f is the translation u + w:
 *   a laminar river, one step of length w per pass. To the left exp(-u)
 *   takes over and the map explodes (a Cantor bouquet of hairs).
 * - With s = exp(-u) the map becomes s -> exp(-w) s exp(s), which has an
 *   attracting fixed point at s = 0 with multiplier exp(-w). The river is
 *   the logarithm of that sink.
 * - The only fixed points are u = -ln w + 2 pi i k, multiplier 1 + w,
 *   repelling for w > 0. The one with k = 0 is the tip of the central hair.
 *   The critical points u = i pi (2k + 1) lie on the calm lines at the top
 *   and bottom edges and run off to the right with everything else.
 *
 * Pipeline: TP2 is (v + exp v) / 2. With zn0 = -pi, zn2 = -2 / pi and
 * zn3 = w / pi the frame coordinate is z = u / pi and
 *   T(z) = z + w / pi - exp(-pi z) / pi.
 * The frame is one period of the map tall (Im u from -pi to pi). Clamp pins
 * an orbit when it reaches the right edge, where the Simplex border is lit.
 * The lanes move only zn3, which is w exactly: one lane changes the step,
 * the other gives w an imaginary part and tilts the river.
 *
 * Fidelity:   identity (clamped orbits are outside the claim)
 * Cell:       TP2 / Clamp / Simplex / DepthEcho
 * Nearest:    maxwell_fringing_field_capacitor uses TP2 once as a static
 *             conformal chart under Torus; here it is iterated as a
 *             dynamical system and the subject is the escaping domain.
 * Height:     age. In the laminar part the number of passes to the right
 *             edge is the integer part of a Fatou coordinate, so the strata
 *             have width w / pi (0.318 at w = 1).
 * Hue clock:  bounded (hue_velocity_base = 0).
 * Fastest lane: 0.023 Hz
 * Status:     designed from source on 2026-11-15; not built, rendered or
 *             measured.
 * @assert sing[0].escapes == 1 && p_X >= 0.9
 */
inline Pattern tp2_minimal_fatou_baker_domain_river() {
  Pattern p;
  p.name = "tp2_minimal_fatou_baker_domain_river";
  p.inner = InnerFunction::TP2;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {-3.1415927f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {-0.63661977f, 0.0f}; // -2 / pi
  p.zn.zn[3] = {0.31830989f, 0.0f};  // w / pi with w = 1

  p.color.hue_rotation = 0.04f;
  p.color.sat_scale = 0.9f;
  p.color.fade = 0.9f;
  p.color.color_mode = as_int(ColorMode::DepthEcho);
  p.color.spiral_angle_coeff = 0.12f; // hue per unit of age
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.2f; // saturation gained with age
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.03f;
  p.color.hue_velocity_drift = 0.0f; // 0 = hue from age only
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.06f;
  p.seed.hue = 0.55f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 0.9f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("river_step_w",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.011f,
                            .amplitude = 0.09f,
                            .offset = 0.31830989f},
           control::ModTarget::Zn3Re)
      .add("river_tilt",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.007f,
                            .amplitude = 0.05f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn3Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.023f,
                            .amplitude = 0.08f,
                            .offset = 0.2f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
