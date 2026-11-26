#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Magnetic Dipole (Minimal 3.5D): an Octagonal Rose Window.
 *
 * Mathematics:
 * - f(z) = (z^2 - 1) / (z^2 + 1), the inner function with bare slots. Its
 *   critical points are 0 and infinity: 0 -> -1 -> 0 is a superattracting
 *   2-cycle (f'(0) = 0), and infinity -> 1 -> 0 falls into it. The
 *   rational cousin of the basilica.
 * - The AmmannBeenker reduction folds the plane into the box and rotates
 *   every point outside the wedge |y| <= tan(pi/8) |x| into the wedge
 *   |arg| <= pi/8 by a multiple of pi/4. Both cycle points lie on the real
 *   axis inside the box, so the fold never touches the cycle.
 * - Measured: 92.9 % of the frame settles on {0, -1}; the other 7.1 % shows
 *   no period up to 39 within 1200 passes and stays restless.
 *
 * Pipeline: MagneticDipole, AmmannBeenker, Cross seed. Both cycle points
 * sit on the lit horizontal axis, so every settled pixel ends bright, and
 * the Cross lines are carried back through the folded map into arcs and
 * spokes: in the numpy model a cyan rose window, eightfold, with a burst at
 * the centre and tracery of circles round it. OKLab hue step 1/16, so the
 * generations shade gently from cyan through blue to violet.
 *
 * Fidelity:   identity
 * Cell:       MagneticDipole / AmmannBeenker / Cross / OKLabColorRotation
 * Nearest:    magnetic_dipole_octagonal_complementary_lakes is this map with
 *             the Simplex border and a parity colouring; square_minimal_
 *             octagonal_dyadic_sunburst is z^2 under the same fold.
 * Height:     age = passes to the cycle.
 * Hue clock:  bounded; step 1/16 +- 0.008.
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-17. The cycle and multiplier
 *             are analytic; the 92.9 / 7.1 % split is from a 400 x 400 grid
 *             in double precision. A numpy model of the pass was drawn to
 *             check composition. Not built, rendered by the engine or
 *             measured.
 * @assert sing[0].period == 2 && p_C >= 0.9
 */
inline Pattern magnetic_dipole_minimal_octagonal_rose_window() {
  Pattern p;
  p.name = "magnetic_dipole_minimal_octagonal_rose_window";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::AmmannBeenker;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0625f;
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

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.52f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("tracery_hue_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.005f,
                            .amplitude = 0.008f,
                            .offset = 0.0625f},
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
