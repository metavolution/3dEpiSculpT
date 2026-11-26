#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: Rotation-Number Sweep of a Neutral Cavity.
 *
 * Mathematics:
 * - The map is the pure rotation z -> e^(i theta) z. It stretches nothing, so
 *   the steady image is the seed averaged over its own rotated copies,
 *   sum_n g^n S(e^(i n theta) z). For theta = 2 pi p / q the copies fall on q
 *   directions and add up to a q-fold figure; for an angle far from every
 *   low-order rational they smear into rings. The width of a lock is about
 *   2 pi / (q tau), tau the memory in steps, so low q locks are wide and
 *   bright and high q locks are narrow.
 * - Two lanes in quadrature on zn0 (cosine on the real part, sine on the
 *   imaginary part, amplitude exactly 1) make theta run uniformly once round
 *   the circle per lane period. The picture passes through every rational:
 *   it is the resonance structure of a rotation, drawn in time.
 *
 * Pipeline: Identity, zn0 = e^(i theta(t)); Clamp (a rotation keeps all but
 * the corners inside); ChladniCymatics seed, whose own four-fold symmetry
 * turns a q lock into an lcm(4, q)-fold rosette. One revolution takes 330 s.
 *
 * Mechanism assertion: neutral orbit fraction ~ 1 at every theta; azimuthal
 * harmonic lcm(4, q) dominant at theta = 2 pi p / q.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_rotation_number_resonance_sweep() {
  Pattern p;
  p.name = "spatial_wave_rotation_number_resonance_sweep";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.04f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.97f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 0.9f;
  p.color.chroma_breath_amp = 0.12f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 0.08f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.05f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::ChladniCymatics;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.48f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.7f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("rotation_angle_cos",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
                            .amplitude = 1.0f,
                            .offset = 0.0f,
                            .phase = 0.25f},
           control::ModTarget::Zn0Re)
      .add("rotation_angle_sin",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.003f,
                            .amplitude = 1.0f,
                            .offset = 0.0f},
           control::ModTarget::Zn0Im)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.017f,
                            .amplitude = 0.02f,
                            .offset = 0.04f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
