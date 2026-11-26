#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: Golden-Angle Fan in the Hyperbolic Sine Family.
 *
 * Mathematics:
 * - In w = 2.6 z the map is w -> beta sinh(w), the sine family turned by a
 *   quarter turn. sinh'(0) = 1, so 0 is a fixed point with multiplier beta.
 *   beta = 0.955 e^(2 pi i g), g = (sqrt 5 - 1)/2: weakly attracting, rotating
 *   by the golden angle (222.5 deg) each step. Near the centre the pullback
 *   therefore carries every older generation outward by 1/0.955 and round by
 *   the golden angle, the most evenly spread rotation there is.
 * - Away from the centre sinh grows like e^|Re w| / 2: at the window edge
 *   |beta sinh 2.6| / 2.6 = 2.4 > 1, so those orbits leave the square and are
 *   clamped. The boundary between the spiral basin and the escaping set is
 *   the Julia set of the family, with hairs along the real direction.
 *
 * Pipeline: Sinh, zn0 = 2.6, zn2 = beta / 2.6; Clamp; Cross seed. The cross is
 * redrawn each generation turned by the golden angle and slightly enlarged: a
 * fan of lines through the centre that sinh bends into hyperbolae further out.
 * The rotation angle breathes slightly around the golden value.
 *
 * Mechanism assertion: a contracting class around the centre and an escaped
 * class at large |Re z|, both with non-negligible area.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_sinh_golden_fan_bouquet() {
  Pattern p;
  p.name = "spatial_wave_sinh_golden_fan_bouquet";
  p.inner = InnerFunction::Sinh;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {2.6f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {-0.2709f, -0.2481f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.045f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.955f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.5f;
  p.color.spiral_radius_coeff = 0.6f;
  p.color.chroma_breath_amp = 0.12f;
  p.color.chroma_breath_freq = 0.09f;
  p.color.chroma_breath_phase = 0.1f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.06f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Cross;
  p.seed.border_width = 0.05f;
  p.seed.hue = 0.1f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.8f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("golden_angle_breath_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.021f,
                            .amplitude = 0.012f,
                            .offset = -0.2709f},
           control::ModTarget::Zn2Re)
      .add("golden_angle_breath_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.021f,
                            .amplitude = 0.012f,
                            .offset = -0.2481f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Im)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.016f,
                            .amplitude = 0.02f,
                            .offset = 0.045f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.027f,
                            .amplitude = 0.04f,
                            .offset = 0.12f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
