#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Magnetic Dipole (Advanced 3.5D): the Witch of Agnesi Doubles Once, and
 * Colour Locks onto the Two-Cycle.
 *
 * Mathematics:
 * - f_c(u) = c / (1 + u^2), c real: on the real line this is Agnesi's
 *   bell curve used as a map. Infinity maps to 0, so again there is one
 *   free critical orbit, 0 -> c -> c / (1 + c^2) -> ...
 * - Fixed point: x (1 + x^2) = c, multiplier -2 x^2 / (1 + x^2). It
 *   attracts while x < 1, that is c < 2, and has multiplier -1 at c = 2.
 * - For c > 2 the 2-cycle is {p, 1/p} with p + 1/p = c, and its
 *   multiplier is exactly 4 / c^2: attracting for every c > 2. The map
 *   doubles once and never again. (c = sqrt 5 gives the cycle
 *   {1/phi, phi} with multiplier 4/5.)
 * - c < 2: the whole real circle lies in the immediate basin of the
 *   fixed point, both critical points with it, so the Julia set is a
 *   Cantor set. c > 2: the two critical points lie in different
 *   components, the map is a hyperbolic deformation of u^-2, and the
 *   Julia set is a Jordan curve through the old fixed point. At the one
 *   doubling the dust closes up into a curve.
 *
 * Pipeline: MagneticDipole with zn0 = 2.1, zn2 = -c / 4.2, zn3 = c / 4.2
 * gives T(z) = f_c(2.1 z) / 2.1 exactly; two Sine lanes in antiphase move
 * c = 2.1 + 0.4 sin, between 1.7 and 2.5. WaveRing is the seed and
 * Opponent the colour, with a hue step of 1/2. Opponent rotation is
 * linear in RGB, so successive passes add as vectors in the chroma plane
 * and each pass reverses them. On a fixed point the reversed copies
 * cancel: chroma is left at (1 - rho) / (1 + rho) of luminance, where
 * rho = fade (1 - seed alpha) is between 0.71 and 0.94 as the rings
 * pass, so 3 to 17 %, and the basin is a grey ring pattern. On a 2-cycle the orbit
 * alternates between two points where the seed differs, the copies no
 * longer cancel, and the two sides of the Julia curve come up in
 * complementary colours. The loop is a lock-in detector tuned to period
 * 2: colour is born at c = 2 and nowhere else. Near c = 2 the multiplier
 * is close to -1 and convergence is slow, so the transition is gradual.
 *
 * Fidelity:   identity for every lane value
 * Cell:       MagneticDipole / Clamp / WaveRing / Opponent
 * Nearest:    gauss_sum_theta_chroma_resonator adds chroma vectors round
 *             a rigid rotation (no attractor); cayley_rational_
 *             superattracting_two_cycle colours a fixed 2-cycle by
 *             parity. Here the same cancellation is used to detect a
 *             bifurcation as it happens.
 * Height:     age = number of passes.
 * Hue clock:  bounded; step 1/2 plus at most 0.01 turn from the len term.
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-16. The slot arithmetic was
 *             checked at c = 1.7, 2.1, 2.5; p + 1/p = c and the attractor
 *             type were confirmed by iteration at seven values of c. The
 *             cancellation ratio is the geometric series and holds where
 *             the clamp to [0, 1] is inactive. Not built, rendered or
 *             measured.
 * @assert sing[0].period == 2 && sing[0].rho_abs within 2 % of 0.907
 */
inline Pattern magnetic_dipole_agnesi_witch_single_doubling_chroma_lock() {
  constexpr float MID = 0.5f;          // 2.1 / 4.2
  constexpr float SWING = 0.0952381f;  // 0.4 / 4.2
  constexpr float SWEEP_HZ = 0.005f;
  Pattern p;
  p.name = "magnetic_dipole_agnesi_witch_single_doubling_chroma_lock";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.1f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {-MID, 0.0f};
  p.zn.zn[3] = {MID, 0.0f};

  p.color.hue_rotation = 0.5f; // each pass reverses the chroma vector
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.94f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.04f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f; // bounded clock: the step stays at 1/2
  p.color.hue_velocity_breath = 0.0004f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.6f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("witch_height_post", // c rises: zn2 = -c / 4.2 falls
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SWEEP_HZ,
                            .amplitude = -SWING,
                            .offset = -MID},
           control::ModTarget::Zn2Re)
      .add("witch_height_shift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = SWEEP_HZ,
                            .amplitude = SWING,
                            .offset = MID},
           control::ModTarget::Zn3Re)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.02f,
                            .offset = 0.04f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
