#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Spatial Wave: Parabolic Trefoil of the Quartic Family.
 *
 * Mathematics:
 * - F12 is 0.7 (z^4 - 1). With u = 1.3 z the map is u -> a (u^4 - 1),
 *   a = 0.91 zn2, conjugate to w -> w^4 + c with c = -a^(4/3).
 * - The main component of the degree-4 Multibrot set is parametrised by the
 *   fixed-point multiplier lambda = 4 w^3: c = w - w^4, |w| <= 4^(-1/3). At
 *   lambda = e^(2 pi i / 3), w = 4^(-1/3) e^(2 pi i / 9), which gives
 *   c = (0.6305, 0.3511) and a = (-c)^(3/4) = (-0.3081, -0.7198), so
 *   zn2 = (-0.3386, -0.7910). That is the parabolic point where a period-3
 *   component is attached: the fixed point has three petals, inside a Julia
 *   set with the four-fold symmetry of z^4.
 * - Moving the parameter on a small circle around a parabolic point crosses
 *   between "fixed point attracts" and "three-cycle attracts". The Julia set
 *   does not vary continuously there (parabolic implosion), so a small smooth
 *   motion of zn2 makes the interior restructure.
 *
 * Pipeline: F12, zn0 = 1.3 so the Julia set (|u| < 1.2) fits the window;
 * Clamp; Simplex border seed. Escaping orbits reach the border and are
 * coloured by generation, giving escape terraces outside; the filled set
 * keeps what the petals carry. zn2 circles the parabolic value with radius
 * 0.012.
 *
 * Mechanism assertion: escaped class and a bounded class both present;
 * image symmetric under a quarter turn up to the motion offset.
 *
 * Status: designed from source on 2026-11-14; not yet built, rendered or
 * measured. Hue clock is bounded (hue_velocity_base = 0).
 */
inline Pattern spatial_wave_quartic_parabolic_trefoil() {
  Pattern p;
  p.name = "spatial_wave_quartic_parabolic_trefoil";
  p.inner = InnerFunction::F12;
  p.reduction = ReductionType::Clamp;
  p.zn.zn[0] = {1.3f, 0.0f};
  p.zn.zn[1] = {0.0f, 0.0f};
  p.zn.zn[2] = {-0.3386f, -0.791f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.color.hue_rotation = 0.07f; // hue step per generation
  p.color.sat_scale = 1.0f;
  p.color.fade = 0.94f;
  p.color.color_mode = as_int(ColorMode::SpatialWave);
  p.color.spiral_angle_coeff = 0.75f;
  p.color.spiral_radius_coeff = 0.9f;
  p.color.chroma_breath_amp = 0.1f;
  p.color.chroma_breath_freq = 0.07f;
  p.color.chroma_breath_phase = 0.1f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.06f;
  p.color.hue_velocity_drift = 0.0f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 1.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.06f;
  p.seed.hue = 0.86f;
  p.seed.hue_speed = 0.03f;
  p.seed.alpha = 0.92f;
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f;
  p.speed = 0.8f;

  p.motion
      .add("parabolic_circle_re",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.019f,
                            .amplitude = 0.012f,
                            .offset = -0.3386f},
           control::ModTarget::Zn2Re)
      .add("parabolic_circle_im",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.019f,
                            .amplitude = 0.012f,
                            .offset = -0.791f,
                            .phase = 0.25f},
           control::ModTarget::Zn2Im)
      .add("generation_hue_step",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.015f,
                            .amplitude = 0.025f,
                            .offset = 0.07f},
           control::ModTarget::HueRot)
      .add("relief_depth",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.028f,
                            .amplitude = 0.04f,
                            .offset = 0.1f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
