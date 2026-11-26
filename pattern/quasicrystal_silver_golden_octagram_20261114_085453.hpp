#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

// Diophantine phi : sqrt(2) bi-irrational envelope via QuasiPeriodicWave (spiral_a=sqrt(2) branch-cut cancellation) over F2 (sin*cosh).
/**
 * Irrational / Quasicrystalline Envelope 2: Silver-Golden Octagram Quasicrystal.
 *
 * Analytical derivation:
 * - Inner = F2 (sin(z)*cosh(z), id 22): orthogonal product of real-periodic
 *   standing waves and imaginary-hyperbolic sheets.
 * - Pre-affine zn0 scales by sqrt(2) at silver angle 2*pi*(sqrt(2)-1); post-affine
 *   zn2 scales by 1 + (sqrt(2)-1)/phi = 1.25600635 at conjugate golden angle
 *   -2*pi/phi^2, preventing commensurate alignment with Torus mirror seams.
 * - Kernel coupling exploit in QuasiPeriodicWave (color_mode = 14):
 *     wave3 = sin(2*pi * (spiral_a * sqrt(2) * angle + hv_drift * time))
 *   where angle in [-0.5, 0.5]. Setting spiral_a = sqrt(2) makes the azimuthal
 *   multiplier spiral_a * sqrt(2) == 2.0000000 (an exact integer!), eliminating
 *   the atan2(+-pi) branch-cut discontinuity while radial waves wave1 (1/phi)
 *   and wave2 (1/phi * phi = 1.0) maintain exact 1 : phi golden aperiodicity.
 */
inline Pattern quasicrystal_silver_golden_octagram() {
  constexpr float PI_F = 3.14159265358979323846f;
  constexpr float SQRT2 = 1.4142135624f;
  constexpr float INV_SILVER = SQRT2 - 1.0f;    // 0.4142135624f
  constexpr float PHI = 1.6180339887f;
  constexpr float INV_PHI = PHI - 1.0f;         // 0.6180339887f
  constexpr float THETA_AG = 2.0f * PI_F * INV_SILVER;
  constexpr float THETA_AU = -2.0f * PI_F * INV_PHI * INV_PHI;
  constexpr float POST_MAG = 1.0f + INV_SILVER * INV_PHI; // 1.25600635f

  Pattern p;
  p.name = "quasicrystal_silver_golden_octagram";
  p.inner = InnerFunction::F2;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {SQRT2 * std::cos(THETA_AG), SQRT2 * std::sin(THETA_AG)};
  p.zn.zn[1] = {0.019f * INV_PHI, -0.019f * INV_SILVER};
  p.zn.zn[2] = {POST_MAG * std::cos(THETA_AU), POST_MAG * std::sin(THETA_AU)};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.042f * INV_PHI;
  p.zn.zn0_anim_amp = 0.030f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.042f * INV_SILVER;
  p.zn.zn1_anim_amp = 0.022f;
  p.zn.zn1_anim_k = 5.0f;

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.08f;
  p.color.fade = 0.982f;
  p.color.color_mode = as_int(ColorMode::QuasiPeriodicWave);
  p.color.spiral_angle_coeff = SQRT2;     // sqrt(2)*sqrt(2) == 2 seamless azimuth
  p.color.spiral_radius_coeff = INV_PHI;  // wave1=1/phi, wave2=(1/phi)*phi=1.0
  p.color.chroma_breath_amp = 0.22f;
  p.color.chroma_breath_freq = INV_PHI;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.0f;                  // bounded hue clock (no secular drift)
  p.color.hue_velocity_breath = 0.016f * INV_PHI;    // golden breath modulation
  p.color.hue_velocity_drift = -0.008f * INV_SILVER; // incommensurate silver drift
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;

  p.seed.type = SeedType::Cross;          // unit-peak axes (intensity==1 at z_x=0 or z_y=0)
  p.seed.border_width = 0.06f;
  p.seed.hue = INV_PHI;
  p.seed.hue_speed = 0.03f * INV_SILVER;
  p.seed.alpha = 0.95f;                   // 1.0^2 * 0.95 => normalized octagram filament peak
  p.seed.pulse_freq = 0.06f * INV_PHI;
  p.seed.pulse_amp = 0.0f;

  p.speed = 0.78f;

  p.motion
      .add("radial_quasicrystal_pitch",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.045f * INV_PHI,
                            .amplitude = 0.12f,
                            .offset = INV_PHI},
           control::ModTarget::SpiralRadiusCoeff)
      .add("silver_chroma_envelope",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.045f * INV_SILVER,
                            .amplitude = 0.06f,
                            .offset = 0.22f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
