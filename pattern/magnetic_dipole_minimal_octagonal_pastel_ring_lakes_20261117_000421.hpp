#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Magnetic Dipole (Minimal 3.5D): Pastel Ring Lakes under an Octagonal Fold.
 *
 * Mathematics:
 * - f(z) = (z^2 - 1) / (z^2 + 1) with bare slots, under the AmmannBeenker
 *   fold: the superattracting 2-cycle 0 -> -1 -> 0 survives the fold (both
 *   points are on the real axis inside the box), and 92.9 % of the frame
 *   settles on it (measured).
 * - Every lake is a conformal copy of a neighbourhood of 0 or of -1, so a
 *   pattern centred on 0 reappears, bent but intact, in every lake.
 *
 * Pipeline: MagneticDipole, AmmannBeenker, and the WaveRing seed, whose
 * rings are centred exactly on the cycle point 0 and expand with time. Each
 * lake therefore carries its own copy of the expanding rings, and all
 * copies breathe in step: a field of soft concentric pools, larger at the
 * centre and at the sides, smaller towards the octagonal folds (numpy
 * model). Seed alpha 0.7 lets the older generations show through as
 * pastel; OKLab hue step 1/10.
 *
 * Fidelity:   identity
 * Cell:       MagneticDipole / AmmannBeenker / WaveRing / OKLabColorRotation
 * Nearest:    magnetic_dipole_minimal_octagonal_rose_window is the same map
 *             with the Cross seed (lines, not rings).
 * Height:     age = passes to the cycle.
 * Hue clock:  bounded; step 1/10 +- 0.01.
 * Fastest lane: 0.02 Hz
 * Status:     designed from source on 2026-11-17. Same map and grid
 *             measurement as the rose window. A numpy model of the pass was
 *             drawn to check composition. Not built, rendered by the engine
 *             or measured.
 * @assert sing[0].period == 2 && p_C >= 0.9
 */
inline Pattern magnetic_dipole_minimal_octagonal_pastel_ring_lakes() {
  Pattern p;
  p.name = "magnetic_dipole_minimal_octagonal_pastel_ring_lakes";
  p.inner = InnerFunction::MagneticDipole;
  p.reduction = ReductionType::AmmannBeenker;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
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

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.12f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 0.7f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("pool_hue_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.006f,
                            .amplitude = 0.01f,
                            .offset = 0.1f},
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
