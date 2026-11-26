#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: Period Doubling of the Exponential at L = -e.
 *
 * Mathematics:
 * - w -> L e^w with real negative L. At a fixed point w* = L e^(w*) the
 *   multiplier is L e^(w*) = w* itself. It equals -1 when w* = -1, that is
 *   when L = -e exactly. For -e < L < 0 the fixed point attracts, each step
 *   landing on the other side of it; for L < -e it gives way to an attracting
 *   two-cycle.
 * - This is the counterpart, on the negative axis, of the saddle-node at
 *   L = 1/e on the positive one. In both the basin is bounded by hairs
 *   reaching into the right half-plane, where e^w is large.
 *
 * Pipeline: Exp, zn0 = 2.4, zn1 = -0.6, zn2 = L / 2.4, zn3 = 0.25, so that
 * w = 2.4 z - 0.6 and the critical fixed point w = -1 sits at z = -0.17;
 * Clamp; Cross seed. The lane sweeps L from -2.12 to -3.32 and back, through
 * -e = -2.718: one alternating focus becomes two.
 *
 * Mechanism assertion: contracting class on the left, escaped class on the
 * right; attracting period 1 for L > -e, 2 for L < -e.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_exp_minus_e_period_doubling() {
  Pattern p;
  p.name = "spatial_wave_exp_minus_e_period_doubling";
  p.inner = InnerFunction::Exp;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.4f, 0.0f};
  p.zn.zn[1] = {-0.6f, 0.0f};
  p.zn.zn[2] = {-1.1326f, 0.0f};
  p.zn.zn[3] = {0.25f, 0.0f};

  p.color.hue_rotation = 0.05f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.3f;
  p.color.spiral_radius_coeff = 0.8f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.3f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.8f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("L_sweep_through_minus_e",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.011f,
                            .amplitude = 0.25f,
                            .offset = -1.1326f},
           control::ModTarget::Zn2Re)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.016f,
                            .amplitude = 0.02f,
                            .offset = 0.05f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.026f,
                            .amplitude = 0.04f,
                            .offset = 0.1f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
