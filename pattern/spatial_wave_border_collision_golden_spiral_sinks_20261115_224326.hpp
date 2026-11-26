#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave (Advanced 3.5D): Spiral Sinks across a Fault, and Border
 * Collisions.
 *
 * Mathematics:
 * - With SymmetryBreak and the affine slots,
 *       T(z) = L (z + d H(z)) + zn3,   L = 0.9 e^{i 137.5 deg},  d = 0.5,
 *   where H is 1 above the real axis and 0 below. On each side T is a
 *   contracting spiral: ratio 0.9, golden-angle turn, about its own focus.
 *   The lower map has focus a = zn3 / (1 - L); the upper one has focus
 *   b = a + d L / (1 - L) = a + (-0.235, 0.097).
 * - A focus is a fixed point of T only while it lies on its own side. If
 *   zn3 carries a above the fault, or b below it, that fixed point does
 *   not lose stability; it ceases to exist. This is a border-collision
 *   bifurcation, the characteristic event of piecewise-smooth dynamics,
 *   and no smooth map has it.
 * - An orbit that spirals toward a focus on the far side must cross the
 *   fault on the way, and is then turned toward the other focus. Such
 *   orbits can close up into cycles that visit both sides. Since T
 *   contracts area by 0.81 at every step, every attractor has area zero,
 *   and several coexist.
 * - Found by iterating the map over this lane sweep: a 3-cycle is present
 *   throughout; a 4-cycle exists only in the middle of the sweep (for
 *   Im a between about -0.105 and 0.008); the fixed point is a at one end,
 *   b at the other, and both at once in between. Every attractor lies
 *   within 0.75 of the origin, so Clamp touches only transients from the
 *   corners.
 *
 * Pipeline: SymmetryBreak with zn0 = 0.2 and zn2 = 5 L; Clamp; WaveRing.
 * Generation n is a ring wave in |T^n(z)|; under a golden-angle spiral
 * contraction that draws ring trains along spiral arms into each
 * attractor, cut along the pre-images of the fault. The frame is a mosaic
 * of basins. Two Sine lanes in phase move both foci straight across the
 * fault, Im a from -0.18 to +0.12; the basins of a cycle open and close
 * at once when it is born or destroyed.
 *
 * Fidelity:   identity (the list of cycles is numerical)
 * Cell:       SymmetryBreak / Clamp / WaveRing / SpatialWave
 * Nearest:    spatial_wave_goetz_pentagon_piecewise_rotation_mosaic and
 *             symmetry_break_minimal_quarter_turn_torus_quilt use the same
 *             slip without contraction: area is preserved and nothing
 *             attracts. Here the map is dissipative and the subject is
 *             attractors and their sudden births.
 * Height:     age = number of passes; rings crowd by 1 / 0.9 per pass
 *             toward each attractor.
 * Hue clock:  bounded (hue_velocity_base = 0).
 * Fastest lane: 0.025 Hz
 * Status:     designed from source on 2026-11-15; the attractors were
 *             found in a separate arithmetic check of the map in exactly
 *             this parameter form; not built, rendered or measured.
 * @assert p_C >= 0.99 && lambda_med within 1 % of -0.1054
 */
inline Pattern spatial_wave_border_collision_golden_spiral_sinks() {
  Pattern p;
  p.name = "spatial_wave_border_collision_golden_spiral_sinks";
  p.inner = InnerFunction::SymmetryBreak;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {0.2f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {-3.3181597f, 3.0397066f};    // 5 * 0.9 e^{i 137.5 deg}
  p.zn.zn[3] = {0.17712158f, -0.12129933f}; // a (1 - L), a = (0.117, -0.03)

  p.color.hue_rotation = 0.06f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 1.1f;
  p.color.chroma_breath_amp = 0.07f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.05f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.33f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.5f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("foci_across_fault_re", // 0.15 * Re(i (1 - L))
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 0.09119120f,
                            .offset = 0.17712158f},
           control::ModTarget::Zn3Re)
      .add("foci_across_fault_im", // 0.15 * Im(i (1 - L))
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.004f,
                            .amplitude = 0.24954479f,
                            .offset = -0.12129933f},
           control::ModTarget::Zn3Im)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.025f,
                            .amplitude = 0.03f,
                            .offset = 0.07f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
