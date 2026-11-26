#pragma once

#include "base.hpp"

namespace epi::presets {

inline Pattern ocean_current() {
  Pattern p;
  p.name = "ocean_current";
  p.inner = InnerFunction::Cos;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {0.98f, 0.02f};
  p.zn.zn[2] = {0.99f, 0.005f};
  p.color.hue_rotation = 0.003f;
  p.color.sat_scale = 1.005f;
  p.color.fade = 0.992f;

  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.12f;
  p.color.spiral_radius_coeff = 0.09f;
  p.color.chroma_breath_amp = 0.08f;
  p.color.chroma_breath_freq = 0.14f;
  p.color.chroma_breath_phase = 0.05f;
  p.color.hue_velocity_base = 0.006f;
  p.color.hue_velocity_breath = 0.004f;
  p.color.hue_velocity_drift = 0.001f;
  p.color.luma_shape_depth = 0.25f;
  p.color.luma_shape_center = 1.2f;

  p.seed.border_width = 0.12f;
  p.seed.hue_speed = 0.04f;
  p.seed.alpha = 0.8f;
  p.speed = 0.4f;

  // ── Motion: slow hue tide + aperiodic translation drift ──────────────────
  // hue_drift: slow tide-cycle on hue_rot (45 s, oceanic timescale).
  // zn1_wander: NoiseHash drift on Zn1Re/Im — current meanders organically.
  p.motion = epi::control::PatternMotion::slow_color_drift(0.003f, 0.005f, 45.0f);
  p.motion.add("current_re",
               epi::control::ModLane{.wave         = epi::control::Waveform::NoiseHash,
                                     .frequency_hz = 1.0f / 30.0f,
                                     .amplitude    = 0.010f,
                                     .offset       = 0.0f,
                                     .seed         = 7u},
               epi::control::ModTarget::Zn1Re);
  p.motion.add("current_im",
               epi::control::ModLane{.wave         = epi::control::Waveform::NoiseHash,
                                     .frequency_hz = 1.0f / 30.0f,
                                     .amplitude    = 0.010f,
                                     .offset       = 0.0f,
                                     .seed         = 8u},
               epi::control::ModTarget::Zn1Im);
  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.105f;
  p.zn.zn0_anim_amp = 0.036f;
  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.125f;
  p.zn.zn1_anim_amp = 0.039f;
  p.zn.zn1_anim_k = 7.0f;
  p.seed.pulse_freq = 0.077f;
  p.seed.pulse_amp = 0.18f;


  return p;
}

} // namespace epi::presets
