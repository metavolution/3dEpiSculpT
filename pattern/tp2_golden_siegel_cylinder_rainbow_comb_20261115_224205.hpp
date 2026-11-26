#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Advanced 3.5D: A Siegel Disc Unrolled, with a Comb Coloured by Position.
 *
 * Mathematics:
 * - The Fatou family f(u) = u + w - exp(-u) commutes with u -> u + 2 pi i,
 *   so it is a map of the cylinder. With s = exp(-u) it becomes
 *   s -> L s exp(s), L = exp(-w). For w purely imaginary |L| = 1, and for
 *   the golden rotation number the fixed point s = 0 has a Siegel disc: a
 *   region on which the map is conjugate to a rigid rotation. For this
 *   family its boundary is a quasicircle through the critical point
 *   s = -1 (Geyer 2001).
 * - Unrolled by the logarithm, the disc is a half-cylinder to the right.
 *   Every orbit there stays on a closed curve round the cylinder and
 *   advances along it by the golden rotation: the curves are nearly
 *   straight far to the right and wave more and more toward the left,
 *   until the last one, which is fractal. Beyond it lies the chaotic part.
 *   This is the picture of KAM curves and their breakdown, in an exactly
 *   holomorphic setting.
 * - The frame is one period of the cylinder tall, so Grid wraps the
 *   periodic direction exactly. Orbits in the disc do not drift sideways
 *   and never meet the other wrap.
 *
 * Pipeline: TP2 with zn0 = -pi, zn1 = -1.7, zn2 = -2 / pi; the frame is
 * u = pi z + 1.7 and T(z) = z - exp(-u) / pi + i Im zn3, with
 * Im zn3 = -0.76393202, a step of -0.381966 of the circumference. The
 * Cross seed gives a horizontal line: generation n puts a copy of it at
 * height 0.764 n mod 2, the three-gap comb of the golden rotation, and
 * each tooth follows the conjugacy, so the teeth bend and crowd as they
 * near the boundary. The seed's vertical line becomes a band of invariant
 * curves near x = 0.
 * - The hue step per pass is 0.381966 turn, the same number as the
 *   rotation. Tooth n has hue 0.382 n and height 0.764 n mod 2, so inside
 *   the disc hue equals height / 2 up to the small radial hue term of the
 *   pipeline: the comb is a rainbow ordered by position. Outside the disc
 *   that order is lost.
 *
 * Fidelity:   identity inside the disc; the chaotic side meets the wrap in
 *             x and is outside the claim.
 * Cell:       TP2 / Grid / Cross / SpatialWave
 * Nearest:    spatial_wave_minimal_three_gap_kronecker is the same comb
 *             under a rigid translation of a flat torus; here the
 *             rotation is the linearised form of a nonlinear map and it
 *             ends at a boundary. siegel_disk_golden_mean_linearizer is a
 *             Siegel disc of a quadratic map seen in the plane.
 * Height:     age = n, which inside the disc is the position of a tooth
 *             along the cylinder in units of the rotation.
 * Hue clock:  bounded (hue_velocity_base = 0).
 * Fastest lane: 0.021 Hz
 * Status:     designed from source on 2026-11-15; in a separate arithmetic
 *             check orbits starting at x >= -0.2 kept their x to within
 *             0.2 over 3000 passes, and to within 0.05 from x >= 0.1,
 *             while those from x <= -0.35 wandered across the frame; not
 *             built, rendered or measured.
 * @assert p_N >= 0.55 && sing[0].escapes == 0
 */
inline Pattern tp2_golden_siegel_cylinder_rainbow_comb() {
  Pattern p;
  p.name = "tp2_golden_siegel_cylinder_rainbow_comb";
  p.inner = InnerFunction::TP2;
  p.reduction = ReductionType::Grid;
  p.zn.zn[0] = {-3.1415927f, 0.0f};
  p.zn.zn[1] = {-1.7f, 0.0f};
  p.zn.zn[2] = {-0.63661977f, 0.0f};          // -2 / pi
  p.zn.zn[3] = {-0.54112681f, -0.76393202f}; // -1.7 / pi, -2 / phi^2

  p.color.hue_rotation = 0.38196601f; // 1 / phi^2 turn, equal to the rotation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.94f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 1.0f;
  p.color.chroma_breath_amp = 0.01f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.003f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.01f;
  p.seed.alpha = 0.8f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("rotation_detune", // leaves the golden number by +-0.002 turn
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.005f,
                            .amplitude = 0.004f,
                            .offset = -0.76393202f},
           control::ModTarget::Zn3Im)
      .add("comb_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.008f,
                            .amplitude = 0.15f,
                            .offset = 0.0f},
           control::ModTarget::SeedHue)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.021f,
                            .amplitude = 0.008f,
                            .offset = 0.01f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
