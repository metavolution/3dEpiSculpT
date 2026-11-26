#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Invert (Advanced 3.5D): an Octagonal Kleinian Rose.
 *
 * Mathematics:
 * - T(z) = k / z, then the AmmannBeenker reduction: the box fold, and then
 *   every point outside the wedge |y| <= tan(pi/8) |x| is rotated by a
 *   multiple of pi/4 into the wedge |arg| <= pi/8.
 * - Every piece is still a Mobius or anti-Mobius map, and every one keeps
 *   the circle |z| = sqrt k (rotations about 0 do, and so do inversion and
 *   conjugation). But the rotation acts only on part of the plane and is
 *   not a reflection, so the passes are no longer elements of one discrete
 *   group: this is a piecewise semigroup with an eightfold fold.
 * - Even so, every orbit was found to end on a 2-cycle, at every k tried
 *   from 0.8 to 1.0. The cycles pair a point outside the circle with its
 *   inverse inside it, as in the plain mirror box.
 *
 * Pipeline: Invert, AmmannBeenker, zn2 = k on a Sine lane from 0.8 to 1.0
 * at 0.005 Hz. Simplex border, OKLab hue step 1/10. In a numpy model of the
 * pass, at k = 0.8 an octagonal frame encloses an eight-petal rose of
 * nested circles; at k = 1 the petals open into solid sectors bounded by
 * circular arcs, with lace between them. The lane lets the rose bloom and
 * close.
 *
 * Fidelity:   identity for every lane value
 * Cell:       Invert / AmmannBeenker / Simplex / OKLabColorRotation
 * Nearest:    invert_schottky_dust_condenses_to_ford_packing is the same
 *             inversion in the plain box (a true reflection group);
 *             ammann_beenker_quasicrystal uses this fold with cos.
 * Height:     age = passes since landing.
 * Hue clock:  bounded; step 1/10.
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-16. On a 300 x 300 grid in
 *             double precision, 100 % of points were on a 2-cycle within
 *             400 passes at k = 0.8, 0.85, 0.95, 0.99, 1.0 (400 x 400 at
 *             0.8, 0.9, 1.0: median landing 1, 2, 5 passes, 90 % within 3,
 *             5, 14). No 64-pass exponent was below -0.01. The rose
 *             description comes from the numpy model. Not built, rendered by
 *             the engine or measured.
 * @assert p_X == 0 && p_C <= 0.05
 */
inline Pattern invert_ammann_beenker_octagonal_kleinian_rose() {
  constexpr float K_MID = 0.9f;
  constexpr float K_SWING = 0.1f; // k from 0.8 to 1.0
  Pattern p;
  p.name = "invert_ammann_beenker_octagonal_kleinian_rose";
  p.inner = InnerFunction::Invert;
  p.reduction = ReductionType::AmmannBeenker;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {K_MID, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.1f;
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

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.08f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("rose_bloom",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.005f,
                            .amplitude = K_SWING,
                            .offset = K_MID},
           control::ModTarget::Zn2Re)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.02f,
                            .amplitude = 0.025f,
                            .offset = 0.05f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
