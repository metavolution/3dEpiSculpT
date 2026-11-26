#pragma once

#include "base.hpp"

namespace epi::presets {

// [1B] Radial squaring w=c z^2 at c=1/sqrt2: the repelling invariant circle |z|=1/c (multiplier 2) is tangent to the frame corners -- exact onset of escaped corner cells.
/**
 * 4x3 Orthogonal Spatial Wave Matrix -- cell 1B.
 *
 * Row 1 (Isotropic / Radial) x Column B (Critical Bifurcation).
 * - T(z) = c z^2 (Square, zn2 = c real). Radial dynamics rho -> c rho^2 has
 *   the repelling invariant circle rho* = 1/c (multiplier 2): inside it every
 *   orbit collapses to 0 (smooth Fresnel rings sin(20 c r^2)), outside it
 *   orbits escape into the Clamp boundary where angle doubling fractures the
 *   rings into 4*2^k azimuthal cells.
 * - c* = 1/sqrt(2) puts rho* = sqrt(2) exactly on the frame corners: the
 *   bifurcation point between 'all smooth' (c < c*) and 'corner cells'.
 * - Motion lane sweeps zn2 = c*(1 +- 0.05) at f_1 (12-TET ladder), moving
 *   rho* between 1.347 (cells nucleate) and 1.489 (smooth) every period.
 * - fade = e^{-1/20}: tau = 20 steps (critical slowing at the marginal orbit).
 * - QuasiPeriodicWave (mode 14) with spiral_a = 0 and hv_drift = 0 keeps the
 *   hue field radial: waves at spiral_r*ln r and phi*spiral_r*ln r only.
 *
 * Temporal ladder: f_1 = 0.06 * 2^(1/12) = 0.06357 Hz (12-TET; no two
 * cells share a frequency). Seed hue = 1/12 on the matrix hue wheel.
 */
inline Pattern spatial_wave_radial_critical_ring_fracture() {
  Pattern p;
  p.name = "spatial_wave_radial_critical_ring_fracture";
  p.inner = InnerFunction::Square;
  p.reduction = ReductionType::Clamp;

  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {0.7071068f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.9512294f;
  p.color.color_mode = as_int(ColorMode::QuasiPeriodicWave);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 1.442695f;
  p.color.chroma_breath_amp = 0.15f;
  p.color.chroma_breath_freq = 0.0635678f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f; // O9 bounded hue clock
  p.color.hue_velocity_breath = 0.099852f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.0833333f;
  p.seed.hue_speed = 0.015f;
  p.seed.alpha = 0.9f;
  p.seed.pulse_freq = 0.0635678f;
  p.seed.pulse_amp = 0.0f; // envelope == 1: normalized, non-strobing injection

  p.speed = 0.5f;

  p.motion
      .add("julia_circle_sweep",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0635678f,
                            .amplitude = 0.0353553f,
                            .offset = 0.7071068f},
           control::ModTarget::Zn2Re);

  return p;
}

} // namespace epi::presets
