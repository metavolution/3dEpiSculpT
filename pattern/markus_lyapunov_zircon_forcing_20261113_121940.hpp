#pragma once

#include "base.hpp"

namespace epi::presets {

/**
 * Markus-Lyapunov Fractal: Periodically Forced Quadratic Map ("Zircon Zity").
 *
 * Scientific foundation:
 * - M. Markus & B. Hess (1989): iterate the logistic map while switching its
 *   growth rate between two values in a fixed sequence (A, B, A, B, ...).
 *   Colouring the (r_A, r_B) plane by the Lyapunov exponent
 *   lambda = lim (1/N) * sum ln|f'(x_n)| produces the famous swallow-shaped
 *   stability islands floating in a chaotic sea.
 *
 * Mapping to the 6-stage OpenCL / Vulkan pipeline:
 * - Inner = P2 (0.9*(1 - z^2)): the quadratic map. With pre-scale k = 1.2 and
 *   post-gain c the real-axis dynamics are u -> mu*(1 - u^2), mu = 0.9*k*c.
 * - Motion lane "ab_forcing_sequence": a SQUARE wave on Zn2Re alternates the
 *   gain between c_A = 1.27 and c_B = 1.13, i.e. mu = 1.37 / 1.22. Both lie in
 *   the chaotic band (period doubling accumulates near mu ~ 1.18), so the
 *   field stays mixing while the two forcing phases reshape it: the Markus
 *   A/B sequence executed in time.
 * - The critical point z = 0 maps to 0.9*c ~ 1.08, which the torus fold sends
 *   to ~0.92, inside the wide seed border (0.10): the critical orbit is
 *   re-seeded every pass, so there is no dark super-attracting basin.
 * - Color = DepthEcho: hue follows the history channel (.w), the number of
 *   passes since last seeding; slowly expanding (small lambda) regions keep
 *   long histories, fast ones short: an escape-time proxy for lambda.
 */
inline Pattern markus_lyapunov_zircon_forcing() {
  Pattern p;
  p.name = "markus_lyapunov_zircon_forcing";
  p.inner = InnerFunction::P2;
  p.reduction = ReductionType::Torus;
  p.zn.zn[0] = {1.20f, 0.06f};
  p.zn.zn[1] = {0.010f, -0.008f};
  p.zn.zn[2] = {1.20f, -0.05f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn1_animate = true;
  p.zn.zn1_anim_speed = 0.09f;
  p.zn.zn1_anim_amp = 0.025f;
  p.zn.zn1_anim_k = 2.0f;

  p.color.hue_rotation = 0.012f;
  p.color.sat_scale = 1.10f;
  p.color.fade = 0.987f;
  p.color.color_mode = as_int(ColorMode::DepthEcho);
  p.color.spiral_angle_coeff = 1.6f;
  p.color.spiral_radius_coeff = 0.70f;
  p.color.chroma_breath_amp = 0.22f;
  p.color.chroma_breath_freq = 0.15f;
  p.color.chroma_breath_phase = 0.0f;
  p.color.hue_velocity_base = 0.020f;
  p.color.hue_velocity_breath = 0.012f;
  p.color.hue_velocity_drift = 0.006f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.0f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.10f;
  p.seed.hue = 0.0f;
  p.seed.hue_speed = 0.07f;
  p.seed.alpha = 0.93f;
  p.seed.pulse_freq = 0.08f;
  p.seed.pulse_amp = 0.15f;
  p.speed = 0.7f;

  p.motion
      .add("ab_forcing_sequence",
           control::ModLane{.wave = control::Waveform::Square,
                            .frequency_hz = 0.25f,
                            .amplitude = 0.07f,
                            .offset = 1.20f},
           control::ModTarget::Zn2Re)
      .add("exponent_hue_gain",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.05f,
                            .amplitude = 0.30f,
                            .offset = 1.6f},
           control::ModTarget::SpiralAngleCoeff)
      .add("stability_island_chroma",
           control::ModLane{.wave = control::Waveform::Triangle,
                            .frequency_hz = 0.07f,
                            .amplitude = 0.07f,
                            .offset = 0.22f},
           control::ModTarget::ChromaBreathAmp);

  return p;
}

} // namespace epi::presets
