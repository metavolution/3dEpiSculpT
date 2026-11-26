#pragma once

#include "base.hpp"

namespace epi::presets {

// [2C] Golden-spiral similarity beta=phi^(1/3) e^(i pi/6) about z*!=0 (Lambda=ln(phi)/3): Cross-line copies envelope log-spiral caustics; L4 fade 0.9013 => alpha=3.295, A(512)=1.5%.
/**
 * 4x3 Orthogonal Spatial Wave Matrix -- cell 2C.
 *
 * Row 2 (Chiral / Logarithmic Spiral) x Column C (Harmonic Cascade).
 * - T(z) = beta z + gamma with beta = phi^(1/3) e^{i pi/6}: three steps turn
 *   90 deg and grow by phi, i.e. the golden spiral (pitch b = 2 ln(phi)/pi).
 * - gamma = 0.25 places the fixed point z* = gamma/(1-beta) off the Cross
 *   axes, so the rotated/shrunk copies of the two Cross lines no longer pass
 *   through z*: their envelope is a family of log-spiral caustic arms.
 * - Law L4 with Lambda = ln(phi)/3 and Cross f_S = 10: A(512) = 0.015 =>
 *   alpha = 3.2952, g = 0.901338 (tau = 9.627 steps).
 * - LogPolarColorSpiral (mode 11, absolute hue, luma_d = 0) with spiral_a = 1
 *   and spiral_r = -1/(2 pi b) makes the isochromes the same golden spiral.
 *
 * Temporal ladder: f_5 = 0.06 * 2^(5/12) = 0.08009 Hz (12-TET; no two
 * cells share a frequency). Seed hue = 5/12 on the matrix hue wheel.
 */
inline Pattern spatial_wave_chiral_golden_spiral_cascade() {
  Pattern p;
  p.name = "spatial_wave_chiral_golden_spiral_cascade";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;

  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.0167008f, 0.5869925f};
  p.zn.zn[3] = {0.25f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.9013379f;
  p.color.color_mode = as_int(ColorMode::LogPolarColorSpiral);
  p.color.spiral_angle_coeff = 1.0f;
  p.color.spiral_radius_coeff = -0.5195217f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 0.0800904f;
  p.color.chroma_breath_phase = 6.2831853f;
  p.color.hue_velocity_base = 0.0f; // O9 bounded hue clock
  p.color.hue_velocity_breath = 0.1258057f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.4166667f;
  p.seed.hue_speed = 0.018f;
  p.seed.alpha = 0.9f;
  p.seed.pulse_freq = 0.0800904f;
  p.seed.pulse_amp = 0.0f; // envelope == 1: normalized, non-strobing injection

  p.speed = 0.75f;

  p.motion
      .add("spiral_hue_precession",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0494986f,
                            .amplitude = 0.08f,
                            .offset = 0.0f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
