#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: Saddle-Node Explosion of the Exponential Family.
 *
 * Mathematics:
 * - w -> b e^w with real b. For b < 1/e there are two real fixed points, one
 *   attracting, and the attracting basin fills the plane up to a Cantor
 *   bouquet of hairs. At b = 1/e they merge at w = 1 (a saddle-node). For
 *   b > 1/e there is no attracting point and the Julia set is the whole
 *   plane (Devaney): the basin vanishes at once.
 * - Just past the bifurcation orbits still linger near w = 1, for a number of
 *   steps that grows like pi / sqrt(e (b - 1/e)): the bottleneck left by the
 *   vanished fixed points.
 *
 * Pipeline: Exp, zn0 = 2.4, zn1 = 0.6, zn2 = b / 2.4, zn3 = -0.25, so that
 * w = 2.4 z + 0.6 and the merge point w = 1 sits at z = 0.17; Clamp; WaveRing
 * seed at the centre, inside the basin. The lane sweeps b from 0.30 to 0.45
 * across 1/e = 0.3679: a filled basin, then the bottleneck, then escape
 * everywhere, and back.
 *
 * Mechanism assertion: contracting class large for b < 1/e; escaped class
 * near 1 for b > 1/e, with the escape time peaking just above 1/e.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_exp_saddle_node_explosion() {
  Pattern p;
  p.name = "spatial_wave_exp_saddle_node_explosion";
  p.inner = InnerFunction::Exp;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.4f, 0.0f};
  p.zn.zn[1] = {0.6f, 0.0f};
  p.zn.zn[2] = {0.15625f, 0.0f};
  p.zn.zn[3] = {-0.25f, 0.0f};

  p.color.hue_rotation = 0.045f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.955f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.3f;
  p.color.spiral_radius_coeff = 0.7f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.04f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.84f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("b_sweep_through_one_over_e",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.01f,
                            .amplitude = 0.03125f,
                            .offset = 0.15625f},
           control::ModTarget::Zn2Re)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.017f,
                            .amplitude = 0.02f,
                            .offset = 0.045f},
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
