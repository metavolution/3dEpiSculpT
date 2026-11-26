#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Identity (Advanced 3.5D): a Vortex-Antivortex Crystal Forced by the
 * Topology of the p4 Orbifold.
 *
 * Mathematics:
 * - T(z) = i z + c on the Grid torus R^2 / 2Z^2: a quarter turn, exact on
 *   the torus because i 2Z^2 = 2Z^2. With c = 0.6 + 0.2i it fixes
 *   z* = c/(1 - i) = 0.2 + 0.4i and z* + 1 + i (order-4 centres) and swaps
 *   z* + 1 and z* + i (order-2 centres): the wallpaper group p4.
 * - Opponent colour with step 1/4 over this 4-periodic map gives chroma
 *   C(z) = sum_j i^j S(T^j z) (weights near 1). At an order-4 centre only
 *   the Taylor terms d^a conj(d)^b with a - b = -1 mod 4 survive, so C winds
 *   once backwards: a vortex of charge -1, forced at both order-4 centres.
 * - C(T z) = -i C(z), so |C| is T-invariant and the two order-2 centres
 *   carry equal index. The Chladni seed is continuous and periodic (zero
 *   for r >= 1), so C is a continuous map on the torus and its indices sum
 *   to 0 (Poincare-Hopf). Other zeros come in T-orbits of four equal
 *   charges, so 2 ind(order-2) - 2 + 4k = 0: the order-2 centres carry an
 *   odd charge, never zero, +1 when there are no extra vortices. A
 *   checkerboard crystal of vortices and antivortices, neutral overall.
 * - A lane moves c round a circle; the centres move with z* and the
 *   plates do not. The colour field changes everywhere, but the charges at
 *   the centres cannot change: they are fixed by the group, not the seed.
 *
 * Pipeline: Identity, zn0 = i, zn1 = 0, zn2 = 1, zn3 = c on quadrature lanes
 * (|c| = 0.632, 333 s); Grid; Chladni at alpha 0.12; fade 0.99; Opponent
 * step 1/4. The light channel shows the 4-fold average of the plates.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Identity / Grid / ChladniCymatics / Opponent
 * Nearest:    identity_hexagonal_phased_array_colour_vortex and
 *             invert_mobius_colour_vortex_charge_ladder (single vortices in
 *             the plane or sphere); here a lattice of them on a torus, with
 *             the antivortices forced by Poincare-Hopf.
 * Height:     age = passes; the four generations are the four group
 *             elements.
 * Hue clock:  bounded; step 1/4 +- 0.004.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; centres evaluated
 *             arithmetically. With rho = fade (1 - seed alpha) < 1 the cores
 *             are displaced by order 1 - rho; when a Chladni nodal kink passes
 *             over a centre the local charge is momentarily undefined. Not
 *             built, rendered or measured.
 * @assert p_N >= 0.95
 */
inline Pattern identity_p4_orbifold_vortex_antivortex_crystal() {
  constexpr float SLIDE_HZ = 0.003f;
  constexpr float C_ABS = 0.63245553f;
  Pattern p;
  p.name = "identity_p4_orbifold_vortex_antivortex_crystal";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {0.0f, 1.0f}; // i: a quarter turn
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.6f, 0.2f}; // c; order-4 centre z* = 0.2 + 0.4i

  p.color.hue_rotation = 0.25f; // charge -1 at the order-4 centres
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
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 0.12f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("crystal_slide_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SLIDE_HZ,
                            .amplitude = C_ABS,
                            .offset = 0.0f,
                            .phase = 0.30120819f},
           control::ModTarget::Zn3Re)
      .add("crystal_slide_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SLIDE_HZ,
                            .amplitude = C_ABS,
                            .offset = 0.0f,
                            .phase = 0.05120819f},
           control::ModTarget::Zn3Im)
      .add("vortex_tuning",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 0.004f,
                            .offset = 0.25f},
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
