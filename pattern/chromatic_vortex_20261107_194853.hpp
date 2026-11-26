#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern chromatic_vortex() {
  Pattern p;
  p.name = "chromatic_vortex";
  p.inner = InnerFunction::Exp;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {0.92f, 0.08f};
  p.zn.zn[2] = {0.97f, 0.03f};
  p.color.hue_rotation = 0.015f;
  p.color.sat_scale = 1.03f;
  p.color.fade = 0.975f;

  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.35f;
  p.color.spiral_radius_coeff = 0.24f;
  p.color.chroma_breath_amp = 0.10f;
  p.color.chroma_breath_freq = 0.19f;
  p.color.chroma_breath_phase = 0.07f;
  p.color.hue_velocity_base = 0.019f;
  p.color.hue_velocity_breath = 0.011f;
  p.color.hue_velocity_drift = 0.006f;
  p.color.luma_shape_depth = 0.22f;
  p.color.luma_shape_center = 1.8f;

  p.seed.border_width = 0.06f;
  p.seed.hue_speed = 0.15f;
  p.seed.alpha = 1.0f;
  p.speed = 1.0f;

  // ── Motion layer: HSLS axis precession + hue vortex sweep ────────────
  //
  // The Exp inner function generates logarithmic spirals — the attractor
  // is fundamentally rotational.  Modulating the HSLS phi_2/psi_2 pair
  // in quadrature adds a second, slower rotational degree of freedom in
  // colour space: the chromatic "orientation" of the vortex precesses over
  // 28 seconds independently of the spatial spiral rate.
  //
  // The hue_vortex lane uses a slightly incommensurate period (22s) to
  // prevent the two motions from phase-locking — the vortex changes colour
  // personality at ~beat(28,22) = 77-second intervals.
  p.motion = control::PatternMotion::dual_axis_drift(
      /*phi2_centre=*/0.05f, /*psi2_centre=*/0.10f,
      /*amp=*/0.040f, /*period_s=*/28.0f);
  p.motion.add("hue_vortex",
      control::ModLane{.wave         = control::Waveform::Sine,
                       .frequency_hz = 1.0f / 22.0f,
                       .amplitude    = 0.007f,
                       .offset       = 0.015f},  // resting = preset hue_rotation
      control::ModTarget::HueRot);
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.101f;
  p.zn.zn0_anim_amp = 0.041f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.167f;
  p.zn.zn1_anim_amp = 0.039f;
  p.zn.zn1_anim_k = 5.0f;
  p.seed.pulse_freq = 0.093f;
  p.seed.pulse_amp = 0.19f;


  return p;
}

} // namespace epi::presets
