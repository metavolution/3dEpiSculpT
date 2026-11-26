#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Quartic F12 (Advanced 3.5D): the Quartic Period-Doubling Cascade, a
 * Different Universality Class.
 *
 * Mathematics:
 * - With the F12 inner 0.7 (u^4 - 1), zn0 = 1.45, zn2 = 1/(0.7 x 1.45),
 *   zn3 = (1 + c)/1.45, T is w -> w^4 + c in w = 1.45 z. For real c the real
 *   map is unimodal with a quartic critical point.
 * - Feigenbaum universality depends on the order of the critical point.
 *   For w^4 + c the superstable parameters of period 2^k are c = 0, -1,
 *   -1.1457142, -1.1652666, -1.1679392, -1.1683052, -1.1683554,
 *   -1.1683623, -1.1683632 (Newton on f^(2^k)(0) = 0); their gaps shrink
 *   by delta(4) = 7.285 (quadratic: 4.669) and accumulate at
 *   c_inf(4) = -1.1683634 (extrapolated).
 * - (i w)^4 = w^4, so every Julia set of the family has the frame's quarter-
 *   turn symmetry: each doubling pinches the dark basins in four
 *   directions at once.
 * - A Sine lane takes c from c_inf(4) (the rest state: the quartic edge of
 *   chaos, no interior) to -1 (the quartic basilica) and back, 0.0015 Hz.
 *   Near c_inf the Sine slows quadratically, so successive doublings,
 *   spaced 7.285 times closer each, come into view at shrinking intervals.
 * - Exactness: |c| <= 1.1684, so K lies in |w| <= 1.2466, |z| <= 0.860,
 *   inside the Simplex band; the border (|w| >= 1.45) stays clamped.
 *
 * Pipeline: F12; Clamp; Simplex border; fade 0.94; OKLab step 1/8.
 *
 * Fidelity:   identity for every lane value
 * Cell:       F12 / Clamp / Simplex / OKLabColorRotation
 * Nearest:    spatial_wave_feigenbaum_julia_cascade and
 *             square_minimal_feigenbaum_point_edge_of_chaos (the quadratic
 *             class); quartic_minimal_four_fold_basilica_turning (c = -1).
 * Height:     age = passes before exit.
 * Hue clock:  bounded; step 1/8 +- 0.005.
 * Fastest lane: 0.02 Hz
 * Status:     designed by derivation on 2026-11-17; superstable parameters,
 *             delta(4) and c_inf(4) computed arithmetically (c_inf is an
 *             extrapolation, good to about 1e-7). Not built, rendered or
 *             measured.
 * @assert sing[0].escapes == 0
 */
inline Pattern quartic_feigenbaum_cascade_other_universality() {
  Pattern p;
  p.name = "quartic_feigenbaum_cascade_other_universality";
  p.inner = InnerFunction::F12;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.45f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.98522167f, 0.0f};  // 1/(0.7 x 1.45)
  p.zn.zn[3] = {-0.11611269f, 0.0f}; // (1 + c_inf(4))/1.45

  p.color.hue_rotation = 0.125f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.94f;
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.0f;
  p.color.chroma_breath_amp = 0.05f;
  p.color.chroma_breath_freq = 0.1f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.0f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.06f;
  p.seed.hue = 0.95f;
  p.seed.hue_speed = 0.0f;
  p.seed.alpha = 1.0f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("cascade_unfold",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0015f,
                            .amplitude = 0.05805634f,
                            .offset = -0.05805634f,
                            .phase = 0.75f}, // starts at c_inf(4)
           control::ModTarget::Zn3Re)
      .add("cascade_hue",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
                            .amplitude = 0.005f,
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
