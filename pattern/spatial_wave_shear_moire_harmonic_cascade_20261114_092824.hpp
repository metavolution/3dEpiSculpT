#pragma once

#include "base.hpp"

namespace epi::presets {

// [4C] Moire inner 0.5(sin w + sin(w e^(i2deg))) along oblique (2,1) shear w=1.8 e^(i atan(1/2)) z, zn2=20/27 (|T'(0)|=4/3, Lambda~ln(4/3)+0.020): L4 fade 0.8395 => alpha=3.137, A(512)=1.5%.
/**
 * 4x3 Orthogonal Spatial Wave Matrix -- cell 4C.
 *
 * Row 4 (Anisotropic Moire Beat) x Column C (Harmonic Cascade).
 * - T(z) = (20/27) M(1.8 e^{i atan(1/2)} z), M = 0.5(sin w + sin(w e^{i 2deg}))
 *   (Moire inner 37): zn0 = 1.8 e^{i atan(1/2)} drives w along the oblique
 *   (2, 1) lattice shear axis into sin's anisotropic periodic/hyperbolic
 *   regime (rot90 asymmetry > 0.45), while zn2 = 20/27 keeps the origin
 *   Jacobian |T'(0)| = |zn0 * zn2| = 4/3 exact.
 * - Lambda = ln(4/3) + <ln|M'|> ~ 0.2877 + 0.0202 (uniform mean over the
 *   folded domain; the orbit descriptor reports the exact orbit mean).
 * - Law L4 with the smooth WaveRing seed (f_S = 20/pi; a Simplex border ramp
 *   would inject kink harmonics): A(512) = 0.015 =>
 *   alpha = 3.1368, g = 0.839451 (tau = 5.714 steps).
 * - MoireInterference (mode 12) adds the 1.05-ratio log-radial grating beat
 *   to hue (breath_amp) and saturation (luma_d); spiral_r = 6 keeps the
 *   colour grating below Nyquist for r > 6/(2 pi 128) = 0.0075 (4 px).
 *
 * Temporal ladder: f_11 = 0.06 * 2^(11/12) = 0.11326 Hz (12-TET; no two
 * cells share a frequency). Seed hue = 11/12 on the matrix hue wheel.
 */
inline Pattern spatial_wave_shear_moire_harmonic_cascade() {
  Pattern p;
  p.name = "spatial_wave_shear_moire_harmonic_cascade";
  p.inner = InnerFunction::Moire;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.6099689f, 0.8049845f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.7407407f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.839451f;
  p.color.color_mode = as_int(ColorMode::MoireInterference);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 6.0f;
  p.color.chroma_breath_amp = 0.25f;
  p.color.chroma_breath_freq = 0.1132649f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f; // O9 bounded hue clock
  p.color.hue_velocity_breath = 0.1779161f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.15f;
  p.color.luma_shape_center = 0.5f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.9166667f;
  p.seed.hue_speed = 0.012f;
  p.seed.alpha = 0.9f;
  p.seed.pulse_freq = 0.1132649f;
  p.seed.pulse_amp = 0.0f; // envelope == 1: normalized, non-strobing injection

  p.speed = 0.75f;

  p.motion
      .add("moire_hue_precession",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0700016f,
                            .amplitude = 0.08f,
                            .offset = 0.0f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
