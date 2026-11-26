#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Magnetic Dipole (Advanced 3.5D): Complementary Lakes, Coloured by the
 * Parity of Arrival.
 *
 * Mathematics:
 * - f(z) = (z^2 - 1) / (z^2 + 1) under the AmmannBeenker fold, as in the
 *   rose window: superattracting 2-cycle 0 -> -1 -> 0, untouched by the
 *   fold; 92.9 % of the frame settles on it (measured), 7.1 % stays
 *   restless.
 * - The basin of a 2-cycle splits in two: points that reach 0 after an
 *   even number of passes and points that reach -1 (the lakes of f^2).
 *   Measured: 68.9 % of the frame is at 0 and 24.0 % at -1 after 1200
 *   passes.
 *
 * Pipeline: MagneticDipole, AmmannBeenker, Simplex border, Opponent colour
 * with a hue step of exactly 1/2. The cycle point -1 lies on the border
 * band (seed fully opaque there) and 0 does not, so every settled pixel is
 * reset by the seed at -1 once every two passes and then turned through
 * half a hue turn per pass on the way back to the pixel. Opponent rotation
 * is linear in RGB, so half a turn is an exact chroma reversal: the two
 * lakes of f^2 come up in complementary colours, red and cyan in the numpy
 * model, a red field with strings of cyan pearls. The image is stationary
 * (the seed does not move), so nothing alternates at frame rate. A HueRot
 * lane detunes the step by +-0.02 at 0.005 Hz, which turns the two colours
 * slowly in opposite senses and brings them back to opposition.
 *
 * Fidelity:   identity
 * Cell:       MagneticDipole / AmmannBeenker / Simplex / Opponent
 * Nearest:    magnetic_dipole_agnesi_witch_single_doubling_chroma_lock
 *             uses the same chroma reversal to detect a 2-cycle being born;
 *             cayley_rational_superattracting_two_cycle colours a fixed
 *             2-cycle by parity on another map. Here the lakes are folded
 *             eightfold.
 * Height:     age = passes to the cycle.
 * Hue clock:  bounded; step 1/2 +- 0.02.
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-17. Lake fractions from a
 *             400 x 400 grid in double precision. A numpy model of the pass
 *             (linear chroma, as Opponent is) was drawn. Not built, rendered
 *             by the engine or measured.
 * @assert sing[0].period == 2 && p_C >= 0.9
 */
inline Pattern magnetic_dipole_octagonal_complementary_lakes() {
  Pattern p;
  p.name = "magnetic_dipole_octagonal_complementary_lakes";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::AmmannBeenker;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.5f; // exact chroma reversal per pass
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::Opponent);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.04f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0004f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.08f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.01f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("opposition_detune",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.005f,
                            .amplitude = 0.02f,
                            .offset = 0.5f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.02f,
                            .offset = 0.04f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
