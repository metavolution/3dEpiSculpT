#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Advanced 3.5D): the Pillowcase; Borsuk-Ulam Forces a Vortex at
 * Each of its Four Corners.
 *
 * Mathematics:
 * - T(z) = -z + c on the Grid torus, c = 0.6 + 0.2i: a half turn, exact
 *   (-1 is a Gaussian unit), T^2 = id. Its fixed points on the torus solve
 *   2z = c mod 2Z^2: z = c/2 + {0, 1, i, 1 + i}, i.e. (0.3, 0.1),
 *   (-0.7, 0.1), (0.3, -0.9), (-0.7, -0.9). The quotient of the torus by T is
 *   the pillowcase sphere, with these four as its corners (the orbifold of
 *   the Lattes maps, and of the Weierstrass p-function).
 * - Opponent with step 1/2 on the 2-cycle: with equal weights the chroma is
 *   C = S - S o T, so C(T z) = -C(z). At a corner z*, C(z*) = -C(z*) = 0,
 *   and C is odd about z*: an odd map from a small circle to C \ {0} has
 *   odd degree (Borsuk-Ulam). So every corner carries a vortex of odd
 *   charge, never zero; the seed decides only the signs.
 * - The Chladni seed is periodic and continuous, so C is continuous on the
 *   torus and its charges sum to 0 (Poincare-Hopf). Other zeros come in
 *   T-pairs of equal charge, so the four corners' charges sum to an even
 *   number: +1 +1 -1 -1 when there are no others.
 * - A lane moves c round a circle; the corners move with c/2, rigidly, over
 *   the fixed plates: the colour field changes everywhere, the four
 *   vortices stay.
 *
 * Pipeline: Identity, zn0 = -1, zn2 = 1, zn3 = c on quadrature lanes
 * (|c| = 0.632, 333 s); Grid; Chladni at alpha 0.12; fade 0.99; Opponent
 * step 1/2.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Identity / Grid / ChladniCymatics / Opponent
 * Nearest:    identity_p4_orbifold_vortex_antivortex_crystal (the quarter
 *             turn, where the order-4 charges are fixed at -1); here the half
 *             turn, where only the parity is fixed, by Borsuk-Ulam.
 * Height:     age = passes; the two generations are the two group
 *             elements.
 * Hue clock:  bounded; step 1/2 +- 0.004.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; corners evaluated
 *             arithmetically. With rho = fade (1 - alpha I) < 1,
 *             C(z*) = S(z*)(1 - rho)/(1 - rho^2) is small, not zero: each core
 *             is displaced by order 1 - rho, its odd charge kept. A Chladni
 *             nodal kink crossing a corner leaves the charge momentarily
 *             undefined. Not built, rendered or measured.
 * @assert p_N >= 0.95
 */
inline Pattern vortex_evolved_v2() {
  constexpr float SLIDE_HZ = 0.003f;
  constexpr float C_ABS = 0.63245553f;
  Pattern p;
  p.name = "vortex_evolved_v2";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = { 1.01f, 0.05f }; // Mutated
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.6f, 0.2f};  // c; corners at c/2 + {0, 1, i, 1 + i}

  p.color.hue_rotation = 0.5f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.99f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.03f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.1f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.12f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("pillow_slide_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SLIDE_HZ,
                            .amplitude = C_ABS,
                            .offset = 0.0f,
                            .phase = 0.30120819f},
           control::ModTarget::Zn3Re)
      .add("pillow_slide_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SLIDE_HZ,
                            .amplitude = C_ABS,
                            .offset = 0.0f,
                            .phase = 0.05120819f},
           control::ModTarget::Zn3Im)
      .add("parity_tuning",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 0.004f,
                            .offset = 0.5f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.015f,
                            .offset = 0.03f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
