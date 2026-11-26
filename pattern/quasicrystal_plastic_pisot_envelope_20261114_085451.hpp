#pragma once

#include "base.hpp"

#include <cmath>

namespace epi::presets {

// Plastic-constant Pisot scaling (rho^3 = rho + 1, rho = 1.32471796) with golden-angle rotation and rho : rho^2 SpatialWave envelope.
/**
 * Irrational / Quasicrystalline Envelope 1: Plastic-Constant Pisot Quasicrystal.
 *
 * Analytical derivation:
 * - Let rho = 1.3247179572f (unique real root of rho^3 = rho + 1, minimal Pisot
 *   number) and phi = 1.6180339887f (golden ratio, phi^2 = phi + 1).
 * - Pre-affine zn0 scales by rho at golden angle theta_phi = 2*pi / phi^2, while
 *   post-affine zn2 scales by phi / rho = 1.22141775 at plastic angle
 *   theta_rho = 2*pi * (rho - 1); net stretch |zn0|*|zn2| = phi = 1.61803399.
 * - Pre-translation zn1 = (0.1 / rho^3, 0.1 / phi^3) breaks rational parity
 *   under AmmannBeenker 8-fold octant folding so mirror planes never coincide
 *   with a periodic subgrid.
 * - Color = SpatialWave (mode 4) with Pisot spatial frequency ratio
 *   spiral_r : breath_freq = rho : rho^2 and zero linear hue drift.
 */
inline Pattern quasicrystal_plastic_pisot_envelope() {
  constexpr float PI_F = 3.14159265358979323846f;
  constexpr float PHI = 1.6180339887f;
  constexpr float RHO = 1.3247179572f;         // plastic constant: rho^3 = rho + 1
  constexpr float RHO_SQ = RHO * RHO;          // 1.7548776662f = rho + 1/rho
  constexpr float POST_MAG = PHI / RHO;        // 1.22141775f => |zn0*zn2| = phi
  constexpr float THETA_PHI = 2.0f * PI_F / (PHI * PHI); // golden angle 2.39996323 rad
  constexpr float THETA_RHO = 2.0f * PI_F * (RHO - 1.0f); // plastic angle 2.04026311 rad

  Pattern p;
  p.name = "quasicrystal_plastic_pisot_envelope";
  p.inner = InnerFunction::Cos;
  p.reduction = ReductionType::AmmannBeenker;

  p.zn.zn[0] = {RHO * std::cos(THETA_PHI), RHO * std::sin(THETA_PHI)};
  p.zn.zn[1] = {0.1f / (RHO * RHO_SQ), 0.1f / (PHI * PHI * PHI)};
  p.zn.zn[2] = {POST_MAG * std::cos(THETA_RHO), POST_MAG * std::sin(THETA_RHO)};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.04f / RHO;
  p.zn.zn0_anim_amp = 0.032f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.04f / PHI;
  p.zn.zn1_anim_amp = 0.024f;
  p.zn.zn1_anim_k = 8.0f;

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.06f;
  p.color.fade = 0.978f; // tau = 45 steps (Law L3)
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 1.0f / PHI;     // 0.61803399f horizontal slope
  p.color.spiral_radius_coeff = RHO;           // wave1 spatial freq = rho
  p.color.chroma_breath_amp = 0.16f;
  p.color.chroma_breath_freq = RHO_SQ;         // wave2 spatial freq = rho^2 (ratio 1:rho)
  p.color.chroma_breath_phase = 0.16f / RHO;
  p.color.hue_velocity_base = 0.0f;            // bounded quasiperiodic hue clock (O9)
  p.color.hue_velocity_breath = 0.018f * RHO;
  p.color.hue_velocity_drift = -0.006f * PHI;
  p.color.luma_shape_depth = 0.14f;
  p.color.luma_shape_center = PHI;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.075f;
  p.seed.hue = 1.0f / RHO;
  p.seed.hue_speed = 0.035f / PHI;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.05f * RHO;
  p.seed.pulse_amp = 0.0f;

  p.speed = 0.80f;

  p.motion
      .add("pisot_wave_envelope",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.05f / RHO,
                            .amplitude = 0.045f,
                            .offset = 0.16f},
           control::ModTarget::ChromaBreathAmp)
      .add("golden_phase_envelope",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.05f / PHI,
                            .amplitude = 0.035f,
                            .offset = 0.16f / RHO},
           control::ModTarget::ChromaBreathPhase);

  return p;
}

} // namespace epi::presets
