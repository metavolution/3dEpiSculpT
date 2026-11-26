#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: the Exponential Family Round its Unit Circle.
 *
 * Mathematics:
 * - w -> lambda (e^w - 1) fixes 0 with multiplier exactly lambda. It is the
 *   exponential family normalised at its fixed point. For |lambda| < 1 the
 *   Fatou set is the single basin of 0 and the Julia set is a Cantor bouquet
 *   of hairs reaching to the right, where e^w is large.
 * - zn3 = -zn2 keeps the normalisation, and both are linear in lambda, so
 *   four lanes (two quadrature pairs, the second pair half a period out of
 *   phase) move lambda = 0.985 e^(i theta(t)) once round, just inside the
 *   unit circle. Near theta = 2 pi p / q orbits approach 0 through q
 *   channels and the basin grows q arms between the hairs; in between it is
 *   nearly a rotation domain.
 *
 * Pipeline: Exp, zn0 = 2.2, zn2 = lambda / 2.2, zn3 = -lambda / 2.2; Clamp;
 * WaveRing seed centred on the fixed point, so the rings are carried round
 * and outward by the dynamics. At the right edge |lambda (e^2.2 - 1)| = 7.9,
 * so those orbits leave and are clamped. One tour takes 290 s.
 *
 * Mechanism assertion: contracting class with rate 0.985 at z = 0 for every
 * theta; escaped class at large Re z.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_exponential_multiplier_tour() {
  Pattern p;
  p.name = "spatial_wave_exponential_multiplier_tour";
  p.inner = InnerFunction::Exp;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.2f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.4477f, 0.0f};
  p.zn.zn[3] = {-0.4477f, 0.0f};

  p.color.hue_rotation = 0.04f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.96f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.4f;
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
  p.seed.hue = 0.02f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.84f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("multiplier_cos",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0035f,
                            .amplitude = 0.4477f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Re)
      .add("multiplier_sin",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0035f,
                            .amplitude = 0.4477f,
                            .offset = 0.0f},
           control::ModTarget::Zn2Im)
      .add("normalisation_cos",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0035f,
                            .amplitude = 0.4477f,
                            .offset = 0.0f,
                            .phase = 0.75f},
           control::ModTarget::Zn3Re)
      .add("normalisation_sin",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0035f,
                            .amplitude = 0.4477f,
                            .offset = 0.0f,
                            .phase = 0.5f},
           control::ModTarget::Zn3Im);

  return p;
}

} // namespace epi::presets
