#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern aurora() {
  Pattern p;
  p.name = "aurora";
  p.inner = InnerFunction::Sin;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {0.97f, 0.03f};
  p.zn.zn[1] = {0.0f, 0.005f};
  p.zn.zn[2] = {0.99f, 0.01f};
  p.color.hue_rotation = 0.008f;
  p.color.sat_scale = 1.015f;
  p.color.fade = 0.988f;

  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.28f;
  p.color.spiral_radius_coeff = 0.15f;
  p.color.chroma_breath_amp = 0.07f;
  p.color.chroma_breath_freq = 0.22f;
  p.color.chroma_breath_phase = 0.06f;
  p.color.hue_velocity_base = 0.010f;
  p.color.hue_velocity_breath = 0.007f;
  p.color.hue_velocity_drift = 0.004f;
  p.color.luma_shape_depth = 0.30f;
  p.color.luma_shape_center = 1.6f;

  p.seed.border_width = 0.10f;
  p.seed.hue_speed = 0.06f;
  p.seed.alpha = 0.85f;
  p.speed = 0.6f;

  // ── Motion layer: spectral precession + gentle hue drift ──────────────
  //
  // Aurora moves at a geologic tempo (speed=0.6).  The HSLS axis pair
  // phi_2/psi_2 controls the secondary colour-axis drift rates; modulating
  // them 90° out of phase (quadrature) causes the azimuth/elevation drift
  // rates to trace an ellipse in parameter space — non-repeating chrominance
  // evolution that feels like the slow sweep of real aurora curtains.
  //
  // A long 44-second period keeps the change imperceptible frame-to-frame
  // but produces clear spectral variance across minutes.  The hue_drift
  // lane oscillates hue_rotation ±0.004 around the preset's 0.008 resting
  // value, giving a gentle warm-to-cool sweep every 36 seconds.
  p.motion = control::PatternMotion::dual_axis_drift(
      /*phi2_centre=*/0.05f, /*psi2_centre=*/0.10f,
      /*amp=*/0.028f, /*period_s=*/44.0f);
  p.motion.add("hue_drift",
      control::ModLane{.wave         = control::Waveform::Sine,
                       .frequency_hz = 1.0f / 36.0f,
                       .amplitude    = 0.004f,
                       .offset       = 0.008f},  // resting = preset hue_rotation
      control::ModTarget::HueRot);
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.126f;
  p.zn.zn0_anim_amp = 0.042f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.156f;
  p.zn.zn1_anim_amp = 0.042f;
  p.zn.zn1_anim_k = 4.0f;
  p.seed.pulse_freq = 0.128f;
  p.seed.pulse_amp = 0.16f;


  return p;
}

} // namespace epi::presets
