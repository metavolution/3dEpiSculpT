#pragma once

#include "base.hpp"

namespace epi::presets {

// Dark-field soliton collision (FluidVortex |w|*exp(-cos(u)*sinh(v)), Torus odd-integer level-set ribbons w=0.030, fade=0.68) yielding >88% sparsity and unit-peak crests.
/**
 * Dark-Field Caustic / Soliton Regime 2: Shear-Channel Soliton Collision.
 *
 * Analytical derivation:
 * - Inner = FluidVortex evaluates f(w) = w * exp(i * sin(w)) for w = u + i*v.
 *   Since i*sin(u + i*v) = -cos(u)*sinh(v) + i*sin(u)*cosh(v), the exact
 *   modulus is |f(w)| = |w| * exp(-cos(u) * sinh(v)), creating colliding
 *   dispersion wavepackets with steepest-gradient overtaking crests.
 * - Reduction = Torus + Seed = Simplex (border_width w = 0.030):
 *     * Under Torus folding, Simplex is identically 0.000 everywhere except
 *       within +-w of odd-integer level sets t_{z,x} = 2k+1 or t_{z,y} = 2k+1
 *       (covering only 1 - (1 - 0.030)^2 = 5.91% of domain measure per step!).
 *     * With fade = 0.68 (tau = 2.59 steps), only 2-3 sharp soliton echo
 *       trains exceed 0.05 luminance (>88% dark-field spatial sparsity), while
 *       on the exact odd-integer soliton crests intensity == 1.0 and
 *       seed.alpha = 0.98 (pulse_amp = 0.0) lock peak amplitude at 0.98.
 * - Color = OKLabColorRotation (mode 17) rotates crest chroma in perceptual
 *   OKLCH space with zero dark-background lift.
 */
inline Pattern darkfield_kdv_soliton_collision() {
  Pattern p;
  p.name = "darkfield_kdv_soliton_collision";
  p.inner = InnerFunction::FluidVortex;
  p.reduction = ReductionType::Torus;

  p.zn.zn[0] = {1.85f, 0.22f};
  p.zn.zn[1] = {0.12f, -0.08f};
  p.zn.zn[2] = {1.65f, -0.24f};
  p.zn.zn[3] = {0.0f, 0.0f};

  p.zn.zn0_animate = true;
  p.zn.zn0_anim_speed = 0.038f;
  p.zn.zn0_anim_amp = 0.030f;

  p.color.hue_rotation = 0.0f;
  p.color.sat_scale = 1.12f;
  p.color.fade = 0.68f; // tau = 2.59 steps: 2-3 crisp colliding soliton crests
  p.color.color_mode = as_int(ColorMode::OKLabColorRotation);
  p.color.spiral_angle_coeff = 2.0f;
  p.color.spiral_radius_coeff = 0.55f;
  p.color.chroma_breath_amp = 0.16f;
  p.color.chroma_breath_freq = 0.08f;
  p.color.chroma_breath_phase = 2.0f;
  p.color.hue_velocity_base = 0.0f;
  p.color.hue_velocity_breath = 0.014f;
  p.color.hue_velocity_drift = -0.005f;
  p.color.luma_shape_depth = 0.0f;
  p.color.luma_shape_center = 0.5f;

  p.seed.type = SeedType::Simplex;
  p.seed.border_width = 0.030f; // 5.91% per-step level-set measure => >88% dark-field
  p.seed.hue = 0.46f;
  p.seed.hue_speed = 0.035f;
  p.seed.alpha = 0.98f;    // normalized constructive soliton peak 0.98
  p.seed.pulse_freq = 0.05f;
  p.seed.pulse_amp = 0.0f; // lock env(t) == 1.0

  p.speed = 0.75f;

  p.motion
      .add("soliton_channel_phase",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.035f,
                            .amplitude = 0.22f,
                            .offset = -0.24f},
           control::ModTarget::Zn2Im)
      .add("soliton_packet_drift",
           control::ModLane{.wave = control::Waveform::Sine,
                            .frequency_hz = 0.024f,
                            .amplitude = 0.08f,
                            .offset = -0.08f,
                            .phase = 0.25f},
           control::ModTarget::Zn1Im);

  return p;
}

} // namespace epi::presets
