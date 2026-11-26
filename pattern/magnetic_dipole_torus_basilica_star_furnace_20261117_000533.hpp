#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Magnetic Dipole (Advanced 3.5D): the Basilica's Rational Cousin as a Star
 * Furnace in a Mirror Box.
 *
 * Mathematics:
 * - f(z) = (z^2 - 1) / (z^2 + 1): superattracting 2-cycle 0 -> -1 -> 0,
 *   with infinity -> 1 -> 0 feeding it. Its poles are +-i, on the frame
 *   edge: near them f is huge and the Torus fold (reflections in the box
 *   mirrors) returns it, many times over, into the frame.
 * - The fold leaves both cycle points alone (-1 is on the mirror x = -1).
 *   Measured: 100 % of a 300 x 300 grid settles on the cycle within 400
 *   passes, median 14 passes and 90 % within 26: much slower than the
 *   octagonal fold, because points near the poles bounce around the box
 *   for a while first. On a 400 x 400 grid after 400 passes, 51.7 % sit at
 *   0 and 48.3 % at -1.
 *
 * Pipeline: MagneticDipole, Torus, Cross seed. The axes are lit and both
 * cycle points lie on the horizontal one. In the numpy model, a cyan cross
 * burns at the centre with flame-like spikes along the diagonals, inside a
 * red circle, over a fine grain of slow-settling points. A HueRot lane
 * sweeps the per-pass step between 1/16 and 3/16 at 0.004 Hz, so the
 * flames, which are many passes deep, cycle through hues while the cross
 * at depth 0 to 2 barely changes: depth shown as speed of colour.
 *
 * Fidelity:   identity
 * Cell:       MagneticDipole / Torus / Cross / OKLabColorRotation
 * Nearest:    magnetic_dipole_minimal_octagonal_rose_window is the same map
 *             under the octagonal fold (calm, eightfold); here the plain box
 *             fold gives fourfold fire.
 * Height:     age = passes to the cycle (deep in the flames).
 * Hue clock:  bounded; step 1/8 +- 1/16. A pixel n passes deep turns
 *             n times faster than the lane: about 0.04 turns/s at n = 26.
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-17. Cycle analytic; settling
 *             statistics from a 400 x 400 grid in double precision. A numpy
 *             model of the pass was drawn at step 1/8. Not built, rendered
 *             by the engine or measured.
 * @assert sing[0].period == 2 && p_C >= 0.99
 */
inline Pattern magnetic_dipole_torus_basilica_star_furnace() {
  Pattern p;
  p.name = "magnetic_dipole_torus_basilica_star_furnace";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.125f;
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
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.52f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("flame_depth_hue_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 0.0625f,
                            .offset = 0.125f},
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
