#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: the Dottie Number and its Period Doubling, w -> L cos w.
 *
 * Mathematics:
 * - Pressing "cos" again and again on a calculator converges to 0.739085...,
 *   the Dottie number, the attracting fixed point of cos. Its multiplier is
 *   -sin(0.739) = -0.674: each step lands on the other side, closer.
 * - In the family w -> L cos w the multiplier is -L sin w*. It reaches -1 at
 *   L = 1.319 (w* = 0.8603, where tan w = 1/w): the fixed point gives way to
 *   a two-cycle, the first step of a period-doubling cascade.
 * - In the complex plane cos grows like e^|Im w| / 2, so points far from the
 *   real axis leave. The basin of the attractor is bounded by the Julia set
 *   of the family, with hairs reaching up and down.
 *
 * Pipeline: Cos, zn0 = 2.4 (window w in [-2.4, 2.4]^2), zn2 = L / 2.4; Clamp;
 * ChladniCymatics seed. The lane sweeps L from 1.0 to 1.6 and back, through
 * the doubling: one alternating focus becomes two.
 *
 * Mechanism assertion: contracting class on a band around the real axis,
 * escaped class above and below; attracting period 1 below L = 1.319, 2 just
 * above.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_dottie_cosine_period_doubling() {
  Pattern p;
  p.name = "spatial_wave_dottie_cosine_period_doubling";
  p.inner = InnerFunction::Cos;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.4f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.5417f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.05f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.95f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.8f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.33f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.75f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("gain_L_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.012f,
                            .amplitude = 0.125f,
                            .offset = 0.5417f},
           control::ModTarget::Zn2Re)
      .add("basin_twist",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.019f,
                            .amplitude = 0.02f,
                            .offset = 0.0f},
           control::ModTarget::Zn2Im)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.016f,
                            .amplitude = 0.02f,
                            .offset = 0.05f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
