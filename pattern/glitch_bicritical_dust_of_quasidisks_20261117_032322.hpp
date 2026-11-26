#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Glitch (Advanced 3.5D): a Bicritical Quartic; when its Free Critical
 * Point Escapes, the Julia Set Becomes a Dust of Quasidisks.
 *
 * Mathematics:
 * - The Glitch inner is g(u) = u^4 + 0.1 u^3 (inner_registry.hpp id 38):
 *   critical points u = 0 (double) and u = -0.075 (simple). In
 *   v = u / 0.075, g(0.075 v) = 0.075^3 x 0.0125 x (6 v^4 + 8 v^3). With
 *   zn0 = 0.075 s, zn1 = zn3 = 0, zn2 = a / (2 s 0.075^3 0.0125), s = 1.7,
 *   T is the family G_a(v) = a (3 v^4 + 4 v^3) in v = s z (checked against
 *   direct evaluation). Critical points v = 0 (local degree 3) and v = -1.
 * - 0 is a superattracting fixed point for every a: its basin is always
 *   there. The free critical value is G_a(-1) = -a. Bisection: the free
 *   orbit stays bounded for a < a* = 1.3691 and escapes for a > a*.
 * - a < a*: both critical orbits bounded, J connected.
 * - a > a*: one critical point escapes, the other is superattracting.
 *   Near the basin of 0, G is polynomial-like of degree 3 with one
 *   critical point, hybrid equivalent to v^3: the basin's component of K
 *   is a quasidisk. Every other component of K is a preimage of it (a
 *   quasidisk) or a point (Branner-Hubbard): K is a Cantor dust in which
 *   countably many dark discs float, nested by preimage depth.
 * - A Sine lane on zn2 (linear in a) moves a over [1, 2], crossing a* twice
 *   per cycle: a connected set shatters into a dust of moons and
 *   re-forms.
 * - Exactness: |G_a(v)| > |v| once a (3 r^4 - 4 r^3) > r; for a >= 1 that
 *   holds past r = 1.4846, so K lies in |z| <= 0.873, inside the Simplex
 *   band, and the border (|v| >= 1.7) stays clamped.
 *
 * Pipeline: Glitch; Clamp; Simplex border; fade 0.94; OKLab step 1/6; the
 * a-lane at 0.0015 Hz, rest at a = 1.5 (dust phase).
 *
 * Fidelity:   identity for every lane value
 * Cell:       Glitch / Clamp / Simplex / OKLabColorRotation
 * Nearest:    no other row has two independent critical orbits; the
 *             unicritical rows (square_*, cube_*, quartic_*) shatter all at
 *             once into point dust, this one keeps its discs.
 * Height:     age = passes before exit.
 * Hue clock:  bounded; step 1/6 +- 0.005.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; normalisation, a* and
 *             the escape bound computed arithmetically. Below a* whether the
 *             free point falls into the immediate basin or a preimage of it
 *             is not computed. zn2 is large (about 8e4), with g of order 1e-4:
 *             fine in float, but worth a look if the shader ever reorders the
 *             product. At rest (a = 1.5) one critical orbit escapes and the
 *             other is superattracting, so the census should see both an
 *             escaped and a contracting class; which critical point the
 *             census calls sing[0] is not known, so the assertion avoids it.
 *             Not built, rendered or measured.
 * @assert p_X >= 0.05 && p_C >= 0.05
 */
inline Pattern glitch_bicritical_dust_of_quasidisks() {
  constexpr float ZN2_PER_A = 55773.42048f; // 1 / (2 x 1.7 x 0.075^3 x 0.0125)
  Pattern p;
  p.name = "glitch_bicritical_dust_of_quasidisks";
  p.inner = InnerFunction::Glitch;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.1275f, 0.0f}; // 0.075 s
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.5f * ZN2_PER_A, 0.0f}; // a = 1.5
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.16666667f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.94f;
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
  p.seed.hue = 0.7f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("free_critical_escape",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0015f,
                            .amplitude = 0.5f * ZN2_PER_A,
                            .offset = 1.5f * ZN2_PER_A},
           control::ModTarget::Zn2Re)
      .add("moon_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
                            .amplitude = 0.005f,
                            .offset = 0.16666667f},
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
