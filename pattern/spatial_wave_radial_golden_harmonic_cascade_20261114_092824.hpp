#pragma once

#include "base.hpp"

namespace epi::presets {

// [1C] Isotropic golden minification beta=phi (Lambda=ln phi) with L4 fade g=phi^-(alpha-2)/2=0.7607 => alpha=3.137, aliased energy A(512)=1.5%.
/**
 * 4x3 Orthogonal Spatial Wave Matrix -- cell 1C.
 *
 * Row 1 (Isotropic / Radial) x Column C (Multi-Scale Harmonic Cascade).
 * - T(z) = phi z (Identity, zn2 = phi): nested WaveRing copies at radii
 *   phi^-k with amplitudes g^k form a log-periodic target cascade.
 * - Law L4: alpha = 2 + 2|ln g|/Lambda with Lambda = ln phi. Requiring the
 *   aliased fraction A(512) = (256/f_S)^-(alpha-2) = 0.015 for f_S = 20/pi
 *   gives alpha = 3.1368 and g = 0.760688 (tau = 3.656 steps).
 * - Points beyond |phi z| > 1 clamp onto the frame where the seed envelope
 *   is <= e^{-2}, so the boundary never feeds anisotropic energy back.
 * - LumaLockedHueDrift (mode 10) preserves Rec.601 luma exactly (energy
 *   normalized); spiral_r = 1/ln(phi) gives one hue turn per golden radial
 *   step, so each cascade generation carries the same hue phase.
 *
 * Temporal ladder: f_2 = 0.06 * 2^(2/12) = 0.06735 Hz (12-TET; no two
 * cells share a frequency). Seed hue = 2/12 on the matrix hue wheel.
 */
inline Pattern spatial_wave_radial_golden_harmonic_cascade() {
  Pattern p;
  p.name = "spatial_wave_radial_golden_harmonic_cascade";
  p.inner = InnerFunction::Identity;
  p.reduction = ReductionType::Clamp;

  p.zn.zn[0] = {1.0f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {1.618034f, 0.0f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.7606882f;
  p.color.color_mode = as_int(ColorMode::LumaLockedHueDrift);
  p.color.spiral_angle_coeff = 0.0f;
  p.color.spiral_radius_coeff = 2.0780869f;
  p.color.chroma_breath_amp = 0.0f;
  p.color.chroma_breath_freq = 0.0673477f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f; // O9 bounded hue clock
  p.color.hue_velocity_breath = 0.1057896f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;

  p.seed.type = SeedType::WaveRing;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.1666667f;
  p.seed.hue_speed = 0.01f;
  p.seed.alpha = 0.9f;
  p.seed.pulse_freq = 0.0673477f;
  p.seed.pulse_amp = 0.0f; // envelope == 1: normalized, non-strobing injection

  p.speed = 0.75f;

  p.motion
      .add("golden_hue_precession",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.0416232f,
                            .amplitude = 0.08f,
                            .offset = 0.0f},
           control::ModTarget::HueRot);

  return p;
}

} // namespace epi::presets
