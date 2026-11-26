#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Advanced 3.5D: The Airplane and the Birth of Period Three.
 *
 * Mathematics:
 * - On the real axis z -> z^2 + c is chaotic for c just above -7/4. At
 *   c = -7/4 exactly a pair of period-3 orbits is born in a tangent
 *   (saddle-node) bifurcation; this opens the period-3 window. At
 *   c = -1.7548777, the real root of c^3 + 2 c^2 + c + 1, the attracting
 *   orbit is 0 -> c -> c^2 + c -> 0 and the Julia set is the Airplane.
 *   The orbit doubles to period 6 near c = -1.7685, below this sweep.
 * - Above -7/4 nothing attracts and almost every point escapes, but the
 *   third iterate has three narrow channels where the orbit is about to
 *   exist. An orbit that enters one needs of the order of
 *   (c + 7/4)^(-1/2) passes to get through: Pomeau-Manneville
 *   intermittency of type I. Escape time rises without bound over the
 *   places where the Airplane's bodies will be.
 * - Below -7/4 the channels have closed. Those places are basins of the
 *   3-cycle and never escape.
 *
 * Pipeline: Square with zn2 = 2 and zn3 = c / 2, which is z^2 + c in the
 * frame coordinate z / 2, so the frame holds |Re z|, |Im z| <= 2 and the
 * whole Julia set (it reaches +-1.915 on the real axis and meets the
 * imaginary axis no higher than 0.4). Clamp with the Simplex border is the
 * bailout: an orbit is lit
 * on the first pass that takes it out of the square, so age is escape
 * time. One Triangle lane takes c from -1.744 to -1.762 and back, crossing
 * -7/4 at constant speed: on the way in the escape bands crowd into the
 * channels and the channels close to black; on the way out they reopen.
 *
 * Fidelity:   identity
 * Cell:       Square / Clamp / Simplex / DepthEcho
 * Nearest:    douady_rabbit_three_phase_ring_trap is period 3 off the real
 *             axis, reached through the 1/3 bulb with no tangency;
 *             spatial_wave_feigenbaum_julia_cascade is period doubling.
 *             Neither has a saddle-node or intermittency.
 * Height:     age = escape time; it diverges over the channels as c
 *             approaches -7/4 from above.
 * Hue clock:  bounded (hue_velocity_base = 0).
 * Fastest lane: 0.019 Hz
 * Status:     designed from source on 2026-11-15; not built, rendered or
 *             measured. The assertion is for c below -7/4.
 * @assert sing[0].period == 3 && p_X >= 0.9
 */
inline Pattern square_airplane_period_three_tangent_birth() {
  Pattern p;
  p.name = "square_airplane_period_three_tangent_birth";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {2.0f, 0.0f};
  p.zn.zn[3] = {-0.8765f, 0.0f}; // c / 2 with c = -1.753

  p.color.hue_rotation = 0.035f;
  p.color.sat_scale = 0.92f;
  p.color.fade = 0.955f;
  p.color.color_mode = as_int(ColorMode::DepthEcho);
  p.color.spiral_angle_coeff = 0.10f; // hue per unit of age
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.15f; // saturation gained with age
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.03f;
  p.color.hue_velocity_drift = 0.0f; // 0 = hue from age only
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.03f; // inside 0.97; the Julia set ends at 0.958
  p.seed.hue = 0.6f;
  p.seed.hue_speed = 0.02f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("parameter_c_half", // c from -1.744 to -1.762; -7/4 is -0.875
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.005f,
                            .amplitude = 0.0045f,
                            .offset = -0.8765f},
           control::ModTarget::Zn3Re)
      .add("border_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.013f,
                            .amplitude = 0.12f,
                            .offset = 0.6f},
           control::ModTarget::SeedHue)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.019f,
                            .amplitude = 0.06f,
                            .offset = 0.15f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
